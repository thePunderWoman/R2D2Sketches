

//-----------------------------------------------------------------------------------------------
// Servo Expander "Dome Stealth Servo Expander" 
// 
// Jessica Janiuk (thePunderWoman) 1-2023
// Iterated from John Thompson (FrettedLogic)
// "Shamelessly paraphrased from Chris James' foundation"
//--------------------------------------------------------------

#include <Wire.h>
#include <VarSpeedServo.h>

// outgoing i2c Commands
String Packet;
int count = 0;
byte sum;

// incoming i2c command
int i2cCommand = 0;
#define MYI2ADDR 12      //I2C Address for DOME SERVO EXPANDER = 10

// I2C address constants
#define STEALTH 0
#define BODY 9
#define FRONT_PSI 22
#define REAR_PSI 23
#define ASTROPIXELS 10
#define HPS 25

unsigned long loopTime; // Time variable

#define STATUS_LED 13

// Dome panel names for our Arduino pins, USE THESE PHYSICAL PINS FOR SPECIFIC DOME SERVO
// Board Pins #2-#13. Panels marked based on Club Spec Panel numbering. PP=Pie Panels, P=Lower Panels, DT=Dome Topper
#define PP1_SERVO_PIN 2   //  PP1   Pie Panel         
#define PP2_SERVO_PIN 3   //  PP2   Pie Panel         
#define PP5_SERVO_PIN 4   //  PP5   Pie Panel         
#define PP6_SERVO_PIN 5   //  PP6   Pie Panel         
// #define DT_SERVO_PIN 6    //  DT    Dome Topper Panel  
#define P1_SERVO_PIN 7    //  P1    Low Panel        
#define P2_SERVO_PIN 8    //  P2    Low Panel        
#define P3_SERVO_PIN 9    //  P3    Low Panel        
#define P4_SERVO_PIN 10   //  P4    Low Panel        
#define P7_SERVO_PIN 11   //  P7    Low Panel         
#define P10_SERVO_PIN 12  //  P10   Low Panel        
#define P11_SERVO_PIN 6   //  P11   Low Panel
#define P13_SERVO_PIN 13  //  P13  Low Panel 

// Create an array of VarSpeedServo type, containing 5 elements/entries. 
// Note: Arrays are zero indexed. See http://arduino.cc/en/Reference/array

#define NBR_SERVOS 12  // Number of Servos (12)
VarSpeedServo Servos[NBR_SERVOS];

// Note: These reference internal numbering not physical pins for array use

#define PP1 0    //  PP1   Pie Panel         
#define PP2 1    //  PP2   Pie Panel         
#define PP5 2    //  PP5   Pie Panel        
#define PP6 3    //  PP6   Pie Panel         
// #define DT 4    // DT    Dome Topper Panel
#define P1 5    // P1    Low Panel        
#define P2 6    // P2    Low Panel        
#define P3 7    // P3    Low Panel        
#define P4 8    // P4    Low Panel        
#define P7 9    // P7    Low Panel        
#define P10 10  // P10   Low Panel        
#define P11 4   // P11   Low Panel
#define P13 11  // P13   Low Panel  
// #define P11_P13 11  // P11 & P13  Low Panels  

  //  Servos in open position = 1000ms ( 0 degrees )  
//  Servos in closed position = 2000ms ( 180 degrees ) 

#define FIRST_SERVO_PIN 2 //First Arduino Pin for Servos

// Dome Panel Open/Close values, can use servo value typically 1000-2000ms or position 0-180 degress. 
// #define PANEL_OPEN 1100     //  (40)  0=open position, 1000 us                          
// #define PANEL_CLOSE 2000   //   (170) 180=close position, 2000 us                        
// #define PANEL_HALFWAY 1400  // 90=midway point 1500ms, use for animations etc. 
// #define PANEL_PARTOPEN 1650  //Partially Open, approximately 1/4    
#define PANEL_OPEN 2000     //  (40)  0=open position, 1000 us                          
#define PANEL_CLOSE 1100   //   (170) 180=close position, 2000 us                        
#define PANEL_HALFWAY 1800  // 90=midway point 1500ms, use for animations etc. 
#define PANEL_PARTOPEN 1450  //Partially Open, approximately 1/4    

#define PANEL_MIN 1100
#define PANEL_MAX 2000
           
// Panel Speed Variables 0-255, higher number equals faster movement
#define SPEED 200            
#define FASTSPEED 255       
#define OPENSPEED 230   
#define CLOSESPEED 230

// Variables so program knows where things are
boolean PiesOpen=false;
boolean AllOpen=false;
boolean LowOpen=false;
boolean OpenClose=false;


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
    Serial.println("Checksum: ");
    Serial.println(sum);
    Wire.endTransmission();
  }
  else {
    Wire.endTransmission();
  }

  Serial.println("Sent I2C Command: ");
  Serial.println(I2Ccmd);
  Serial.println(" to: ");
  Serial.println(addr);
}

void sendToBody(uint8_t I2Ccmd) {
  Wire.beginTransmission(BODY);
  Wire.write(I2Ccmd);
  Wire.endTransmission();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(57600); // DEBUG
  
  Serial.println("Stealth Dome Server Expander - 2017");

  Serial.println("My i2c address: ");
  Serial.println(MYI2ADDR);
  Wire.begin(MYI2ADDR);                // Start I2C Bus as a Slave (Device Number 10)
  Wire.onReceive(receiveEvent); // register i2c receive event
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  Serial.println("Waiting for i2c command");
}

//------------------------------------------------------
// receive Event

void receiveEvent(int howMany) {
  i2cCommand = Wire.read();    // receive i2c command
  Serial.print("Command Code:");
  Serial.println(i2cCommand);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                                //      
//                                                                         Command Sequences                                                                      //
//                                                                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Open/Close Pie Panels////////////////////////////////////////////////////////////////////////////////////////

void OpenClosePies() {  // Note: This may seem backwards but the Close command ("if") is first and then the Open ("else")second, see Arduino reference guide

    digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
    //Open or close  All Pie Panels, each command will trigger an open or close command
    
    Serial.print("Pie Panels:");
    
    if (PiesOpen) { // Close the Pie Panels if PiesOpen is true
      Serial.println("Closing");
      PiesOpen=false;

      resetHolos();

      //.attach allows servos to operate
      Servos[PP1].attach(PP1_SERVO_PIN,PANEL_MIN,PANEL_MAX); 
      Servos[PP2].attach(PP2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP5].attach(PP5_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP6].attach(PP6_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      // Servos[DT].attach(DT_SERVO_PIN,PANEL_MIN,PANEL_MAX);

      sendToBody(7); // Play happy vocalization
      sendI2C(STEALTH, "$02", true); // sends an I2C command to Stealth folder two (chat sounds)

      //closes panels, "true" statement allows servo to reach position before executing next .write
      Servos[PP1].write(PANEL_CLOSE,SPEED,true); // the "true" uses servo postion to ensure full position prior to next .write line. Good for smooth sequence verses delays
      Servos[PP2].write(PANEL_CLOSE,SPEED,true);
      Servos[PP5].write(PANEL_CLOSE,SPEED,true);
      Servos[PP6].write(PANEL_CLOSE,SPEED,true);
      // Servos[DT].write(PANEL_CLOSE,SPEED,true);            
      
      delay(800);
      // .detach disables servos      
      Servos[PP1].detach();
      Servos[PP2].detach();
      Servos[PP5].detach();
      Servos[PP6].detach();
      // Servos[DT].detach();
      
      Serial.println("Closed");
       
    } else { // Open Pie Panels
      Serial.println("Opening Pie Panels");
      PiesOpen=true;

      delay(100);

       //.attach allows servos to operate
      Servos[PP1].attach(PP1_SERVO_PIN,PANEL_MIN,PANEL_MAX); 
      Servos[PP2].attach(PP2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP5].attach(PP5_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP6].attach(PP6_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      // Servos[DT].attach(DT_SERVO_PIN,PANEL_MIN,PANEL_MAX);

      sendToBody(7); // Play happy vocalization
      sendI2C(STEALTH, "$01", true);  // Stealth sound folder one (general sounds)

      for (int i=0; i<2; i++) { 
        // open pies in wave
       
        Servos[PP2].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP1].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP6].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP5].write(PANEL_OPEN,FASTSPEED,true);
  
        //  Servos[DT].write(PANEL_OPEN,FASTSPEED,true);
       
        // close pies in opposite wave
       
        Servos[PP5].write(PANEL_CLOSE,FASTSPEED,true);
        Servos[PP6].write(PANEL_CLOSE,FASTSPEED,true);
        Servos[PP1].write(PANEL_CLOSE,FASTSPEED,true);
        Servos[PP2].write(PANEL_CLOSE,FASTSPEED,true);
        //  Servos[DT].write(PANEL_CLOSE,FASTSPEED,true);

        // reopen pies
        Servos[PP2].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP1].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP6].write(PANEL_OPEN,FASTSPEED,true);
        Servos[PP5].write(PANEL_OPEN,FASTSPEED,true);
        //  Servos[DT].write(PANEL_OPEN,FASTSPEED,true);
      }

      delay(1000);
      // .detach disables servos      
      Servos[PP1].detach();
      Servos[PP2].detach();
      Servos[PP5].detach();
      Servos[PP6].detach();
      // Servos[DT].detach();

      Serial.println("Opened Pies");
    }
    // end "for" loop
    
    i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
    digitalWrite(STATUS_LED, LOW);
}


// Open/Close Low panels///////////////////////////////////////////////////////////////////////////////////

void OpenCloseLow() { 

    digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
   
    //Open or close  Low Panels
    
    Serial.print("Low Panels: ");
    if (LowOpen) { // Close the Low Panels
      Serial.println("Closing");   
      LowOpen=false;

      resetHolos();

      // attach servos 
      Servos[P1].attach(P1_SERVO_PIN,PANEL_MIN,PANEL_MAX); // add servo min and max to limit travel if needed or expand range, normally 1000-2000
      Servos[P2].attach(P2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P3].attach(P3_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P4].attach(P4_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P7].attach(P7_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P10].attach(P10_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P11].attach(P11_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P13].attach(P13_SERVO_PIN,PANEL_MIN,PANEL_MAX);

      sendToBody(7); // Play happy vocalization
      sendI2C(STEALTH, "$02", true); //plays random sound in specified soundbank folder 

      Servos[P4].write(PANEL_CLOSE,SPEED,true);
      Servos[P2].write(PANEL_CLOSE,SPEED,true);
      Servos[P1].write(PANEL_CLOSE,SPEED,true);
      Servos[P3].write(PANEL_CLOSE,SPEED,true);
      Servos[P11].write(PANEL_CLOSE,SPEED,true);
      Servos[P13].write(PANEL_CLOSE,SPEED,true);
      Servos[P7].write(PANEL_CLOSE,SPEED,true);
      Servos[P10].write(PANEL_CLOSE,SPEED,true);
     
      
      delay(1000);
      // Detach from the Low panels      

      Servos[P1].detach();
      Servos[P2].detach();
      Servos[P3].detach();
      Servos[P4].detach();
      Servos[P7].detach();
      Servos[P10].detach();
      Servos[P11].detach();
      Servos[P13].detach();
      
      Serial.println("Closed");
       
    } else { // Open Low Panels
      Serial.println("Lows Opening");
      LowOpen=true;

      sendI2C(HPS, "F102\r", false); // transmit to device #25 HP Board

      // attach servos
      Servos[P1].attach(P1_SERVO_PIN);
      Servos[P2].attach(P2_SERVO_PIN);
      Servos[P3].attach(P3_SERVO_PIN);
      Servos[P4].attach(P4_SERVO_PIN);
      Servos[P7].attach(P7_SERVO_PIN);
      Servos[P10].attach(P10_SERVO_PIN);
      Servos[P11].attach(P11_SERVO_PIN);
      Servos[P13].attach(P13_SERVO_PIN);
      
      sendToBody(7); // Play happy vocalization
      sendI2C(STEALTH, "$02", true); //plays random sound in specified soundbank folder 

       // Wave these panels in order specified below using "for" loop to repeat
      for (int i=0; i<2; i++) { 

        // P1,(P11,P13,P10) P2,P3,P4,P7
        // open
        Servos[P1].write(PANEL_OPEN,SPEED,true);
        Servos[P11].write(PANEL_OPEN,SPEED); // don't wait for this panel to open fully prior to next movement
        Servos[P13].write(PANEL_OPEN,SPEED); // don't wait for this panel to open fully prior to next movement
        Servos[P10].write(PANEL_OPEN,SPEED);     // don't wait for this panel to open fully prior to next movement
        Servos[P2].write(PANEL_OPEN,SPEED,true);
        Servos[P3].write(PANEL_OPEN,SPEED,true); 
        Servos[P4].write(PANEL_OPEN,SPEED,true); 
        Servos[P7].write(PANEL_OPEN,SPEED,true); 
      
        // P7,P4,P3,P2,P1,P11,P13,P10
        //close
        Servos[P7].write(PANEL_CLOSE,SPEED,true);
        Servos[P4].write(PANEL_CLOSE,SPEED,true);
        Servos[P3].write(PANEL_CLOSE,SPEED,true);
        Servos[P2].write(PANEL_CLOSE,SPEED,true);
        Servos[P1].write(PANEL_CLOSE,SPEED,true);
        delay(50); // used delay to time opposite dome side panel for better flow
        Servos[P11].write(PANEL_CLOSE,SPEED,true);
        Servos[P13].write(PANEL_CLOSE,SPEED,true);
        delay(50); // used delay to panel for better flow
        Servos[P10].write(PANEL_CLOSE,SPEED,true);
      }

      // write open
      Servos[P10].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P11].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P13].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P1].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P2].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P3].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P4].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P7].write(PANEL_OPEN,FASTSPEED,true);
     

      delay(1000);
      // Detach from the Low panels      

      Servos[P1].detach();
      Servos[P2].detach();
      Servos[P3].detach();
      Servos[P4].detach();
      Servos[P7].detach();
      Servos[P10].detach();
      Servos[P11].detach();
      Servos[P13].detach();
      
      Serial.println("Lows Opened");
    }
    i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
    digitalWrite(STATUS_LED, LOW);
}

//Open/Close All Panels////////////////////////////////////////////////////////////////////////////////////////////

void OpenCloseAll () {

    digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
    //Open or close  All Panels

    Serial.print("All Panels: ");
    
    if (AllOpen) { // Close all panels
      Serial.println("Closing");
      AllOpen=false;     

      sendToBody(7); // Play happy vocalization
      sendI2C(STEALTH, "$02", true); // I2C to Stealth for soundbank 2, chat

      //  Attach, write servo (min, max) range to ensure each panel opens and closes properly 

      Servos[PP1].attach(PP1_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP2].attach(PP2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP5].attach(PP5_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP6].attach(PP6_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      // Servos[DT].attach(DT_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P1].attach(P1_SERVO_PIN);
      Servos[P2].attach(P2_SERVO_PIN);
      Servos[P3].attach(P3_SERVO_PIN);
      Servos[P4].attach(P4_SERVO_PIN);
      Servos[P7].attach(P7_SERVO_PIN);
      Servos[P10].attach(P10_SERVO_PIN);
      Servos[P11].attach(P11_SERVO_PIN);
      Servos[P13].attach(P13_SERVO_PIN);

      // Write pies to close

      // Servos[DT].write(PANEL_CLOSE,SPEED,true);
      Servos[PP2].write(PANEL_CLOSE,SPEED,true);
      Servos[PP6].write(PANEL_CLOSE,SPEED,true);
      Servos[PP5].write(PANEL_CLOSE,SPEED,true);
      Servos[PP1].write(PANEL_CLOSE,SPEED,true);
     
      //Write low panels to close
      Servos[P10].write(PANEL_CLOSE,SPEED,true);
      Servos[P7].write(PANEL_CLOSE,SPEED,true);
      Servos[P4].write(PANEL_CLOSE,SPEED,true);
      Servos[P3].write(PANEL_CLOSE,SPEED,true);
      Servos[P2].write(PANEL_CLOSE,SPEED,true);
      Servos[P1].write(PANEL_CLOSE,SPEED,true);
      Servos[P11].write(PANEL_CLOSE,SPEED,true);
      Servos[P13].write(PANEL_CLOSE,SPEED,true);
     

      delay(500);

      // Detach ALL after close     

      Servos[PP1].detach();
      Servos[PP2].detach();
      Servos[PP5].detach();
      Servos[PP6].detach();
      // Servos[DT].detach();
      Servos[P1].detach();
      Servos[P2].detach();
      Servos[P3].detach();
      Servos[P4].detach();
      Servos[P7].detach();
      Servos[P10].detach();
      Servos[P11].detach();
      Servos[P13].detach();

      Serial.println("Closed All Dome");
  
    } else { // Open all Panels
      Serial.println("Opening");
      AllOpen=true;

      //  Attach, write servo (min, max) range to ensure each panel opens and closes properly, adjust for your servos
      Servos[PP1].attach(PP1_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP2].attach(PP2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP5].attach(PP5_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[PP6].attach(PP6_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      // Servos[DT].attach(DT_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P1].attach(P1_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P2].attach(P2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P3].attach(P3_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P4].attach(P4_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P7].attach(P7_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P10].attach(P10_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P11].attach(P11_SERVO_PIN,PANEL_MIN,PANEL_MAX);
      Servos[P13].attach(P13_SERVO_PIN,PANEL_MIN,PANEL_MAX);

      sendToBody(7);
      sendI2C(STEALTH, "$02", true); // I2C to Stealth for soundbank 2, chat

      //Write Pies open
      Servos[PP2].write(PANEL_OPEN,SPEED,true);
      Servos[PP5].write(PANEL_OPEN,SPEED,true);
      // Servos[DT].write(PANEL_OPEN,SPEED,true);
      Servos[PP1].write(PANEL_OPEN,SPEED,true);
      Servos[PP6].write(PANEL_OPEN,SPEED,true);
      
      //Write lows
      Servos[P10].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P11].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P13].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P1].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P2].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P3].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P4].write(PANEL_OPEN,FASTSPEED,true);
      Servos[P7].write(PANEL_OPEN,FASTSPEED,true);
     

      // Wave these panels back and forth like waving hello
      for (int i=0; i<2; i++) {   //Loop x number of times, 2 in this case

        // P1 & P2 twinkle up and down
        Servos[P1].write(PANEL_HALFWAY,FASTSPEED,true);
        Servos[P1].write(PANEL_OPEN,FASTSPEED);
        delay(80);
        Servos[P2].write(PANEL_OPEN,FASTSPEED,true);
        Servos[P2].write(PANEL_HALFWAY,FASTSPEED);
        delay(80);
        Servos[P2].write(PANEL_OPEN,FASTSPEED);
        
        
        delay(100);

        //PP1 & PP6 twinkle up and down
        Servos[PP2].write(PANEL_HALFWAY,FASTSPEED,true);
        Servos[PP2].write(PANEL_OPEN,FASTSPEED,true);
        delay(80);
        Servos[PP5].write(PANEL_HALFWAY,FASTSPEED,true);
        Servos[PP5].write(PANEL_OPEN,FASTSPEED,true);
      }   

      delay(800);

      // Detach when open     
      Servos[PP1].detach();
      Servos[PP2].detach();
      Servos[PP5].detach();
      Servos[PP6].detach();
      // Servos[DT].detach();
      Servos[P1].detach();
      Servos[P2].detach();
      Servos[P3].detach();
      Servos[P4].detach();
      Servos[P7].detach();
      Servos[P10].detach();
      Servos[P11].detach();
      Servos[P13].detach();

      
      Serial.println("Opened All Dome");
    }
    i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
    digitalWrite(STATUS_LED, LOW);
}

void scream() {
  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  Serial.print("Scream Sequence: Start");

  sendI2C(HPS, "A007C\r", false);
  sendI2C(HPS, "X105\r", false);
  sendI2C(HPS, "T106\r", false);
  sendI2C(FRONT_PSI, "4T5\r", false);
  sendI2C(REAR_PSI, "5T5\r", false);
  sendI2C(ASTROPIXELS, "@0T4\r", false);
  sendToBody(5);
  sendI2C(STEALTH, "$06", true); // I2C to Stealth for soundbank 6, scream

  // TODO: Add dome servo animations
  
  waitTime(8000); // wait 8 seconds before resetting
  
  resetHolos();
  resetLogics();
  resetPSIs();

  Serial.println("Scream Sequence: Complete");
  digitalWrite(STATUS_LED, LOW); 
}

void overload() {
  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
  Serial.print("Overload Sequence: Start");

  sendI2C(HPS, "A007C\r", false);
  sendI2C(FRONT_PSI, "4T4\r", false);
  sendI2C(REAR_PSI, "5T4\r", false);
  sendI2C(ASTROPIXELS, "@0T4\r", false);
  sendToBody(15);

  Serial.println("Overload Sequence: Complete");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW); 
}

void heart() {
  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
  Serial.print("Heart: Start");

  sendI2C(HPS, "A006|10\r", false);
  sendI2C(FRONT_PSI, "4T7\r", false);
  sendI2C(ASTROPIXELS, "@1MYou're\rWonderful", false);

  Serial.println("Heart: Complete");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void helloThere() {
  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
  Serial.print("Hello There: Start");

  sendI2C(ASTROPIXELS, "@1MHello\rThere", false);

  Serial.println("Hello There: Complete");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW); 
}

// Trigger the Leia sequence
void leiaMode() {
  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board     
  Serial.print("Leia Sequence: Start");

  sendI2C(HPS, "S1|36\r", false);
  sendI2C(FRONT_PSI, "4T6|36\r", false);
  sendI2C(REAR_PSI, "5T6|36\r", false);
  sendI2C(ASTROPIXELS, "@0T6\r", false);
  sendToBody(6);
  sendI2C(STEALTH, "$14", true); // play Leia Long sequence
  sendI2C(STEALTH, "tmprnd=40", true); // temporarily disable random sounds
  delay(500);

  Serial.println("Leia Sequence: Complete");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
  digitalWrite(STATUS_LED, LOW);
}

void resetHolos() {
  sendI2C(HPS, "S9\r", false);
  Serial.print("Holo Projectors reset");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
}

void resetLogics() {
  sendI2C(ASTROPIXELS, "@0T1\r", false);
  Serial.print("Astropixels reset");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
}

void resetPSIs() {
  // Send I2C command to front PSI
  sendI2C(FRONT_PSI, "4T1\r", false);
  sendI2C(REAR_PSI, "5T1\r", false);
  Serial.print("PSI Pros reset");
  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
}

void resetBody() {
  sendToBody(1);
  Serial.print("Body controller reset");
}

//RESET SERVOS, HOLOS, MAGIC PANEL  ================================================================================================

void resetAll() {

  digitalWrite(STATUS_LED, HIGH); // turn on STATUS LED so we can visually see we got the command on the board 

  Serial.println("Reset Dome Panels/Holos");

  // Attach
  Servos[PP1].attach(PP1_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[PP2].attach(PP2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[PP5].attach(PP5_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[PP6].attach(PP6_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  // Servos[DT].attach(DT_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P1].attach(P1_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P2].attach(P2_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P3].attach(P3_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P4].attach(P4_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P7].attach(P7_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P10].attach(P10_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P11].attach(P11_SERVO_PIN,PANEL_MIN,PANEL_MAX);
  Servos[P13].attach(P13_SERVO_PIN,PANEL_MIN,PANEL_MAX);

  // Close
  Servos[PP1].write(PANEL_CLOSE,SPEED);
  Servos[PP2].write(PANEL_CLOSE,SPEED);
  Servos[PP5].write(PANEL_CLOSE,SPEED);
  Servos[PP6].write(PANEL_CLOSE,SPEED);
  // Servos[DT].write(PANEL_CLOSE,SPEED);
  
  
  // Write low panels 
  Servos[P1].write(PANEL_CLOSE,SPEED);
  Servos[P2].write(PANEL_CLOSE,SPEED);
  Servos[P3].write(PANEL_CLOSE,SPEED);
  Servos[P4].write(PANEL_CLOSE,SPEED);
  Servos[P7].write(PANEL_CLOSE,SPEED);
  Servos[P10].write(PANEL_CLOSE,SPEED);
  Servos[P11].write(PANEL_CLOSE,SPEED);
  Servos[P13].write(PANEL_CLOSE,SPEED);


  delay(1000); // wait for servos to close
  // Detach servos

  Servos[PP1].detach();
  Servos[PP2].detach();
  Servos[PP5].detach();
  Servos[PP6].detach();
  // Servos[DT].detach();
  Servos[P1].detach();
  Servos[P2].detach();
  Servos[P3].detach();
  Servos[P4].detach();
  Servos[P7].detach();
  Servos[P10].detach();
  Servos[P11].detach();
  Servos[P13].detach();

  Serial.print("Dome Panels Closed,Reset");

  resetHolos();
  resetLogics();
  resetPSIs();
  resetBody();

  i2cCommand=-1; // always reset i2cCommand to -1, so we don't repeatedly do the same command
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
    
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////Main Loop////////////

//  Use Stealth Config.txt file on USB to trigger I2C and specific "Switch/Case" below for routine. See Stealth Manual for config info
//  Example for Stealth: Button: b=1,4,10,2 (Button One, I2C, Expander Address 10, Case 2 (OpenClose Pies))
//  Example for Stealth: Gesture: g=2,4,10,4 (Gesture Up once, I2C, Expander Address 10, Case 4 (OpenCloseAll))

void loop() {

  delay(50);
  loopTime = millis(); 
  

 // Check for new i2c command
  switch(i2cCommand) {
    
   case 1: // RESET can be triggered also using this case number, will reset and close see above 
          resetAll();
          break; // break used to end each case

   case 2:  
          OpenClosePies();  
          break;

   case 3: 
          OpenCloseLow();  
          break;

   case 4: 
          OpenCloseAll();
          break;
    
   case 5:
          leiaMode();  // can be used to test other device code commands such as holos, magic panel etc.
          break;

   case 6:
          heart();  // can be used to test other device code commands such as holos, magic panel etc.
          break;

   case 7:
          helloThere();  // can be used to test other device code commands such as holos, magic panel etc.
          break;

   case 10:
          scream();
          break;

  default: // Catch All
    case 0: 
          digitalWrite(STATUS_LED, LOW);
          i2cCommand=-1;    
          break;
  }
 
}
