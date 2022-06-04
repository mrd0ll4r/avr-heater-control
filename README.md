# avr-heater-control

A project that uses an ATmega8 to control a bunch of relays used for our heating.

Our setup consists of three heating elements with 500W each, sequentially inside an air duct, heating incoming air.
The three elements are controlled by one relay each.
We use very slow PWM, with a period of ten seconds and duty cycle increments of one second.
The elements are maxed out sequentially, i.e., 50% total heating is achieved by having the first element on permanently, the second 50% of the time, and the third one not at all.

There is a connection between this MCU and the one [controlling the ventilation](../avr-fan-control).
The heating is only activated if the other MCU indicates the incoming ventilation is running.
This connection is usually pulled up, and driven low by the ventilation MCU while the ventilation is running.

Additionally, there are two relays which have no use (yet).

All of this is controlled via I2C.
One status byte indicates whether there was a watchdog reset and whether heating is disabled due to the ventilation not running.
The other byte is used to set the heating level.

## License

MIT, but would be nice if you would link back here.
