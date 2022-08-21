#include "hardware/i2c.h"
#include <stdlib.h>

struct i2c_transaction_entry
{
    const struct i2c_device_ops *ops;
    void *userdata;
    struct i2c_transaction_entry *next;
};

struct i2c_hw
{
    struct i2c_transaction_entry *transaction_handlers;
};

static i2c_hw_t i2c0_hw_inst = {0};

i2c_hw_t *i2c0_hw = &i2c0_hw_inst;
i2c_inst_t i2c0_inst = {
    .hw = &i2c0_hw_inst,
    .restart_on_next = true
};


void i2c_register_transaction_handler(i2c_inst_t *i2c,
                                      const struct i2c_device_ops *ops,
                                      void *userdata)
{
    struct i2c_transaction_entry * e =
        malloc(sizeof(struct i2c_transaction_entry));
    e->userdata = userdata;
    e->ops = ops ;
    e->next = i2c->hw->transaction_handlers;

    i2c->hw->transaction_handlers = e;
}

uint i2c_init(i2c_inst_t *i2c, uint baudrate)
{
    return baudrate;
}

size_t i2c_get_write_available(i2c_inst_t *i2c)
{
    return 1;
}

size_t i2c_get_read_available(i2c_inst_t *i2c)
{
    return 0;
}

void i2c_write_raw_blocking(i2c_inst_t *i2c, const uint8_t *src, size_t len)
{
    abort();
}

void i2c_read_raw_blocking(i2c_inst_t *i2c, uint8_t *dst, size_t len)
{
    abort();
}

static void i2c_start_all(i2c_inst_t *i2c)
{
    struct i2c_hw *hw = i2c->hw;
    struct i2c_transaction_entry *handler = hw->transaction_handlers;
    while (handler) {
        handler->ops->start(handler->userdata);
        handler = handler->next;
    }
}

static void i2c_stop_all(i2c_inst_t *i2c)
{
    struct i2c_hw *hw = i2c->hw;
    struct i2c_transaction_entry *handler = hw->transaction_handlers;
    while (handler) {
        handler->ops->stop(handler->userdata);
        handler = handler->next;
    }
}

static int i2c_do_trans(i2c_hw_t *hw, uint8_t byte)
{
    int r = -1;
    struct i2c_transaction_entry *handler = hw->transaction_handlers;
    while (handler) {
        int tr = handler->ops->trans(handler->userdata, byte);
        handler = handler->next;
        if (tr==0) {
            r = 0;
        }
    }
    return r;
}
int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop)
{
    int r = -1;
    struct i2c_hw *hw = i2c->hw;

    if (i2c->restart_on_next)
        i2c_start_all(i2c);

    r = i2c_do_trans(hw, (addr & 0xFE) );

    if (r==0) {
        do {
            r = i2c_do_trans(hw, *src);
            src++;
            len--;
            if (r<0)
                break;
        } while (len);
    }

    if (nostop) {
        i2c->restart_on_next = false;
    } else {
        i2c_stop_all(i2c);
        i2c->restart_on_next = true;
    }
    if (r<0)
        return PICO_ERROR_GENERIC;
    return r;
}

int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop)
{
    i2c_start_all(i2c);
    i2c_stop_all(i2c);
    return -1;
}
