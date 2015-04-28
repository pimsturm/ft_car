// CxemCAR 1.0 (06.01.2013)
// Project Page: http://english.cxem.net/mcu/mcu3.php
// Adapted for Funduino motor shield

#include <EEPROM.h>
#include <AFMotor.h>

const int light01 =   14;  // additional channel 1

const char cmdL = 'L';      // UART-command for left motor
const char cmdR = 'R';      // UART-command for right motor
const char cmdH = 'H';      // UART-command for additional channel (for example Horn)
const char cmdF = 'F';      // UART-command for EEPROM operation
const char cmdr = 'r';      // UART-command for EEPROM operation (read)
const char cmdw = 'w';      // UART-command for EEPROM operation (write)

// The maximum number of characters allowed in a command string on the command prompt
#define MAX_COMMAND_LENGTH	12

char echoCommand = 1;
char command;         // command
char commandBuffer[MAX_COMMAND_LENGTH];  // This buffer will be treated as a string which holds the command
AF_DCMotor leftMotor(1, MOTOR12_64KHZ);
AF_DCMotor rightMotor(2, MOTOR12_64KHZ);

unsigned long currentTime, lastTimeCommand, autoOff;

void setup() {
  Serial.begin(9600);       // initialization UART

  pinMode(light01, OUTPUT);    // additional channel

  initSettings();
  currentTime = millis();                // read the time elapsed since application start
}

void loop() {
  if (echoCommand == 1)Serial.print("> "); //Print the command prompt character
  getCommand(commandBuffer);  //Wait for the user to enter a command and store it in the commandBuffer string
  if (echoCommand == 1)Serial.println(""); //Send a new-line to the terminal

  if (!processCommand(commandBuffer))
    Serial.println("Invalid Command"); //Check the command for validity and execute the command in the commandBuffer string
  delay(10);

  lastTimeCommand = millis();                    // read the time elapsed since application start
  if (millis() >= (lastTimeCommand + autoOff)) {   // compare the current timer with variable lastTimeCommand + autoOff
    leftMotor.run(RELEASE);
    rightMotor.run(RELEASE);
  }
}

void initSettings() {
  // Find out if this is the first time the code is being executed
  char runCount = EEPROM.read(0);    // Read the value from the first location in EEPROM memory

  if (runCount != 1) {
    // Code has never been run before, set defaults
    autoOff = 2500;    // Switch off if no command was received for 2.5 seconds.
    echoCommand = 1;  // Echo the commands and debug information to the terminal.

    EEPROM.write(0, 1); // set the run count.
    EEPROM.write(1, 1); // The auto off function is on.

    // 2 - 4 number of seconds before switching off.
    EEPROM.write(2, '0');
    EEPROM.write(3, '2');
    EEPROM.write(4, '5');

    EEPROM.write(5, echoCommand); // Echo the commands and debug information to the terminal.
  }
  else {
    // Read the settings from EEPROM memory

    uint8_t swAutoOff = EEPROM.read(1);   // read EEPROM "is activated or not stopping the car when losing connection"
    if (swAutoOff == '1') {               // if activated
      char varData[3];
      varData[0] = EEPROM.read(2);
      varData[1] = EEPROM.read(3);
      varData[2] = EEPROM.read(4);
      autoOff = atoi(varData) * 100;      // variable autoOff ms
    }
    else {
      // Never switch off
      autoOff = 999999;
    }
  }

}

//Function Definitions
//Function: getCommand(char *)
//Inputs: char * command - the string to store an entered command
//Return: 0 - Command was too long
//        1 - Successfully loaded command to 'command' string
//usage: getCommand(string_name);
char getCommand(char * command)
{
  char receiveChar = 0, commandBufferCount = 0;

  // Get a command from the prompt (A command can have a maximum of MAX_COMMAND_LENGTH characters). Command is ended with a carriage return ('Enter' key)
  while (Serial.available() <= 0); // Wait for a character to come into the UART
  receiveChar = Serial.read();  // Get the character from the UART and put it into the receiveChar variable
  // Keep adding characters to the command string until the carriage return character is received (or tab for EEPROM commands)
  while (receiveChar != '\r' && receiveChar != '\t') {
    *command = receiveChar;    // Add the current character to the command string
    if (echoCommand == 1)Serial.print(*command++); // Print the current character, then move the end of the command sting.
    // Don't echo the character!
    else *command++;
    commandBufferCount++;    // Increase the character count variable
    if (commandBufferCount == (2 * MAX_COMMAND_LENGTH))return 0; // If the command is too long, exit the function and return an error

    while (Serial.available() <= 0); // Wait for a character to come into the UART
    receiveChar = Serial.read();  // Get the character from the UART and put it into the receiveChar variable
  }
  *command = '\0';		// Terminate the command string with a NULL character. This is so commandBuffer[] can be treated as a string.
  if (echoCommand == 1)
  {
    Serial.println("");
    Serial.println(command);
  }

  return 1;
}


//Function: processCommand(char *)
//Inputs: char * command - the string which contains the commandto be checked for validity and executed
//Outputs: 0 - Invalid command was sent to function
//         1 - Command executed
//usage: processCommand(string_name);
char processCommand(char * command)
{
  char commandLength = strlen(command);  // Find out how many characters are in the command string
  char motorDirection;

  char commandType = command[0];
  int commandValue = atoi(command + 1);

  if (echoCommand == 1) {
    Serial.print(">> processCommand: ");
    Serial.println(command);
    Serial.print("> Command length: ");
    Serial.println(commandLength);
  }

  switch (commandType)
  {
    case cmdF:
      if (command[1] == cmdr) {		    // if EEPROM data read command
        Serial.print("FData:");
        for (int i = 1; i < 6; i++)
        {
          Serial.write(EEPROM.read(i));     // read value from the memory with address <i> and print it to UART
        }
        Serial.print("\r\n");	      // mark the end of the transmission of data EEPROM
      }
      else if (command[1] == cmdw) {	    // if EEPROM write command
        for (int i = 1; i < 6; i++)
        {
          byte value;
          if (2 + i < commandLength)       // value starts at index 2
          {
            value = command[2 + i];
          }
          else
          {
            value = 0;
          }
          EEPROM.write(i, value);              // z1 record to a memory with address <i>
        }
        initSettings();		      // reinitialize the timer
        Serial.print("FWOK\r\n");	      // send a message that the data is successfully written to EEPROM
      }
      break;

    case cmdL:
    case cmdR:

      if (commandValue < 0)
      {
        commandValue = 255 - abs(commandValue);
        motorDirection = BACKWARD;
        if (echoCommand == 1) {
          Serial.println("> Backward");
        }
      }
      else
      {
        motorDirection = FORWARD;
        if (echoCommand == 1) {
          Serial.println("> Forward");
        }
      }

      if (commandType == cmdL)
      {
        leftMotor.setSpeed(commandValue);
        leftMotor.run(motorDirection);
        if (echoCommand == 1) {
          Serial.println("> Left");
        }
      }
      else
      {
        rightMotor.setSpeed(commandValue);
        rightMotor.run(motorDirection);
        if (echoCommand == 1) {
          Serial.println("> Right");
        }
      }

      break;

    case cmdH:
      if (echoCommand == 1) {
        Serial.println("light01");
      }
      if (commandValue)
      {
        if (echoCommand == 1) {
          Serial.println("High");
        }
        digitalWrite(light01, HIGH);           // additional channel
      }
      else
      {
        if (echoCommand == 1) {
          Serial.println("Low");
        }
        digitalWrite(light01, LOW);
      }
      break;
  }

  return 1;
}


