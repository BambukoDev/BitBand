# What it is?
**BitBand** is a portable mini-computer that you put on your arm, inspired by Pip-Boy from Fallout games.

# How it works?
The core of the project stands in the Raspberry Pi Pico W microcontroller, which is responsible for communication with the rest of the components using the FreeRTOS library.
The device was equipped with 5 buttons and a rotary encoder, which allow to control the device.
The project was written in C and C++.

## Components
- Raspberry Pi Pico W
- 20x4 LCD display
- 5 buttons
- 20 step rotary encoder
- MicroSD card reader
- RTC clock
- accelerometer

## What functionality does it have?
- Ability to be programmed from the PC (updates to software)
- In future most of the hard-coded functionality will be moved to .lua files
### Main Menu
- Displaying time, temperature and voltage of the battery
### Selection Menu
- Allows to choose one of the many options using the encoder and buttons

### Mobile app
- Currently in progress...

## What you can use it for?

# History of the project
