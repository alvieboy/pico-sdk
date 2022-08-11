#ifndef VFS_H__
#define VFS_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/time.h>
#include <utime.h>
#include <dirent.h>

#define VFS_DIR_SUPPORT

#define PICO_VFS_BASE_PATH_MAX 16
#include <stdio.h>
//#define FILENAME_MAX 128

extern struct _reent * __getreent (void);


typedef int vfs_index_t;

typedef void *vfs_lock_t;

struct __dirstream
{
    vfs_index_t vfs_index;
    // For non-reentrant readdir
    struct dirent dir_iter;
};

/*
 VFS operations
 */
typedef struct _pico_vfs_ops
{
    int (*open)(void *drvctx, const char * path, int flags, int mode);
    int (*close)(void *drvctx, int fd);
    ssize_t (*read)(void *drvctx, int fd, void * dst, size_t size);
    ssize_t (*write)(void *drvctx, int fd, const void * data, size_t size);
    int (*fstat)(void *drvctx, int fd, struct stat * st);
    off_t (*lseek)(void *drvctx, int fd, off_t offset, int whence);
    ssize_t (*pread)(void *drvctx, int fd, void * dst, size_t size, off_t offset);
    ssize_t (*pwrite)(void *drvctx, int fd, const void *src, size_t size, off_t offset);
    int (*fcntl)(void *drvctx, int fd, int cmd, int arg);
    int (*ioctl)(void *drvctx, int fd, int cmd, va_list args);
    int (*fsync)(void *drvctx, int fd);

    int (*stat)(void *drvctx, const char * path, struct stat * st);
    int (*link)(void *drvctx, const char* n1, const char* n2);
    int (*unlink)(void *drvctx, const char *path);
    int (*rename)(void *drvctx, const char *src, const char *dst);
    DIR* (*opendir)(void *drvctx, const char* name);
    struct dirent* (*readdir)(void *drvctx, DIR* pdir);
    int (*readdir_r)(void *drvctx, DIR* pdir, struct dirent* entry, struct dirent** out_dirent);
    long (*telldir)(void *drvctx, DIR* pdir);
    void (*seekdir)(void *drvctx, DIR* pdir, long offset);
    int (*closedir)(void *drvctx, DIR* pdir);
    int (*mkdir)(void *drvctx, const char* name, mode_t mode);
    int (*rmdir)(void *drvctx, const char* name);
    int (*access)(void *drvctx, const char *path, int amode);
    int (*truncate)(void *drvctx, const char *path, off_t length);
    int (*utime)(void *drvctx, const char *path, const struct utimbuf *times);

#ifdef VFS_TERMIOS_SUPPORT
    int (*tcsetattr)(void *drvctx, int fd, int optional_actions, const struct termios *p);
    int (*tcgetattr)(void *drvctx, int fd, struct termios *p);
    int (*tcdrain)(void *drvctx, int fd);
    int (*tcflush)(void *drvctx, int fd, int select);
    int (*tcflow)(void *drvctx, int fd, int action);
    pid_t (*tcgetsid_p)(void *drvctx, void *ctx, int fd);
    pid_t (*tcgetsid)(void *drvctx, int fd);
    int (*tcsendbreak)void *drvctx, (int fd, int duration);
#endif // VFS_TERMIOS_SUPPORT

} pico_vfs_ops_t;

vfs_index_t pico_vfs_init(void);
vfs_index_t pico_vfs_register(const char *path, const pico_vfs_ops_t *ops, void *drvctx);
vfs_index_t pico_vfs_register_fd_range(const pico_vfs_ops_t *vfs, void *drvctx,
                                       int min_fd, int max_fd);
int pico_vfs_register_fd_range_for_vfs_index(vfs_index_t index,
                                       int min_fd, int max_fd);
/* This should be used with care! */
pico_vfs_ops_t *pico_vfs_get_vfs_ops_for_index(int index);

/* Locking primitives to be used by drivers */
void pico_vfs_lock_init(vfs_lock_t lock);
void pico_vfs_lock_acquire(vfs_lock_t lock);
void pico_vfs_lock_release(vfs_lock_t lock);

int pico_vfs_concat_path(const char *path1, const char *path2, char *dest, size_t destlen);

#endif
