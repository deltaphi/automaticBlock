#include <EEPROMEx.h>

#include <LocoNet.h>

#include <digitalWriteFast.h>

#define DO_DEBUG

// Harware Version for Prototype
#define HW_Ver 0x01
// Hardware Version for HFD-2-L2 modules
// #define HW_Ver 0x02
// Hardware Version for HFD-3-L2 modules
// #define HW_Ver 0x03
// include HW_Ver before this!
#include "hw_config.h"

// Software version. 0x00 for development version
#define SW_VER 0x00

// maximum time any relay coil is active in milliseconds
#define SWITCH_ACTIVE_TIME_MS 50

#define sensorTimeThresholdFactor 10.0

#ifdef DO_DEBUG
#define DEBUG(x) Serial.print(x)
#else
#define DEBUG(x)
#endif

// Macro for reading a sensor value and possibly sending an update message
#define UPDATE_SENSOR(stateArray, timeArray, timeThreshold, reportedArray, sensorIdx, tempVar, addrArray, currentTime) \
  (tempVar) = digitalReadFast(SENSOR_##sensorIdx); \
  if ((stateArray)[(sensorIdx)] != (tempVar)) { \
    (stateArray)[(sensorIdx)] = (tempVar); \
    ((timeArray)[(sensorIdx)]) = currentTime; \
  } else \
  if (((reportedArray)[(sensorIdx)] != (stateArray)[(sensorIdx)]) && ((currentTime) - (timeArray)[(sensorIdx)] > ((sensorTimeThresholdFactor) * sensorTimeThreshold))) { \
    LocoNet.reportSensor((addrArray)[(sensorIdx)], ((stateArray)[(sensorIdx)] == LOW ? HIGH : LOW)); \
    (reportedArray)[(sensorIdx)] = (stateArray)[(sensorIdx)]; \
    DEBUG(sensorIdx); DEBUG(": "); \
    DEBUG((stateArray)[(sensorIdx)] == LOW ? "HIGH\n" : "LOW\n"); \
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
  
// Define the arrays holding the addresses
uint16_t sensorAddress[SENSOR_COUNT];
uint16_t switchAddress[SWITCH_COUNT];

// Define the array for holding the sensor states
// TODO: This could be done more efficiently, however, this
// kind of storage allows for seamless integration with LocoNetClass::notifySensorState()

uint8_t sensorState[SENSOR_COUNT];
unsigned long sensorStateTime[SENSOR_COUNT];
uint8_t sensorStateReported[SENSOR_COUNT];

// For how long does a sensor value have to be stable before we report it?
uint8_t sensorTimeThreshold(0);
// Value is in 10ms. I.e., a value of 50 would cause the code to wait for 500ms (1/2s) before reporting!

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

void setupPinMode() {
  {
  pinModeFast(SENSOR_0, INPUT);
  pinModeFast(SENSOR_1, INPUT);
  pinModeFast(SENSOR_2, INPUT);
  pinModeFast(SENSOR_3, INPUT);
  pinModeFast(SENSOR_4, INPUT);
  pinModeFast(SENSOR_5, INPUT);
  pinModeFast(SENSOR_6, INPUT);
  pinModeFast(SENSOR_7, INPUT);
  }

  {
  pinModeFast(SWITCH_0_RT, OUTPUT);
  pinModeFast(SWITCH_0_GN, OUTPUT);
  pinModeFast(SWITCH_1_RT, OUTPUT);
  pinModeFast(SWITCH_1_GN, OUTPUT);
  pinModeFast(SWITCH_2_RT, OUTPUT);
  pinModeFast(SWITCH_2_GN, OUTPUT);
  pinModeFast(SWITCH_3_RT, OUTPUT);
  pinModeFast(SWITCH_3_GN, OUTPUT);
  }
}

void setupInitialValues() {
 {
  digitalWriteFast(SWITCH_0_RT, LOW);
  digitalWriteFast(SWITCH_0_GN, LOW);
  digitalWriteFast(SWITCH_1_RT, LOW);
  digitalWriteFast(SWITCH_1_GN, LOW);
  digitalWriteFast(SWITCH_2_RT, LOW);
  digitalWriteFast(SWITCH_2_GN, LOW);
  digitalWriteFast(SWITCH_3_RT, LOW);
  digitalWriteFast(SWITCH_3_GN, LOW);
  }
 
}

#ifdef DO_DEBUG
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
#endif

void setup() {
  // Start Serial for debugging
#ifdef DO_DEBUG
  Serial.begin(115200);
  Serial.print(F("Starting up...\n"));
  //Serial.print(freeRam());
  Serial.print("ModuleAddress: ");
  Serial.print(moduleAddress);
  Serial.print("\n");
#endif

  // Clear the switch timer array
  //memset(switchTimerId, (int8_t) TIMER_NOT_AN_EVENT, SWITCH_COUNT);
  memset(switchTimerActive, false, SWITCH_COUNT);
  
  //Serial.println("Setting Pin Modes");
  //delay(500);
  
  // Set pin modes
  setupPinMode();
  setupInitialValues();
  
// Click a relay to show that we are alive
  //digitalWriteFast(SWITCH_0_RT, HIGH);
  //delay(50);
  //digitalWriteFast(SWITCH_0_RT, LOW);
  //delay(1000);
  //digitalWriteFast(SWITCH_0_GN, HIGH);
  //delay(50);
  //digitalWriteFast(SWITCH_0_GN, LOW);

  #ifdef DO_DEBUG
  Serial.print(F("Reading LNCVs\n"));
  #endif
  
  // Read LNCVs
  programmingMode = false;
  loadLNCV();

  #ifdef DO_DEBUG
  Serial.print("ModuleAddress: ");
  Serial.print(moduleAddress);
  Serial.print("\n");
  #endif
  if (moduleAddress == 0) {
    moduleAddress = 1;
  }

  // Start LocoNet
  LocoNet.init(LOCONET_TX_PIN);
  
  // Initialize the sensor state
	checkSensors();
  
  if (reportInitial) {
    for (int i(0); i < SENSOR_COUNT; ++i) {
			// To force a report, set to the opposite of what we currently see
  	  sensorStateReported[i] = (sensorState[i] == LOW ? HIGH : LOW);
    }
  } else {
  	for (int i(0); i < SENSOR_COUNT; ++i) {
			// To suppress a report, set to the value we currently see
  	  sensorStateReported[i] = sensorState[i];
  	}
  }
  
#ifdef DO_DEBUG
//  Serial.print(freeRam());
  Serial.print(F("Setup done.\n"));
#endif
}


#ifdef DO_DEBUG
void printPacket(lnMsg* LnPacket) {
  Serial.print("LoconetPacket ");
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
    Serial.print(LnPacket->ub.payload.D[i], HEX);
  }
  Serial.print("\n");
}
#endif

LocoNetCVClass LNCVhandler;

void handleSwitchTimers() {
  unsigned long currentTime(millis());
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 0, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 1, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 2, currentTime)
  HANDLE_SWITCH_TIMER(switchTimerActive, switchLastActive, 3, currentTime)
}

void checkSensors() {
   // check Sensors for modification and send out updates
  uint8_t tempVar;
  unsigned long now(millis());
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 0, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 1, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 2, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 3, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 4, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 5, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 6, tempVar, sensorAddress, now);
  UPDATE_SENSOR(sensorState, sensorStateTime, sensorTimeThreshold, sensorStateReported, 7, tempVar, sensorAddress, now);
}

void loop() {
  handleSwitchTimers();
  
  // Process Loconet
  LnPacket = LocoNet.receive();
  uint8_t packetConsumed(0);
  if( LnPacket ) {
    packetConsumed = LocoNet.processSwitchSensorMessage(LnPacket);
    if (packetConsumed == 0) {
#ifdef DO_DEBUG
      printPacket(LnPacket);
#endif
      packetConsumed = LNCVhandler.processLNCVMessage(LnPacket);
    }
  }
  
  checkSensors();
}

void loadLNCV() {
  // Load Module Address
  moduleAddress = EEPROM.readInt(0);

  reportInitial = EEPROM.readBit(3, 0);

  sensorTimeThreshold = EEPROM.readByte(5);
  if (sensorTimeThreshold == 0xFF) {
    sensorTimeThreshold = 0;
  }

  // Load Sensor Addresses
  for (unsigned int i(0); i < SENSOR_COUNT; ++i) {
    uint8_t lncvAddress(10 + (i*2));
    sensorAddress[i] = EEPROM.readInt(lncvAddress);
    if (sensorAddress[i] == 0xFFFF) {
      sensorAddress[i] = i;
    }
    sensorAddress[i] += 1;
  }
  
  // Load Switch Addresses
  for (unsigned int i(0); i < SWITCH_COUNT; ++i) {
    uint8_t lncvAddress(30 + (i*2));
    switchAddress[i] = EEPROM.readInt(lncvAddress);
    if (switchAddress[i] == 0xFFFF) {
      switchAddress[i] = i;
    }
    switchAddress[i] += 1;
  }
  
  // Nothing else to be loaded right now
}

void resetLNCV() {
  moduleAddress = 0xFFFF;
  reportInitial = true;
  sensorTimeThreshold = 0xFF;
  // Store Sensor Addresses
  for (int i(0); i < SENSOR_COUNT; ++i) {
    uint8_t lncvAddress(10 + (i*2));
    sensorAddress[i] = sensorAddress[i] + 1;
  }
  
  // Store Switch Addresses
  for (int i(0); i < SWITCH_COUNT; ++i) {
    uint8_t lncvAddress(30 + (i*2));
    switchAddress[i] = switchAddress[i] + 1;
  }
}

void commitLNCV() {
  // Store Module Address
  EEPROM.updateInt(0, moduleAddress);
  
  // Store ReportInitial Bit
  EEPROM.updateBit(3, 0, reportInitial);
  
  // Store reporting threshold
  EEPROM.updateByte(5, sensorTimeThreshold);
  
  // Store Sensor Addresses
  for (int i(0); i < SENSOR_COUNT; ++i) {
    uint8_t lncvAddress(10 + (i*2));
    EEPROM.updateInt(lncvAddress, sensorAddress[i] - 1);
  }
  
  // Store Switch Addresses
  for (int i(0); i < SWITCH_COUNT; ++i) {
    uint8_t lncvAddress(30 + (i*2));
    EEPROM.updateInt(lncvAddress, switchAddress[i] - 1);
  }
  
  // Nothing else to be stored right now
  loadLNCV();
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
    } else if (lncvAddress == 2) {
        // reporting threshold
        lncvValue = sensorTimeThreshold;
        return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(10, lncvAddress, 18)) {
      lncvValue = sensorAddress[lncvAddress - 10];
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
    } else if (lncvAddress == 2) {
        // reporting threshold
        if (IS_IN_RANGE(0, lncvValue, 254)) {
          sensorTimeThreshold = lncvValue;
          return LNCV_LACK_OK;
        } else {
          return LNCV_LACK_ERROR_OUTOFRANGE;
        }
    } else if (lncvAddress == 8) {
        // reset all configuration data
        resetLNCV();
        return LNCV_LACK_OK;
    } else if (IS_IN_RANGE(10, lncvAddress, 18)) {
      sensorAddress[lncvAddress - 10] = lncvValue;
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
