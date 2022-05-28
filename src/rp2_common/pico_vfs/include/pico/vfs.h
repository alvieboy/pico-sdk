#ifndef VFS_H__
#define VFS_H__

#include <sys/types.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/time.h>
#include <utime.h>

#define VFS_DIR_SUPPORT

typedef int vfs_index_t;

/*
 VFS operations
 */
typedef struct _pico_vfs_ops
{
    int (*open)(const char * path, int flags, int mode);
    int (*close)(int fd);
    ssize_t (*read)(int fd, void * dst, size_t size);
    ssize_t (*write)(int fd, const void * data, size_t size);
    int (*fstat)(int fd, struct stat * st);
    off_t (*lseek)(int fd, off_t size, int mode);
    ssize_t (*pread)(int fd, void * dst, size_t size, off_t offset);
    ssize_t (*pwrite)(int fd, const void *src, size_t size, off_t offset);
    int (*fcntl)(int fd, int cmd, int arg);
    int (*ioctl)(int fd, int cmd, va_list args);
    int (*fsync)(int fd);

    int (*stat)(const char * path, struct stat * st);
    int (*link)(const char* n1, const char* n2);
    int (*unlink)(const char *path);
    int (*rename)(const char *src, const char *dst);
    DIR* (*opendir)(const char* name);
    struct dirent* (*readdir)(DIR* pdir);
    int (*readdir_r)(DIR* pdir, struct dirent* entry, struct dirent** out_dirent);
    long (*telldir)(DIR* pdir);
    void (*seekdir)(DIR* pdir, long offset);
    int (*closedir)(DIR* pdir);
    int (*mkdir)(const char* name, mode_t mode);
    int (*rmdir)(const char* name);
    int (*access)(const char *path, int amode);
    int (*truncate)(const char *path, off_t length);
    int (*utime)(const char *path, const struct utimbuf *times);

#ifdef VFS_TERMIOS_SUPPORT
    int (*tcsetattr)(int fd, int optional_actions, const struct termios *p);
    int (*tcgetattr)(int fd, struct termios *p);
    int (*tcdrain)(int fd);
    int (*tcflush)(int fd, int select);
    int (*tcflow)(int fd, int action);
    pid_t (*tcgetsid_p)(void *ctx, int fd);
    pid_t (*tcgetsid)(int fd);
    int (*tcsendbreak)(int fd, int duration);
#endif // CONFIG_VFS_SUPPORT_TERMIOS

} pico_vfs_ops_t;

vfs_index_t pico_vfs_init(void);
vfs_index_t pico_vfs_register(const char *path, const pico_vfs_ops_t *ops);
vfs_index_t pico_vfs_register_fd_range(const pico_vfs_ops_t *vfs,
                                       int min_fd, int max_fd);
int pico_vfs_register_fd_range_for_vfs_index(vfs_index_t index,
                                       int min_fd, int max_fd);
/* This should be used with care! */
pico_vfs_ops_t *pico_vfs_get_vfs_for_index(int index);

#endif
