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

LocoNetRequestQueue lnReqQueue;

#define STRECKENBLOCK_LENGTH 2

Streckenblock blocks[STRECKENBLOCK_LENGTH];

#define EXIT_SIGNAL 132

void processLocoNetRequestQueue() {
  lnReqQueue.processNextMessage();
}

void setup() {
  // First initialize the LocoNet interface
  LocoNet.init();

  // Configure the serial port for 57600 baud
  Serial.begin(115200);
  
  Serial.print("Blockstraßensteuerung startet.\n");
  // Note: Set 1 and 2 to RED!!!
  // Note: Init 3 as GREEN, 4 as RED!!!
  // TODO: How to initialize everything after a reboot?
  blocks[0] = Streckenblock(1, &lnReqQueue, 1, 2, 131);
  blocks[1] = Streckenblock(2, &lnReqQueue, 3, 4, 132);
  
  blocks[0].setAfter(&blocks[1]);
  blocks[1].setBefore(&blocks[0]);
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
  Serial.print("Received: Sensor "); Serial.print(Address); Serial.print(", State "); Serial.print(State); Serial.print("\n");
  for (int i(0); i < STRECKENBLOCK_LENGTH; ++i) {
    blocks[i].processSensorNotification(Address, State);
  }
}

void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  // If this was the exit signal and it was set to green, send this through the Steckenblocks
  if (Address == EXIT_SIGNAL) {
    // last block does not have to do anything
    if (SWITCH_RED == Direction) {
      blocks[STRECKENBLOCK_LENGTH-2].notifyContinue(Streckenblock::STOP);
    } else {
      blocks[STRECKENBLOCK_LENGTH-2].notifyContinue(Streckenblock::RUN);
    }
  }
}
