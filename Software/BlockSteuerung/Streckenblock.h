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

#ifndef STRECKENBLOCK_H__
#define STRECKENBLOCK_H__

#include "Sensor.h"
#include "LocoNetRequestQueue.h"

#ifdef STRECKENBLOCK_H__DEBUG
void printStreckenblockState();
#endif

class Streckenblock {
#ifdef STRECKENBLOCK_H__DEBUG
  friend void printStreckenblockState();
#endif
  public:
    enum track_state { STOP, RUN };
  protected:
    uint8_t id;
    LocoNetRequestQueue * lnReqQueue;
    Sensor mainSensor;
    Sensor frontSensor;
    uint16_t switchAddress;
    Streckenblock * before;
    Streckenblock * after;
    bool continueBit;
    bool frontSensorWasOccupied;
    
    /* handling sensor events */
    void frontSensorOccupied();
    void frontSensorFree();
    void mainSensorOccupied();
    void mainSensorFree();
    /* Handling the track becoming free */
    void trackFree();

public:
    void requestSwitchRed();
    void requestSwitchGreen();

protected:
    uint8_t getId() const { return id; }

  public:
    Streckenblock(): id(-1), lnReqQueue(NULL), mainSensor(0), frontSensor(0), switchAddress(0),
                      before(NULL), after(NULL), continueBit(false), frontSensorWasOccupied(false) {}
    
    Streckenblock(int id, LocoNetRequestQueue * lnReqQueue, uint16_t mainSensorAddress, uint16_t frontSensorAddress, uint16_t switchAddress):
        id(id), lnReqQueue(lnReqQueue), mainSensor(mainSensorAddress), frontSensor(frontSensorAddress), switchAddress(switchAddress),
        before(NULL), after(NULL), continueBit(false), frontSensorWasOccupied(false) {}
    
  ~Streckenblock() { before = NULL; after = NULL; lnReqQueue = NULL; };
    
    inline void setBefore(Streckenblock * before) { this->before = before; }
    inline void setAfter(Streckenblock * after) { this->after = after; }
    
    inline bool isFree() const { return mainSensor.isFree() && frontSensor.isFree(); }
    /* Returns true when the sensor is owned by this Streckenblock */
    bool processSensorNotification(uint16_t Address, uint8_t State);
    
    /* These methods notify before-blocks that the train should be started/stopped. Each block is responsible for checking its continue-bit and passing it on it if necessary */
    void notifyContinue(track_state state);
    
    /* Notifies this track that after is free */
    void notifyAfterTrackIsFree();

  protected:
#ifdef STRECKENBLOCK_H__DEBUG
    void printState() { Serial.print(F("Block ")); Serial.print(id); Serial.print(F(" is ")); 
      if (isFree()) {
        Serial.print(F("FREE.\n"));
      } else {
        Serial.print(F("OCCUPIED.\n"));
      }
    }
#endif
};

#endif
