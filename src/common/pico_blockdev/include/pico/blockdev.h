#ifndef BLOCKDEV_H__
#define BLOCKDEV_H__

#include <inttypes.h>

typedef struct pico_blockdev__ pico_blockdev_t;

typedef struct
{
    uint32_t sector_size;
    uint32_t total_sectors;
} pico_blockdev_info_t;

typedef struct
{
    int (*init)(pico_blockdev_t *dev);
    int (*read_sector)(pico_blockdev_t *dev, unsigned char* data, uint32_t start_sector, unsigned count);
    int (*write_sector)(pico_blockdev_t *dev, const unsigned char* data, uint32_t start_sector, unsigned count);
    int (*ioctl)(pico_blockdev_t *dev, unsigned char cmd, void* data);
} pico_blockdev_ops_t;

struct pico_blockdev__
{
    const pico_blockdev_ops_t *ops;
    struct pico_blockdev__ *parent;
    /* Other dev-specific data below */
};


/*
 Supported IOCTLs
 */
#define PICO_IOCTL_BLKGETSIZE (0) /* Get device size in sectors */
#define PICO_IOCTL_BLKSSZGET (1)  /* Get sector size in bytes */
#define PICO_IOCTL_BLKROGET (2)   /* Get readonly flag */
#define PICO_IOCTL_BLKFLSBUF (3)  /* Sync */

int pico_blockdev_read_sector(pico_blockdev_t *dev, unsigned char* data, uint32_t start_sector, unsigned count);
int pico_blockdev_write_sector(pico_blockdev_t *dev, const unsigned char* data, uint32_t start_sector, unsigned count);
int pico_blockdev_ioctl(pico_blockdev_t *dev, unsigned char cmd, void* data);
int pico_blockdev_init(pico_blockdev_t *dev);

// Debug
#define BLKDEV_INFO(drv, x...)
#define BLKDEV_WARN(drv, x...)
#define BLKDEV_ERROR(drv, x...)
#define BLKDEV_DEBUG(drv, x...)

#endif
