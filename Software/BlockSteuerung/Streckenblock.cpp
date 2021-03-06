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
#include "MCP.h"

#ifdef STRECKENBLOCK_H__DEBUG
#define DEBUG(x) Serial.print(x)
#define DEBUG2(x, y) Serial.print(x, y)
#else
#define DEBUG(x)
#define DEBUG2(x, y)
#endif

void reportExitSignal(uint8_t, uint8_t);

bool Streckenblock::processSensorNotification(uint16_t Address, uint8_t State) {
  bool change(false);
  if (mainSensor.getAddress() == Address) {
    change = mainSensor.processNotification(State);
    DEBUG(id);
    DEBUG(": main sensor ");
    DEBUG(mainSensor.getAddress());
#ifdef STRECKENBLOCK_H__DEBUG
    change ? DEBUG(" changed") : DEBUG(" constant");
    if (mainSensor.isFree()) {
      DEBUG(F(" FREE\n"));
    } else {
      DEBUG(F(" OCCUPIED\n"));
    }
#endif
    if (change) {
#ifdef STRECKENBLOCK_H__DEBUG
      printState();
#endif
      if (mainSensor.isFree()) {
        mainSensorFree();
      } else {
        mainSensorOccupied();
      }
    }
    
#ifdef STRECKENBLOCK_H__DEBUG
  printStreckenblockState();
#endif
    return true;
  }
  if (frontSensor.getAddress() == Address) {
    change = frontSensor.processNotification(State);
    DEBUG(id);
    DEBUG(": front sensor ");
    DEBUG(frontSensor.getAddress());
#ifdef STRECKENBLOCK_H__DEBUG
    change ? Serial.print(" changed") : Serial.print(" constant");
    if (frontSensor.isFree()) {
      Serial.print(F(" FREE\n"));
    } else {
      Serial.print(F(" OCCUPIED\n"));
    }
#endif
    if (change && !frontSensorWasOccupied) {
#ifdef STRECKENBLOCK_H__DEBUG
      printState();
#endif
      if (frontSensor.isFree()) {
        frontSensorFree();
      } else {
        frontSensorOccupied();
      }
    }
    
#ifdef STRECKENBLOCK_H__DEBUG
  printStreckenblockState();
#endif
    return true;
  }
  return false;
}

void Streckenblock::frontSensorOccupied() {
  DEBUG(id);
  DEBUG(F(": frontSensorOccupied "));
  if (frontSensorWasOccupied) {
    DEBUG(F("aborted\n."));
    return;
  }
  DEBUG(F("running.\n"));
  
  frontSensorWasOccupied = true;
  
  DEBUG(id);
  DEBUG(F(": frontSensorOccupied: continueBit is "));
  if (continueBit == false) {
    DEBUG(F("FALSE, "));
    // Only do stuff if this is not the continuation of a train
    if (after == NULL) {
      DEBUG(F("After is NULL\n"));
      // Last block. stop the train.
      actExitSignalRequestedState();
      // Set continue bit, "next" block now has full control over the train.
      //continueBit = true;
    } else {
      DEBUG(F("After "));
      DEBUG(after->getId());
      DEBUG(F(" is PRESENT and "));
      // check if the after block is occupied
      if (after->isFree()) {
        DEBUG(F("FREE\n"));
        // Train can leave right away, this is now a continuation.
        continueBit = true;
        requestSwitchGreen();
      } else {
        DEBUG(F("OCCUPIED\n"));
        // Train must wait
        requestSwitchRed();
      }
    } 
  } else 
    DEBUG(F("TRUE\n"));
}

void Streckenblock::frontSensorFree() { 
    DEBUG(id);
	DEBUG(": MainSensor Free\n");
	if (isFree()) { 
		trackFree();
	};
}

void Streckenblock::mainSensorOccupied() {}

void Streckenblock::mainSensorFree() { 
    DEBUG(id); 
	DEBUG(": MainSensor Free\n");
	if (isFree()) {
		trackFree();
	};
}

void Streckenblock::trackFree() {
  DEBUG(id);
  DEBUG(F(": Track Free\n"));
  continueBit = false; // reset continue
  frontSensorWasOccupied = false; // reset front sensor hysteresis
  requestSwitchGreen(); // turn on moving trains to the front
  
  // Reset the exit signal to RED
  if (after == NULL) {
    exitSignalRequestedState = SWITCH_RED;
    reportExitSignal(id, SWITCH_RED);
  }
  
  if (before != NULL) {
    before->notifyAfterTrackIsFree(); // tell the before track that this one is now free
  }
}

void Streckenblock::notifyAfterTrackIsFree() {
  // Next track is free
  DEBUG(id);
  DEBUG(F(": notifyAfterTrackIsFree\n"));
  
  /* Only use this if the frontSensor is/was occupied.
   * this way, a train can be stopped by an obstacle appearing suddenly on the next track */
  if (frontSensorWasOccupied) {
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
  mcp.requestDigitalWrite(mcpAddressRed, MCP_PIN_OFF);
  if (continueBit) {
    mcp.requestDigitalWrite(mcpAddressGreen, MCP_PIN_OFF);
  } else {
    mcp.requestDigitalWrite(mcpAddressGreen, MCP_PIN_ON);
  }
  lnReqQueue->postSwitchRequest(switchAddress, SWITCH_GREEN);
  if (before != NULL) {
    before->notifyContinue(RUN);
  }
}

void Streckenblock::requestSwitchRed() {
  mcp.requestDigitalWrite(mcpAddressGreen, MCP_PIN_OFF);
  if (continueBit) {
    mcp.requestDigitalWrite(mcpAddressRed, MCP_PIN_OFF);
  } else {
    mcp.requestDigitalWrite(mcpAddressRed, MCP_PIN_ON);
  }
  lnReqQueue->postSwitchRequest(switchAddress, SWITCH_RED);
  if (before != NULL) {
    before->notifyContinue(STOP);
  }
}

void Streckenblock::notifyExitSignalSwitchRequest(uint8_t state, uint8_t byteRed, uint8_t byteGreen) {
  if (after != NULL) {
    return;
  }
  
  exitSignalRequestedState = state;
  if (exitSignalRequestedState == SWITCH_RED) {
    mcp.requestDigitalWrite(byteRed, MCP_PIN_ON);
    mcp.requestDigitalWrite(byteGreen, MCP_PIN_OFF);
  } else {
    mcp.requestDigitalWrite(byteRed, MCP_PIN_OFF);
    mcp.requestDigitalWrite(byteGreen, MCP_PIN_ON);
  }
  
  actExitSignalRequestedState();
}

void Streckenblock::actExitSignalRequestedState() {
  if ((after == NULL) && frontSensorWasOccupied && !isFree()) {
    // Track is occupied, react.
    if (exitSignalRequestedState == SWITCH_RED) {
      requestSwitchRed();
    } else {
      requestSwitchGreen();
    }
  }
}
