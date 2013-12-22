#ifndef MCP_H__
#define MCP_H__

#include "Arduino.h"

// This class implements support for talking to the MCPs connected via SPI

#include <SPI.h>

#define NUM_MCP (5)

#define CHIP_SELECT_PIN (4)

// Button 1 is HP0
#define BUTTON1 0x04
// Button 2 is HP2
#define BUTTON2 0x08

#define BUTTON_MASK (BUTTON1 | BUTTON2)

#define INTERRUPT_PIN (2)

// Encode a set of chip, bank and pin into a single byte
#define GET_MCP_ADDRESS_AS_BYTE(chip, bank, pin) ((chip << 5) | ((bank == 'B') ? 0x08 : 0x00) | (pin & 0x07))

// Extract the chip address from the above byte
#define GET_CHIP_FROM_BYTE(addressByte) (addressByte >> 5)
// Extrace the bank register from the above byte
#define GET_BANK_NUM_FROM_BYTE(addressByte) ((addressByte & 0x08) ? 1 : 0)
#define GET_PIN_FROM_BYTE(addressByte) (addressByte % 8)
// Extract the pin including the bank from the address byte
#define GET_LONGPIN_FROM_BYTE(addressByte) (addressByte % 16)

#define MCP_PIN_ON LOW
#define MCP_PIN_OFF HIGH

class MCP {
protected:
  uint8_t mcpRegisterState[NUM_MCP*2];
  uint8_t mcpRegisterFuture[NUM_MCP*2];
  
  uint8_t buttonState;

  uint8_t mcpRead(uint8_t pinNumber, uint8_t reg, uint8_t addrBits);
  uint8_t mcpWrite(uint8_t pinNumber, uint8_t reg, uint8_t value, uint8_t addrBits);

public:
  void init();
  void readButtons();
  void commitChanges();
  void requestDigitalWrite(uint8_t addressByte, uint8_t state);
};

void buttonAPressed();
void buttonBPressed();


#endif 
// MCP_H__
