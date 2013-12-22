#include "MCP.h"
#include <SPI.h>

#undef MCP_DEBUG

#ifdef MCP_DEBUG
#define DEBUG(x) Serial.print(x)
#define DEBUG2(x, y) Serial.println(x, y)
#else
#define DEBUG(x)
#define DEBUG2(x, y)
#endif

// Base address for all MCP23S17
#define MCP_BASE_ADDRESS 0x40 
// R/W-Bit to set in the Address. Set for Read, Clear for Write
#define MCP_RW_BIT 0x01

#define MCP_IOCON_BANK 0x80
#define MCP_IOCON_MIRROR 0x40
#define MCP_IOCON_SEQOP 0x20
#define MCP_IOCON_DISSLW 0x10
#define MCP_IOCON_HAEN 0x08
#define MCP_IOCON_ODR 0x04
#define MCP_IOCON_INTPOL 0x02

#define BANK 0

#if BANK==0

#define IODIRA   0x00
#define IODIRB   0x01
#define IPOLA    0x02
#define IPOLB    0x03
#define GPINTENA 0x04
#define GPINTENB 0x05
#define DEFVALA  0x06
#define DEFVALB  0x07
#define INTCONA  0x08
#define INTCONB  0x09
#define IOCON   0x0A
//define IOCON   0x0B
#define GPPUA    0x0C
#define GPPUB    0x0D
#define INTFA    0x0E
#define INTFB    0x0F
#define INTCAPA  0x10
#define INTCAPB  0x11
#define GPIOA    0x12
#define GPIOB    0x13
#define OLATA    0x14
#define OLATB    0x15

#else

#define IODIRA   0x00
#define IODIRB   0x10
#define IPOLA    0x01
#define IPOLB    0x11
#define GPINTENA 0x02
#define GPINTENB 0x12
#define DEFVALA  0x03
#define DEFVALB  0x13
#define INTCONA  0x04
#define INTCONB  0x14
#define IOCON   0x05
//define IOCON   0x15
#define GPPUA    0x06
#define GPPUB    0x16
#define INTFA    0x07
#define INTFB    0x17
#define INTCAPA  0x08
#define INTCAPB  0x18
#define GPIOA    0x09
#define GPIOB    0x19
#define OLATA    0x0A
#define OLATB    0x1A

#endif

void MCP::commitChanges() {
  for (uint8_t i(0); i < NUM_MCP*2; ++i) {
    if (mcpRegisterState[i] != mcpRegisterFuture[i]) {
      uint8_t chipAddr(i & 0b00001110);
      DEBUG(F("Writing Register "));
      DEBUG2(i, DEC);
      if ((i % 2) == 0) {
        // Bank A
        mcpWrite(CHIP_SELECT_PIN, GPIOA, mcpRegisterFuture[i], chipAddr);
      } else {
        // Bank B
        mcpWrite(CHIP_SELECT_PIN, GPIOB, mcpRegisterFuture[i], chipAddr);
      }
      mcpRegisterState[i] = mcpRegisterFuture[i];
    }
  }
}

void MCP::init() {
  // Configure ChipSelect
  DEBUG(F("Configure ChipSelect\n"));
  pinMode(CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(CHIP_SELECT_PIN, HIGH);
  
  // Enable SPI
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  
  // Configure the IO Condition Register
  // Enable Hardware addressing
  // Configure the Interrupt pin for Open Drain (NOTE: I probably fried those pins on my MCPs).
  //
  // Note: This is done for Address 0 and Address 7. There is a bug in some MCPs which causes them to
  // always read the highest bit, even if hardware addressing was disabled. This works around this problem.
  DEBUG(F("Configure IOCON\n"));
  mcpWrite(CHIP_SELECT_PIN, IOCON, MCP_IOCON_ODR | MCP_IOCON_HAEN, 0b0000000);
  mcpWrite(CHIP_SELECT_PIN, IOCON, MCP_IOCON_ODR | MCP_IOCON_HAEN, 0b0001110);
     
  // Configure output pins
  DEBUG(F("Configure IODIR\n"));
  for (uint8_t i(0); i < NUM_MCP; ++i) {
    uint8_t addr(i << 1);
    mcpWrite(CHIP_SELECT_PIN, IODIRA, 0x00, addr);
    if (i == 3) {
      // Special: This bank on this chip has two inputs for the buttons.
      mcpWrite(CHIP_SELECT_PIN, IODIRB, BUTTON_MASK, addr);
    } else {
      mcpWrite(CHIP_SELECT_PIN, IODIRB, 0x00, addr);
    }
  }
   
  // Initialize the output registers.
  // Set to different values to force an initial write.
  // Note: 0x00 is ON, 0x00 is OFF.
  DEBUG(F("Inizialize register buffer\n"));
  for (uint8_t i(0); i < NUM_MCP*2; ++i) {
    mcpRegisterState[i] = 0x00;
    mcpRegisterFuture[i] = 0xFF;
  }
  
  // Run the update
  DEBUG(F("Apply register buffer\n"));
  commitChanges();
  
  // Initialize interrupt (NOTE: This appears to be broken on my chips)
  //pinMode(INTERRUPT_PIN, INPUT);
  //mcpWrite(CHIP_SELECT_PIN, DEFVALA, 0x00);
  //mcpWrite(CHIP_SELECT_PIN, GPINTENB, BUTTON_MASK);
  //mcpWrite(CHIP_SELECT_PIN, INTCONB, 0x00);
  //attachInterrupt(INTERRUPT_NUMBER, interruptPin2, FALLING)
}

void MCP::readButtons() {
  uint8_t buttonFuture(mcpRead(CHIP_SELECT_PIN, GPIOB, (3 << 1)));
  buttonFuture &= BUTTON_MASK;
  if (buttonState != buttonFuture) {
    // Change detected
    boolean button1pressed(((buttonState & BUTTON1) == 0x00) && ((buttonFuture & BUTTON1) != 0x00));
    boolean button2pressed(((buttonState & BUTTON2) == 0x00) && ((buttonFuture & BUTTON2) != 0x00));
    
    if (button1pressed != button2pressed) {
      if (button1pressed) {
        buttonAPressed();
      } else {
        buttonBPressed();
      }
    //} else if (buttonBpressed) {
      //Serial.println(F("Both Buttons pressed."));
    }
    
    buttonState = buttonFuture;  
  }
}

uint8_t MCP::mcpWrite(uint8_t pinNumber, uint8_t reg, uint8_t value, uint8_t addrBits = 0xFF) {
  uint8_t returnValue;
  addrBits &= 0b0001110;
  addrBits |= MCP_BASE_ADDRESS;
#ifdef MCP_DEBUG
  Serial.print(addrBits, HEX);
  Serial.print(F(" Writing To Register 0x"));
  Serial.print(reg, HEX);
  Serial.print(F(" value 0x"));
  Serial.println(value, HEX);
#endif
  digitalWrite(pinNumber, LOW);
  SPI.transfer(addrBits);
  SPI.transfer(reg);
  returnValue = SPI.transfer(value);
  digitalWrite(pinNumber, HIGH);
  return returnValue;
}

void MCP::requestDigitalWrite(uint8_t addressByte, uint8_t state) {
#ifdef MCP_DEBUG
  Serial.print(F("Requesting wrie for Chip 0x"));
  Serial.print(GET_CHIP_FROM_BYTE(addressByte), HEX);
  Serial.print(F(", Bank 0x"));
  Serial.print(GET_BANK_NUM_FROM_BYTE(addressByte));
  Serial.print(F(", Pin 0x"));
  Serial.print(GET_PIN_FROM_BYTE(addressByte));
  Serial.print(F(" (Register: "));
  Serial.print((2*GET_CHIP_FROM_BYTE(addressByte)) + GET_BANK_NUM_FROM_BYTE(addressByte), DEC);
  Serial.println(")");
#endif
  uint8_t mask(1 << GET_PIN_FROM_BYTE(addressByte));
  if (state == LOW) {
    mcpRegisterFuture[(2*GET_CHIP_FROM_BYTE(addressByte)) + GET_BANK_NUM_FROM_BYTE(addressByte)] &= ~mask;
  } else {
    mcpRegisterFuture[(2*GET_CHIP_FROM_BYTE(addressByte)) + GET_BANK_NUM_FROM_BYTE(addressByte)] |= mask;
  }
}

uint8_t MCP::mcpRead(uint8_t pinNumber, uint8_t reg, uint8_t addrBits = 0xFF) {
  uint8_t returnValue;
  addrBits &= 0b0001110;
  addrBits |= MCP_BASE_ADDRESS | MCP_RW_BIT;
  //Serial.print(addrBits, HEX);
  //Serial.print(F(" Reading from Register 0x"));
  //Serial.print(reg, HEX);
  //Serial.print(F(" value 0x"));
  digitalWrite(pinNumber, LOW);
  SPI.transfer(addrBits);
  SPI.transfer(reg);
  returnValue = SPI.transfer(0x55);
  digitalWrite(pinNumber, HIGH);
  //Serial.println(returnValue, HEX);
  return returnValue;
}
