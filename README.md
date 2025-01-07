# i2c-lcd pico  ![Build](https://img.shields.io/badge/Build-Passing-green)
A simple library for managing character LCD's over the I2C bus on a Raspberry Pi Pico.


## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Documentation](#documentation)
- [Upcoming](#upcoming)

# Features
- Single header file
- Generic `print(string)` method
- Built to dynamically support display sizes

# Installation

This project relies only on the Raspberry Pi Foundation's [C/C++ SDK](https://github.com/raspberrypi/pico-sdk). With the release of the 2.0.0 version, they also bundled all of their required toolchain components into the Visual Studio Code extension for the Pico. I strongly suggest you choose that route. It is still possible to build from the command line, even in Windows, but you will suffer for it. Use the extension and the debugger with your Pico. 

Once you have the extension installed, you can:
- Use an existing project or create a new one, and copy the header file into your project directory and link with `include "characterLCD.hpp"`.
- OR clone this repo, and attempt to import the directory into the VScode extension. I've personally had mixed results with this. It may or may not work, in the future I will narrow down exactly how to make this method work each time.

Once the header is in your project, the last step will be adding:
- `hardware_i2c`
- `hardware_clocks`
to your CmakeLists.txt file in the project directory, in the `target_link_libraries` arguments, with the project executable target. 

# Usage
After you've linked the .hpp header file, the object constructor can be used with no arguments to accept the class defaults. You can view the defaults in the header file in the class declaration, and change them to match your hardware and wiring arrangement. Alternatively, you can use the overloaded constructor which takes all unique arguments required at once. Up to you, there is no difference between the objects post-creation. 

For the time being, the generic `.print(string)` method is recommended for all cases in combination with `.moveCursor(<row>,0)` just before each print statement to tell the display where you would like to print. Just insert your row number (starting from zero) in place of `<row>`. In my spare time in the future I will roll out some more specific methods like those listed above to make this a bit more useful. For now, `.print(string)` handles a few cases.
- If the string is less than the row size, it will append white space in an internal buffer until it reaches row size, then pass the buffer off to be printed.
- If the string is equal to the row size, it does nothing to it and just passes it on to be printed.
- If the string is longer than the row, but this method was used, it will passively scroll over the string with a mock sliding-window effect. This does not use the display's ability to move the entire display over the memory contents, so it can do this by individual row and exceed the display's interal limit of 40 bytes per row. Other future methods will explicity call that internal sliding display option, however. 

# Documentation
## Class Members
### `int TxBytesSent` 
An explicit integer type only to serve as a holder for the return value of the Pico SDK's `i2c_write_blocking()` method. Should this method ever fail to send the bytes it was given, it will return the macro "PICO_ERROR_GENERIC", or -1. The SDK could stand to be more clear on that, so for simplicity's sake, the value is checked at `<1` (bytes sent) explicitly and not using the macro. The error check use `printf()` to log exact byte that failed to send, so if you experience problems, connect over the serial port at speed: 115200 and watch for output.

### `const uint8_t setEnableLow` 
Used to set the Enable pin to a low state after the data was sent with the pin in the high state. Enable in the low state indicates to the LCD that the transmission is complete and new data is ready to process. `setEnableLow` is set to 0b00001000 simply due to the behavior of the LCD backlight in combination with the i2c backpacks. Otherwise, this value could be left at 0x00, but after a pause in transmissions, the display backlight will darken.

### `displayMemoryIndex` 
This array contains, in order, the starting memory addresses of each row of a display, up to 4 rows. Note that this only works for displays up to 4 rows and 20 columns. For the displays with longer rows, such as the 4x40 display, the memory addressing is mirrored because it doesn't actually have enough space for each character. I do not recommend using this library for the 4x40 displays for the time being, unless you know what you're doing.


## Class Methods


### `void prepare_command(uint8_t data)`
### `void prepare_character(uint8_t data);`
### `inline void lcd_send_byte(const uint8_t mostNibble, const uint8_t leastNibble);`
### `void init_display();`


### `DisplayControl();`
### `DisplayControl(i2c_inst* I2C, uint8_t SDA, uint8_t SCL, uint8_t hardware_address, uint8_t display_rows, uint8_t display_columns);`
### `void print(string text);`
### `void flashLED();`
### `void moveCursor(uint8_t row, uint8_t column);`

# Upcoming
    - ![InDev](https://img.shields.io/badge/In_Development-red) `printFullRow()` Writes the full 40-byte length of a row in memory.
    - ![Build](https://img.shields.io/badge/Experimental-yellow) `printLong()` Unbound-size print method to scroll over text longer than 40 bytes.
    - ![InDev](https://img.shields.io/badge/In_Development-red) `printFormat()` Format text appearance on display with arguments.
    - ![InDev](https://img.shields.io/badge/In_Development-red) `printAdv()` Allows for writing anywhere in the display memory.

[//]: # (Feature table!)

| Feature Name | Status | Description |
| :-----------: | :-----------: | :-----------:
| `printFullRow()` | ![InDev](https://img.shields.io/badge/In_Development-red) | Writes the full 40-byte length of a row in memory. |
| `printLong()` | ![Build](https://img.shields.io/badge/Experimental-yellow) | Unbound-size print method to scroll over text longer than 40 bytes. |
| `printFormat()` | ![InDev](https://img.shields.io/badge/In_Development-red) | Format text appearance, may end up as a few methods. |
| `printAdv()` | ![InDev](https://img.shields.io/badge/In_Development-red) | Write anywhere in the display memory with start and stop bounds. |