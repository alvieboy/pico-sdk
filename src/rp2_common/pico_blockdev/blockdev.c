#include "pico/blockdev.h"

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
