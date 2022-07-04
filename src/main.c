/*
* main.c
*
* Created: 02-Jul-22 2:39:59 PM
* Author : Leo
*/

#ifndef F_CPU
# define F_CPU 8000000UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "twislave.h"
#include "relay.h"

// The pin on which the ventilation-active line from the other MCU is connected.
// This pin is HIGH if the ventilation is _off_, and by default.
// It's driven LOW by the other MCU if the air-in ventilation is on.
#define VENTILATION_ACTIVE_PIN (1 << PINC3)

// A tick-counter driven by a timer.
// This counts modulo some number.
volatile uint8_t timer_ticks;

// Initialize outputs.
static void io_init() {
    io_init_relays();

    // Mark the ventilation-active pin as an input.
    DDRC &= ~VENTILATION_ACTIVE_PIN;
}

void init_timer() {
    // See http://www.8bit-era.cz/arduino-timer-interrupts-calculator.html
    // Or https://www.arduinoslovakia.eu/application/timer-calculator

    // ! Make sure this fits with PWM_TICK_FREQUENCY_HZ !

    TCCR1A = 0; // set entire TCCR1A register to 0
    TCCR1B = 0; // same for TCCR1B
    TCNT1 = 0; // initialize counter value to 0
    // set compare match register for 1 Hz increments
    OCR1A = 31249; // = 8000000 / (256 * 1) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS12, CS11 and CS10 bits for 256 prescaler
    TCCR1B |= (1 << CS12);
    // enable timer compare interrupt
    TIMSK |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) // every 1s
{
    timer_ticks = (timer_ticks + 1) % (PWM_PERIOD_SECONDS * PWM_TICK_FREQUENCY_HZ);
}

int main(void) {
    // Current heater level and state of the two extra relays.
    uint8_t heater_level = 0;
    uint8_t relay_4 = 0;
    uint8_t relay_5 = 0;

    // Set inputs/outputs.
    io_init();

    // Clear I2C register buffer.
    for (int i = 0; i < i2c_buffer_size; i++) {
        i2cdata[i] = 0;
    }
    i2cdata[1] = heater_level;

    if (MCUCSR & (1 << WDRF)) {
        // A reset by the watchdog has occurred.
        // Signal this by clearing bit 2 in status byte.
        i2cdata[0] &= ~(I2C_BIT_WDT_RESET);
        // Clear flag for next time.
        MCUCSR &= ~(1 << WDRF);
    } else {
        // 1 means no WDT reset occurred.
        i2cdata[0] |= I2C_BIT_WDT_RESET;
    }

    // Enable watchdog to restart if we didn't reset it for 2 seconds.
    wdt_enable(WDTO_2S);
    // Enable I2C.
    init_twi_slave(I2C_SLAVE_ADDRESS);
    // Init timer
    init_timer();

    // Enable Interrupts.
    sei();

    while (1) {
        // Reset watchdog timer.
        wdt_reset();

        // Get heater level from I2C.
        {
            cli();

            // Read I2C data.
            uint8_t i2c_data_byte = i2cdata[1];
            // Check if the heater level is valid
            if ((i2c_data_byte & I2C_HEATER_BIT_MASK) < NUM_HEATER_LEVELS) {
                // If yes: extract values.
                heater_level = (i2c_data_byte & I2C_HEATER_BIT_MASK);
                relay_4 = (i2c_data_byte & I2C_RELAY_4_MASK);
                relay_5 = (i2c_data_byte & I2C_RELAY_5_MASK);
            } else {
                // If not: ignore I2C input, write current state to I2C buffer.
                i2c_data_byte = heater_level;
                if (relay_4)
                    i2c_data_byte |= I2C_RELAY_4_MASK;
                if (relay_5)
                    i2c_data_byte |= I2C_RELAY_5_MASK;
                i2cdata[1] = i2c_data_byte;
            }

            sei();
        }

        // Drive relays, depending on whether ventilation is enabled or not.
        if (!(PINC & VENTILATION_ACTIVE_PIN)) {
            // If the pin is low, ventilation is on, and we're good.
            i2cdata[0] &= ~I2C_BIT_HEATER_DISABLED;
            drive_relays(heater_level, relay_4, relay_5, timer_ticks);
        } else {
            // If the pin is high, the ventilation is off, and thus we won't heat.
            // We still set the free relays.
            i2cdata[0] |= I2C_BIT_HEATER_DISABLED;
            drive_relays(0, relay_4, relay_5, 0);
        }

        _delay_ms(10);
    }
}