#include "pico/blockdev.h"
#include <stdlib.h>

extern void pico_blockdev_scan_partitions(pico_blockdev_t *dev);

int pico_blockdev_read_sector(pico_blockdev_t *dev, unsigned char* data, uint32_t start_sector, unsigned count)
{
    if (dev->ops->read_sector) {
        return (*dev->ops->read_sector)(dev, data, start_sector, count);
    } else {
        return -1;
    }
}

int pico_blockdev_write_sector(pico_blockdev_t *dev, const unsigned char* data, uint32_t start_sector, unsigned count)
{
    if (dev->ops->write_sector) {
        return (*dev->ops->write_sector)(dev, data, start_sector, count);
    } else {
        return -1;
    }
}

int pico_blockdev_ioctl(pico_blockdev_t *dev, unsigned char cmd, void* data)
{
    if (dev->ops->ioctl) {
        return (*dev->ops->ioctl)(dev, cmd, data);
    } else {
        return -1;
    }
}

int pico_blockdev_init(pico_blockdev_t *dev)
{
    dev->childs = 0;
    if (dev->ops->init) {
        return (*dev->ops->init)(dev);
    } else {
        return -1;
    }
}

int pico_blockdev_get_children(pico_blockdev_t *dev)
{
    return dev->childs;
}

void __attribute__((weak)) pico_blockdev_register_event(pico_blockdev_t *dev)
{
}
void __attribute__((weak)) pico_blockdev_unregister_event(pico_blockdev_t *dev)
{
}

int pico_blockdev_register(pico_blockdev_t *dev)
{
    if (!dev->parent)
    {
        pico_blockdev_scan_partitions(dev);
    }
    pico_blockdev_register_event(dev);

    return 0;
}
