/*
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Alvaro Lopes
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_I2C_H
#define _HARDWARE_I2C_H

#include "pico.h"
#include "pico/time.h"

// PICO_CONFIG: PARAM_ASSERTIONS_ENABLED_I2C, Enable/disable assertions in the I2C module, type=bool, default=0, group=hardware_i2c
#ifndef PARAM_ASSERTIONS_ENABLED_I2C
#define PARAM_ASSERTIONS_ENABLED_I2C 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** \file hardware/i2c.h
 *  \defgroup hardware_i2c hardware_i2c
 *
 * I2C Controller API
 *
 * The I2C bus is a two-wire serial interface, consisting of a serial data line SDA and a serial clock SCL. These wires carry
 * information between the devices connected to the bus. Each device is recognized by a unique 7-bit address and can operate as
 * either a “transmitter” or “receiver”, depending on the function of the device. Devices can also be considered as masters or
 * slaves when performing data transfers. A master is a device that initiates a data transfer on the bus and generates the
 * clock signals to permit that transfer. The first byte in the data transfer always contains the 7-bit address and
 * a read/write bit in the LSB position. This API takes care of toggling the read/write bit. After this, any device addressed
 * is considered a slave. 
 *
 * This API allows the controller to be set up as a master or a slave using the \ref i2c_set_slave_mode function.
 *
 * The external pins of each controller are connected to GPIO pins as defined in the GPIO muxing table in the datasheet. The muxing options
 * give some IO flexibility, but each controller external pin should be connected to only one GPIO.
 *
 * Note that the controller does NOT support High speed mode or Ultra-fast speed mode, the fastest operation being fast mode plus
 * at up to 1000Kb/s.
 *
 * See the datasheet for more information on the I2C controller and its usage.
 *
 * \subsection i2c_example Example
 * \addtogroup hardware_i2c
 * \include bus_scan.c
 */

typedef struct i2c_hw i2c_hw_t;

typedef struct i2c_inst i2c_inst_t;

// PICO_CONFIG: PICO_DEFAULT_I2C, Define the default I2C for a board, min=0, max=1, group=hardware_i2c
// PICO_CONFIG: PICO_DEFAULT_I2C_SDA_PIN, Define the default I2C SDA pin, min=0, max=29, group=hardware_i2c
// PICO_CONFIG: PICO_DEFAULT_I2C_SCL_PIN, Define the default I2C SCL pin, min=0, max=29, group=hardware_i2c

/** The I2C identifiers for use in I2C functions.
 *
 * e.g. i2c_init(i2c0, 48000)
 *
 *  \ingroup hardware_i2c
 * @{
 */
extern i2c_inst_t i2c0_inst;
extern i2c_inst_t i2c1_inst;

#define i2c0 (&i2c0_inst) ///< Identifier for I2C HW Block 0
#define i2c1 (&i2c1_inst) ///< Identifier for I2C HW Block 1

#define i2c_default i2c0

/** @} */

// ----------------------------------------------------------------------------
// Setup

/*! \brief   Initialise the I2C HW block
 *  \ingroup hardware_i2c
 *
 * Put the I2C hardware into a known state, and enable it. Must be called
 * before other functions. By default, the I2C is configured to operate as a
 * master.
 *
 * The I2C bus frequency is set as close as possible to requested, and
 * the actual rate set is returned
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param baudrate Baudrate in Hz (e.g. 100kHz is 100000)
 * \return Actual set baudrate
 */
uint i2c_init(i2c_inst_t *i2c, uint baudrate);

/*! \brief   Disable the I2C HW block
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 *
 * Disable the I2C again if it is no longer used. Must be reinitialised before
 * being used again.
 */
void i2c_deinit(i2c_inst_t *i2c);

/*! \brief  Set I2C baudrate
 *  \ingroup hardware_i2c
 *
 * Set I2C bus frequency as close as possible to requested, and return actual
 * rate set.
 * Baudrate may not be as exactly requested due to clocking limitations.
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param baudrate Baudrate in Hz (e.g. 100kHz is 100000)
 * \return Actual set baudrate
 */
uint i2c_set_baudrate(i2c_inst_t *i2c, uint baudrate);

/*! \brief  Set I2C port to slave mode
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param slave true to use slave mode, false to use master mode
 * \param addr If \p slave is true, set the slave address to this value
 */
void i2c_set_slave_mode(i2c_inst_t *i2c, bool slave, uint8_t addr);

// ----------------------------------------------------------------------------
// Generic input/output

struct i2c_inst {
    i2c_hw_t *hw;
    bool restart_on_next;
};


/*! \brief Attempt to write specified number of bytes to address, blocking until the specified absolute time is reached.
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to write to
 * \param src Pointer to data to send
 * \param len Length of data in bytes to send
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \param until The absolute time that the block will wait until the entire transaction is complete. Note, an individual timeout of
 *           this value divided by the length of data is applied for each byte transfer, so if the first or subsequent
 *           bytes fails to transfer within that sub timeout, the function will return with an error.
 *
 * \return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present, or PICO_ERROR_TIMEOUT if a timeout occurred.
 */
int i2c_write_blocking_until(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, absolute_time_t until);

/*! \brief  Attempt to read specified number of bytes from address, blocking until the specified absolute time is reached.
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to read from
 * \param dst Pointer to buffer to receive data
 * \param len Length of data in bytes to receive
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \param until The absolute time that the block will wait until the entire transaction is complete.
 * \return Number of bytes read, or PICO_ERROR_GENERIC if address not acknowledged, no device present, or PICO_ERROR_TIMEOUT if a timeout occurred.
 */
int i2c_read_blocking_until(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, absolute_time_t until);

/*! \brief Attempt to write specified number of bytes to address, with timeout
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to write to
 * \param src Pointer to data to send
 * \param len Length of data in bytes to send
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \param timeout_us The time that the function will wait for the entire transaction to complete. Note, an individual timeout of
 *           this value divided by the length of data is applied for each byte transfer, so if the first or subsequent
 *           bytes fails to transfer within that sub timeout, the function will return with an error.
 *
 * \return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present, or PICO_ERROR_TIMEOUT if a timeout occurred.
 */
static inline int i2c_write_timeout_us(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint timeout_us) {
    absolute_time_t t = make_timeout_time_us(timeout_us);
    return i2c_write_blocking_until(i2c, addr, src, len, nostop, t);
}

int i2c_write_timeout_per_char_us(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop, uint timeout_per_char_us);

/*! \brief  Attempt to read specified number of bytes from address, with timeout
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to read from
 * \param dst Pointer to buffer to receive data
 * \param len Length of data in bytes to receive
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \param timeout_us The time that the function will wait for the entire transaction to complete
 * \return Number of bytes read, or PICO_ERROR_GENERIC if address not acknowledged, no device present, or PICO_ERROR_TIMEOUT if a timeout occurred.
 */
static inline int i2c_read_timeout_us(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us) {
    absolute_time_t t = make_timeout_time_us(timeout_us);
    return i2c_read_blocking_until(i2c, addr, dst, len, nostop, t);
}

int i2c_read_timeout_per_char_us(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_per_char_us);

/*! \brief Attempt to write specified number of bytes to address, blocking
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to write to
 * \param src Pointer to data to send
 * \param len Length of data in bytes to send
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \return Number of bytes written, or PICO_ERROR_GENERIC if address not acknowledged, no device present.
 */
int i2c_write_blocking(i2c_inst_t *i2c, uint8_t addr, const uint8_t *src, size_t len, bool nostop);

/*! \brief  Attempt to read specified number of bytes from address, blocking
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param addr 7-bit address of device to read from
 * \param dst Pointer to buffer to receive data
 * \param len Length of data in bytes to receive
 * \param nostop  If true, master retains control of the bus at the end of the transfer (no Stop is issued),
 *           and the next transfer will begin with a Restart rather than a Start.
 * \return Number of bytes read, or PICO_ERROR_GENERIC if address not acknowledged or no device present.
 */
int i2c_read_blocking(i2c_inst_t *i2c, uint8_t addr, uint8_t *dst, size_t len, bool nostop);


/*! \brief Determine non-blocking write space available
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \return 0 if no space is available in the I2C to write more data. If return is nonzero, at
 * least that many bytes can be written without blocking.
 */
size_t i2c_get_write_available(i2c_inst_t *i2c);

/*! \brief Determine number of bytes received
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \return 0 if no data available, if return is nonzero at
 * least that many bytes can be read without blocking.
 */
size_t i2c_get_read_available(i2c_inst_t *i2c);

/*! \brief Write direct to TX FIFO
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param src Data to send
 * \param len Number of bytes to send
 *
 * Writes directly to the I2C TX FIFO which is mainly useful for
 * slave-mode operation.
 */
void i2c_write_raw_blocking(i2c_inst_t *i2c, const uint8_t *src, size_t len);

/*! \brief Read direct from RX FIFO
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param dst Buffer to accept data
 * \param len Number of bytes to read
 *
 * Reads directly from the I2C RX FIFO which is mainly useful for
 * slave-mode operation.
 */
void i2c_read_raw_blocking(i2c_inst_t *i2c, uint8_t *dst, size_t len);

/*! \brief Return the DREQ to use for pacing transfers to/from a particular I2C instance
 *  \ingroup hardware_i2c
 *
 * \param i2c Either \ref i2c0 or \ref i2c1
 * \param is_tx true for sending data to the I2C instance, false for receiving data from the I2C instance
 */
uint i2c_get_dreq(i2c_inst_t *i2c, bool is_tx);


/*
 Host mode
 */
#define I2C_DEVICE_NOACK (-1)

struct i2c_device_ops
{
    void (*start)(void *);
    void (*stop)(void *);
    int (*trans)(void *, uint8_t byte);
};

void i2c_register_transaction_handler(i2c_inst_t *spi,
                                      const struct i2c_device_ops *ops,
                                      void *userdata);

#ifdef __cplusplus
}
#endif

#endif
