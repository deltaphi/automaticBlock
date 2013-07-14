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

#ifndef LOCONETREQUESTQUEUE_H__
#define LOCONETREQUESTQUEUE_H__

#include <LocoNet.h>

// number of queue entries.
#define LNRQ_LENGTH 20


#define SWITCH_RED 0
#define SWITCH_GREEN 32

typedef struct LocoNetRequestQueueEntry {
  enum opcode_t { OP_SWITCH_ON, OP_SWITCH_OFF };
  
  opcode_t opcode;
  uint16_t address;
  uint8_t direction;
  
  void print() {
    Serial.print("LocoNetRequest(");
    switch (opcode) {
      case OP_SWITCH_ON:
        Serial.print("OP_SWITCH_ON, ");
      break;
      case OP_SWITCH_OFF:
        Serial.print("OP_SWITCH_OFF, ");
      break;
  };
  Serial.print(address);
  Serial.print(", ");
  switch (direction) {
    case SWITCH_RED:
      Serial.print("RED)");
      break;
    case SWITCH_GREEN:
      Serial.print("GREEN)");
      break;
    default:
      Serial.print(direction);
      Serial.print(")");
  }
  }
  
} LocoNetRequestQueueEntry;

class LocoNetRequestQueue {
  protected:
    LocoNetRequestQueueEntry queue[2*LNRQ_LENGTH];
    uint8_t queueStart; // points to next non-processed message
    uint8_t queueEnd; // points to next slot where a message can be inserted
    uint8_t getQueueLength() const;
    unsigned long lastSendTime;
  public:
    LocoNetRequestQueue(): queueStart(0), queueEnd(0), lastSendTime(0) {}
    ~LocoNetRequestQueue() {}
    // Should be called in every iteration
    void loop();
    
    void processNextMessage();
    bool postSwitchRequest(uint16_t Address, uint8_t direction);
    inline bool isEmpty() const { return queueStart == queueEnd; }
};

#endif
