// ft car
// Inspired by
// CxemCAR 1.0 (06.01.2013)
// Project Page: http://english.cxem.net/mcu/mcu3.php
// Adapted for Funduino motor shield and cmdMessenger

#include <EEPROMex.h>
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
  kStatus,               // Command to report status
  kIdentify,             // Command to identify device
};

AF_DCMotor leftMotor(1, MOTOR12_64KHZ);
AF_DCMotor rightMotor(2, MOTOR12_64KHZ);


class Settings
{
    int16_t numSeconds = 20;

  public:
    Settings()
    {
      initSettings();
    }

    void setNumSeconds(int16_t numberOfSeconds)
    {
      numSeconds = numberOfSeconds;
    }

    int getNumSeconds()
    {
      return numSeconds;
    }

    // Update the EEPROM
    void update()
    {
      EEPROM.writeInt(1, numSeconds);
    }

    // Read the settings from the EEPROM
    void initSettings()
    {
      numSeconds = EEPROM.readInt(1);
    }
};

Settings ftSettings = Settings();

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
  if (hasExpired(previousCommand, ftSettings.getNumSeconds()))
  {
    // No new commands received; stop the car.
    // Watchdog checks every 3 seconds; NumSeconds should normally be > 3.
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
  cmdMessenger.attach(kSettings, onSettings);
  cmdMessenger.attach(kIdentify, onIdentifyRequest);
}

// Called when a received command has no attached function
void onUnknownCommand()
{
  cmdMessenger.sendCmd(kStatus, "Command without attached callback");
}

// Callback function to respond to indentify request. This is part of the 
// Auto connection handshake. 
void onIdentifyRequest()
{
  // Here we will send back our communication identifier. Make sure it 
  // corresponds the Id in the C# code. Use F() macro to store ID in PROGMEM
    
  // You can make a unique identifier per device
  cmdMessenger.sendCmd(kIdentify, F("BFAF4176-766E-436A-ADF2-96133C02B03C"));
    
  // Record the time the last command is received
  previousCommand = millis();
  
}

// Callback function that sets led on or off
void onSetLed()
{
  // Read led state argument, interpret string as boolean
  boolean ledState = cmdMessenger.readBoolArg();

  // Set led
  digitalWrite(light01, ledState ? HIGH : LOW);

  // Send back status that describes the led state
  cmdMessenger.sendCmd(kLight, (int)ledState);

  // Record the time the last command is received
  previousCommand = millis();
  
}

// Callback function: change speed and direction of the left motor.
void onLeftMotor()
{
  runMotor(leftMotor);
}

// Callback function: change speed and direction of the right motor.
void onRightMotor()
{
  runMotor(rightMotor);
}

// Change speed and direction of one of the motors.
void runMotor(AF_DCMotor motor)
{
  // The argument defines the speed and direction of the motor:
  // a negative value indicates a backward direction.
  int16_t speedMotor =  cmdMessenger.readInt16Arg();
  motor.setSpeed(abs(speedMotor));

  // if the speed is negative, the motor must run backward
  motor.run(speedMotor > 0 ? FORWARD : BACKWARD);

  // Record the time the last command is received
  previousCommand = millis();
  
}

// Callback function: change settings
void onSettings()
{
  // Each setting is an argument
  // If no arguments are supplied, the current settings are returned

  // 1. number of seconds until the car stops after receiving the last command.
  int16_t numSeconds = cmdMessenger.readInt16Arg();
  if (cmdMessenger.isArgOk())
  {
    // An argument was supplied,
    // update the settings.
    ftSettings.setNumSeconds(numSeconds);

    // Update EEPROM
    ftSettings.update();
  }

  // Return the current settings
  cmdMessenger.sendCmdStart(kSettings);
  cmdMessenger.sendCmdArg(ftSettings.getNumSeconds());
  cmdMessenger.sendCmdEnd();
}

//------------- Utility functions ---------------

// Returns true if it has been more than interval (in ms) ago. Used for periodic actions
bool hasExpired(unsigned long &prevTime, unsigned long interval) {
  if (  millis() - prevTime > interval ) {
    prevTime = millis();
    return true;
  } else
    return false;
}


