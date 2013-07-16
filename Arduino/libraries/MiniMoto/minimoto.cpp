#include <Arduino.h>
#include "minimoto.h"

// The address of the part is set by a jumper on the board. 
//  See datasheet or schematic for details; default is 0xCC.
MiniMoto::MiniMoto(byte addr)
{
  _addr = addr;
  
  // This sets the bit rate of the bus; I want 100kHz. See the
  //  datasheet for details on how I came up with this value.
  TWBR = 72;
  
  
}

// Send the drive command over I2C to the DRV8830 chip. Bits 7:2 are the speed
//  setting; range is 0-63. Bits 1:0 are the mode setting:
//  - 00 = HI-Z
//  - 01 = Reverse
//  - 10 = Forward
//  - 11 = H-H (brake)
void MiniMoto::drive(int speed)
{
  byte regValue = 0;                // Set up a clear byte for the command.
  regValue = (byte)abs(speed);      // Find the byte-ish abs value of the input
  if (regValue > 63) regValue = 63; // Cap the value at 63.
  regValue = regValue<<2;           // Left shift to make room for bits 1:0
  if (speed < 0) regValue |= 0x01;  // Set bits 1:0 based on sign of input.
  else           regValue |= 0x02;
  
  xlWriteBytes(0x00, &regValue, 1);  
}

// Coast to a stop by hi-z'ing the drivers.
void MiniMoto::stop()
{
  byte regValue = 0;                // See above for bit 1:0 explanation.
  
  xlWriteBytes(0x00, &regValue, 1); 
}

// Stop the motor by providing a heavy load on it.
void MiniMoto::brake()
{
  byte regValue = 0x03;                // See above for bit 1:0 explanation.
  
  xlWriteBytes(0x00, &regValue, 1); 
}

// Private function that reads some number of bytes from the accelerometer.
void MiniMoto::xlReadBytes(byte addr, byte *buffer, byte len)
{
  byte temp = 0;
  // First, we need to write the address we want to read from, so issue a write
  //  with that address. That's two steps: first, the slave address:
  TWCR = START_COND;          // Send a start condition         
  while (!(TWCR&(1<<TWINT))); // When TWINT is set, start is complete, and we
                              //  can initiate data transfer.
  TWDR = _addr;          // Load the slave address
  TWCR = CLEAR_TWINT;         // Clear TWINT to begin transmission (I know,
                              //  it LOOKS like I'm setting it, but this is
                              //  how we clear that bit. Dumb.)
  while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  // Now, we need to send the address we want to read from:
  TWDR = addr;                // Load the slave address
  TWCR = CLEAR_TWINT;        // Clear TWINT to begin transmission (I know,
                              //  it LOOKS like I'm setting it, but this is
                              //  how we clear that bit. Dumb.)
  while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  TWCR = STOP_COND;
  
  // Now, we issue a repeated start (by doing what we just did again), and this
  //  time, we set the READ bit as well.
  TWCR = START_COND;          // Send a start condition
  while (!(TWCR&(1<<TWINT))); // When TWINT is set, start is complete, and we
                              //  can initiate data transfer.
  TWDR = (_addr) | I2C_READ;  // Load the slave address and set the read bit
  TWCR = CLEAR_TWINT;        // Clear TWINT to begin transmission (I know,
                              //  it LOOKS like I'm setting it, but this is
                              //  how we clear that bit. Dumb.)
  while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  
  // Now, we can fetch data from the slave by clearing TWINT, waiting, and
  //  reading the data. Rinse, repeat, as often as needed.
  for (byte i = 0; i < len; i++)
  {
    if (i == len-1) TWCR = CLEAR_TWINT; // Clear TWINT to begin transmission (I know,
                                //  it LOOKS like I'm setting it, but this is
                                //  how we clear that bit. Dumb.)
    else TWCR = NEXT_BYTE;
    while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
    buffer[i] = TWDR;           // Now our data can be fetched from TWDR.
  }
  // Now that we're done reading our data, we can transmit a stop condition.
  TWCR = STOP_COND;
}

void MiniMoto::xlWriteBytes(byte addr, byte *buffer, byte len)
{
  // First, we need to write the address we want to read from, so issue a write
  //  with that address. That's two steps: first, the slave address:
  TWCR = START_COND;          // Send a start condition         
  while (!(TWCR&(1<<TWINT))); // When TWINT is set, start is complete, and we
                              //  can initiate data transfer.
  TWDR = _addr;          // Load the slave address
  TWCR = CLEAR_TWINT;         // Clear TWINT to begin transmission (I know,
                              //  it LOOKS like I'm setting it, but this is
                              //  how we clear that bit. Dumb.)
  while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  // Now, we need to send the address we want to read from:
  TWDR = addr;                // Load the slave address
  TWCR |= CLEAR_TWINT;         // Clear TWINT to begin transmission (I know,
                              //  it LOOKS like I'm setting it, but this is
                              //  how we clear that bit. Dumb.)
  while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  
  // Now, we can send data to the slave by putting data into TWDR, clearing
  //  TWINT, and waiting for TWINT. Rinse, repeat, as often as needed.
  for (byte i = 0; i < len; i++)
  {
    TWDR = buffer[i];           // Now our data can be sent to TWDR.
    TWCR |= CLEAR_TWINT;        // Clear TWINT to begin transmission (I know,
                                //  it LOOKS like I'm setting it, but this is
                                //  how we clear that bit. Dumb.)
    while (!(TWCR&(1<<TWINT))); // Wait for TWINT again.
  }
  // Now that we're done writing our data, we can transmit a stop condition.
  TWCR = STOP_COND;
}