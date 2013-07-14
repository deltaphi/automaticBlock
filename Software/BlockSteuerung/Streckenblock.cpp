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

#include "Streckenblock.h"

bool Streckenblock::processSensorNotification(uint16_t Address, uint8_t State) {
  bool change(false);
  if (mainSensor.getAddress() == Address) {
    change = mainSensor.processNotification(State);
    Serial.print(id);
    Serial.print(": main sensor ");
    change ? Serial.print("true\n") : Serial.print("false\n");
    if (change) {
      printState();
      if (mainSensor.isFree()) {
        mainSensorFree();
      } else {
        mainSensorOccupied();
      }
    }
    return true;
  }
  if (frontSensor.getAddress() == Address) {
    change = frontSensor.processNotification(State);
    Serial.print(id);
    Serial.print(": main sensor ");
    change ? Serial.print("true\n") : Serial.print("false\n");
    if (change) {
      printState();
      if (frontSensor.isFree()) {
        frontSensorFree();
      } else {
        frontSensorOccupied();
      }
    }
    return true;
  }
  return false;
}

void Streckenblock::frontSensorOccupied() {
  if (continueBit == false) {
    // Only do stuff if this is not the continuation of a train
    if (after == NULL) {
      // Last block. stop the train.
      requestSwitchRed();
      // Set continue bit, "next" block now has full control over the train.
      continueBit = true;
    } else {
      // check if the after block is occupied
      if (after->isFree()) {
        // Train can leave right away, this is now a continuation.
        continueBit = true;
        requestSwitchGreen();
      } else {
        // Train must wait
        requestSwitchRed();
      }
    } 
  }
}

void Streckenblock::frontSensorFree() { if (isFree()) { trackFree(); }; }

void Streckenblock::mainSensorOccupied() {}

void Streckenblock::mainSensorFree() { if (isFree()) { trackFree(); }; }

void Streckenblock::trackFree() {
  continueBit = false; // reset continue
  requestSwitchGreen(); // turn on moving trains to the front
  if (before != NULL) {
    before->notifyAfterTrackIsFree(); // tell the before track that this one is now free
  }
}

void Streckenblock::notifyAfterTrackIsFree() {
  // Next track is free
  
  /* Only use this of the frontSensor is occupied.
   * this way, a train can be stopped by a obstacle appearing suddenly on the next track */
  if (frontSensor.isOccupied()) {
    // If there was a train here, its now a continuation.
    continueBit = true;
    requestSwitchGreen(); // run trains
  }
}

void Streckenblock::notifyContinue(track_state state) {
  if (continueBit) {
    switch (state) { // Handle the state change
      case STOP: requestSwitchRed(); break;
      case RUN: requestSwitchGreen(); break;
    }
  }
}

void Streckenblock::requestSwitchGreen() {
  lnReqQueue->postSwitchRequest(switchAddress, SWITCH_GREEN);
  if (before != NULL) {
    before->notifyContinue(RUN);
  }
}


void Streckenblock::requestSwitchRed() {
  lnReqQueue->postSwitchRequest(switchAddress, SWITCH_RED);
  if (before != NULL) {
    before->notifyContinue(STOP);
  }
}
