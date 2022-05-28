#include "pico/vfs.h"
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/reent.h>

extern struct _reent * __getreent (void);

#define MAX_FDS 16
#define ESP_VFS_PATH_MAX 16
#define VFS_MAX_COUNT 4
#define LEN_PATH_PREFIX_IGNORED SIZE_MAX /* special length value for VFS which is never recognised by open() */

struct __dirstream
{
    vfs_index_t vfs_index;
};

typedef struct pico_vfs_fd_table_
{
    uint8_t vfs_index;
    uint8_t local_fd;
    bool permanent;
} pico_vfs_fd_table_t;

typedef struct pico_vfs_entry_ {
    pico_vfs_ops_t ops;

    char path_prefix[ESP_VFS_PATH_MAX]; // path prefix mapped to this VFS

    size_t path_prefix_len; // micro-optimization to avoid doing extra strlen
    uint8_t index;             // index of this structure in s_vfs array
} pico_vfs_entry_t;

#define FD_TABLE_ENTRY_UNUSED   (pico_vfs_fd_table_t) { .vfs_index = -1, .local_fd = -1, .permanent = false }

static pico_vfs_fd_table_t s_fd_table[MAX_FDS];
static pico_vfs_entry_t* s_vfs[VFS_MAX_COUNT] = { 0 };
static uint8_t s_vfs_count = 0;
static bool vfs_initialised = false;

static inline void pico_vfs_table_lock()
{
}

static inline void pico_vfs_table_unlock()
{
}

static const char* translate_path(const pico_vfs_entry_t* vfs, const char* src_path)
{
    assert(strncmp(src_path, vfs->path_prefix, vfs->path_prefix_len) == 0);
    if (strlen(src_path) == vfs->path_prefix_len) {
        // special case when src_path matches the path prefix exactly
        return "/";
    }
    return src_path + vfs->path_prefix_len;
}

static const pico_vfs_entry_t* pico_vfs_get_vfs_for_path(const char* path)
{
    const pico_vfs_entry_t* best_match = NULL;
    ssize_t best_match_prefix_len = -1;
    size_t len = strlen(path);
    printf("VFS match cnt=%d\n", s_vfs_count);
    for (size_t i = 0; i < s_vfs_count; ++i) {
        const pico_vfs_entry_t* vfs = s_vfs[i];
        if (vfs)
            printf("VFS match len=%d preflen=%d\r\n", len, vfs->path_prefix_len);
        else
            printf("NULL VFS\r\n");

        if (!vfs || vfs->path_prefix_len == LEN_PATH_PREFIX_IGNORED) {
            continue;
        }
        // match path prefix
        if (len < vfs->path_prefix_len ||
            memcmp(path, vfs->path_prefix, vfs->path_prefix_len) != 0) {
            continue;
        }
        // this is the default VFS and we don't have a better match yet.
        if (vfs->path_prefix_len == 0 && !best_match) {
            best_match = vfs;
            continue;
        }
        // if path is not equal to the prefix, expect to see a path separator
        // i.e. don't match "/data" prefix for "/data1/foo.txt" path
        if (len > vfs->path_prefix_len &&
                path[vfs->path_prefix_len] != '/') {
            continue;
        }
        // Out of all matching path prefixes, select the longest one;
        // i.e. if "/dev" and "/dev/uart" both match, for "/dev/uart/1" path,
        // choose "/dev/uart",
        // This causes all s_vfs_count VFS entries to be scanned when opening
        // a file by name. This can be optimized by introducing a table for
        // FS search order, sorted so that longer prefixes are checked first.
        if (best_match_prefix_len < (ssize_t) vfs->path_prefix_len) {
            best_match_prefix_len = (ssize_t) vfs->path_prefix_len;
            best_match = vfs;
        }
    }
    return best_match;
}

static inline const pico_vfs_entry_t *get_vfs_entry_for_index(int index)
{
    if (index < 0 || index >= s_vfs_count) {
        return NULL;
    } else {
        return s_vfs[index];
    }
}

pico_vfs_ops_t *pico_vfs_get_vfs_for_index(int index)
{
    if (index < 0 || index >= s_vfs_count) {
        return NULL;
    } else {
        return &s_vfs[index]->ops;
    }
}


static inline bool pico_vfs_valid_fd(int fd)
{
    return (fd < MAX_FDS) && (fd >= 0);
}

static const pico_vfs_entry_t *pico_vfs_get_vfs_for_fd(int fd)
{
    const pico_vfs_entry_t *vfs = NULL;

    if (pico_vfs_valid_fd(fd)) {
        const int index = s_fd_table[fd].vfs_index;
        vfs = get_vfs_entry_for_index(index);
    }
    return vfs;
}


static int pico_vfs_local_fd(const pico_vfs_entry_t *vfs, int global_fd)
{
    int local_fd = -1;

    if (vfs && pico_vfs_valid_fd(global_fd)) {
        local_fd = s_fd_table[global_fd].local_fd; // single read -> no locking is required
    }

    return local_fd;
}

static int pico_vfs_register_common(const char *base_path, size_t len, const pico_vfs_ops_t *ops, int *vfs_index)
{
    if (len != LEN_PATH_PREFIX_IGNORED) {
        if ((len != 0 && len < 2) || (len > ESP_VFS_PATH_MAX)) {
            return EINVAL;
        }
        if ((len > 0 && base_path[0] != '/') || (len>0 && base_path[len - 1] == '/')) {
            return EINVAL;
        }
    }
    pico_vfs_entry_t *entry = (pico_vfs_entry_t*) malloc(sizeof(pico_vfs_entry_t));
    if (entry == NULL) {
        return ENOMEM;
    }
    size_t index;
    for (index = 0; index < s_vfs_count; ++index) {
        if (s_vfs[index] == NULL) {
            break;
        }
    }
    if (index == s_vfs_count) {
        if (s_vfs_count >= VFS_MAX_COUNT) {
            free(entry);
            return ENOMEM;
        }
        ++s_vfs_count;
    }
    s_vfs[index] = entry;

    if (len != LEN_PATH_PREFIX_IGNORED) {
        strcpy(entry->path_prefix, base_path); // we have already verified argument length
    } else {
        bzero(entry->path_prefix, sizeof(entry->path_prefix));
    }
    memcpy(&entry->ops, ops, sizeof(pico_vfs_ops_t));

    entry->path_prefix_len = len;
    entry->index = index;

    if (vfs_index) {
        *vfs_index = index;
    }

    return 0;
}

int pico_vfs_register(const char* base_path, const pico_vfs_ops_t* vfs)
{
    return pico_vfs_register_common(base_path, strlen(base_path), vfs, NULL);
}

int pico_vfs_register_fd_range_for_vfs_index(vfs_index_t index,
                                             int min_fd, int max_fd)
{
    pico_vfs_table_lock();
    for (int i = min_fd; i < max_fd; ++i) {
        if (s_fd_table[i].vfs_index != -1) {

            for (int j = min_fd; j < i; ++j) {
                if (s_fd_table[j].vfs_index == index) {
                    s_fd_table[j] = FD_TABLE_ENTRY_UNUSED;
                }
            }
            pico_vfs_table_unlock();
            return EINVAL;
        }
        s_fd_table[i].permanent = true;
        s_fd_table[i].vfs_index = index;
        s_fd_table[i].local_fd = i;
    }
    pico_vfs_table_unlock();
    return 0;
}

int pico_vfs_register_fd_range(const pico_vfs_ops_t *vfs, int min_fd, int max_fd)
{
    if (min_fd < 0 || max_fd < 0 || min_fd > MAX_FDS || max_fd > MAX_FDS || min_fd > max_fd) {
        return EINVAL;
    }

    int index = -1;

    int ret = pico_vfs_register_common("", LEN_PATH_PREFIX_IGNORED, vfs, &index);

    if (ret == 0) {
        ret = pico_vfs_register_fd_range_for_vfs_index(index, min_fd, max_fd);
        if (ret !=0) {
#if 0
            pico_vfs_table_lock();
            free(s_vfs[index]);
            s_vfs[index] = NULL;
            pico_vfs_table_unlock();
#endif
            index = -1;
        }
    } else {
        index = -1;
    }

    return index;
}


#define VFSDECL_R(fd, reent) \
    const pico_vfs_entry_t* vfs = pico_vfs_get_vfs_for_fd(fd);   \
    const int local_fd = pico_vfs_local_fd(vfs, fd);         \
    if (vfs == NULL || local_fd < 0) {                  \
        __errno_r(reent) = EBADF;                       \
        return -1;                                      \
    }

#define VFSCALL_R(rettype, reent, name, ...)  \
    rettype ret; \
    if (vfs->ops.name != NULL) { \
       __errno_r(reent) = ENOSYS; \
       ret = -1; \
    } else { \
       ret = (*vfs->ops.name)( __VA_ARGS__ );  \
    }

#define VFSCALL_R_N(rettype, reent, name, ...)  \
    rettype ret; \
    if (vfs->ops.name != NULL) { \
       __errno_r(reent) = ENOSYS; \
       ret = NULL; \
    } else { \
       ret = (*vfs->ops.name)( __VA_ARGS__ );  \
    }

#define VFS_GENERIC_REENT_CALL(rettype, name, reent, fd, ...) \
    VFSDECL_R(fd, reent); \
    VFSCALL_R(rettype, reent, name, local_fd, __VA_ARGS__); \
    return ret;


off_t pico_vfs_lseek(struct _reent *r, int fd, off_t size, int mode)
{
    VFS_GENERIC_REENT_CALL(off_t, lseek, r, fd, size, mode );
}

ssize_t pico_vfs_write(struct _reent *r, int fd, const void * data, size_t size)
{
    VFS_GENERIC_REENT_CALL(ssize_t, write, r, fd, data, size);
}

int pico_vfs_open(struct _reent *r, const char * path, int flags, int mode)
{
    const pico_vfs_entry_t* vfs = pico_vfs_get_vfs_for_path(path);

    if (vfs == NULL) {
        __errno_r(r) = ENOENT;
        return -1;
    }

    const char *path_within_vfs = translate_path(vfs, path);
    int fd_within_vfs;

    VFSCALL_R(int, r, open, path_within_vfs, flags, mode );

    /* ret now holds error or the vfs-local fd */

    if (ret >= 0)
    {
        pico_vfs_table_lock();

        for (int i = 0; i < MAX_FDS; ++i) {
            if (s_fd_table[i].vfs_index == -1) {
                s_fd_table[i].permanent = false;
                s_fd_table[i].vfs_index = vfs->index;
                s_fd_table[i].local_fd = fd_within_vfs;
                pico_vfs_table_unlock();
                /* Success. Return global fd */
                return i;
            }
        }

        pico_vfs_table_unlock();
        do {
            VFSCALL_R(int, r, close, fd_within_vfs);
            (void)ret;
        } while (0);
        __errno_r(r) = ENOMEM;
        return -1;

    }
    return ret;
}

void vfs_debug(const char *p)
{
    char temp[80];
    sprintf(temp,"%s %p id %d\r\n", p, s_vfs[0], s_vfs_count);
    stdio_vfs_write(1, temp, strlen(temp));
}

DIR* pico_vfs_opendir(const char* name)
{
    const pico_vfs_entry_t * vfs = pico_vfs_get_vfs_for_path(name);

    printf("OPENDIR: attempt open %s, vfs %p\r\n", name, vfs);
    printf("OPENDIR: vfs entries %d idx 0 %p\r\n", s_vfs_count, s_vfs[0]);

    struct _reent* r = __getreent();
    if (vfs == NULL) {
        __errno_r(r) = ENOENT;
        return NULL;
    }

    const char* path_within_vfs = translate_path(vfs, name);

    VFSCALL_R_N(DIR*, r, opendir, path_within_vfs);

    if (ret != NULL) {
        ret->vfs_index = vfs->index;
    }

    return ret;
}


ssize_t _write_r(struct _reent *r, int fd, const void * data, size_t size) __attribute__((alias("pico_vfs_write")));
DIR* opendir(const char* name) __attribute__((alias("pico_vfs_opendir")));

static DIR* pico_vfs_root_opendir(const char* name)
{
    printf("ROOT_OPENDIR: attempt open %s\r\n", name);
    return NULL;
}

vfs_index_t pico_vfs_init(void)
{
#if 0 /* we don't have atomics... */
    if (atomic_exchange_explicit(&vfs_initialised, true, __ATOMIC_ACQ_REL)) {
        return EBUSY;
    }
#else
    if (vfs_initialised) {
        return EBUSY;
    }
    vfs_initialised = 1;
#endif

    pico_vfs_table_lock();

    for (int i = 0; i < MAX_FDS; ++i) {
        s_fd_table[i].vfs_index = -1;
    }

    pico_vfs_table_unlock();


    // Init root VFS, mainly for root directory iteration.
    const pico_vfs_ops_t rootops =
    {
        .opendir = &pico_vfs_root_opendir
        /*struct dirent* (*readdir)(DIR* pdir);
        int (*readdir_r)(DIR* pdir, struct dirent* entry, struct dirent** out_dirent);
        long (*telldir)(DIR* pdir);
        void (*seekdir)(DIR* pdir, long offset);
        int (*closedir)(DIR* pdir);
        */
    };
    int index = -1;
    int ret = pico_vfs_register_common("",
                                       0,
                                       &rootops,
                                       &index);

    vfs_debug("l");

    sleep_ms(5000);

    if (ret<0)
        return ret;

    vfs_debug("s");

    return index;
}

