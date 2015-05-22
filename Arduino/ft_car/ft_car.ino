// ft car 
// Inspired by
// CxemCAR 1.0 (06.01.2013)
// Project Page: http://english.cxem.net/mcu/mcu3.php
// Adapted for Funduino motor shield and cmdMessenger

#include <EEPROM.h>
#include <AFMotor.h>
#include <CmdMessenger.h>  

const int light01 =   14;  // additional channel 1
unsigned long previousCommand = 0;   // Last time a command was received

// Attach a new CmdMessenger object to the default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial);

// List of recognized commands
enum
{
  kLeftMotor,
  kRightMotor,
  kLight,
  kSettings,
  kStatus              , // Command to report status
};

AF_DCMotor leftMotor(1, MOTOR12_64KHZ);
AF_DCMotor rightMotor(2, MOTOR12_64KHZ);


//------------------ Main ---------------

void setup() {
  Serial.begin(9600);       // initialization UART

  // Adds newline to every command
  cmdMessenger.printLfCr();   

  // Attach my application's user-defined callback methods
  attachCommandCallbacks();

  // Send the status to the PC that says the Arduino has booted
  cmdMessenger.sendCmd(kStatus, "Arduino has started!");

  previousCommand = millis();
  pinMode(light01, OUTPUT);    // additional channel

}

void loop() {
  if (hasExpired(previousCommand, 20000))
  {
    // No new commands received; stop the car.
    leftMotor.run(RELEASE);
    rightMotor.run(RELEASE);
  }
  
  // Process incoming serial data, and perform callbacks
  cmdMessenger.feedinSerialData();
  
}

//------------ Callbacks -----------------------

// Callbacks define on which received commands we take action
void attachCommandCallbacks()
{
  // Attach callback methods
  cmdMessenger.attach(onUnknownCommand);
  cmdMessenger.attach(kLight, onSetLed);
  cmdMessenger.attach(kLeftMotor, onLeftMotor);
  cmdMessenger.attach(kRightMotor, onRightMotor);
}

// Called when a received command has no attached function
void onUnknownCommand()
{
  cmdMessenger.sendCmd(kStatus,"Command without attached callback");
}

// Callback function that sets led on or off
void onSetLed()
{
  // Read led state argument, interpret string as boolean
  boolean ledState = cmdMessenger.readBoolArg();

  // Set led
  digitalWrite(light01, ledState?HIGH:LOW);

  // Send back status that describes the led state
  cmdMessenger.sendCmd(kLight,(int)ledState);
}

void onLeftMotor()
{
  runMotor(leftMotor);
}

void onRightMotor()
{
  runMotor(rightMotor);
}

void runMotor(AF_DCMotor motor)
{
  int16_t speedMotor =  cmdMessenger.readInt16Arg();
  motor.setSpeed(abs(speedMotor));
  
  // if the speed is negative, the motor must run backward
  motor.run(speedMotor > 0?FORWARD:BACKWARD);

  // Record the time the last command is receivedL
  previousCommand = millis();

}

//------------- Utility functions ---------------

// Returns if it has been more than interval (in ms) ago. Used for periodic actions
bool hasExpired(unsigned long &prevTime, unsigned long interval) {
  if (  millis() - prevTime > interval ) {
    prevTime = millis();
    return true;
  } else     
    return false;
}


