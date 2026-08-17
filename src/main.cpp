#include <Arduino.h>

/*
- current working with one button logic for timed motion 
  -> uncomment the relevant sections for four button individual motion logic

- common ground and 5 V on perf board
- power supply power and ground connections
- current draw can be set via the onboard vref trimpot
*/

/*pin definitions*/
// enable pins
#define EN_PIN 4
#define EN_PIN2 3

// step pins
#define STEP_PIN 9
#define STEP_PIN2 10

// direction pins
#define DIR_PIN 5
#define DIR_PIN2 11

/*button variables*/
// motor 1 controls
// const int cw_button = 13;
const int trigger_button = 13;
const int ccw_button = 12;

// motor 2 controls
const int cw_button2 = 7;
const int ccw_button2 = 2;

/*speed-related variables*/
const int steps_per_rev = 200;
// ms1 & ms2 pins set to gnd for 8 microsteps as per datasheet
const int microsteps = 8;
const float rpm = 6;

unsigned long step_interval_us;
unsigned long last_step_time1 = 0;
unsigned long last_step_time2 = 0;

enum stateMachine {
  IDLE,
  PRESS,
};

stateMachine currentState = IDLE;

bool prev_triggered = false;
unsigned long state2_time = 0;
const unsigned long state2_duration = 1750;

void setup() {
  Serial.begin(9600);

  // pin mode declarations
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  pinMode(EN_PIN2, OUTPUT);
  digitalWrite(EN_PIN2, HIGH);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(STEP_PIN2, OUTPUT);

  pinMode(DIR_PIN, OUTPUT);
  pinMode(DIR_PIN2, OUTPUT);

  // pinMode(cw_button, INPUT_PULLUP);
  pinMode(trigger_button, INPUT_PULLUP);
  pinMode(ccw_button, INPUT_PULLUP);
  pinMode(cw_button2, INPUT_PULLUP);
  pinMode(ccw_button2, INPUT_PULLUP);

  step_interval_us = (60UL * 1000000UL) / ((unsigned long)steps_per_rev * microsteps * rpm);
}

bool prev_cw = false, prev_ccw = false;
bool prev_cw2 = false, prev_ccw2 = false;

void loop() {
  switch (currentState) {
    case IDLE: {
      bool triggered = digitalRead(trigger_button) == LOW;
      digitalWrite(EN_PIN, HIGH);
      digitalWrite(EN_PIN2, HIGH);

      if (triggered && !prev_triggered) {
        Serial.println("state 1 --> state 2");
        currentState = PRESS;
        state2_time = millis();
        digitalWrite(EN_PIN, LOW);
        digitalWrite(EN_PIN2, LOW);
      }
      prev_triggered = triggered;
      break;
    }

    case PRESS: {
      if (millis() - state2_time < state2_duration) {
        if (micros() - last_step_time1 >= step_interval_us) {
          last_step_time1 = micros();
          digitalWrite(DIR_PIN, HIGH);
          digitalWrite(STEP_PIN, HIGH);
          delayMicroseconds(2);
          digitalWrite(STEP_PIN, LOW);
        }

        if (micros() - last_step_time2 >= step_interval_us) {
          last_step_time2 = micros();
          digitalWrite(DIR_PIN2, LOW);
          digitalWrite(STEP_PIN2, HIGH);
          delayMicroseconds(2);
          digitalWrite(STEP_PIN2, LOW);
        }
      }
      else {
        Serial.println("state 2 is done :p");
        digitalWrite(EN_PIN, HIGH);
        digitalWrite(EN_PIN2, HIGH);
      }
      break;
    }
  }
  
}

/* ******FSM pseudocode!

States overview:
- State 1 (Idle): Waiting for button press
- State 2: Tension and press
- State 3: Check sensor
- State 4: Cut & Seal
- State 5: Pull excess foil
- State 6: Release
--> cycle complete, return to State 1

State 1:
If button press is detected -> transition to State 2
If no button press is detected -> stay in State 1
motors remain off throughout state

State 2:
Motor one moves clockwise and motor two moves counterclockwise for a set amount of time
Motors are held
Transition to State 3

State 3:
if sensor data is within range -> transition to State 4
If sensor data is not within range after a certain number of checks -> stay in State 3
Possible go to a subState for repositioning and reenter State 3 instead

State 4:
Whatever mechanism (heat?) cuts and seals the septum
Transition to State 4

State 5:
The two motors spin in the same direction for a set amount of time
Excess foil is pulled, resetting the system for the next cycle
Transition to State 5

State 6:
The motors are released
Cycle is complete, return to state 1


Current unknowns/assumptions:
- As of right now, I'm assuming that the logic is time-based
    - alternatively, we could use position sensors, which would change the FSM logic, 
      because there would be additional checks
- If we use time-based logic, the system needs to be robust enough to ensure consistency
    - this is something I can test, but initially, I'm not confident
- For State 3, there may be a reposition subState if the sensor data is "incorrect"
    - something like releasing tension and regaining it

*/