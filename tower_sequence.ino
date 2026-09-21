#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

// ============================================================
// SETTINGS
// ============================================================
//   CH0 = base, CH1 = shoulder, CH2 = elbow,
//   CH3 = wrist pitch, CH4 = wrist rotate, CH5 = gripper

#define NUM_SERVOS   6
#define NUM_JOINTS   5
#define GRIPPER      5

#define PULSE_MIN    100
#define PULSE_MAX    650

#define STEP_SIZE    2     // pulse change per step
#define NORMAL_DELAY 15    // ms per step (normal speed)
#define SETTLE_MS    200   // pause after each sequence step

// ===== SEQUENCE START (replace this block with the GUI's code) =====
// section: 0 = start, 1-3 = block 1-3, 4 = finish
// arm step     : {section, 0, CH0, CH1, CH2, CH3, CH4, stepDelay}
// gripper step : {section, 1, CH5, 0, 0, 0, 0, stepDelay}
// stepDelay = ms per 2-pulse step (15 normal, 40 slow)
const int SEQ[][8] = {
  {0, 0, 375, 375, 375, 375, 375, 15}   // 1 home
};
// ===== SEQUENCE END =====

const int SEQ_LEN = sizeof(SEQ) / sizeof(SEQ[0]);

int currentPulse[NUM_SERVOS];

// ============================================================
// SERVO CONTROL
// ============================================================
void setPulse(uint8_t ch, int pulse) {
  pulse = constrain(pulse, PULSE_MIN, PULSE_MAX);
  pwm.setPWM(ch, 0, pulse);
  currentPulse[ch] = pulse;
}

void printPulseList() {
  Serial.print("(");
  for (int i = 0; i < NUM_SERVOS; i++) {
    Serial.print(currentPulse[i]);
    if (i < NUM_SERVOS - 1) Serial.print(", ");
  }
  Serial.println(")");
}

// Move one servo gradually
void smoothMoveServo(uint8_t ch, int target, int stepDelay) {
  target = constrain(target, PULSE_MIN, PULSE_MAX);
  while (currentPulse[ch] != target) {
    int diff = target - currentPulse[ch];
    setPulse(ch, currentPulse[ch] + constrain(diff, -STEP_SIZE, STEP_SIZE));
    delay(stepDelay);
  }
}

// Move all 5 arm joints together (gripper is NOT touched)
void moveToPose(const int *target, int stepDelay) {
  bool done = false;
  while (!done) {
    done = true;
    for (int ch = 0; ch < NUM_JOINTS; ch++) {
      int goal = constrain(target[ch], PULSE_MIN, PULSE_MAX);
      int diff = goal - currentPulse[ch];
      if (diff != 0) {
        done = false;
        setPulse(ch, currentPulse[ch] + constrain(diff, -STEP_SIZE, STEP_SIZE));
      }
    }
    delay(stepDelay);
  }
  printPulseList();
}

// ============================================================
// SEQUENCE PLAYBACK
// ============================================================
bool stopRequested() {
  while (Serial.available()) {
    String l = Serial.readStringUntil('\n');
    l.trim();
    if (l.equalsIgnoreCase("X")) return true;
  }
  return false;
}

// section = -1 plays everything, 1-3 plays only that block
void runSequence(int section) {
  if (section < 0) Serial.println("RUNNING WHOLE TOWER");
  else { Serial.print("RUNNING BLOCK "); Serial.println(section); }

  for (int i = 0; i < SEQ_LEN; i++) {
    if (section >= 0 && SEQ[i][0] != section) continue;
    if (stopRequested()) {
      Serial.println("STOPPED");
      return;
    }
    Serial.print("Step ");
    Serial.print(i + 1);
    Serial.print("/");
    Serial.println(SEQ_LEN);

    int d = SEQ[i][7] > 0 ? SEQ[i][7] : NORMAL_DELAY;
    if (SEQ[i][1] == 1) {
      smoothMoveServo(GRIPPER, SEQ[i][2], d);
      printPulseList();
    } else {
      moveToPose(&SEQ[i][2], d);
    }
    delay(SETTLE_MS);
  }
  Serial.println("SEQUENCE COMPLETE");
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(500);

  // Start at the first arm step of the sequence (usually home)
  int startPose[NUM_JOINTS] = {375, 375, 375, 375, 375};
  int startGrip = 150;
  bool foundPose = false, foundGrip = false;
  for (int i = 0; i < SEQ_LEN; i++) {
    if (!foundPose && SEQ[i][1] == 0) {
      for (int j = 0; j < NUM_JOINTS; j++) startPose[j] = SEQ[i][j + 2];
      foundPose = true;
    }
    if (!foundGrip && SEQ[i][1] == 1) {
      startGrip = SEQ[i][2];
      foundGrip = true;
    }
  }
  for (int ch = 0; ch < NUM_JOINTS; ch++) {
    setPulse(ch, startPose[ch]);
    delay(200);   // one at a time to limit current spike
  }
  setPulse(GRIPPER, startGrip);

  Serial.println();
  Serial.println("Commands:");
  Serial.println("  P                     = play the whole tower");
  Serial.println("  1 / 2 / 3             = play only that block");
  Serial.println("  X                     = stop the sequence");
  Serial.println("  M ch pulse [delay]    = move one servo");
  Serial.println("  G p0 p1 p2 p3 p4 [d]  = move arm to a pose");
  Serial.println("  S                     = print current pulses");
  Serial.println();
  printPulseList();
  Serial.println("READY");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  char cmd = toupper(line.charAt(0));

  if (cmd == 'P') {
    runSequence(-1);
  }
  else if (cmd == '1' || cmd == '2' || cmd == '3') {
    runSequence(cmd - '0');
  }
  else if (cmd == 'X') {
    // nothing running; ignore
  }
  else if (cmd == 'S') {
    printPulseList();
  }
  else if (cmd == 'M') {
    int ch = -1, pulse = -1, d = NORMAL_DELAY;
    int n = sscanf(line.c_str() + 1, "%d %d %d", &ch, &pulse, &d);
    if (n >= 2 && ch >= 0 && ch < NUM_SERVOS && pulse >= PULSE_MIN && pulse <= PULSE_MAX) {
      smoothMoveServo(ch, pulse, d > 0 ? d : NORMAL_DELAY);
      printPulseList();
    } else {
      Serial.println("Invalid. Use: M <ch 0-5> <pulse 100-650> [delay]");
    }
  }
  else if (cmd == 'G') {
    int p[NUM_JOINTS];
    int d = NORMAL_DELAY;
    int n = sscanf(line.c_str() + 1, "%d %d %d %d %d %d", &p[0], &p[1], &p[2], &p[3], &p[4], &d);
    if (n >= NUM_JOINTS) {
      moveToPose(p, d > 0 ? d : NORMAL_DELAY);
    } else {
      Serial.println("Invalid. Use: G p0 p1 p2 p3 p4 [delay]");
    }
  }
  else {
    Serial.println("Unknown command.");
  }

  // Tells the GUI this command is finished
  Serial.println("OK");
}
