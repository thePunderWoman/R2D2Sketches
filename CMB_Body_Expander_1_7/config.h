// CMB Servo Expander Body v 1.7
//Configuration Header

// -------------------------------------------------
// Define some constants to help reference objects,
// pins, servos, leds etc by name instead of numbers
// -------------------------------------------------

// I2C Address
#define I2CAddress 9      //I2C Address for Body Servo Expander

// Destination I2C addresses
#define STEALTH_CNTL 0    // Main Stealth Controller (Master for the Body I2C bus)
#define BODY_EXP 9        // Body Servo Expander
#define DOME_EXP 12       // Dome Servo Expander
#define LOGICS 10         // Rseries Logics (Teensy version)
#define MAGIC_PNL 20      // Magic Panel
#define PSI_FRNT 22       // Front PSI
#define PSI_REAR 23       // Rear PSI
#define FLTHY_HP 25       // FlthyHP Breakout Board
#define PERI_LIFT 32      // Periscope Lifter

// Our Status LED on Arduino Digital IO #13, this is the built in LED on the Pro Mini
#define STATUS_LED 13

// These are easy names for our Arduino I/O pins and what they're used for
// Note on the Servo expander board, numbering starts at 1,2,3 etc.
// but internally we add 1 to get the real Arduino number.

#define LEFT_DOOR_SERVO_PIN 2        // Left Front Door Servo. Arduino Digitial IO #2
#define RIGHT_DOOR_SERVO_PIN 3       // Right Front Door Servo. Arduino Digital IO #3
#define TOP_UTIL_ARM_SERVO_PIN 4  // Top Utility Arm. Arduino Digitial IO #4
#define BOTTOM_UTIL_ARM_SERVO_PIN 5     // Bottom Utility Arm. Arduino Digitial IO #5
#define CBI_DOOR_SERVO_PIN 6         // Charge Bay Door. Arduino Digitial IO #6
#define DATA_DOOR_SERVO_PIN 7        // Data Panel Door. Arduino Digitial IO #7
#define CBI_SWITCH_PIN 8            // Charge Bay On/Off. Arduino Digital IO #8
#define DP_SWITCH_PIN 9           // Data Panel On/Off. Arduino Digital IO #9
#define VM_SWITCH_PIN 14             // Volt Meter On/Off. Arudion Digital IO #14 (A0)



// Create an array of VarSpeedServo type, containing 7 elements/entries.
// Note: Arrays are zero indexed. See http://arduino.cc/en/Reference/array

#define NBR_SERVOS 6  // Number of Servos
VarSpeedServo Servos[NBR_SERVOS]; // An Array of Servos, numbered 0 thru 5.

// More easy names  so we can reference the array element/row by name instead
// trying to remember the number.
// Note: This is different to the pin number we attach each servo to.

#define LEFT_DOOR 0
#define RIGHT_DOOR 1
#define CBI_DOOR 2
#define DATA_DOOR 3
#define BOTTOM_UTIL_ARM 4
#define TOP_UTIL_ARM 5

#define FIRST_SERVO_PIN 2 //First Arduino Pin for Servos
#define NEUTRAL 90 //Start/Neutral/Center Position of our servos

//Servo Speeds
#define UTILITYARMSSPEED 100 // servo speed for opening utility arms. 1=super slow, 255=fastest
#define UTILITYARMSSPEED2 45 // servo speed for opening utility arms. 1=super slow, 255=fastest
#define UTILITYARMSSPEED3 35 // servo speed for opening utility arms. 1=super slow, 255=fastest

#define DOOR_OPEN_SPEED 120 // servo speed for opening door
#define DOOR_CLOSE_SPEED 100 // servo speed for opening door

#define SCREAM_SPEED 250

//Tweaked Pulse Widths. Usually 1000-2000 or 500-2500
#define ARMMINPULSE 600
#define ARMMAXPULSE 2400

#define RIGHT_DOOR_MINPULSE 650
#define RIGHT_DOOR_MAXPULSE 2200

#define LEFT_DOOR_MINPULSE 650
#define LEFT_DOOR_MAXPULSE 2200

#define CBI_DOOR_MINPULSE 650
#define CBI_DOOR_MAXPULSE 2200

#define DATA_DOOR_MINPULSE 650
#define DATA_DOOR_MAXPULSE 2200

//Servo Positions
#define TOP_ARM_OPEN 650
#define BOTTOM_ARM_OPEN 750
#define LEFT_DOOR_OPEN 1300
#define RIGHT_DOOR_OPEN 1400
#define CBI_DOOR_OPEN 1780
#define DATA_DOOR_OPEN 1690

#define TOP_ARM_CLOSE 1780
#define BOTTOM_ARM_CLOSE 1800
#define LEFT_DOOR_CLOSE 750
#define RIGHT_DOOR_CLOSE 2100
#define CBI_DOOR_CLOSE 1200
#define DATA_DOOR_CLOSE 1000

// change this to match which Arduino pins you connect your panel to,
// which can be any 3 digital pins you have available.
#define DATAIN_PIN 10
#define CLOCK_PIN  11
#define LOAD_PIN   12

//Set this to which Analog Pin you use for the voltage in.
#define analoginput A3

//Battery Voltage
//#define TWELVE // Uncomment if you have a 12v system
// #define EIGHTEEN //Uncomment if you have an 18v system
#define TWENTYFOUR //Uncomment if you have a 24v system

// If your JEDI Device can send at 9600 baud, uncomment this line.
// The Current Teeces interface runs at a mindnumbingly slow 2400 only!

#define _9600BAUDSJEDI_

// Uncomment this if you want Debug output.
#define DEBUG

// Uncomment this to show the Volt Meter output in Debug
//#define DEBUG_VM

// Voltage divider capable of accepting up to 30v input
#define R1 98800.0     // >> resistance of R1 in ohms << the more accurate these values are
#define R2 10020.0     // >> resistance of R2 in ohms << the more accurate the measurement will be

// change this to match your hardware chain
#define CBI      0 // CBI first in chain (device 0)
#define DATAPORT 1 // the dataport second in chain (device 1)

// Number of Maxim chips that are connected
#define NUMDEV 2   // One for the dataport, one for the battery indicator

// the dataport is quite dim, so I put it to the maximum
#define DATAPORTINTENSITY 15  // 15 is max
#define CBIINTENSITY 15  // 15 is max

// uncomment this to test the LEDS one after the other at startup
//#define TEST
// This will revert to the old style block animation for comparison
//#define LEGACY

// If you are using the voltage monitor uncomment this
#define monitorVCC

// the timing values below control the various effects. Tweak to your liking.
// values are in ms. The lower the faster.
#define TOPBLOCKSPEED   70
#define BOTTOMLEDSPEED  200
#define REDLEDSPEED     500
#define BLUELEDSPEED    500
#define BARGRAPHSPEED   200
#define CBISPEED        50

// Uncomment this if you want an alternate effect for the blue LEDs, where it moves
// in sync with the bar graph
//#define BLUELEDTRACKGRAPH

//LED Voltage indicator thresholds
#ifdef TWELVE
  #define greenVCC 12.5    // Green LED on if above this voltage
  #define yellowVCC 12.0   // Yellow LED on if above this voltage
  #define redVCC 11.5      // Red LED on if above this voltage
#elif defined(EIGHTEEN)
  #define greenVCC 18.0    // Green LED on if above this voltage
  #define yellowVCC 17.0   // Yellow LED on if above this voltage
  #define redVCC 15.0      // Red LED on if above this voltage
#elif defined(TWENTYFOUR)
  #define greenVCC 25.0    // Green LED on if above this voltage
  #define yellowVCC 24.0   // Yellow LED on if above this voltage
  #define redVCC 23.0      // Red LED on if above this voltage
#endif

//Setup Debug Switch
#ifdef DEBUG
#define DEBUG_PRINT_LN(msg)  Serial.println(msg)
#define DEBUG_PRINT(msg)  Serial.print(msg)
#define DEBUG_PRINT_LN_DEC(msg, dec_places) Serial.println(msg, dec_places)
#define DEBUG_PRINT_DEC(msg, dec_places) Serial.print(msg, dec_places)
#else
#define DEBUG_PRINT_LN(msg)
#define DEBUG_PRINT(msg)
#define DEBUG_PRINT_LN_DEC(msg, dec_places)
#define DEBUG_PRINT_DEC(msg, dec_places)
#endif

// Command processing stuff
// maximum number of characters in a command (63 chars since we need the null termination)
#define CMD_MAX_LENGTH 64

// memory for command string processing
char cmdString[CMD_MAX_LENGTH];
