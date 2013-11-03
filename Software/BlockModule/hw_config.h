#ifndef __HW_CONFIG_H__
#define __HW_CONFIG_H__

// Loconet Pin
// currently, nohting specific.

#define LOCONET_TX_PIN 7

#if (HW_Ver == 1)

// define what Arduino pins serve as which function on this hardware
// Give the arduino pin number. comments contain the atmel port and the pin on the DIL-28 package

// Comments: AVR Pin Name and DIL28 Pin Number

#define SENSOR_COUNT 8
#define SENSOR_0 (A5) // PC5, 28
#define SENSOR_1 (A4) // PC4, 27
#define SENSOR_2 (A3) // PC3, 26
#define SENSOR_3 (A2) // PC2, 25
#define SENSOR_4 (A1) // PC1, 24
#define SENSOR_5 (A0) // PC0, 23
#define SENSOR_6 (2)  // PD2,  4
#define SENSOR_7 (3)  // PD3,  5

#define SWITCH_COUNT 4
#define SWITCH_0_RT (10) // PB2, 16
#define SWITCH_0_GN (9)  // PB1, 15
#define SWITCH_1_RT (4)  // PD4,  6
#define SWITCH_1_GN (11) // PB3, 17
#define SWITCH_2_RT (13) // PB5, 13
#define SWITCH_2_GN (12) // PB4, 18
#define SWITCH_3_RT (5)  // PD5, 11
#define SWITCH_3_GN (6)  // PD6, 12

#elif (HW_Ver == 2)

#define SENSOR_COUNT 8
#define SENSOR_0 (A5) // PC5, 28
#define SENSOR_1 (A4) // PC4, 27
#define SENSOR_2 (A3) // PC3, 26
#define SENSOR_3 (A2) // PC2, 25
#define SENSOR_4 (A1) // PC1, 24
#define SENSOR_5 (A0) // PC0, 23
#define SENSOR_6 (10) // PB2, 16
#define SENSOR_7 (9)  // PB1, 15

#define SWITCH_COUNT 4
#define SWITCH_0_RT (6)  // PD6, 12
#define SWITCH_0_GN (5)  // PD5, 11
#define SWITCH_1_RT (11) // PB3, 17
#define SWITCH_1_GN (12) // PB4, 18
#define SWITCH_2_RT (4)  // PD4,  6
#define SWITCH_2_GN (13) // PB5, 19
#define SWITCH_3_RT (3)  // PD3,  5
#define SWITCH_3_GN (2)  // PD2,  4

#elif (HW_Ver == 3)

#define SENSOR_COUNT 8
#define SENSOR_0 (A5) // PC5, 28
#define SENSOR_1 (A4) // PC4, 27
#define SENSOR_2 (A3) // PC3, 26
#define SENSOR_3 (A2) // PC2, 25
#define SENSOR_4 (A1) // PC1, 24
#define SENSOR_5 (A0) // PC0, 23
#define SENSOR_6 (10) // PB2, 16
#define SENSOR_7 (9)  // PB1, 15

#define SWITCH_COUNT 4
#define SWITCH_0_RT (12) // PB4, 18
#define SWITCH_0_GN (11) // PB3, 17
#define SWITCH_1_RT (13) // PB5, 19
#define SWITCH_1_GN (4)  // PD4,  6
#define SWITCH_2_RT (6)  // PD6, 12
#define SWITCH_2_GN (5)  // PD5, 11
#define SWITCH_3_RT (3)  // PD3,  5
#define SWITCH_3_GN (2)  // PD2,  4

#elif (HW_Ver == 4)

#define SENSOR_COUNT 0

#define SWITCH_COUNT 4
#define SWITCH_0_RT (2) // PD2, 4
#define SWITCH_0_GN (3) // PD3, 5
#define SWITCH_1_RT (4) // PD4, 6
#define SWITCH_1_GN (5)  // PD5, 11
#define SWITCH_2_RT (6)  // PD6, 12
#define SWITCH_2_GN (A3)  // PC3, 26
#define SWITCH_3_RT (A5)  // PC5, 28
#define SWITCH_3_GN (A4)  // PC4, 27

#endif

#endif
