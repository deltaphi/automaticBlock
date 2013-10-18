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

#include <Event.h>

#include <LocoNet.h>

#include "LocoNetRequestQueue.h"
#include "Streckenblock.h"

#ifdef DO_DEBUG
#define DEBUG(x) Serial.print(x)
#else
#define DEBUG(x)
#endif

LocoNetRequestQueue lnReqQueue;

#define STRECKENBLOCK_LENGTH 17

Streckenblock blocks[STRECKENBLOCK_LENGTH];

#define EXIT_SIGNAL 132

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

void setup() {
  // First initialize the LocoNet interface
  LocoNet.init(7);

  // Configure the serial port for 57600 baud
#ifdef DO_DEBUG
  Serial.begin(115200);
#endif
  
  DEBUG("Blockstraßensteuerung startet.\n");
  // Note: Set 1 and 2 to RED!!!
  // Note: Init 3 as GREEN, 4 as RED!!!
  // TODO: How to initialize everything after a reboot?
  blocks[0] = Streckenblock(0, &lnReqQueue, 513, 514, 200);
  blocks[1] = Streckenblock(1, &lnReqQueue, 515, 516, 201);
  blocks[2] = Streckenblock(2, &lnReqQueue, 517, 518, 202);
  blocks[3] = Streckenblock(3, &lnReqQueue, 519, 520, 203);
  blocks[4] = Streckenblock(4, &lnReqQueue, 521, 522, 204);
  blocks[5] = Streckenblock(5, &lnReqQueue, 523, 524, 205);
  blocks[6] = Streckenblock(6, &lnReqQueue, 525, 526, 206);
  blocks[7] = Streckenblock(7, &lnReqQueue, 527, 528, 207);
  blocks[8] = Streckenblock(8, &lnReqQueue, 529, 530, 208);
  //blocks[9] = Streckenblock(1, &lnReqQueue, 531, 532, 209);
  blocks[9] = Streckenblock(9, &lnReqQueue, 533, 534, 210);
  blocks[10] = Streckenblock(10, &lnReqQueue, 535, 536, 211);
  blocks[11] = Streckenblock(11, &lnReqQueue, 537, 538, 212);
  blocks[12] = Streckenblock(12, &lnReqQueue, 539, 540, 213);
  blocks[13] = Streckenblock(13, &lnReqQueue, 541, 542, 214);
  blocks[14] = Streckenblock(14, &lnReqQueue, 543, 544, 215);
  blocks[15] = Streckenblock(15, &lnReqQueue, 545, 546, 216);
  blocks[16] = Streckenblock(16, &lnReqQueue, 547, 548, 217);
  
  for (uint8_t i(0); i < (STRECKENBLOCK_LENGTH - 1); ++i) {
    blocks[i+1].setBefore(&blocks[i]);
    blocks[i].setAfter(&blocks[i+1]);
  }
  
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
    // last block does not have to do anything
    if (SWITCH_RED == Direction) {
      blocks[STRECKENBLOCK_LENGTH-1].requestSwitchRed(); //notifyContinue(Streckenblock::STOP);
    } else {
      blocks[STRECKENBLOCK_LENGTH-1].requestSwitchGreen(); //notifyContinue(Streckenblock::RUN);
    }
  }
}
