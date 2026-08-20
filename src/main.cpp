#include <Arduino.h>

/*
- common ground and 5 V on perf board
- power supply power and ground connections
- current draw can be set via the onboard vref trimpot
- operating at 17.5 V
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

#define SENSOR_PIN 8

// button pin
const int trigger_button = 13;

/*speed-related variables*/
const int steps_per_rev = 200;
// ms1 & ms2 pins set to gnd for 8 microsteps as per datasheet
const int microsteps = 8;
const float rpm = 6;
const float pull_rpm = 17;

unsigned long step_interval_us;
unsigned long pull_step_interval_us;
unsigned long last_step_time1 = 0;
unsigned long last_step_time2 = 0;
unsigned long press_start_time = 0;
const unsigned long press_duration = 2100;
unsigned long state2_time = 0;
const unsigned long state2_duration_timeout = 10000;

unsigned long pull_foil_time = 0;
const unsigned long pull_foil_duration = 2000;

enum stateMachine {
  IDLE,
  TENSION,
  PRESS,
  PULL_FOIL,
};

stateMachine currentState = IDLE;

bool prev_triggered = false;

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

  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(trigger_button, INPUT_PULLUP);

  step_interval_us = (60UL * 1000000UL) / ((unsigned long)steps_per_rev * microsteps * rpm);
  pull_step_interval_us = (60UL * 1000000UL) / ((unsigned long)steps_per_rev * microsteps * pull_rpm);
}

void loop() {
  switch (currentState) {
    case IDLE: {
      bool triggered = digitalRead(trigger_button) == LOW;
      digitalWrite(EN_PIN, HIGH);
      digitalWrite(EN_PIN2, HIGH);

      if (triggered && !prev_triggered) {
        Serial.println("state 1 --> state 2");
        currentState = TENSION;
        state2_time = millis();
        digitalWrite(EN_PIN, LOW);
        digitalWrite(EN_PIN2, LOW);
      }
      prev_triggered = triggered;
      break;
    }

    // updated so that motor 2 holds while motor 1 spins
    case TENSION: {
      bool flag_blocked = digitalRead(SENSOR_PIN) == HIGH;

      // motor 1 holding torque
      digitalWrite(EN_PIN, LOW);

      if (!flag_blocked) {
        // motor 2 rotating ccw
        if (micros() - last_step_time2 >= step_interval_us) {
          last_step_time2 = micros();
          digitalWrite(DIR_PIN2, LOW);
          digitalWrite(STEP_PIN2, HIGH);
          delayMicroseconds(2);
          digitalWrite(STEP_PIN2, LOW);
        }
        // if flag is never detected, timeout and return to state 1
        if (millis() - state2_time >= state2_duration_timeout) {
          Serial.println("PRESS timeout: sensor never detected blocked");
          currentState = IDLE;
        }
      }
      else {
        Serial.println("flag detected: transitioning to the next state");

        currentState = PRESS;
        press_start_time = millis();
      }
      break;
    }

    case PRESS: {
      // motors holding torque from previous state
      digitalWrite(EN_PIN, LOW);
      digitalWrite(EN_PIN2, LOW);

      if (millis() - press_start_time < press_duration) {
        // put the press business here
        // currently timed, but pass through whatever condition
      }
      else {
        Serial.println("press complete");
        currentState = PULL_FOIL;
        pull_foil_time = millis();
      }
      break;
    }

    case PULL_FOIL: {
      // motor 1 cw
      if (millis() - pull_foil_time < pull_foil_duration) {
        if (micros() - last_step_time1 >= pull_step_interval_us) {
          last_step_time1 = micros();
          digitalWrite(DIR_PIN, HIGH);
          digitalWrite(STEP_PIN, HIGH);
          delayMicroseconds(2);
          digitalWrite(STEP_PIN, LOW);
        }

        // motor 2 cw
        if (micros() - last_step_time2 >= pull_step_interval_us) {
          last_step_time2 = micros();
          digitalWrite(DIR_PIN2, HIGH);
          digitalWrite(STEP_PIN2, HIGH);
          delayMicroseconds(2);
          digitalWrite(STEP_PIN2, LOW);
        }
      }
      else {
        Serial.println("pull foil done yay");
        currentState = IDLE;
      }
      break;
    }
  }
}

/* ******FSM pseudocode!

States overview:
- State 1 (Idle): Waiting for button press
- State 2: Tension and check sensor
- State 3: Press
- State 4: Pull excess foil
--> cycle complete, return to State 1

State 1:
If button press is detected -> transition to State 2
If no button press is detected -> stay in State 1
motors remain off throughout state

State 2:
Motor two moves counterclockwise and motor one is held until the flag is seen by the sensor
If not, there is a hard-cut timeout, which returns to State 1
Transition to State 3

State 3:
Cut and seal (tbd)

State 4:
Both motors rotate clockwise to pull excess foil
Cycle is complete, return to State 1 (IDLE)


Current unknowns/assumptions:
- logic for the press state is not yet implemented, so there's a timed placeholder for now
*/