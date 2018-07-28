/*
  Gocupi Arduino Code
  Reads movement commands over serial and controls two stepper motors 
*/

// comment out to disable PENUP support
#define ENABLE_PENUP

// For RAMPS 1.4
#define X_STEP_PIN         A0
#define X_DIR_PIN          A1
#define X_ENABLE_PIN       38
#define X_MIN_PIN           3
#define X_MAX_PIN           2

#define Y_STEP_PIN         A6
#define Y_DIR_PIN          A7
#define Y_ENABLE_PIN       A2
#define Y_MIN_PIN          14
#define Y_MAX_PIN          15

#define Z_STEP_PIN         46
#define Z_DIR_PIN          48
#define Z_ENABLE_PIN       A8
#define Z_MIN_PIN          18
#define Z_MAX_PIN          19

#define E_STEP_PIN         26
#define E_DIR_PIN          28
#define E_ENABLE_PIN       24

#define SDPOWER            -1
#define SDSS               53
#define LED_PIN            13

#define SERVO1             11

#define FAN_PIN            9

#define PS_ON_PIN          12
#define KILL_PIN           -1

#define HEATER_0_PIN       10
#define HEATER_1_PIN       8
#define TEMP_0_PIN         A13
#define TEMP_1_PIN         A14
#define TEMP_2_PIN         A15

// Constants and global variables
// --------------------------------------
const int LED_PINS_COUNT = 1;
const int LED_PINS[LED_PINS_COUNT] = {
  LED_PIN}; // the pins of all of the leds, first 3 are status lights, 4th is receive indicator
const int LEFT_ENABLE_PIN = Y_ENABLE_PIN;
const int LEFT_STEP_PIN = Y_STEP_PIN;
const int LEFT_DIR_PIN = Y_DIR_PIN;
const int RIGHT_ENABLE_PIN = Z_ENABLE_PIN;
const int RIGHT_STEP_PIN = Z_STEP_PIN;
const int RIGHT_DIR_PIN = Z_DIR_PIN;
//Set micro stepping pins here
//#define M0_PIN 8
//#define M1_PIN 7
//#define M2_PIN 6

//Set the micro stepping level here (Defaulting to 1/16th as that doesn't require modification to Processing Control app source to support 1/32nd)
//DRV8825 Microstepping configuration
//
// M0  M1  M2  STEP MODE
// 0   0   0   Full step (2-phase excitation) with 71% current
// 1   0   0   1/2 step (1-2 phase excitation)
// 0   1   0   1/4 step (W1-2 phase excitation)
// 1   1   0   8 microsteps/step
// 0   0   1   16 microsteps/step
// 1   0   1   32 microsteps/step
// 0   1   1   32 microsteps/step
// 1   1   1   32 microsteps/step
//#define M0_STEP HIGH
//#define M1_STEP HIGH
//#define M2_STEP LOW


#ifdef ENABLE_PENUP
#include <Servo.h>
Servo penUpServo;
char penTransitionDirection; // -1, 0, 1
const int PENUP_SERVO_PIN = 14;
const long PENUP_TRANSITION_US = 524288; // time to go from pen up to down, or down to up
const int PENUP_TRANSITION_US_LOG = 19; // 2^19 = 524288
const long PENUP_COOLDOWN_US = 1250000;
const long PENUP_ANGLE = 170;
const long PENDOWN_ANGLE = 85;
#endif

const unsigned int TIME_SLICE_US = 2048; // number of microseconds per time step
const unsigned int TIME_SLICE_US_LOG = 11; // log base 2 of TIME_SLICE_US
const unsigned int POS_FACTOR = 8; // fixed point factor each position is multiplied by
const unsigned int POS_FACTOR_LOG = 3; // log base 2 of POS_FACTOR, used after multiplying two fixed point numbers together

const char RESET_COMMAND = 0x80; // -128, (128) command to reset
const char PENUP_COMMAND = 0x81; // -127, (129) command to lift pen
const char PENDOWN_COMMAND = 0x7F; // 127,(127) command to lower pen

const unsigned int MOVE_DATA_CAPACITY = 1024;
char moveData[MOVE_DATA_CAPACITY]; // buffer of move data, circular buffer
unsigned int moveDataStart = 0; // where data is currently being read from
unsigned int moveDataLength = 0; // the number of items in the moveDataBuffer
unsigned int moveDataRequestPending = 0; // number of bytes requested

char leftDelta, rightDelta; // delta in the current slice
long leftStartPos, rightStartPos; // start position for this slice
long leftCurPos, rightCurPos; // current position of the spools

unsigned long curTime; // current time in microseconds
unsigned long sliceStartTime; // start of current slice in microseconds


// setup
// --------------------------------------
void setup() {
  Serial.begin(57600);
  Serial.setTimeout(0);

  // setup pins
  for(int ledIndex = 0; ledIndex < LED_PINS_COUNT; ledIndex++) {
    pinMode(LED_PINS[ledIndex], OUTPUT);
    digitalWrite(LED_PINS[ledIndex], HIGH);
  }

//  pinMode(M0_PIN, OUTPUT);
//  pinMode(M1_PIN, OUTPUT);
//  pinMode(M2_PIN, OUTPUT);
//  digitalWrite(M0_PIN, M0_STEP);
//  digitalWrite(M1_PIN, M1_STEP);
//  digitalWrite(M2_PIN, M2_STEP);
    
  pinMode(LEFT_STEP_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_STEP_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);	
  pinMode(LEFT_ENABLE_PIN, OUTPUT);
  pinMode(RIGHT_ENABLE_PIN, OUTPUT);
  digitalWrite(LEFT_ENABLE_PIN, LOW);
  digitalWrite(RIGHT_ENABLE_PIN, LOW);
#ifdef ENABLE_PENUP
  penUpServo.attach(PENUP_SERVO_PIN);
  penUpServo.write(PENUP_ANGLE);
  delay(1000);
  penUpServo.write(PENDOWN_ANGLE);
  delay(1000);
  penUpServo.write(PENUP_ANGLE);
#endif  

  ResetMovementVariables();

  delay(500);
  UpdateReceiveLed(false);
  UpdateStatusLeds(0);
}

// Reset all movement variables
// --------------------------------------
void ResetMovementVariables()
{
  leftDelta = rightDelta = leftStartPos = rightStartPos = leftCurPos = rightCurPos = 0;
  sliceStartTime = curTime;

#ifdef ENABLE_PENUP
  penTransitionDirection = 0;
  penUpServo.write(PENUP_ANGLE);
#endif  
}

// Main execution loop
// --------------------------------------
void loop() {
  curTime = micros();
  if (curTime < sliceStartTime) { // protect against 70 minute overflow
    sliceStartTime = 0;
  }

  long curSliceTime = curTime - sliceStartTime;

#ifdef ENABLE_PENUP
  if (penTransitionDirection) {
    UpdatePenTransition(curSliceTime);
    if (!penTransitionDirection) {
      sliceStartTime = curTime;
    }
  } else {	
#endif
    // move to next slice if necessary
    while(curSliceTime > TIME_SLICE_US) {
      SetSliceVariables();
      curSliceTime -= TIME_SLICE_US;
      sliceStartTime += TIME_SLICE_US;

#ifdef ENABLE_PENUP	
      if (penTransitionDirection) {
        sliceStartTime = curTime;
        return;
      }
#endif      
    }
	
    UpdateStepperPins(curSliceTime);
#ifdef ENABLE_PENUP    
  }
#endif  

  ReadSerialMoveData();
  RequestMoreSerialMoveData();
}

// Update stepper pins
// --------------------------------------
void UpdateStepperPins(long curSliceTime) {
  long leftTarget = ((long(leftDelta) * curSliceTime) >> TIME_SLICE_US_LOG) + leftStartPos;
  long rightTarget = ((long(rightDelta) * curSliceTime) >> TIME_SLICE_US_LOG) + rightStartPos;

  int leftSteps = (leftTarget - leftCurPos) >> POS_FACTOR_LOG;
  int rightSteps = (rightTarget - rightCurPos) >> POS_FACTOR_LOG;

  boolean leftPositiveDir = true;
  if (leftSteps < 0) {
    leftPositiveDir = false;
    leftSteps = -leftSteps;
  }
  boolean rightPositiveDir = true;
  if (rightSteps < 0) {
    rightPositiveDir = false;
    rightSteps = -rightSteps;
  }

  do {
    if (leftSteps) {
      Step(LEFT_STEP_PIN, LEFT_DIR_PIN, leftPositiveDir);
      if (leftPositiveDir) {
        leftCurPos += POS_FACTOR;
      } else {
        leftCurPos -= POS_FACTOR;
      }
      leftSteps--;
      
      UpdateStatusLeds(leftCurPos >> 13);
    }

    if (rightSteps) {
      Step(RIGHT_STEP_PIN, RIGHT_DIR_PIN, rightPositiveDir);
      if (rightPositiveDir) {
        rightCurPos += POS_FACTOR;
      } else {
        rightCurPos -= POS_FACTOR;
      }
      rightSteps--;
    }

    if (leftSteps || rightSteps) {
      delayMicroseconds(50); // delay a small amount of time before refiring the steps to smooth things out
    } else {
      break;
    }
  } while(true);
}

// Update pen position
// --------------------------------------
#ifdef ENABLE_PENUP
void UpdatePenTransition(long curSliceTime) {
	
  //int targetAngle = ((float)(PENDOWN_ANGLE - PENUP_ANGLE) * ((float)curSliceTime / (float)PENUP_TRANSITION_US)) + PENUP_ANGLE;
  //if (targetAngle > PENDOWN_ANGLE) {
    //targetAngle = PENDOWN_ANGLE;
    
    if (curSliceTime > PENUP_COOLDOWN_US) {
      penTransitionDirection = 0; // are done moving the pen servo
    }
  //}

  if (penTransitionDirection == 1) {
	//targetAngle = 180 - targetAngle;
    penUpServo.write(PENUP_ANGLE);
  } else if (penTransitionDirection == -1) {
    penUpServo.write(PENDOWN_ANGLE);
  }

  //penUpServo.write(targetAngle);
}
#endif

// Update status leds
// --------------------------------------
void UpdateStatusLeds(int value) {
  // output the time to the leds in binary
//  digitalWrite(LED_PINS[0], value & 0x1);
//  digitalWrite(LED_PINS[1], value & 0x2);
//  digitalWrite(LED_PINS[2], value & 0x4);
}

// Update receive leds
// --------------------------------------
void UpdateReceiveLed(boolean value) {
  digitalWrite(LED_PINS[0], value);
}

// Step
// --------------------------------------
void Step(int stepPin, int dirPin, boolean dir) {
  digitalWrite(dirPin, dir);

  digitalWrite(stepPin, LOW);
  digitalWrite(stepPin, HIGH);
}

// Set all variables based on the data currently in the buffer
// --------------------------------------
void SetSliceVariables() {
  // set slice start pos to previous slice start plus previous delta
  leftStartPos = leftStartPos + long(leftDelta);
  rightStartPos = rightStartPos + long(rightDelta);

  if (moveDataLength < 2) {
    leftDelta = rightDelta = 0;
  } else {
    leftDelta = MoveDataGet();
    rightDelta = MoveDataGet();
    
#ifdef ENABLE_PENUP	
    if (leftDelta == PENUP_COMMAND) {
      leftDelta = rightDelta = 0;
      penTransitionDirection = 1;
    } else if (leftDelta == PENDOWN_COMMAND) {
      leftDelta = rightDelta = 0;
      penTransitionDirection = -1;
    }
#else
    if (leftDelta == PENUP_COMMAND || leftDelta == PENDOWN_COMMAND) {
       leftDelta = rightDelta = 0;
    }
#endif    
  }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    

// Stop everything and blink the status led value times
// --------------------------------------
void Blink(char value) {
 int counts = value;
  if (counts<0) counts=-counts;

  UpdateReceiveLed(false);
  for(int i=0;i<counts;i++) {
   delay(1000);
   UpdateReceiveLed(true);
   delay(1000);
   UpdateReceiveLed(false);
    
  }
  delay(100000);
}

// Read serial data if its available
// --------------------------------------
void ReadSerialMoveData() {     

  if(Serial.available()) {
    char value = Serial.read();
    
    // Check if this value is the sentinel reset value
    if (value == RESET_COMMAND) {
      ResetMovementVariables();
      moveDataRequestPending = 0;
      moveDataLength = 0;
      UpdateReceiveLed(false);
      return;
    }

    MoveDataPut(value);
    moveDataRequestPending--;

    if (!moveDataRequestPending) {
      UpdateReceiveLed(false);
    }
  }
}

// Put a value onto the end of the move data buffer
// --------------------------------------
void MoveDataPut(char value) {
  int writePosition = moveDataStart + moveDataLength;
  if (writePosition >= MOVE_DATA_CAPACITY) {
    writePosition = writePosition - MOVE_DATA_CAPACITY;
  }

  moveData[writePosition] = value;

  if (moveDataLength == MOVE_DATA_CAPACITY) { // full, overwrite existing data
    moveDataStart++;
    if (moveDataStart == MOVE_DATA_CAPACITY) {
      moveDataStart = 0;
    }
  } 
  else {
    moveDataLength++;
  }
}

// Return a piece of data sitting in the moveData buffer, removing it from the buffer
// --------------------------------------
char MoveDataGet() {
  if (moveDataLength == 0) {
    return 0;
  }

  char result = moveData[moveDataStart];
  moveDataStart++;
  if (moveDataStart == MOVE_DATA_CAPACITY) {
    moveDataStart = 0;
  }
  moveDataLength--;

  return result;
}

// Return the amount of data sitting in the moveData buffer
// --------------------------------------
void RequestMoreSerialMoveData() {
  if (moveDataRequestPending > 0 || MOVE_DATA_CAPACITY - moveDataLength < 128)
    return;

  // request 128 bytes of data
  Serial.write(128);
  moveDataRequestPending = 128;
  UpdateReceiveLed(true);
}


