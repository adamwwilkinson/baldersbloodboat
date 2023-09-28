
#define BUTTON_L 0
#define BUTTON_R 35

#define SERVO 2

#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

#define SENSOR_ECHO 36
#define SENSOR_TRIG 37

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

void forwards() {
  digitalWrite(HBRIDGE_CCW, LOW);
  pwm(HBRIDGE_CW, 10, 70);
}

/**
 * degrees given is degree of servo
 * if wanna turn right servo goes left
 * value of -90 makes servo go left makes boat go right
 **/
void turn(int degrees) {
  int degree_to_microseconds = map(degrees, -90, 90, 1000, 2000);

  /**
   * the 200 value here comes from the wavelength of 20ms and the coversion from
   * microseconds to percentage
   * ...
   * fine i wont make it a magic number
   * jesus dylan
   **/

  int wavelength_to_percentage_factor = 200;

  // 1ms pulse is -90 degrees, 1.5 ms pulse is 0 degrees, 2ms pulse is 90
  int percentage = degree_to_microseconds / wavelength_to_percentage_factor;

  pwm(SERVO, 20, percentage);
}

// remove forward momentum by blipping reverse for a second
void abrupt_stop() {
  static long last = millis();
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);

  if (millis() - last > 1000) {
    digitalWrite(HBRIDGE_CCW, LOW);
  }
}

// read datasheet if ur curious on why below does what it does
// https://download.altronics.com.au/files/datasheets_Z6322Doc2.pdf
void sensor_ping() {
  digitalWrite(SENSOR_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(SENSOR_TRIG, LOW);
}

bool echo_recieved() {
  if (digitalRead(SENSOR_ECHO) == HIGH) {
    return true;
  }
  return false;
}

// function called every 60ms, checks distance and if too close calls for a stop
// function also moves boat forward
void sensor() {
  static long last = millis();

  long start, journey_time;
  int centimeters = 0;

  if (millis() - last > 60) {
    start = millis();
    sensor_ping();
  }

  if (echo_recieved()) {
    journey_time = millis() - start;
  } else {
    journey_time = 0;
  }

  if (journey_time > 0) {
    // formula wants it in micro but journey is in milli
    centimeters = journey_time * 1000 / 58;
  }

  if (centimeters < 10 && centimeters > 0) {
    abrupt_stop();
  } else {
    forwards();
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

  pinMode(SENSOR_ECHO, INPUT_PULLDOWN);
  pinMode(SENSOR_TRIG, OUTPUT);
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

  /**
   * i think i made this function too powerful but oh well
   * this mf controls forwards and backwards of boats but it stops me from
   * trying to be too smart with pointers and shii so hopefully its more
   * readable
   */
  sensor();
}
