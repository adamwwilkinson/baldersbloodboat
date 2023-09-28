
#define BUTTON_L 0
#define BUTTON_R 35

#define SERVO 2
#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

// wavelength in milliseconds
void pwm(int pin, int wave_length, int ratio) {
  static long last = millis();
  static boolean up = true;

  float uptime = wave_length * ratio / 100.0;

  float downtime = wave_length * (100 - ratio) / 100.0;

  if (millis() - last > downtime && !up) {
    digitalWrite(pin, HIGH);
    up = true;
    last = millis();
  }

  if (millis() - last > uptime && up) {
    digitalWrite(pin, LOW);
    up = false;
    last = millis();
  }
}

void forwards() { pwm(HBRIDGE_CW, 10, 70); }

/**
 * degrees given is degree of servo
 * if wanna turn right servo goes left
 * value of -90 makes servo go left makes boat go right
 **/
void turn(int degrees) {
  int degree_to_microseconds = map(degrees, -90, 90, 1000, 2000);

  // 1ms pulse is -90 degrees, 1.5 ms pulse is 0 degrees, 2ms pulse is 90
  int percentage = degree_to_microseconds / 200;

  pwm(SERVO, 20, percentage);
}

// Remove forward momenting by blipping reverse for a second
void abrupt_stop() {
  static long last = millis();
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);

  if (millis() - last > 1000) {
    digitalWrite(HBRIDGE_CCW, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  pinMode(SERVO, OUTPUT);
  pinMode(HBRIDGE_CW, OUTPUT);
  pinMode(HBRIDGE_CCW, OUTPUT);
}

void loop() {
  static int pwm_value_index = 2;
  int pwm_values[5] = {-90, -45, 0, 45, 90};

  if (digitalRead(BUTTON_L) == LOW && pwm_value_index > 0) {
    Serial.print("-1");
    pwm_value_index--;
    delay(100);
  }

  if (digitalRead(BUTTON_R) == LOW && pwm_value_index < 4) {
    Serial.print("+1");
    pwm_value_index++;
    delay(100);
  }

  turn(pwm_values[pwm_value_index]);
}
