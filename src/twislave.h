/*
 * twislave.h
 *
 * Created: 22-Jul-21 5:42:55 PM
 * Author: Leo
 * Source: https://rn-wissen.de/wiki/index.php/TWI_Slave_mit_avr-gcc
 */

#ifndef TWISLAVE_H_
#define TWISLAVE_H_

#include <util/twi.h>
#include <avr/interrupt.h>
#include <stdint.h>

// I2C data buffer size.
// One byte status register plus one byte with 6 bits heater level and two bits for the extra two relays.
#define i2c_buffer_size 2

// Masks for the I2C data byte.
// Rightmost (low) six bits are for heater level.
#define I2C_HEATER_BIT_MASK 0x3F
// Leftmost (high) two bits are for the free relays.
#define I2C_RELAY_4_MASK 0x40
#define I2C_RELAY_5_MASK 0x80

// I2C slave address.
#define I2C_SLAVE_ADDRESS 0x23

// Positions in the I2C status byte.
#define I2C_BIT_HEATER_DISABLED 0x01
#define I2C_BIT_WDT_RESET 0x02

volatile uint8_t i2cdata[i2c_buffer_size];

// Initializes TWI with the given address.
// I2C addresses are 7 bytes, init_twi_slave will shift the given address by one bit.
void init_twi_slave(uint8_t addr);

// Don't change below here

// Old versions of AVR-GCC do interrupt stuff differently.
#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
#error "This library requires AVR-GCC 3.4.5 or later, update to newer AVR-GCC compiler"
#endif

//Schutz vor unsinnigen Buffergroessen
#if (i2c_buffer_size > 254)
#error buffer size needs to be less than 254.
#endif

#if (i2c_buffer_size < 2)
#error buffer size needs to be at least two bytes.
#endif

#endif /* TWISLAVE_H_ */