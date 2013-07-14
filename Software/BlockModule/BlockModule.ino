//#include <Event.h>
//#include <Timer.h>

#include <EEPROMEx.h>

#include <LocoNet.h>

#include <digitalWriteFast.h>

//#define DO_DEBUG

// Harware Version for Prototype
#define HW_Ver 0x03
// include HW_Ver before this!
#include "hw_config.h"

// Software version. 0x00 for development version
#define SW_VER 0x00

// maximum time any relay coil is active in milliseconds
#define SWITCH_ACTIVE_TIME_MS 50

// Macro for reading (initializing) a sensor value
#define READ_SENSOR(stateArray, sensorIdx) ((stateArray)[(sensorIdx)] = digitalReadFast(SENSOR_##sensorIdx))
//#define CHECK_SENSOR(stateArray, sensorIdx, tempVar) (stateArray[sensorIdx] == (tempVar = digitalReadFast(SENSOR_##sensorIdx)) ? false : {array[sensorIdx] = tempVar; true })
#define UPDATE_SENSOR(stateArray, sensorIdx, tempVar, addrArray) \
  (tempVar) = digitalReadFast(SENSOR_##sensorIdx); \
  if ((stateArray)[(sensorIdx)] != (tempVar)) { \
    (stateArray)[(sensorIdx)] = (tempVar); \
    LocoNet.reportSensor((addrArray)[(sensorIdx)], ((stateArray)[(sensorIdx)] == LOW ? HIGH : LOW)); \
  }

// Macro to express whether var is in [lower, higher)
#define IS_IN_RANGE(lower, var, higher) ((lower) <= (var) && (var) < (higher))

// Macro for checking a switch address and then setting the switch
#define HANDLE_SWITCH(Address, addrArray, switchTimerActiveArray, switchTimerTimeArray, Direction, switchIdx) \
  if ((addrArray)[(switchIdx)] == (Address)) { \
    if ((Direction) == 0) { \
      digitalWriteFast(SWITCH_##switchIdx##_RT, LOW); \
      digitalWriteFast(SWITCH_##switchIdx##_GN, HIGH); \
    } else { \
      digitalWriteFast(SWITCH_##switchIdx##_GN, LOW); \
      digitalWriteFast(SWITCH_##switchIdx##_RT, HIGH); \
    } \
    (switchTimerActiveArray)[(switchIdx)] = true; \
    (switchTimerTimeArray)[(switchIdx)] = millis(); \
  }
  
#define HANDLE_SWITCH_TIMER(switchTimerActiveArray, switchTimerTimeArray, switchIdx, currentTime) \
  if ((switchTimerActiveArray)[(switchIdx)] && ((currentTime) - (switchTimerTimeArray)[(switchIdx)] > SWITCH_ACTIVE_TIME_MS)) { \
    digitalWriteFast(SWITCH_##switchIdx##_RT, LOW); \
    digitalWriteFast(SWITCH_##switchIdx##_GN, LOW); \
    (switchTimerActiveArray)[(switchIdx)] = false; \
  }
  
//#define HANDLE_SWITCH(Address, addrArray, switchTimerIdArray, Direction, switchIdx) \
//  if ((addrArray)[(switchIdx)] == (Address)) { \
//    timer.stop((switchTimerIdArray)[(switchIdx)]); \
//    if ((Direction) == 0) { \
//      digitalWriteFast(SWITCH_##switchIdx##_RT, LOW); \
//      (switchTimerIdArray)[(switchIdx)] = timer.pulseImmediate(SWITCH_##switchIdx##_GN, SWITCH_ACTIVE_TIME_MS, HIGH); \
//    } else { \
//      digitalWriteFast(SWITCH_##switchIdx##_GN, LOW); \
//      (switchTimerIdArray)[(switchIdx)] = timer.pulseImmediate(SWITCH_##switchIdx##_RT, SWITCH_ACTIVE_TIME_MS, HIGH); \
//    } \
//  }

// Define the arrays holding the addresses
uint16_t sensorAddress[SENSOR_COUNT];
uint16_t switchAddress[SWITCH_COUNT];

// Define the array for holding the sensor states
// TODO: This could be done more efficiently, however, this
// kind of storage allows for seamless integration with LocoNetClass::notifySensorState()
uint8_t sensorState[SENSOR_COUNT];

//int8_t switchTimerId[SWITCH_COUNT];
boolean switchTimerActive[SWITCH_COUNT];
unsigned long switchLastActive[SWITCH_COUNT];

// Parameters for addressing this module in LNCV programming
boolean programmingMode;

// ArtNr is 50020
#define ARTNR (5002)
uint16_t moduleAddress;

boolean reportInitial;

/* NOTE ON LNCV STORAGE
 * LNCV-Values are stores ass double-bytes. Meaning, on an ATMega8, we get 128 Values.
 * Value Assignment: LNCV 0 is the module Address.
 * LNCV 1, Bit 0 is "report initial"
 * LNCV 10-17 are the sensor addresses
 * LNCV 18-29 are reserved for sensor parameters
 * LNCV 30-33 are the switch addresses
 * LNCV 34-59 are reserved for switch parameters
 * WARNING: LNCV-Numbers might be jumbled, since we are using the EEPROMEx library
 */

lnMsg *LnPacket;

//Timer timer;

void loadLNCV();

void setup() {
  // Start Serial for debugging
#ifdef DO_DEBUG
  Serial.begin(115200);
#endif

  // Clear the switch timer array
  //memset(switchTimerId, (int8_t) TIMER_NOT_AN_EVENT, SWITCH_COUNT);
  memset(switchTimerActive, false, SWITCH_COUNT);
  
  // Set pin modes
  pinModeFast(SENSOR_0, INPUT);
  pinModeFast(SENSOR_1, INPUT);
  pinModeFast(SENSOR_2, INPUT);
  pinModeFast(SENSOR_3, INPUT);
  pinModeFast(SENSOR_4, INPUT);
  pinModeFast(SENSOR_5, INPUT);
  pinModeFast(SENSOR_6, INPUT);
  pinModeFast(SENSOR_7, INPUT);

  pinModeFast(SWITCH_0_RT, OUTPUT);
  pinModeFast(SWITCH_0_GN, OUTPUT);
  pinModeFast(SWITCH_1_RT, OUTPUT);
  pinModeFast(SWITCH_1_GN, OUTPUT);
  pinModeFast(SWITCH_2_RT, OUTPUT);
  pinModeFast(SWITCH_2_GN, OUTPUT);
  pinModeFast(SWITCH_3_RT, OUTPUT);
  pinModeFast(SWITCH_3_GN, OUTPUT);

  digitalWriteFast(SWITCH_0_RT, LOW);
  digitalWriteFast(SWITCH_0_GN, LOW);
  digitalWriteFast(SWITCH_1_RT, LOW);
  digitalWriteFast(SWITCH_1_GN, LOW);
  digitalWriteFast(SWITCH_2_RT, LOW);
  digitalWriteFast(SWITCH_2_GN, LOW);
  digitalWriteFast(SWITCH_3_RT, LOW);
  digitalWriteFast(SWITCH_3_GN, LOW);

// Click a relay to show that we are alive
  //digitalWriteFast(SWITCH_0_RT, HIGH);
  //delay(50);
  //digitalWriteFast(SWITCH_0_RT, LOW);
  //delay(1000);
  //digitalWriteFast(SWITCH_0_GN, HIGH);
  //delay(50);
  //digitalWriteFast(SWITCH_0_GN, LOW);

  // Read LNCVs
  programmingMode = false;
  loadLNCV();

  #ifdef DO_DEBUG
  Serial.print("ModuleAddress: ");
  Serial.print(moduleAddress);
  Serial.print("\n");
  #endif

  // Start LocoNet
  LocoNet.init(LOCONET_TX_PIN);
  
  // Initialize the sensor state
  READ_SENSOR(sensorState, 0);
  READ_SENSOR(sensorState, 1);
  READ_SENSOR(sensorState, 2);
  READ_SENSOR(sensorState, 3);
  READ_SENSOR(sensorState, 4);
  READ_SENSOR(sensorState, 5);
  READ_SENSOR(sensorState, 6);
  READ_SENSOR(sensorState, 7);
  
  if (reportInitial) {
    for (int i(0); i < SENSOR_COUNT; ++i) {
      delay(10);
      LocoNet.reportSensor(sensorAddress[i], (sensorState[i] == LOW ? HIGH : LOW));
    }
  }
}

void printPacket(lnMsg* LnPacket) {
  Serial.print(F("LoconetPacket 2"));
  Serial.print(LnPacket->ub.command, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.mesg_size, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.SRC, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.DSTL, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.DSTH, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.ReqId, HEX);
  Serial.print(" ");
  Serial.print(LnPacket->ub.PXCT1, HEX);
  for (int i(0); i < 7; ++i) {
    Serial.print(" ");
    Serial.print(LnPacket->ub.D[i], HEX);
  }
  Serial.print("\n");
}

void loop() {
  //timer.update();
  unsigned long currentTime(millis());
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 0, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 1, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 2, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 3, currentTime)
  
  // Process Loconet
  LnPacket = LocoNet.receive();
  uint8_t packetConsumed(0);
  if( LnPacket ) {
    packetConsumed = LocoNet.processSwitchSensorMessage(LnPacket);
    if (packetConsumed == 0) {
      #ifdef DO_DEBUG
      printPacket(LnPacket);
      #endif
      packetConsumed = LocoNet.processLNCVMessage(LnPacket);
    }
  }
  
  // check Sensors for modification and send out updates
  uint8_t tempVar;
  UPDATE_SENSOR(sensorState, 0, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 1, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 2, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 3, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 4, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 5, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 6, tempVar, sensorAddress);
  UPDATE_SENSOR(sensorState, 7, tempVar, sensorAddress);
}

void loadLNCV() {
  // Load Module Address
  moduleAddress = EEPROM.readInt(0);

  reportInitial = EEPROM.readBit(3, 0);

  // Load Sensor Addresses
  for (int i(0); i < SENSOR_COUNT; ++i) {
    uint8_t lncvAddress(10 + (i*2));
    sensorAddress[i] = EEPROM.readInt(lncvAddress);
    if (sensorAddress[i] == 0xFFFF) {
      sensorAddress[i] = i + 1;
    }
  }
  
  // Load Switch Addresses
  for (int i(0); i < SWITCH_COUNT; ++i) {
    uint8_t lncvAddress(30 + (i*2));
    switchAddress[i] = EEPROM.readInt(lncvAddress);
    if (switchAddress[i] == 0xFFFF) {
      switchAddress[i] = i + 1;
    }
  }
  
  // Nothing else to be loaded right now
}

void commitLNCV() {
  // Store Module Address
  EEPROM.updateInt(0, moduleAddress);
  
  // Store ReportInitial Bit
  EEPROM.updateBit(3, 0, reportInitial);
  
  // Store Sensor Addresses
  for (int i(0); i < SENSOR_COUNT; ++i) {
    uint8_t lncvAddress(10 + (i*2));
    EEPROM.updateInt(lncvAddress, sensorAddress[i]);
  }
  
  // Store Switch Addresses
  for (int i(0); i < SWITCH_COUNT; ++i) {
    uint8_t lncvAddress(30 + (i*2));
    EEPROM.updateInt(lncvAddress, switchAddress[i]);
  }
  
  // Nothing else to be stored right now
}

// Handle the Switches
void notifySwitchRequest( uint16_t Address, uint8_t Output, uint8_t Direction ) {
  // We auto-disable the switches, so we can ignore Output-off-values
  if (Output == 0) {
    return;
  }

  HANDLE_SWITCH(Address, switchAddress, switchTimerActive, switchLastActive, Direction, 0);
  HANDLE_SWITCH(Address, switchAddress, switchTimerActive, switchLastActive, Direction, 1);
  HANDLE_SWITCH(Address, switchAddress, switchTimerActive, switchLastActive, Direction, 2);
  HANDLE_SWITCH(Address, switchAddress, switchTimerActive, switchLastActive, Direction, 3);
}
  
// Following here: LNCV handling

int8_t notifyLNCVread( uint16_t ArtNr, uint16_t lncvAddress, uint16_t, uint16_t & lncvValue ) {
  #ifdef DO_DEBUG
  Serial.print("notifyLNCVread\n");
  #endif
  if (programmingMode && ArtNr == ARTNR) {
    if (lncvAddress == 0) {
        // module address
        lncvValue = moduleAddress;
        return LNCV_LACK_OK;
    } else if (lncvAddress == 1) {
        // settings
        lncvValue = (reportInitial ? 0x01 : 0x00);
        return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(10, lncvAddress, 18)) {
      // Sensor Adresses carry an offset - Sensor 1 is encoded as 0x0000.
      lncvValue = sensorAddress[lncvAddress - 10] + 1;
      return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(30, lncvAddress, 34)) {
      lncvValue = switchAddress[lncvAddress - 30];
      return LNCV_LACK_OK;
    }
    return LNCV_LACK_ERROR_UNSUPPORTED;
  } else {
    return -1;
  }
}

int8_t notifyLNCVprogrammingStart( uint16_t & ArtNr, uint16_t & ModuleAddress ) {
  #ifdef DO_DEBUG
  Serial.print("notifyLNCVProgrammingStart ");
  Serial.print(ArtNr);
  Serial.print(" ");
  Serial.print(ModuleAddress);
  Serial.print("\n");
  #endif
  //delay(10); // without some delay, apparently we cannot enter programming mode???
  // Enter programming mode. If we already are in programming mode,
  // we simply send a response and nothing else happens.
  if (ArtNr == ARTNR && (ModuleAddress == moduleAddress || ModuleAddress == 0xFFFF)) {
    programmingMode = true;
    if (ModuleAddress == 0xFFFF) {
      ModuleAddress = moduleAddress;
    }
    return LNCV_LACK_OK;
  } else {
    return LNCV_LACK_ERROR_GENERIC;
  }
}

/**
 * Notifies the code on the reception of a write request
 * Changes are applied immediately, but only persist across a reset
 * if the programming sequence is prroperly ended.
 */
int8_t notifyLNCVwrite( uint16_t ArtNr, uint16_t lncvAddress, uint16_t lncvValue ) {
  #ifdef DO_DEBUG
  Serial.print("notifyLNCVwrite\n");
  #endif
  if (programmingMode && ArtNr == ARTNR) {
    if (lncvAddress == 0) {
        // module address
        moduleAddress = lncvValue;
        return LNCV_LACK_OK;
    } else if (lncvAddress == 1) {
        // settings
        reportInitial = (lncvValue & 0x01);
        return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(10, lncvAddress, 18)) {
      // Sensor Adresses carry an offset - Sensor 1 is encoded as 0x0000.
      sensorAddress[lncvAddress - 10] = lncvValue - 1;
      return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(30, lncvAddress, 34)) {
      switchAddress[lncvAddress - 30] = lncvValue;
      return LNCV_LACK_OK;
    }
    return LNCV_LACK_ERROR_UNSUPPORTED;
  } else {
    return -1;
  }
}

/**
 * Notifies the code on the reception of a request to end programming mode
 */
void notifyLNCVprogrammingStop( uint16_t ArtNr, uint16_t ModuleAddress ) {
  #ifdef DO_DEBUG
  Serial.print("notifyLNCVProgrammingStop\n");
  #endif
  if (programmingMode && ArtNr == ARTNR && ModuleAddress == moduleAddress) {
    programmingMode = false;
    commitLNCV();
  }
}
