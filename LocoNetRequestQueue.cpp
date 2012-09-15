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

#include "LocoNetRequestQueue.h"

#define SWITCH_ON 16
#define SWITCH_OFF 0

#define INTERVAL_MILLIS 50

void LocoNetRequestQueue::loop() {
  if (isEmpty()) {
    return;
  }
  
  unsigned long now(millis());
  
  // fix overflow
  if (now < lastSendTime) {
    lastSendTime = 0;
  }
  
  // check for elapsed time
  if (now - lastSendTime > INTERVAL_MILLIS) {
    lastSendTime = now;
    processNextMessage();
  }
  
}

bool LocoNetRequestQueue::postSwitchRequest(uint16_t Address, uint8_t direction) {
  // check for sufficient space
  if (LNRQ_LENGTH < getQueueLength()) {
    return false;
  }
  // Coil ON
  queue[queueEnd].opcode = LocoNetRequestQueueEntry::OP_SWITCH_ON;
  queue[queueEnd].address = Address;
  queue[queueEnd].direction = direction;
  
  Serial.print("postSwitchRequest: ");
  queue[queueEnd].print();
  Serial.print("\n");
  
  queueEnd = (queueEnd + 1) % (LNRQ_LENGTH);
  
  return true;
}

void LocoNetRequestQueue::processNextMessage() {
  if (isEmpty()) {
    return;
  }
  
  switch (queue[queueStart].opcode) {
    case LocoNetRequestQueueEntry::OP_SWITCH_ON:
      LocoNet.requestSwitch(queue[queueStart].address, SWITCH_ON, queue[queueStart].direction);
      Serial.print("Sending: ");
      queue[queueStart].print();
      Serial.print("\n");
      queue[queueStart].opcode = LocoNetRequestQueueEntry::OP_SWITCH_OFF;
    break;
    case LocoNetRequestQueueEntry::OP_SWITCH_OFF:
      LocoNet.requestSwitch(queue[queueStart].address, SWITCH_OFF, queue[queueStart].direction);
      Serial.print("Sending: ");
      queue[queueStart].print();
      Serial.print("\n");
      queueStart = (queueStart + 1) % (LNRQ_LENGTH);
    break;
  }
  
}

uint8_t LocoNetRequestQueue::getQueueLength() const {
  if (queueStart <= queueEnd) {
    // no wrapping
    return queueEnd - queueStart;
  } else {
    return (LNRQ_LENGTH - queueStart) + queueEnd - 1;
  }
}
