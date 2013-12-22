/**
 * Copyright 2012 Damian Philipp
 *
 * This file is part of "Automatic Block Signaling".
 * 
 * "Automatic Block Signaling" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * "Automatic Block Signaling" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Diese Datei ist teil von "Automatic Block Signaling".
 *
 * "Automatic Block Signaling" ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Option) jeder späteren
 * veröffentlichten Version, weiterverbreiten und/oder modifizieren.
 *
 * "Automatic Block Signaling" wird in der Hoffnung, dass es nützlich sein wird, aber
 * OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
 * Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Details.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
 */

#include <LocoNet.h>

#include "LocoNetRequestQueue.h"
#include "Streckenblock.h"
#include "MCP.h"
#include <SPI.h>

#define DO_DEBUG

#ifdef DO_DEBUG
#define DEBUG(x) Serial.print(x)
#else
#define DEBUG(x)
#endif

LocoNetRequestQueue lnReqQueue;

#define STRECKENBLOCK_LENGTH 17

Streckenblock blocks[STRECKENBLOCK_LENGTH];

#define EXIT_SIGNAL 132

#define EXIT_SIGNAL_BYTE_RED GET_MCP_ADDRESS_AS_BYTE(3, 'B', 0)
#define EXIT_SIGNAL_BYTE_GREEN GET_MCP_ADDRESS_AS_BYTE(3, 'B', 1)

MCP mcp;

void processLocoNetRequestQueue() {
  lnReqQueue.processNextMessage();
}

#ifdef DO_DEBUG
void printStreckenblockState() {
  // Occupied (0: free, 1: main, 2: front, 3: both)
  // continue bit (X/_)
  for (uint8_t i(0); i < STRECKENBLOCK_LENGTH; ++i) {
    Serial.print(F(" "));
    uint8_t state(0);
    if (blocks[i].frontSensor.isOccupied() || blocks[i].frontSensorWasOccupied) {
      state += 2;
    }
    if (blocks[i].mainSensor.isOccupied()) {
      state += 1;
    }
    Serial.print(state);
  }
  Serial.print("\n");
  for (uint8_t i(0); i < STRECKENBLOCK_LENGTH; ++i) {
    Serial.print(F(" "));
    if (blocks[i].continueBit) {
      Serial.print("X");
    } else {
      Serial.print("_");
    }
  }
  Serial.print("\n");
}
#endif

#define GET_

void setup() {
  // First initialize the LocoNet interface
  LocoNet.init(7);

  // Configure the serial port for 57600 baud
#ifdef DO_DEBUG
  Serial.begin(115200);
#endif
  
  DEBUG("Initializing MCP\n");
  
  mcp.init();
  
  DEBUG("Blockstraßensteuerung startet.\n");
  // Note: Set 1 and 2 to RED!!!
  // Note: Init 3 as GREEN, 4 as RED!!!
  // TODO: How to initialize everything after a reboot?
  // red green front main
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(0, 'A', 6, 7, 2, 3);
  blocks[0] = Streckenblock(0, mcpAddresses, &lnReqQueue, 513, 514, 200);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(0, 'A', 4, 5, 0, 1);
  blocks[1] = Streckenblock(1, mcpAddresses, &lnReqQueue, 515, 516, 201);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(4, 'A', 1, 0, 2, 3);
  blocks[2] = Streckenblock(2, mcpAddresses, &lnReqQueue, 517, 518, 202);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(2, 'B', 6, 7, 4, 5);
  blocks[3] = Streckenblock(3, mcpAddresses, &lnReqQueue, 519, 520, 203);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(2, 'A', 7, 6, 4, 5);
  blocks[4] = Streckenblock(4, mcpAddresses, &lnReqQueue, 521, 522, 204);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(1, 'B', 2, 3, 0, 1);
  blocks[5] = Streckenblock(5, mcpAddresses, &lnReqQueue, 523, 524, 205);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(2, 'A', 2, 3, 1, 0);
  blocks[6] = Streckenblock(6, mcpAddresses, &lnReqQueue, 525, 526, 206);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(0, 'B', 4, 5, 6, 7);
  blocks[7] = Streckenblock(7, mcpAddresses, &lnReqQueue, 527, 528, 207);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(1, 'A', 7, 6, 5, 4);
  blocks[8] = Streckenblock(8, mcpAddresses, &lnReqQueue, 529, 530, 208);
  }
  //{
  //mcpAddresses = STRECKENBLOCK_MCP_STRUCT(0, 'B', 3, 2, 1, 0);
  //blocks[9] = Streckenblock(1, STRECKENBLOCK_MCP_STRUCT(0, 'B', 3, 2, 1, 0), &lnReqQueue, 531, 532, 209);
  //}
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(1, 'A', 3, 2, 1, 0);
  blocks[9] = Streckenblock(9, mcpAddresses, &lnReqQueue, 533, 534, 210);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(1, 'B', 4, 5, 6, 7);
  blocks[10] = Streckenblock(10, mcpAddresses, &lnReqQueue, 535, 536, 211);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(2, 'B', 3, 2, 1, 0);
  blocks[11] = Streckenblock(11, mcpAddresses, &lnReqQueue, 537, 538, 212);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(3, 'A', 4, 5, 6, 7);
  blocks[12] = Streckenblock(12, mcpAddresses, &lnReqQueue, 541, 540, 213);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(3, 'A', 0, 1, 2, 3);
  blocks[13] = Streckenblock(13, mcpAddresses, &lnReqQueue, 539, 542, 214);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(4, 'A', 6, 7, 4, 5);
  blocks[14] = Streckenblock(14, mcpAddresses, &lnReqQueue, 543, 544, 215);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(4, 'B', 6, 7, 1, 0);
  blocks[15] = Streckenblock(15, mcpAddresses, &lnReqQueue, 545, 546, 216);
  }
  {
  Streckenblock::MCPAddresses mcpAddresses = STRECKENBLOCK_MCP_STRUCT(4, 'B', 4, 5, 3, 2);
  blocks[16] = Streckenblock(16, mcpAddresses, &lnReqQueue, 547, 548, 217);
  }
  
  for (uint8_t i(0); i < (STRECKENBLOCK_LENGTH - 1); ++i) {
    blocks[i+1].setBefore(&blocks[i]);
    blocks[i].setAfter(&blocks[i+1]);
  }
  
  // Force the exit signal to RED
  blocks[STRECKENBLOCK_LENGTH - 1].notifyExitSignalSwitchRequest(SWITCH_RED, EXIT_SIGNAL_BYTE_RED, EXIT_SIGNAL_BYTE_GREEN);
  reportExitSignal(blocks[STRECKENBLOCK_LENGTH - 1].getId(), SWITCH_RED);
  
#ifdef DO_DEBUG
  printStreckenblockState();
#endif
}

void loop() {
  // Check for any received LocoNet packets
  lnMsg * RxPacket = LocoNet.receive() ;
  if( RxPacket )
  {
    digitalWrite(13, LOW);
    //printPacket(RxPacket);
    
    LocoNet.processSwitchSensorMessage(RxPacket);
    // Ignore throttle packets 
    digitalWrite(13, HIGH);
  }
  
  // process outgoing messages
  lnReqQueue.loop();
  
  // Check for button presses
  mcp.readButtons();
  
  // Update Display Board
  mcp.commitChanges();
}

void notifySensor( uint16_t Address, uint8_t State ) {
  // check all blocks
  //Serial.print("Received: Sensor "); Serial.print(Address); Serial.print(", State "); Serial.print(State); Serial.print("\n");
  for (int i(0); i < STRECKENBLOCK_LENGTH; ++i) {
    blocks[i].processSensorNotification(Address, State);
  }
}

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  // If this was the exit signal and it was set to green, send this through the Steckenblocks
  if (Address == EXIT_SIGNAL) {
    DEBUG(F("External Switch request: "));
    // last block does not have to do anything
    if (SWITCH_RED == Direction) {
      DEBUG(F("RED\n"));
      blocks[STRECKENBLOCK_LENGTH-1].notifyExitSignalSwitchRequest(SWITCH_RED, EXIT_SIGNAL_BYTE_RED, EXIT_SIGNAL_BYTE_GREEN);
    } else {
      DEBUG(F("GREEN\n"));
      blocks[STRECKENBLOCK_LENGTH-1].notifyExitSignalSwitchRequest(SWITCH_GREEN, EXIT_SIGNAL_BYTE_RED, EXIT_SIGNAL_BYTE_GREEN);
    }
  }
}

void buttonAPressed() {
  // Someone requested HP0
  // Do this via Loconet: Request the entrance signal to go to SWITCH_RED
  DEBUG("HP0 pressed\n");
  Serial.print(EXIT_SIGNAL_BYTE_RED, HEX);
  DEBUG(" ");
  Serial.println(EXIT_SIGNAL_BYTE_GREEN, HEX);
      blocks[STRECKENBLOCK_LENGTH-1].notifyExitSignalSwitchRequest(SWITCH_RED, EXIT_SIGNAL_BYTE_RED, EXIT_SIGNAL_BYTE_GREEN);
  LocoNet.requestSwitch(EXIT_SIGNAL, 0, SWITCH_RED);
}

void buttonBPressed() {
  // Someone requested HP2
  // Do this via Loconet: Request the entrance signal to go to SWITCH_GREEN
  DEBUG("HP2 pressed\n");
      blocks[STRECKENBLOCK_LENGTH-1].notifyExitSignalSwitchRequest(SWITCH_GREEN, EXIT_SIGNAL_BYTE_RED, EXIT_SIGNAL_BYTE_GREEN);
  LocoNet.requestSwitch(EXIT_SIGNAL, 0, SWITCH_GREEN);
}

void reportExitSignal(uint8_t blockID, uint8_t state) {
  DEBUG(F("Reporting Exit Signal "));
  DEBUG(EXIT_SIGNAL);
  DEBUG(" ");
  DEBUG(state);
  DEBUG("\n");
  lnReqQueue.postSwitchRequest(EXIT_SIGNAL, state);
  //LocoNet.sendSwitchReport(EXIT_SIGNAL - 1, state);
}
