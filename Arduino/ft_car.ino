// CxemCAR 1.0 (06.01.2013)
// Project Page: http://english.cxem.net/mcu/mcu3.php
// Adapted for Funduino motor shield

#include <EEPROM.h>
#include <AFMotor.h>

#define HORN 14       // additional channel 1

#define cmdL 'L'      // UART-command for left motor
#define cmdR 'R'      // UART-command for right motor
#define cmdH 'H'      // UART-command for additional channel (for example Horn)
#define cmdF 'F'      // UART-command for EEPROM operation
#define cmdr 'r'      // UART-command for EEPROM operation (read)
#define cmdw 'w'      // UART-command for EEPROM operation (write)

//The maximum number of characters allowed in a command string on the command prompt
#define MAX_COMMAND_LENGTH	12

char echo_command = 1;
char command;         // command
char command_buffer[MAX_COMMAND_LENGTH];  //This buffer will be treated as a string which holds the command
AF_DCMotor left_motor(1, MOTOR12_64KHZ);
AF_DCMotor right_motor(2, MOTOR12_64KHZ);

unsigned long currentTime, lastTimeCommand, autoOFF;

void setup() {
  Serial.begin(9600);       // initialization UART

  pinMode(HORN, OUTPUT);    // additional channel

  timer_init();             // initialization software timer
}

void loop() {
  if (echo_command == 1)Serial.print("> "); //Print the command prompt character
  get_command(command_buffer);  //Wait for the user to enter a command and store it in the command_buffer string
  if (echo_command == 1)Serial.println(""); //Send a new-line to the terminal

  if (!process_command(command_buffer))
    Serial.println("Invalid Command"); //Check the command for validity and execute the command in the command_buffer string
  delay(10);

  lastTimeCommand = millis();                    // read the time elapsed since application start
  if (millis() >= (lastTimeCommand + autoOFF)) {   // compare the current timer with variable lastTimeCommand + autoOFF
    left_motor.run(RELEASE);
    right_motor.run(RELEASE);
  }
}


void timer_init() {
  uint8_t sw_autoOFF = EEPROM.read(0);   // read EEPROM "is activated or not stopping the car when losing connection"
  if (sw_autoOFF == '1') {               // if activated
    char var_Data[3];
    var_Data[0] = EEPROM.read(1);
    var_Data[1] = EEPROM.read(2);
    var_Data[2] = EEPROM.read(3);
    autoOFF = atoi(var_Data) * 100;      // variable autoOFF ms
  }
  else if (sw_autoOFF == '0') {
    autoOFF = 999999;
  }
  else if (sw_autoOFF == 255) {
    autoOFF = 2500;                      // if the EEPROM is blank, dafault value is 2.5 sec
  }
  currentTime = millis();                // read the time elapsed since application start
}

//Function Definitions
//Function: get_command(char *)
//Inputs: char * command - the string to store an entered command
//Return: 0 - Command was too long
//        1 - Successfully loaded command to 'command' string
//usage: get_command(string_name);
char get_command(char * command)
{
  char receive_char = 0, command_buffer_count = 0;

  //Get a command from the prompt (A command can have a maximum of MAX_COMMAND_LENGTH characters). Command is ended with a carriage return ('Enter' key)
  while (Serial.available() <= 0); //Wait for a character to come into the UART
  receive_char = Serial.read();  //Get the character from the UART and put it into the receive_char variable
  //Keep adding characters to the command string until the carriage return character is received (or tab for EEPROM commands)
  while (receive_char != '\r' && receive_char != '\t') {
    *command = receive_char;    //Add the current character to the command string
    if (echo_command == 1)Serial.print(*command++); //Print the current character, then move the end of the command sting.
    //Don't echo the character!
    else *command++;
    command_buffer_count++;    //Increase the character count variable
    if (command_buffer_count == (2 * MAX_COMMAND_LENGTH))return 0; //If the command is too long, exit the function and return an error

    while (Serial.available() <= 0); //Wait for a character to come into the UART
    receive_char = Serial.read();  //Get the character from the UART and put it into the receive_char variable
  }
  *command = '\0';		//Terminate the command string with a NULL character. This is so command_buffer[] can be treated as a string.
  if (echo_command == 1)
  {
    Serial.println("");
    Serial.println(command);
  }

  return 1;
}


//Function: process_command(char *)
//Inputs: char * command - the string which contains the commandto be checked for validity and executed
//Outputs: 0 - Invalid command was sent to function
//         1 - Command executed
//usage: process_command(string_name);
char process_command(char * command)
{
  char command_length = strlen(command);  //Find out how many characters are in the command string
  char motor_direction;

  char command_type = command[0];
  int cmd_val = atoi(command + 1);
  
  Serial.print(">> process_command: ");
  Serial.println(command);
  Serial.print("> Command length: ");
  Serial.println(command_length);

  switch (command_type)
  {
    case cmdF:
      if (command[1] == cmdr) {		    // if EEPROM data read command
        Serial.print("FData:");
        for (int i = 0; i < 4; i++)
        {
          Serial.write(EEPROM.read(i));     // read value from the memory with address <i> and print it to UART
        }
        Serial.print("\r\n");	      // mark the end of the transmission of data EEPROM
      }
      else if (command[1] == cmdw) {	    // if EEPROM write command
        for (int i = 0; i < 4; i++)
        {
          byte value;
          if (2 + i < command_length)       // value starts at index 2
          {
            value = command[2 + i];
          }
          else
          {
            value = 0;
          }
          EEPROM.write(i, value);              // z1 record to a memory with address <i>
        }
        timer_init();		      // reinitialize the timer
        Serial.print("FWOK\r\n");	      // send a message that the data is successfully written to EEPROM
      }
      break;

    case cmdL:
    case cmdR:

      if (cmd_val < 0)
      {
        cmd_val = 255 - abs(cmd_val);
        motor_direction = BACKWARD;
        Serial.println("> Backward");
      }
      else
      {
        motor_direction = FORWARD;
        Serial.println("> Forward");
      }

      if (command_type == cmdL)
      {
        left_motor.setSpeed(cmd_val);
        left_motor.run(motor_direction);
        Serial.println("> Left");
      }
      else
      {
        right_motor.setSpeed(cmd_val);
        right_motor.run(motor_direction);
        Serial.println("> Right");
      }

      break;

    case cmdH:
      Serial.println("Horn");
      if (cmd_val)
      {
        Serial.println("High");
        digitalWrite(HORN, HIGH);           // additional channel
      }
      else
      {
        Serial.println("Low");
        digitalWrite(HORN, LOW);
      }
      break;
  }

  return 1;
}


