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

#ifndef SENSOR_H__
#define SENSOR_H__

#include "Arduino.h"

#ifdef DO_DEBUG
#define SENSOR_H__DEBUG
#endif


class Sensor {
  public:
    enum sensor_state { FREE, OCCUPIED };
  protected:
    uint16_t address;
    sensor_state state;
  public:
    Sensor(uint16_t address): address(address), state(FREE) {}
    ~Sensor() { address = 0; state = FREE; };
    inline sensor_state getState() const { return state; }
    inline bool isFree() const { return state == FREE; }
    inline bool isOccupied() const { return state == OCCUPIED; }
    inline uint16_t getAddress() const { return address; }
    
    /* Return true if the state changed. */
    inline bool processNotification(uint8_t state) {
      sensor_state newState;
      bool changed(false);
      
      if (16 == state) {
        newState = OCCUPIED;
      } else {
        newState = FREE;
      }
      
      if (newState != this->state) {
#ifdef SENSOR_H__DEBUG
        Serial.print("Sensor "); Serial.print(address); Serial.print(" is now ");
        if (OCCUPIED == newState) {
          Serial.print(" OCCUPIED.\n");
        } else {
          Serial.print(" FREE.\n");
        }
#endif
        this->state = newState;
        return true;
      } else {
        return false;
      }
    }
};

#undef SENSOR_H__DEBUG

#endif
