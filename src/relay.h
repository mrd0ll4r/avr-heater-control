//
// Created by leo on 29/05/22.
//

#ifndef FAN_CONTROL_RELAY_H
#define FAN_CONTROL_RELAY_H

#include <avr/io.h>

// Heater relays
#define RELAY_1 (1 << PORTB1)
#define RELAY_2 (1 << PORTB2)
#define RELAY_3 (1 << PORTB3)

// Non-heater relays
#define RELAY_4 (1 << PORTB4)
#define RELAY_5 (1 << PORTB5)

#define HEATER_RELAY_MASK (RELAY_1 | RELAY_2 | RELAY_3 )
#define NON_HEATER_RELAY_MASK (RELAY_4 | RELAY_5)

#define PWM_TICK_FREQUENCY_HZ 1
#define PWM_PERIOD_SECONDS 10
// The number of heater levels, defined by the PWM cycle.
#define NUM_HEATER_LEVELS (PWM_TICK_FREQUENCY_HZ * PWM_PERIOD_SECONDS * 3 + 1)

static uint8_t relay_pattern_heater_inner(uint8_t level, uint8_t pwm_counter) {
    uint8_t out = 0;

    // We calculate whether relays 1,2,3 should be on during this pwm step.
    // Assuming 10 seconds PWM cycle, with a timer that ticks once a second,
    // we end up with 30 levels.
    // If level is <= 10, the level directly corresponds to how long the first
    // relay is active.
    // E.g.: level = 6 => 6s on, 4s off.
    // If level is <= 20, the first relay is on permanently and (level-10)
    // dictates how long the second relay is active.
    // E.g.: level = 13 => relay 1 always on, relay 2 on for 3s, off for 7s.
    // The same goes for level <= 30 and the third relay.

    if (level > 20) {
        out |= (RELAY_1 | RELAY_2);
        uint8_t remainder = level - 20;
        if (remainder > pwm_counter) {
            out |= RELAY_3;
        }
    } else if (level > 10) {
        out |= RELAY_1;
        uint8_t remainder = level - 10;
        if (remainder > pwm_counter) {
            out |= RELAY_2;
        }
    } else {
        if (level > pwm_counter) {
            out |= RELAY_1;
        }
    }

    return out;
}

// Calculates pin states for the heater relays.
// The level must be < NUM_HEATER_LEVELS.
// The pwm_step must be < (PWM_TICK_FREQUENCY_HZ * PWM_PERIOD_SECONDS).
// Internally, this PWMs the three relays.
// One relay will be saturated before the next will be turned on.
static uint8_t relay_pattern_heater(uint8_t level, uint8_t pwm_step) {
    return (relay_pattern_heater_inner(level, pwm_step) & HEATER_RELAY_MASK);
}

// Drives the relay pins with the given configuration.
// The heater_level and pwm_step arguments determine the state of the PWM'ed heater pins.
// The other two parameters can be used to control the remaining two relays.
void drive_relays(uint8_t heater_level, uint8_t relay_4, uint8_t relay_5, uint8_t pwm_step) {
    uint8_t pins = relay_pattern_heater(heater_level, pwm_step);
    if (relay_4) {
        pins |= RELAY_4;
    }
    if (relay_5) {
        pins |= RELAY_5;
    }

    PORTB = pins;
}

// Configures the pins used by the heating system, plus the two additional relays.
void io_init_relays() {
    PORTB = relay_pattern_heater(0, 0);

    DDRB |= ((1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4) | (1 << DDB5));
}

#endif //FAN_CONTROL_RELAY_H
