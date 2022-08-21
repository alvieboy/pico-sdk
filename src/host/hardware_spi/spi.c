/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Alvaro Lopes
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "hardware/spi.h"
#include <stdlib.h>
#include <string.h>

typedef int (*spi_transaction_handler_t)(void*user, const uint8_t *in_data, uint8_t *out_data, uint len);

struct spi_transaction_entry
{
    spi_transaction_handler_t handler;
    void *userdata;
    struct spi_transaction_entry *next;
};

struct spi_hw
{
    struct spi_transaction_entry *transaction_handlers;
};

static spi_hw_t spi0_inst = {0};
spi_hw_t *spi0_hw = &spi0_inst;

void spi_register_transaction_handler(spi_inst_t *spi, spi_transaction_handler_t handler, void *userdata)
{
    struct spi_transaction_entry * e = malloc(sizeof(struct spi_transaction_entry));
    e->userdata = userdata;
    e->handler = handler;
    e->next = ((spi_hw_t*)spi)->transaction_handlers;
    ((spi_hw_t*)spi)->transaction_handlers = e;
}

void spi_reset(spi_inst_t *spi) {
}

void spi_unreset(spi_inst_t *spi) {
}

uint spi_init(spi_inst_t *spi, uint baudrate) {
    return baudrate;
}

void spi_deinit(spi_inst_t *spi) {
    spi_reset(spi);
}

uint spi_set_baudrate(spi_inst_t *spi, uint baudrate) {
    return baudrate;
}

uint spi_get_baudrate(const spi_inst_t *spi) {
    return 0;
}

void spi_set_format(spi_inst_t *spi, uint data_bits, spi_cpol_t cpol, spi_cpha_t cpha, __unused spi_order_t order) {
}

void spi_set_slave(spi_inst_t *spi, bool slave)
{
}

bool spi_is_writable(const spi_inst_t *spi) {
    return true;
}

/*! \brief Check whether a read can be done on SPI device
 *  \ingroup hardware_spi
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \return true if a read is possible i.e. data is present
 */
bool spi_is_readable(const spi_inst_t *spi) {
    return true;
}

/*! \brief Check whether SPI is busy
 *  \ingroup hardware_spi
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \return true if SPI is busy
 */
bool spi_is_busy(const spi_inst_t *spi) {
    return false;
}

int spi_write_read_blocking(spi_inst_t *spi, const uint8_t *src, uint8_t *dst, size_t len)
{
    struct spi_transaction_entry *e;
    spi_hw_t *hw = (spi_hw_t*)spi;

    for (e = hw->transaction_handlers; e; e = e->next)
    {
        size_t handled = e->handler( e->userdata, src, dst, len);
        if (handled>0) {
            return handled;
        }
    }

    return -1;
}

int spi_write_blocking(spi_inst_t *spi, const uint8_t *src, size_t len)
{
    struct spi_transaction_entry *e;
    spi_hw_t *hw = (spi_hw_t*)spi;

    for (e = hw->transaction_handlers; e; e = e->next)
    {
        size_t handled = e->handler( e->userdata, src, NULL, len);
        if (handled>0) {
            return handled;
        }
    }
    return -1;
}

int spi_read_blocking(spi_inst_t *spi, uint8_t repeated_tx_data, uint8_t *dst, size_t len)
{
    struct spi_transaction_entry *e;
    spi_hw_t *hw = (spi_hw_t*)spi;
    uint8_t *dummy = (uint8_t*)malloc(len);
    memset(dummy, repeated_tx_data, len);

    for (e = hw->transaction_handlers; e; e = e->next)
    {
        size_t handled = e->handler( e->userdata, dummy, dst, len);
        if (handled>0) {
            free(dummy);
            return handled;
        }
    }
    free(dummy);
    return -1;
}

// ----------------------------------------------------------------------------
// SPI-specific operations and aliases

// FIXME need some instance-private data for select() and deselect() if we are going that route

/*! \brief Write/Read half words to/from an SPI device
 *  \ingroup hardware_spi
 *
 * Write \p len halfwords from \p src to SPI. Simultaneously read \p len halfwords from SPI to \p dst.
 * Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
 *
 * \note SPI should be initialised with 16 data_bits using \ref spi_set_format first, otherwise this function will only read/write 8 data_bits.
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \param src Buffer of data to write
 * \param dst Buffer for read data
 * \param len Length of BOTH buffers in halfwords
 * \return Number of halfwords written/read
*/
int spi_write16_read16_blocking(spi_inst_t *spi, const uint16_t *src, uint16_t *dst, size_t len);

/*! \brief Write to an SPI device
 *  \ingroup hardware_spi
 *
 * Write \p len halfwords from \p src to SPI. Discard any data received back.
 * Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
 *
 * \note SPI should be initialised with 16 data_bits using \ref spi_set_format first, otherwise this function will only write 8 data_bits.
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \param src Buffer of data to write
 * \param len Length of buffers
 * \return Number of halfwords written/read
*/
int spi_write16_blocking(spi_inst_t *spi, const uint16_t *src, size_t len);

/*! \brief Read from an SPI device
 *  \ingroup hardware_spi
 *
 * Read \p len halfwords from SPI to \p dst.
 * Blocks until all data is transferred. No timeout, as SPI hardware always transfers at a known data rate.
 * \p repeated_tx_data is output repeatedly on TX as data is read in from RX.
 * Generally this can be 0, but some devices require a specific value here,
 * e.g. SD cards expect 0xff
 *
 * \note SPI should be initialised with 16 data_bits using \ref spi_set_format first, otherwise this function will only read 8 data_bits.
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \param repeated_tx_data Buffer of data to write
 * \param dst Buffer for read data
 * \param len Length of buffer \p dst in halfwords
 * \return Number of halfwords written/read
 */
int spi_read16_blocking(spi_inst_t *spi, uint16_t repeated_tx_data, uint16_t *dst, size_t len);

/*! \brief Return the DREQ to use for pacing transfers to/from a particular SPI instance
 *  \ingroup hardware_spi
 *
 * \param spi SPI instance specifier, either \ref spi0 or \ref spi1
 * \param is_tx true for sending data to the SPI instance, false for receiving data from the SPI instance
 */
