#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVOMIN 150
#define SERVOMAX 600
#define NUM_SERVOS 3

int currentAngle[NUM_SERVOS] = {90, 90, 90};

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(10);

  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, currentAngle[i]);
  }

  Serial.println("Enter: <channel 0-2> <angle 0-180>");
  Serial.println("Example: 2 90");
}

void loop() {
  if (Serial.available()) {
    int ch = Serial.parseInt();
    int angle = Serial.parseInt();
    while (Serial.available()) Serial.read();

    if (ch >= 0 && ch < NUM_SERVOS && angle >= 0 && angle <= 180) {
      currentAngle[ch] = angle;
      setServoAngle(ch, angle);

      Serial.print("CH0=");
      Serial.print(currentAngle[0]);
      Serial.print(", CH1=");
      Serial.print(currentAngle[1]);
      Serial.print(", CH2=");
      Serial.println(currentAngle[2]);
    } else {
      Serial.println("Invalid. Format: <channel 0-2> <angle 0-180>");
    }
  }
}

void setServoAngle(uint8_t channel, int angle) {
  int pulse = map(angle, 0, 180, SERVOMIN, SERVOMAX);
  pwm.setPWM(channel, 0, pulse);
}