#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// ============================================================
// PCA9685 SETTINGS
// ============================================================

#define SERVOMIN 150
#define SERVOMAX 600
#define NUM_TRACKED 6

// ONLY THESE 3 SERVOS ARE USED FOR PICK/PLACE
#define BASE      0
#define ARM       1
#define GRIPPER   5

// track current pulse value for channels 0-5
int currentPulse[NUM_TRACKED] = {300, 400, 375, 375, 375, 150};
// (defaults roughly matching PICK_BASE, PICK_ARM, center, center, center, GRIP_OPEN)


// ============================================================
// CHANGE THESE VALUES DURING TRIAL AND ERROR
// ============================================================

// ---------- GRIPPER ----------
#define GRIP_OPEN   150
#define GRIP_CLOSE  500


// ---------- PICKUP POSITION ----------
#define PICK_BASE   300
#define PICK_ARM    400


// ---------- LIFT POSITION ----------
#define LIFT_BASE   300
#define LIFT_ARM    300


// ---------- BUILD LOCATION ----------
#define BUILD_BASE  450
#define BUILD_ARM   300


// ---------- LOWER TO STACK ----------
#define DROP_ARM    400


// ============================================================
// BASIC SERVO CONTROL
// ============================================================

void moveServo(uint8_t channel, int pulse) {

  pwm.setPWM(channel, 0, pulse);

  // update tracked value if within our tracked range
  if (channel < NUM_TRACKED) {
    currentPulse[channel] = pulse;
  }

  Serial.print("CH");
  Serial.print(channel);
  Serial.print(" -> ");
  Serial.println(pulse);
}


void printPulseList() {
  Serial.print("(");
  for (int i = 0; i < NUM_TRACKED; i++) {
    Serial.print(currentPulse[i]);
    if (i < NUM_TRACKED - 1) Serial.print(", ");
  }
  Serial.println(")");
}


// Move the three task servos.
void moveArm(int basePulse, int armPulse, int gripperPulse) {

  moveServo(BASE, basePulse);
  delay(300);

  moveServo(ARM, armPulse);
  delay(300);

  moveServo(GRIPPER, gripperPulse);
  delay(500);
}


// ============================================================
// GRIPPER
// ============================================================

void openGripper() {
  Serial.println("Opening gripper...");
  moveServo(GRIPPER, GRIP_OPEN);
  delay(700);
}


void closeGripper() {
  Serial.println("Closing gripper...");
  moveServo(GRIPPER, GRIP_CLOSE);
  delay(700);
}


// ============================================================
// MOVE TO PICKUP POSITION
// ============================================================

void moveToPickup() {
  Serial.println();
  Serial.println("Moving to PICKUP position...");

  moveServo(BASE, PICK_BASE);
  delay(500);

  moveServo(ARM, PICK_ARM);
  delay(500);

  openGripper();
  delay(500);
}


// ============================================================
// PICK UP OBJECT
// ============================================================

void pickObject() {
  Serial.println();
  Serial.println("PICKING UP OBJECT");

  openGripper();

  Serial.println("Lowering...");
  moveServo(ARM, DROP_ARM);
  delay(700);

  closeGripper();
  delay(700);

  Serial.println("Lifting object...");
  moveServo(ARM, LIFT_ARM);
  delay(1000);
}


// ============================================================
// MOVE TO BUILD LOCATION
// ============================================================

void moveToBuild() {
  Serial.println();
  Serial.println("Moving to BUILD LOCATION...");

  moveServo(BASE, BUILD_BASE);
  delay(700);

  moveServo(ARM, LIFT_ARM);
  delay(700);
}


// ============================================================
// PLACE OBJECT
// ============================================================

void placeObject() {
  Serial.println();
  Serial.println("PLACING OBJECT");

  Serial.println("Lowering object...");
  moveServo(ARM, DROP_ARM);
  delay(700);

  Serial.println("Releasing object...");
  openGripper();
  delay(700);

  Serial.println("Moving arm away...");
  moveServo(ARM, LIFT_ARM);
  delay(700);
}


// ============================================================
// RETURN TO PICKUP
// ============================================================

void returnToPickup() {
  Serial.println();
  Serial.println("Returning to pickup location...");

  moveServo(ARM, LIFT_ARM);
  delay(500);

  moveServo(BASE, PICK_BASE);
  delay(700);

  moveServo(ARM, PICK_ARM);
  delay(700);

  openGripper();
  delay(500);
}


// ============================================================
// PICK + PLACE ONE BLOCK
// ============================================================

void pickAndPlace() {
  moveToPickup();
  pickObject();
  moveToBuild();
  placeObject();
}


// ============================================================
// COMPLETE DEMO
// ============================================================

void runDemo() {
  Serial.println();
  Serial.println("====================================");
  Serial.println("       ROBOT DEMO 2 STARTING");
  Serial.println("====================================");

  Serial.println();
  Serial.println("========== BLOCK 1 ==========");
  pickAndPlace();
  delay(1000);

  Serial.println();
  Serial.println("========== BLOCK 2 ==========");
  returnToPickup();
  pickObject();
  moveToBuild();
  placeObject();
  delay(1000);

  Serial.println();
  Serial.println("========== BLOCK 3 ==========");
  returnToPickup();
  pickObject();
  moveToBuild();
  placeObject();

  Serial.println();
  Serial.println("====================================");
  Serial.println("       DEMO 2 COMPLETE!");
  Serial.println("====================================");
}


// ============================================================
// SETUP
// ============================================================

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  Serial.println();
  Serial.println("====================================");
  Serial.println("      3-SERVO PICK & PLACE");
  Serial.println("====================================");

  Serial.println();
  Serial.println("Commands:");
  Serial.println();
  Serial.println("P = Run full pick/place demo");
  Serial.println("O = Open gripper");
  Serial.println("C = Close gripper");
  Serial.println("H = Go to pickup position");
  Serial.println("B = Go to build position");
  Serial.println();
  Serial.println("Manual:");
  Serial.println("M <channel> <pulse>");
  Serial.println("Example: M 0 350");
  Serial.println();

  openGripper();
  printPulseList();
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  if (Serial.available()) {

    char command = Serial.read();

    if (command == 'P' || command == 'p') {
      runDemo();
    }

    else if (command == 'O' || command == 'o') {
      openGripper();
      printPulseList();
    }

    else if (command == 'C' || command == 'c') {
      closeGripper();
      printPulseList();
    }

    else if (command == 'H' || command == 'h') {
      moveToPickup();
      printPulseList();
    }

    else if (command == 'B' || command == 'b') {
      moveToBuild();
      printPulseList();
    }

    else if (command == 'M' || command == 'm') {

      int channel = Serial.parseInt();
      int pulse = Serial.parseInt();

      while (Serial.available()) {
        Serial.read();
      }

      if (channel >= 0 && channel <= 15 &&
          pulse >= 100 && pulse <= 650) {

        moveServo(channel, pulse);
        printPulseList();

      } else {
        Serial.println(
          "Invalid. Use: M <channel 0-15> <pulse 100-650>"
        );
      }
    }
  }
}
