
#define SERVO 2
#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

// wavelength in milliseconds
void pwm(int pin, int wave_length, int ratio) {
  static long last = millis();

  if (millis() - last > wave_length * ratio / 100.0) {
    digitalWrite(pin, HIGH);
  }

  if (millis() - last > wave_length * (100 - ratio) / 100.0) {
    digitalWrite(pin, LOW);
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
  pinMode(SERVO, OUTPUT);
  pinMode(HBRIDGE_CW, OUTPUT);
  pinMode(HBRIDGE_CCW, OUTPUT);
}

void loop() {}
