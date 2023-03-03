// CMB Servo Expander Body v 1.7
// Based on Work by Chris James (v1.14) 3-10-2017
// Modified by Chris Bozzoli (DBoz)
//
//v1.2 Added Individual Utility Arm Movment 7-10-18
//v1.3 Replaced all delays with watiTime funciton 4-12-20
//v1.4 Rearranged Switch/Case Numbers to align with Dome 4-21-20
//v1.5 Removed Back Door code, renumbered servo pins, Added Open Everything Funciton 5-21-20
//V1.6 Optimized I2C function 8-20-20
//V1.7 Integrated CBI_DataPanel_SA_Breakout 1.0 code 2/24/21
// -------------------------------------------------

//Variable Speed Servo Library with sequencing ability
//https://github.com/netlabtoolkit/VarSpeedServo
#include <VarSpeedServo.h>

#include <LedControl.h>
#include <Wire.h>
#include "config.h"
#include <hcr.h>

float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calculated... since the divider allows for 15 volts

//Incoming I2C Command
int I2CCommand = 0;

int voiceVolume = 90;
int chaVolume = 40;
int chbVolume = 50;
bool muted = false;

unsigned long loopTime; // Time variable

// Some variables to keep track of doors and arms etc.
bool utilityArmOpen = false;
bool topUtilityArmOpen = false;
bool bottomUtilityArmOpen = false;
bool leftDoorOpen = false;
bool rightDoorOpen = false;
bool cbiDoorOpen = false;
bool dataDoorOpen = false;
bool doorsOpen = false;
bool cbi_dataOpen = false;

// Instantiate LedControl driver
LedControl lc = LedControl(DATAIN_PIN, CLOCK_PIN, LOAD_PIN, NUMDEV);

HCRVocalizer HCR(0x1E,Wire);

void setup()
{
  // Setup the Serial for debug and command input
  // initialize suart used to communicate with the JEDI Display at 2400 or 9600 bauds
  // This is set in config.h
  uint16_t baudrate;

#ifdef _9600BAUDSJEDI_
  baudrate = 9600;
#else
  baudrate = 2400;
#endif

  Serial.begin(baudrate);

  Wire.begin(I2CAddress);                   // Start I2C Bus as Slave I2C Address
  Wire.onReceive(receiveEvent);            // register event so when we receive something we jump to receiveEvent();

  // initialize Maxim driver chips
  lc.shutdown(DATAPORT, false);                 // take out of shutdown
  lc.clearDisplay(DATAPORT);                    // clear
  lc.setIntensity(DATAPORT, DATAPORTINTENSITY); // set intensity

  lc.shutdown(CBI, false);                      // take out of shutdown
  lc.clearDisplay(CBI);                         // clear
  lc.setIntensity(CBI, CBIINTENSITY);           // set intensity

#ifdef  TEST// test LEDs
  singleTest();
  waitTime(2000);
#endif

#ifndef monitorVCC
  pinMode(analoginput, INPUT);
#endif

  DEBUG_PRINT_LN(F("Stealth RC - CMB Body Servo Expander v1.7 (3-17-21)"));

  DEBUG_PRINT(F("My i2C Address: "));
  DEBUG_PRINT_LN(I2CAddress);

  pinMode(STATUS_LED, OUTPUT); // turn status led off
  digitalWrite(STATUS_LED, LOW);

  pinMode(VM_SWITCH_PIN, OUTPUT); //Sets digital pin as an output
  pinMode(CBI_SWITCH_PIN, OUTPUT); //Sets digital pin as an output
  pinMode(DP_SWITCH_PIN, OUTPUT); //Sets digital pin as an output

  digitalWrite(CBI_SWITCH_PIN, LOW); //Sets output of digital pin low to turn off lights
  digitalWrite(DP_SWITCH_PIN, LOW);
  digitalWrite(VM_SWITCH_PIN, LOW);

  DEBUG_PRINT(F("Activating Servos"));
  resetServos();
  
  HCR.begin();

  resetVocalizer();
  
  DEBUG_PRINT_LN(F("Setup Complete"));
}

void loop() {
  if (digitalRead(CBI_SWITCH_PIN) == LOW)
  {
    lc.shutdown(CBI, true);
  }
  else
  {
    lc.shutdown(CBI, false);
  }
  if (digitalRead(DP_SWITCH_PIN) == LOW)
  {
    lc.shutdown(DATAPORT, true);
  }
  else
  {
    lc.shutdown(DATAPORT, false);
  }

  // this is the new code. Every block of LEDs is handled independently
  updateTopBlocks();
  bargraphDisplay(0);
  updatebottomLEDs();
  updateRedLEDs();
  updateCBILEDs();
#ifdef monitorVCC
  getVCC();
#endif
#ifndef BLUELEDTRACKGRAPH
  updateBlueLEDs();
#endif
  doCommand();
}

//----------------------------------------------------------------------------
//  Vocalizer Commands
//----------------------------------------------------------------------------

void playScream() {
  HCR.Stimulate(SCARED, EMOTE_STRONG);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void playLeia() {
  HCR.PlayWAV(CH_A, "0000");
  // sendI2C(30, "<CA0000>\r", false);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void playVader() {
  HCR.PlayWAV(CH_A, "0002");
}

void playCantina() {
  HCR.PlayWAV(CH_A, "0003");
}

void playSWTheme() {
  HCR.PlayWAV(CH_A, "0001");
}

// Emote Events

void enableMuse() {
  HCr.SetMuse(true);
  // sendI2C(30, "<M1>\r", false);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void disableMuse() {
  HCR.SetMuse(false);
  // sendI2C(30, "<M0>\r", false);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void resetVocalizer() {
  DEBUG_PRINT_LN(F("RESETTING VOCALIZER"));
  HCR.ResetEmotions();
  HCR.StopWAV(CH_A);
  enableMuse();
}

void toggleMute() {
  if (muted) {
    DEBUG_PRINT_LN(F("UNMUTING"));
    HCR.SetVolume(CH_V, voiceVolume);
    HCR.SetVolume(CH_A, chaVolume);
    muted = false;
  } else {
    DEBUG_PRINT_LN(F("MUTING"));
    HCR.SetVolume(CH_V, 0);
    HCR.SetVolume(CH_A, 0);
    muted = true;
  }
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void sadEmote(int level = EMOTE_MODERATE) {
  HCR.Stimulate(SAD, level);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void happyEmote(int level = EMOTE_MODERATE) {
  HCR.Stimulate(HAPPY, level);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void madEmote(int level = EMOTE_MODERATE) {
  HCR.Stimulate(MAD, level);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void scaredEmote(int level = EMOTE_MODERATE) {
  HCR.Stimulate(SCARED, level);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

void overloadEmote() {
  HCR.Overload();
  // sendI2C(30, "<SE>\r", false);
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
}

// TODO: Add double click functionality to make the Emote level strong
// TODO: Pause emotes while playing

// Events
void Leia() {
  digitalWrite(STATUS_LED, HIGH);

  disableMuse();
  HCR.StopEmote();
  playLeia();
  waitTime(36000); // wait until Leia is done to re-enable
  enableMuse();
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void Vader() {
  digitalWrite(STATUS_LED, HIGH);

  playVader();
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void Theme() {
  digitalWrite(STATUS_LED, HIGH);

  playSWTheme();
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void Cantina() {
  digitalWrite(STATUS_LED, HIGH);

  playCantina();
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void overload() {
  digitalWrite(STATUS_LED, HIGH);
  // TODO: add servo functionality

  overloadEmote();
  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//----------------------------------------------------------------------------
//  Sequences
//----------------------------------------------------------------------------

//-----------------------------------------------------
// Reset/Close All
//-----------------------------------------------------

void resetServos() {

  Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED3);

  Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED3);

  Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
  Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
  Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
  Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
  Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  digitalWrite(CBI_SWITCH_PIN, LOW);
  digitalWrite(DP_SWITCH_PIN, LOW);
  digitalWrite(VM_SWITCH_PIN, LOW);

  waitTime(600); // wait on servos

  // Detach from the servo to save power and reduce jitter
  Servos[TOP_UTIL_ARM].detach();
  Servos[BOTTOM_UTIL_ARM].detach();
  Servos[LEFT_DOOR].detach();
  Servos[RIGHT_DOOR].detach();
  Servos[CBI_DOOR].detach();
  Servos[DATA_DOOR].detach();

  doorsOpen = false;
  leftDoorOpen = false;
  rightDoorOpen = false;
  cbi_dataOpen = false;
  cbiDoorOpen = false;
  dataDoorOpen = false;
  utilityArmOpen = false;
  topUtilityArmOpen = false;
  bottomUtilityArmOpen = false;
}
//-----------------------------------------------------
// Open/Close Everything
//-----------------------------------------------------

void openEverything() {

  digitalWrite(STATUS_LED, HIGH);

  //If everything is open, close everything.
  if (doorsOpen && cbi_dataOpen && utilityArmOpen) {
    resetServos();

  } else { //Open everything
    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED2);

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED2);

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH);
    digitalWrite(DP_SWITCH_PIN, HIGH);
    digitalWrite(VM_SWITCH_PIN, HIGH);

    waitTime(1000); // wait on servos

    // Detach from the servo to save power and reduce jitter
    Servos[TOP_UTIL_ARM].detach();
    Servos[BOTTOM_UTIL_ARM].detach();
    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;
    utilityArmOpen = true;
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = true;
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close both Utility Arms
void UtilityArms() {

  digitalWrite(STATUS_LED, HIGH);

  //if both arms were opened individually, utilityArmOpen is true
  if (topUtilityArmOpen && bottomUtilityArmOpen) {
    utilityArmOpen = true;
  }

  // If the Arms are open then close them
  if (utilityArmOpen) {
    DEBUG_PRINT_LN(F("Close utility arms"));
    utilityArmOpen = false;
    topUtilityArmOpen = false;
    bottomUtilityArmOpen = false;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely
    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (topUtilityArmOpen) { //if top arm is open, open bottom
    DEBUG_PRINT_LN(F("Open bottom arm"));
    utilityArmOpen = true;
    bottomUtilityArmOpen = true;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (bottomUtilityArmOpen) { //if bottom arm is open, open top
    DEBUG_PRINT_LN(F("Open bottom arm"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach();

  } else { // Open both arms if closed
    DEBUG_PRINT_LN(F("Open utility arms"));
    utilityArmOpen = true;
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely
    Servos[BOTTOM_UTIL_ARM].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close Top Utility Arm
void TopUtilityArm() {

  digitalWrite(STATUS_LED, HIGH);

  if (utilityArmOpen) { // If both the Arms are open, close the bottom one
    DEBUG_PRINT_LN(F("Close bottom utility arm"));
    topUtilityArmOpen = true;
    bottomUtilityArmOpen = false;
    utilityArmOpen = false;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach();

  } else if (topUtilityArmOpen) { // If the top arm is open, close it
    DEBUG_PRINT_LN(F("Close top utility arm"));
    topUtilityArmOpen = false;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Close.
    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely

  } else { // Open the Top Arm Only
    DEBUG_PRINT_LN(F("Open top utility arm"));
    topUtilityArmOpen = true;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Open.
    Servos[TOP_UTIL_ARM].write(TOP_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach(); // detach so we can move the arms freely

  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//------------------------------------------------------------------
// Open/Close Bottom Utility Arm
void BottomUtilityArm() {

  digitalWrite(STATUS_LED, HIGH);

  if (utilityArmOpen) { // If both the Arms are open, close the top one
    DEBUG_PRINT_LN(F("Close top utility arm"));
    topUtilityArmOpen = false;
    bottomUtilityArmOpen = true;
    utilityArmOpen = false;

    Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // pull arms slightly beyond closed to make sure they're really closed. Will vary on your droid
    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);

    waitTime(1000);  // wait on arm to reach position

    Servos[TOP_UTIL_ARM].detach();

  } else if (bottomUtilityArmOpen) { // If the bottom arm is open, close it
    DEBUG_PRINT_LN(F("Close bottom utility arm"));
    bottomUtilityArmOpen = false;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Close.
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED); // close at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach(); // detach so we can move the arms freely

  } else { // Open the Bottom Arm Only
    DEBUG_PRINT_LN(F("Open bottom utility arm"));
    bottomUtilityArmOpen = true;

    Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);

    // Set Servo position to Open.
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_OPEN, UTILITYARMSSPEED); // open at moderate speed

    waitTime(1000);  // wait on arm to reach position

    Servos[BOTTOM_UTIL_ARM].detach(); // detach so we can move the arms freely

  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//---------------------------------------------
// S C R E A M
//---------------------------------------------
void Scream() {

  digitalWrite(STATUS_LED, HIGH);

  playScream();
  Servos[TOP_UTIL_ARM].attach(TOP_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[BOTTOM_UTIL_ARM].attach(BOTTOM_UTIL_ARM_SERVO_PIN, ARMMINPULSE, ARMMAXPULSE);
  Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
  Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
  Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
  Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

  for (int i = 0; i < 7; i++) {

    DEBUG_PRINT(F("Loop:"));
    DEBUG_PRINT_LN(i + 1);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, SCREAM_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, SCREAM_SPEED);
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights

    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, SCREAM_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, SCREAM_SPEED);
    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights

    Servos[TOP_UTIL_ARM].write(1250, 255);
    Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, 255);

    waitTime(150);

    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, SCREAM_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, SCREAM_SPEED);
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights

    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, SCREAM_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, SCREAM_SPEED);
    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights

    Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, 255); // open at moderate speed
    Servos[BOTTOM_UTIL_ARM].write(1250, 255); // 0=open all the way

    waitTime(150);

  }

  DEBUG_PRINT_LN(F("Close everything"));

  Servos[TOP_UTIL_ARM].write(TOP_ARM_CLOSE, UTILITYARMSSPEED);
  Servos[BOTTOM_UTIL_ARM].write(BOTTOM_ARM_CLOSE, UTILITYARMSSPEED);
  Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
  Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

  digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
  digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
  digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

  waitTime(1000); // wait on arm to reach position

  DEBUG_PRINT_LN(F("Detach"));

  Servos[LEFT_DOOR].detach();
  Servos[RIGHT_DOOR].detach();
  Servos[CBI_DOOR].detach();
  Servos[DATA_DOOR].detach();
  Servos[TOP_UTIL_ARM].detach();
  Servos[BOTTOM_UTIL_ARM].detach();

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

//-----------------------------------------------------
// D O O R S
//-----------------------------------------------------

void Doors() {

  digitalWrite(STATUS_LED, HIGH);

  //if doors were opened individually, doorsOpen is true
  if (leftDoorOpen && rightDoorOpen && cbiDoorOpen && dataDoorOpen) {
    doorsOpen = true;
  }

  // If the Doors are open then close them
  if (doorsOpen) {
    DEBUG_PRINT_LN(F("Close doors"));
    doorsOpen = false;
    leftDoorOpen = false;
    rightDoorOpen = false;
    cbiDoorOpen = false;
    dataDoorOpen = false;

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on arm to reach position

    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  } else {
    DEBUG_PRINT_LN(F("Open doors"));
    doorsOpen = true;
    leftDoorOpen = true;
    rightDoorOpen = true;
    cbi_dataOpen = true;
    cbiDoorOpen = true;
    dataDoorOpen = true;

    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);

    Servos[TOP_UTIL_ARM].detach();
    Servos[BOTTOM_UTIL_ARM].detach();
    Servos[LEFT_DOOR].detach();
    Servos[RIGHT_DOOR].detach();
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);


}

void openLeftDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (leftDoorOpen) {
    DEBUG_PRINT_LN(F("Close Left Door"));
    leftDoorOpen = false;
    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position
    Servos[LEFT_DOOR].detach();

  } else {
    leftDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Left Door"));
    Servos[LEFT_DOOR].attach(LEFT_DOOR_SERVO_PIN, LEFT_DOOR_MINPULSE, LEFT_DOOR_MAXPULSE);
    Servos[LEFT_DOOR].write(LEFT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
    Servos[LEFT_DOOR].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);

}

void openRightDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (rightDoorOpen) {
    DEBUG_PRINT_LN(F("Close Right Door"));
    rightDoorOpen = false;
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    waitTime(1000); // wait on door to reach position
    Servos[RIGHT_DOOR].detach();

  } else {
    rightDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Right Door"));
    Servos[RIGHT_DOOR].attach(RIGHT_DOOR_SERVO_PIN, RIGHT_DOOR_MINPULSE, RIGHT_DOOR_MAXPULSE);
    Servos[RIGHT_DOOR].write(RIGHT_DOOR_OPEN, DOOR_OPEN_SPEED);
    waitTime(1000);
    Servos[RIGHT_DOOR].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);

}

void openCBIDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (cbiDoorOpen) {
    DEBUG_PRINT_LN(F("Close Charge Bay Door"));
    cbiDoorOpen = false;
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights

    waitTime(1000); // wait on door to reach position
    Servos[CBI_DOOR].detach();

  } else {
    cbiDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay Door"));
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights

    waitTime(1000);
    Servos[CBI_DOOR].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);

}

void openDataDoor() {

  digitalWrite(STATUS_LED, HIGH);

  if (dataDoorOpen) {
    DEBUG_PRINT_LN(F("Close Data Port Door"));
    dataDoorOpen = false;
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on door to reach position
    Servos[DATA_DOOR].detach();

  } else {
    dataDoorOpen = true;
    DEBUG_PRINT_LN(F("Open Data Port Door"));
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);
    Servos[DATA_DOOR].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);

}

void openCBI_DataDoor() {

  digitalWrite(STATUS_LED, HIGH);

  //if doors were opened individually, cbi_dataOpen is true
  if (cbiDoorOpen && dataDoorOpen) {
    cbi_dataOpen = true;
  }

  if (cbi_dataOpen) {
    DEBUG_PRINT_LN(F("Close Charge Bay & Data Door"));
    cbi_dataOpen = false;
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[CBI_DOOR].write(CBI_DOOR_CLOSE, DOOR_CLOSE_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_CLOSE, DOOR_CLOSE_SPEED);

    digitalWrite(CBI_SWITCH_PIN, LOW); //Turns off CBI lights
    digitalWrite(DP_SWITCH_PIN, LOW); //Turns off Data Panel lights
    digitalWrite(VM_SWITCH_PIN, LOW);  //Turns off Voltmeter

    waitTime(1000); // wait on door to reach position
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();

  } else {
    cbi_dataOpen = true;
    DEBUG_PRINT_LN(F("Open Charge Bay & Data Door"));
    Servos[CBI_DOOR].attach(CBI_DOOR_SERVO_PIN, CBI_DOOR_MINPULSE, CBI_DOOR_MAXPULSE);
    Servos[DATA_DOOR].attach(DATA_DOOR_SERVO_PIN, DATA_DOOR_MINPULSE, DATA_DOOR_MAXPULSE);

    Servos[CBI_DOOR].write(CBI_DOOR_OPEN, DOOR_OPEN_SPEED);
    Servos[DATA_DOOR].write(DATA_DOOR_OPEN, DOOR_OPEN_SPEED);

    digitalWrite(CBI_SWITCH_PIN, HIGH); //Turns on CBI lights
    digitalWrite(DP_SWITCH_PIN, HIGH); //Turns on Data Panel lights
    digitalWrite(VM_SWITCH_PIN, HIGH);  //Turns on Voltmeter

    waitTime(1000);
    Servos[CBI_DOOR].detach();
    Servos[DATA_DOOR].detach();
  }

  I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);

}

//----------------------------------------------------------------------------
//  Delay function
//----------------------------------------------------------------------------
void waitTime(unsigned long waitTime)
{
  unsigned long endTime = millis() + waitTime;
  while (millis() < endTime)
  {}// do nothing
}

//----------------------------------------------------------------------------
//  I2C Command Functions
//----------------------------------------------------------------------------

//Send Event
//Requires 3 arguments: Address, Command, whether a checksum is needed
//The Checksum is required if you are sending commands to a Stealth Controller
//example:  sendI2C(22, "3T0\r", false)
//This would send the command "3T0" with a carriage return to address 22 without a checksum

void sendI2C(uint8_t addr, const char *I2Ccmd, bool CheckSum) {
  byte sum = 0; //stores Checksum for Stealth Receiver
  const char *i;
  i = I2Ccmd;
  Wire.beginTransmission(addr);
  while (*i) {
    Wire.write(*i);
    sum += byte(*i); //Checksum for Stealth Controller
    i++;
  }
  if (CheckSum) {
    Wire.write(sum); //Checksum for Stealth Controller
    DEBUG_PRINT(F("Checksum: "));
    DEBUG_PRINT_LN(sum);
    Wire.endTransmission();
  }
  else {
    Wire.endTransmission();
  }

  DEBUG_PRINT(F("Sent I2C Command: "));
  DEBUG_PRINT(I2Ccmd);
  DEBUG_PRINT(F(" to: "));
  DEBUG_PRINT_LN(addr);
}


//Receive Event (Simplified receive event for Integer commands only as come from the Stealth Controller)

void receiveEvent(int howMany) {
  I2CCommand = Wire.read();

  DEBUG_PRINT(F("Received I2C Command Code: "));
  DEBUG_PRINT_LN(I2CCommand);
}

// I2C Command Function

void doCommand() {
  delay(50);
  loopTime = millis(); 

  switch (I2CCommand) {
    case 1: // RESET
      DEBUG_PRINT_LN(F("Got reset message"));
      resetServos();
      resetVocalizer();
      digitalWrite(STATUS_LED, HIGH);
      I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
      break;

    case 2:
      Vader();
      break;

    case 3:
      Theme();
      break;

    case 4:
      Cantina();
      break;

    case 5:
      Scream();
      break;

    case 6:
      Leia();
      break;

    case 7:
      happyEmote();
      break;

    case 8:
      sadEmote();
      break;

    case 9:
      madEmote();
      break;

    case 10:
      scaredEmote();
      break;

    case 11:
      happyEmote(EMOTE_STRONG);
      break;
  
    case 12:
      sadEmote(EMOTE_STRONG);
      break;

    case 13:
      madEmote(EMOTE_STRONG);
      break;

    case 14:
      scaredEmote(EMOTE_STRONG);
      break;

    case 15:
      overload();
      break;

    case 16:
      toggleMute();
      break;

    case 21:
      UtilityArms();
      break;

    case 22:
      Doors();
      break;

    case 23:
      openLeftDoor();
      break;

    case 24:
      openRightDoor();
      break;

    case 25:
      openEverything();
      break;

    case 26:
      openCBIDoor();
      break;

    case 27:
      openDataDoor();
      break;

    case 28:
      openCBI_DataDoor();
      break;

    case 29:
      TopUtilityArm();
      break;

    case 30:
      BottomUtilityArm();
      break;

    default: // Catch All
    case 0:
      digitalWrite(STATUS_LED, LOW);
      I2CCommand = -1; // always reset I2CCommand to -1, so we don't repeatedly do the same command
      break;
  }
}

///////////////////////////////////////////////////
// Test LEDs, each Maxim driver row in turn
// Each LED blinks according to the col number
// Col 0 is just on
// Col 1 blinks twice
// col 2 blinks 3 times, etc...
//

#define TESTDELAY 30
void singleTest()
{
  for (int row = 0; row < 6; row++)
  {
    for (int col = 0; col < 7; col++)
    {
      waitTime(TESTDELAY);
      lc.setLed(DATAPORT, row, col, true);
      waitTime(TESTDELAY);
      for (int i = 0; i < col; i++)
      {
        lc.setLed(DATAPORT, row, col, false);
        waitTime(TESTDELAY);
        lc.setLed(DATAPORT, row, col, true);
        waitTime(TESTDELAY);
      }
    }
  }


  for (int row = 0; row < 4; row++)
  {
    for (int col = 0; col < 5; col++)
    {
      waitTime(TESTDELAY);
      lc.setLed(CBI, row, col, true);
      waitTime(TESTDELAY);
      for (int i = 0; i < col; i++)
      {
        lc.setLed(CBI, row, col, false);
        waitTime(TESTDELAY);
        lc.setLed(CBI, row, col, true);
        waitTime(TESTDELAY);
      }
    }
  }

  lc.setLed(CBI, 4, 5, true);
  waitTime(TESTDELAY);
  lc.setLed(CBI, 5, 5, true);
  waitTime(TESTDELAY);
  lc.setLed(CBI, 6, 5, true);
  waitTime(TESTDELAY);
}

///////////////////////////////////
// animates the two top left blocks
// (green and yellow blocks)
void updateTopBlocks()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed;
  elapsed = millis();
  if ((elapsed - timeLast) < TOPBLOCKSPEED) return;
  timeLast = elapsed;

  lc.setRow(DATAPORT, 4, randomRow(4)); // top yellow blocks
  lc.setRow(DATAPORT, 5, randomRow(4)); // top green blocks

}

///////////////////////////////////
// animates the CBI
//
void updateCBILEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed;
  elapsed = millis();
  if ((elapsed - timeLast) < CBISPEED) return;
  timeLast = elapsed;

#ifdef monitorVCC
  lc.setRow(CBI, random(4), randomRow(random(4)));
#else
  lc.setRow(CBI, random(7), randomRow(random(4)));
#endif
}

////////////////////////////////////
// Utility to generate random LED patterns
// Mode goes from 0 to 6. The lower the mode
// the less the LED density that's on.
// Modes 4 and 5 give the most organic feel
byte randomRow(byte randomMode)
{
  switch (randomMode)
  {
    case 0:  // stage -3
      return (random(256)&random(256)&random(256)&random(256));
      break;
    case 1:  // stage -2
      return (random(256)&random(256)&random(256));
      break;
    case 2:  // stage -1
      return (random(256)&random(256));
      break;
    case 3: // legacy "blocky" mode
      return random(256);
      break;
    case 4:  // stage 1
      return (random(256) | random(256));
      break;
    case 5:  // stage 2
      return (random(256) | random(256) | random(256));
      break;
    case 6:  // stage 3
      return (random(256) | random(256) | random(256) | random(256));
      break;
    default:
      return random(256);
      break;
  }
}

//////////////////////
// bargraph for the right column
// disp 0: Row 2 Col 5 to 0 (left bar) - 6 to 0 if including lower red LED,
// disp 1: Row 3 Col 5 to 0 (right bar)

#define MAXGRAPH 2

void bargraphDisplay(byte disp)
{
  static byte bargraphdata[MAXGRAPH]; // status of bars

  if (disp >= MAXGRAPH) return;

  // speed control
  static unsigned long previousDisplayUpdate[MAXGRAPH] = {0, 0};

  unsigned long currentMillis = millis();
  if (currentMillis - previousDisplayUpdate[disp] < BARGRAPHSPEED) return;
  previousDisplayUpdate[disp] = currentMillis;

  // adjust to max numbers of LED available per bargraph
  byte maxcol;
  if (disp == 0 || disp == 1) maxcol = 6;
  else maxcol = 3; // for smaller graph bars, not defined yet

  // use utility to update the value of the bargraph  from it's previous value
  byte value = updatebar(disp, &bargraphdata[disp], maxcol);
  byte data = 0;
  // transform value into byte representing of illuminated LEDs
  // start at 1 so it can go all the way to no illuminated LED
  for (int i = 1; i <= value; i++)
  {
    data |= 0x01 << i - 1;
  }
  // transfer the byte column wise to the video grid
  fillBar(disp, data, value, maxcol);
}

/////////////////////////////////
// helper for updating bargraph values, to imitate bargraph movement
byte updatebar(byte disp, byte* bargraphdata, byte maxcol)
{
  // bargraph values go up or down one pixel at a time
  int variation = random(0, 3);           // 0= move down, 1= stay, 2= move up
  int value = (int)(*bargraphdata);       // get the previous value
  //if (value==maxcol) value=maxcol-2; else      // special case, staying stuck at maximum does not look realistic, knock it down
  value += (variation - 1);               // grow or shring it by one step
#ifndef BLUELEDTRACKGRAPH
  if (value <= 0) value = 0;              // can't be lower than 0
#else
  if (value <= 1) value = 1;              // if blue LED tracks, OK to keep lower LED always on
#endif
  if (value > maxcol) value = maxcol;     // can't be higher than max
  (*bargraphdata) = (byte)value;          // store new value, use byte type to save RAM
  return (byte)value;                     // return new value
}

/////////////////////////////////////////
// helper for lighting up a bar of LEDs based on a value
void fillBar(byte disp, byte data, byte value, byte maxcol)
{
  byte row;

  // find the row of the bargraph
  switch (disp)
  {
    case 0:
      row = 2;
      break;
    case 1:
      row = 3;
      break;
    default:
      return;
      break;
  }

  for (byte col = 0; col < maxcol; col++)
  {
    // test state of LED
    byte LEDon = (data & 1 << col);
    if (LEDon)
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,true);  // set column bit
      lc.setLed(DATAPORT, 2, maxcol - col - 1, true); // set column bit
      lc.setLed(DATAPORT, 3, maxcol - col - 1, true); // set column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,true);      // set blue column bit
    }
    else
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,false); // reset column bit
      lc.setLed(DATAPORT, 2, maxcol - col - 1, false); // reset column bit
      lc.setLed(DATAPORT, 3, maxcol - col - 1, false); // reset column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,false);     // set blue column bit
    }
  }
#ifdef BLUELEDTRACKGRAPH
  // do blue tracking LED
  byte blueLEDrow = B00000010;
  blueLEDrow = blueLEDrow << value;
  lc.setRow(DATAPORT, 0, blueLEDrow);
#endif
}

/////////////////////////////////
// This animates the bottom white LEDs
void updatebottomLEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < BOTTOMLEDSPEED) return;
  timeLast = elapsed;

  // bottom LEDs are row 1,
  lc.setRow(DATAPORT, 1, randomRow(4));
}

////////////////////////////////
// This is for the two red LEDs
void updateRedLEDs()
{
  static unsigned long timeLast = 0;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < REDLEDSPEED) return;
  timeLast = elapsed;

  // red LEDs are row 2 and 3, col 6,
  lc.setLed(DATAPORT, 2, 6, random(0, 2));
  lc.setLed(DATAPORT, 3, 6, random(0, 2));
}

//////////////////////////////////
// This animates the blue LEDs
// Uses a random delay, which never exceeds BLUELEDSPEED
void updateBlueLEDs()
{
  static unsigned long timeLast = 0;
  static unsigned long variabledelay = BLUELEDSPEED;
  unsigned long elapsed = millis();
  if ((elapsed - timeLast) < variabledelay) return;
  timeLast = elapsed;
  variabledelay = random(10, BLUELEDSPEED);

  /*********experimental, moving dots animation
    static byte stage=0;
    stage++;
    if (stage>7) stage=0;
    byte LEDstate=B00000011;
    // blue LEDs are row 0 col 0-5
    lc.setRow(DATAPORT,0,LEDstate<<stage);
  *********************/

  // random
  lc.setRow(DATAPORT, 0, randomRow(4));
}


void getVCC()
{
  value = analogRead(analoginput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your arduino
  vout = (value * 5.0) / 1024.0; //voltage coming out of the voltage divider
  vin = vout / (R2 / (R1 + R2)); //voltage to display
  if (vin < 0.09) {
    vin = 0.0; //avoids undesirable reading
  }

  lc.setLed(CBI, 6, 5, (vin >= greenVCC));
  lc.setLed(CBI, 5, 5, (vin >= yellowVCC));
  lc.setLed(CBI, 4, 5, (vin >= redVCC));
#ifdef DEBUG_VM
  DEBUG_PRINT(F("Volt Out = "));
  DEBUG_PRINT_DEC(vout, 1);   //Print float "vout" with 1 decimal place
  DEBUG_PRINT(F("\tVolts Calc = "));
  DEBUG_PRINT_LN_DEC(vin, 1);   //Print float "vin" with 1 decimal place
#endif
}
