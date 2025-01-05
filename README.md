# i2c-lcd pico 
A simple library for managing character LCD's over the I2C bus on a Raspberry Pi Pico.

## Table of Contents
- [Installation](#installation)
- [Usage](#usage)
- [Documentation](#documentation)

# Features
- Single header file
- Generic `print` method
    - Alternatives:
    - `printFullRow()`  (In development) 40-byte bound print method. (For combination with display shift methods.)
    - `printLong()`     ![Build](https://img.shields.io/badge/Experimental-yellow) Unbound-size print method to scroll over text longer than 40 bytes.
    - `printFormat()`   (In development) Modifies character-alignment or appearance on display. Options in Documentation.
    - `printAdv()`      (In development) Allows for writing anywhere in the display memory. Stops at the end of a given row.
# Documentation

[C/C++ SDK](https://github.com/raspberrypi/pico-sdk)