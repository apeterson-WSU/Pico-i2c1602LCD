// MIT License

// Copyright (c) 2024 Alex P

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include <string>

using std::string;
using std::to_string;

struct Messages {
    // Some test examples for printing to a display
string microControllerTitle = "RaspberryPi Pico";
string libName = "I2C LCD Library";
string lang = "written in C/C++";
string empty = " ";
int counter = 0;
};

enum class Commands : uint8_t {
    Clear_Display       = 0x01, 
    Return_Home         = 0x02, 
    Set_Entry_Mode      = 0x04,
    Set_Display_Control = 0x08,
    Cursor_Shift        = 0x10,
    Set_Function        = 0x20,
    Set_DDRAM_Address   = 0x80
};

enum class DisplayEntryMode : uint8_t {
    Entry_Left              = 0x02,
    Entry_Shift_Decrement   = 0x00
};

enum class PowerState : uint8_t {
    Display_On  = 0x04,
    Display_Off = 0x00,
    Cursor_On   = 0x02,
    Cursor_Off  = 0x00,
    Blink_On    = 0x01,
    Blink_Off   = 0x00
};

enum class DisplayShift : uint8_t {
    Display_Shift_Right = 0x1C,
    Display_Shift_Left  = 0x18,
    Move_Cursor         = 0x00,
    Move_Right          = 0x04,
    Move_Left           = 0x00
};

enum class Function : uint8_t{
    Four_Bit_Mode   = 0x00,
    Two_Line_Mode   = 0x08,
    FiveXEight_Grid = 0x00
};

enum class Backlight : uint8_t {
    Backlight       = 0x08,
    Backlight_off   = 0x00
};

enum class Flags : uint8_t {
    Set_Enable_High = 0x04,
    Set_Enable_Low  = 0x00,
    Read            = 0x02,
    Write           = 0x00,
    Register_Select = 0x01,
    Command         = 0x00,
    Character       = 0x01
};

class DisplayControl {
    /// External controls
    const uint8_t LEDgpio = 25;

    /// Required class members
        //See Documentation in README for descriptions

    int TxBytesSent = 0;
    const uint8_t setEnableLow = 
      static_cast<uint8_t>(Flags::Set_Enable_Low)
    | static_cast<uint8_t>(Backlight::Backlight);  // 0x00 | 0x08
    uint8_t currRowWrite{0};
    uint8_t currColumnWrite{0};

    /// Memory range of character displays, 40 bytes capacity for each row
    /// row 0 memory addr begins at 0x00
    /// row 1 memory addr begins at 0x40
    /// row 2 memory addr begins at 0x14
    /// row 3 memory addr begins at 0x54
    const uint8_t displayMemoryIndex[4] = {0x00, 0x40, 0x14, 0x54};
    
    // Write buffers
    char writeBuffer[40] = {};

    /// For constructing objects only:
    /// Defaults: 
    /// Pico's default to the i2c0 instance on:
    /// GPIO4 (physical pin 6) SDA - Serial Data Connection 
    /// GPIO5 (physical pin 7) SCL - Serial Clock Connection
    /// 1602A address is 0x27
    ///
    /// See Raspberry Pi Pico pinout diagrams for additional
    /// information and configuration options.

    i2c_inst* I2C{i2c0};
    uint8_t SDA{4}; 
    uint8_t SCL{5}; 
    uint8_t hardware_address{0x27};
    uint8_t rows{2};
    uint8_t columns{16};

    // Private Methods
    void prepare_command(uint8_t data);
    void prepare_character(uint8_t data);
    inline void lcd_send_byte(const uint8_t mostNibble, const uint8_t leastNibble);
    void init_display();

    // Public Methods
    public:
    DisplayControl();
    DisplayControl(i2c_inst* I2C, uint8_t SDA, uint8_t SCL, uint8_t hardware_address, uint8_t display_rows, uint8_t display_columns);
    void print(string text);
    void flashLED();
    void moveCursor(uint8_t row, uint8_t column);
};


DisplayControl::DisplayControl(){
    // Use class defaults

    // Raspberry Pi Pico SDK i2c API's
    i2c_init(I2C, 100'000); // 100kbps
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
    gpio_init(LEDgpio); // LED pin
    gpio_set_dir(LEDgpio, GPIO_OUT);

     // Write white space over the write buffer
     for(const character: writeBuffer){
        writeBuffer[character] = ' ';
    }

    // Initialize display connection
    init_display();
}

DisplayControl::DisplayControl(i2c_inst* I2C, uint8_t SDA, uint8_t SCL, uint8_t hardware_address, uint8_t display_rows, uint8_t display_columns){

    this->I2C = I2C;
    this->SDA = SDA;
    this->SCL = SCL;
    this->hardware_address = hardware_address;
    this->rows = display_rows;
    this->columns = display_columns;

    // Raspberry Pi Pico SDK i2c API's
    i2c_init(I2C, 100'000); // 100kbps
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
    gpio_init(LEDgpio); // LED pin
    gpio_set_dir(LEDgpio, GPIO_OUT);

    // Write white space over the write buffer
    for(const character: writeBuffer){
        writeBuffer[character] = ' ';
    }
    
    // Initialize display connection
    init_display();
}

void DisplayControl::prepare_command(uint8_t data){
    const uint8_t mostNibble = ((data & 0xF0) 
    | static_cast<uint8_t>(Backlight::Backlight))           // 0000 1000
    | static_cast<uint8_t>(Flags::Set_Enable_High);         // 0000 0100
    
    // Shift and parse lower half of the character
    const uint8_t leastNibble = (((data << 4) & 0xF0)
    | static_cast<uint8_t>(Backlight::Backlight))           
    | static_cast<uint8_t>(Flags::Set_Enable_High);         

   lcd_send_byte(mostNibble, leastNibble);
}

void DisplayControl::prepare_character(uint8_t data){
    const uint8_t mostNibble = data & 0xF0
    | static_cast<uint8_t>(Backlight::Backlight)            // 0000 1000
    | static_cast<uint8_t>(Flags::Set_Enable_High)          // 0000 0100
    | static_cast<uint8_t>(Flags::Character);               // 0000 0001

    // Shift and parse lower half of the character
    const uint8_t leastNibble = (data << 4) & 0xF0
    | static_cast<uint8_t>(Backlight::Backlight)
    | static_cast<uint8_t>(Flags::Set_Enable_High)
    | static_cast<uint8_t>(Flags::Character);

    lcd_send_byte(mostNibble, leastNibble);
}

inline void DisplayControl::lcd_send_byte(const uint8_t mostNibble, const uint8_t leastNibble){
    
    /// To view error responses, connect via usb serial port, speed = 115200.

    // Send over i2c bus, maximum tested baud rate is 100kbps.
    TxBytesSent = i2c_write_blocking(I2C, hardware_address, &mostNibble, 1, false);
    if(TxBytesSent < 1) printf("Byte 0x%02X\n failed to send!\n", mostNibble);
    sleep_us(600);
    
    TxBytesSent = i2c_write_blocking(I2C, hardware_address, &setEnableLow, 1, false);
    sleep_us(600);

    TxBytesSent = i2c_write_blocking(I2C, hardware_address, &leastNibble, 1, false);
    if(TxBytesSent < 1) printf("Byte 0x%02X\n failed to send!\n", leastNibble); 
    sleep_us(600);

    TxBytesSent = i2c_write_blocking(I2C, hardware_address, &setEnableLow, 1, false);
    sleep_us(600);
}

void DisplayControl::init_display(){
    printf("Display init started\n");
    prepare_command(0x03);
    sleep_ms(5);
    prepare_command(0x03);
    sleep_ms(5);
    prepare_command(0x03);
    sleep_us(150);
    
    // 4-bit mode - required for i2c communciation 
    prepare_command(0x02);
    printf("(0x03)\n(0x03)\n(0x03)\n(0x02)\nEntered into 4-bit operation mode...\n\n");
    
    // Configure the LCD with 2-line mode, 5x8 dots
    prepare_command(static_cast<uint8_t>(Commands::Set_Function) | static_cast<uint8_t>(Function::Two_Line_Mode));
    printf("(0x28) : Display Function: 2 line mode configured...\n");

    // Entry Mode set
    prepare_command(static_cast<uint8_t>(Commands::Set_Entry_Mode) | static_cast<uint8_t>(DisplayEntryMode::Entry_Left));
    printf("(0x06) : Display Entry Mode: Entry Left configured...\n");
    
    // Clear Display
    prepare_command(static_cast<uint8_t>(Commands::Clear_Display));
    printf("(0x01) : Display cleared...\n");
    sleep_ms(2);

    // Turn on Backlight
    prepare_command(static_cast<uint8_t>(Backlight::Backlight));
    printf("(0x08) : Backlight enabled...\n");

    // Return home
    prepare_command(static_cast<uint8_t>(Commands::Return_Home));
    printf("(0x02) : Cursor returned to home position...\n");

    // Turn on the display, disable cursor, and turn off blinking
    prepare_command(static_cast<uint8_t>(Commands::Set_Display_Control) | static_cast<uint8_t>(PowerState::Display_On));
    printf("(0x0C) : Display on, cursor disabled. Ready to operate. \n");
}

void DisplayControl::flashLED(){
    for(uint8_t i = 0; i < 10; ++i){
        gpio_put(LEDgpio, 1);
        sleep_ms(50);
        gpio_put(LEDgpio, 0);
        sleep_ms(50);
    }
}

void DisplayControl::print(string text){
    int strSize = text.size();
    const uint8_t displayWidth = this->columns; 
    // write the string to a buffer of fixed size to prevent bad cursor behavior 
    if(strSize == displayWidth){
        for (int position{0}; position < 16; ++position){
            writeBuffer[position] = text[position];
        }
    }
    else if(strSize < displayWidth){ 
        int position = 0;
        for (; position < strSize; ++position){
            writeBuffer[position] = text[position];
        }
    // fill trailing empty positions with white space
        while(position < displayWidth){
            writeBuffer[position] = ' '; 
            ++position;
        }
    }
    else{   
    // string is longer than the display
    // iterate over string with small time delay after, then increment 1 index and repeat to scroll the text
    uint8_t iter = strSize - displayWidth;

    for(uint8_t i = 0; i <= iter; ++i){
        moveCursor(currRowWrite,0x00);
        for(uint8_t j = 0; j < displayWidth; ++j){
            prepare_character(text[i+j]);
        }
        sleep_ms(500);
    }
    return;
}

    // Safe strings can now be parsed from a fixed 16-char-length buffer
    for (char character: writeBuffer)
    {
        prepare_character(character);
    }
}

void DisplayControl::shiftDisplayLeft(){
    prepare_command(static_cast<uint8_t>(DisplayShift::Display_Shift_Left)); // 0001 1000
}

void DisplayControl::shiftDisplayRight(){
    prepare_command(static_cast<uint8_t>(DisplayShift::Display_Shift_Right)); // 0001 1100
}

void DisplayControl::moveCursor(uint8_t row, uint8_t column){
    if(row > (this->rows - 1) || column > (this->columns - 1)) return;
    currRowWrite = row;       // important note: these will only reflect where writing *started*, 
    currColumnWrite = column; // a display will auto-increment its cursor after a write to memory.
    prepare_command(static_cast<uint8_t>(Commands::Set_DDRAM_Address) | displayMemoryIndex[row] + column);
}
    ///////////////////////////////////////////////////////////////////////////////////////////////
    ///                     Documentation and summary information                               ///
    ///////////////////////////////////////////////////////////////////////////////////////////////

    /// Written by Alexander Peterson                                                       12/2024
    ///
    ///      //||   ////////
    ///     // ||   //     //
    ///    //  ||   //     //
    ///   //===||   ////////
    ///  //    ||   //
    /// //     ||   //
    ///
    ///
    ///