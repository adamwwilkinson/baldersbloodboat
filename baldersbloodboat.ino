#include <NewPing.h>
#include <WiFi.h>
#include <esp_now.h>

#define BUTTON_L 0
#define BUTTON_R 35

#define SERVO 2

#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

#define SENSOR_ECHO 36
#define SENSOR_TRIG 32
#define MAX_DISTANCE 400

NewPing sonar(SENSOR_TRIG, SENSOR_ECHO, MAX_DISTANCE);

typedef struct struct_message {
  int percent_from_center;
} struct_message;

struct_message camValue;

// wavelength in microseconds
void pwm(int pin, int wave_length, float percentage) {
  float uptime = wave_length * percentage / 100.0;
  float downtime = wave_length * (100 - percentage) / 100.0;

  digitalWrite(pin, HIGH);
  delayMicroseconds(uptime);
  digitalWrite(pin, LOW);
  delayMicroseconds(downtime);
}

void forwards() { pwm(HBRIDGE_CW, 10000, 40); }

/**
 * degrees given is degree of servo
 * if wanna turn right servo goes left
 * value of -90 makes servo go left makes boat go right
 * servo should be from 0.6ms up time to 1.3ms uptime (0.6 makes boat go left)
 **/
void turn(int degrees) {
  int pwm_wavelength = 20000;

  int degree_to_100_percent = map(degrees, -90, 90, 300, 650);

  // 1ms pulse is -90 degrees, 1.5 ms pulse is 0 degrees, 2ms pulse is 90
  float percentage = degree_to_100_percent / 100.0;
  Serial.print("percentage: ");
  Serial.print(percentage);
  Serial.print('\n');

  pwm(SERVO, pwm_wavelength, percentage);
}

// remove forward momentum by blipping reverse for a second
void abrupt_stop() {
  static long last = millis();
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);

  Serial.print("abrupt stop\n");
  delay(1000);
  digitalWrite(HBRIDGE_CCW, LOW);
}

void get_distance(int* p_centimeters) {
  static long last = millis();
  if (millis() - last < 100) {
    return;
  }
  *p_centimeters = sonar.ping_cm();
}

void sensor() {
  static int centimeters = 0;
  static bool stopped = false;

  get_distance(&centimeters);

  // double nesting this is so sad
  if (centimeters > 0 && centimeters < 20) {
    if (!stopped) {
      abrupt_stop();
      stopped = true;
    }
  } else {
    stopped = false;
    Serial.print("forward\n");
    forwards();
  }
}

// likely baller
void PID_servo() {
  static int degrees_old = 0, error_old = 0, error_old2 = 0;
  // todo manually tune these values
  int Kp = 0.2, Ki = 0.05, Kd = 0.3;

  int error_function = camValue.percent_from_center;
  int raw_degrees = degrees_old + Kp * (error_function - error_old) +
                    Ki * (error_function + error_old) / 2 +
                    Kd * (error_function - 2 * error_old + error_old2);

  // invert degrees as servo is inverted
  int degrees = -1 * constrain(raw_degrees, -90, 90);
  turn(degrees);

  error_old2 = error_old;
  error_old = error_function;
  degrees_old = degrees;
}

void test_servo() {
  static int degrees = -90;
  static long last = millis();
  turn(0);
  delay(1000);
  turn(45);
  delay(1000);
  turn(90);
  delay(1000);
}

// very primitive, probably shit
void proportional_servo() {
  int current_error = camValue.percent_from_center;
  int degrees = -1 * map(current_error, -100, 100, -90, 90);
  turn(degrees);
}

void servo() {
  int percent_from_center = camValue.percent_from_center;
  int absolute_percent_from_center = abs(percent_from_center);
  int degrees;

  // pre much just a lookup here rather than a calculation
  // think of it as a slightly more complex bang bang controller
  // lotta complexity in order to add a PID controller when this might
  // just do 90% of the job
  if (absolute_percent_from_center < 5) {
    degrees = 0;
  } else if (absolute_percent_from_center >= 20) {
    degrees = 15;
  } else if (absolute_percent_from_center >= 60) {
    degrees = 60;
  } else {
    degrees = 90;
  }

  // inverted here as we want to turn the servo the opposite way to the boat
  if (percent_from_center > 0) {
    degrees = -degrees;
  }
  turn(degrees);
}

void OnDataRecv(const uint8_t* mac, const uint8_t* incomingData, int len) {
  memcpy(&camValue, incomingData, sizeof(camValue));
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

  pinMode(BUTTON_L, INPUT_PULLUP);
  pinMode(BUTTON_R, INPUT_PULLUP);

  pinMode(SERVO, OUTPUT);

  pinMode(HBRIDGE_CW, OUTPUT);
  pinMode(HBRIDGE_CCW, OUTPUT);

  pinMode(SENSOR_TRIG, OUTPUT);
  pinMode(SENSOR_ECHO, INPUT);
}

void loop() {
  /**
   * handles the recieved cam data and turns the boat using a PID controller
   **/
  // PID_servo();

  // servo should be from 0.6ms up time to 1.3ms uptime (0.6 makes boat go
  // left) for (int i = 60; i < 130; i++) {
  //   int uptime = 10 * i;

  //   digitalWrite(SERVO, HIGH);
  //   delayMicroseconds(uptime);
  //   digitalWrite(SERVO, LOW);
  //   delayMicroseconds(20000 - uptime);
  // }

  // for (int i = 130; i > 60; i--) {
  //   int uptime = 10 * i;

  //   digitalWrite(SERVO, HIGH);
  //   delayMicroseconds(uptime);
  //   digitalWrite(SERVO, LOW);
  //   delayMicroseconds(20000 - uptime);
  // }
  // delay(500);

  /**
   * i think i made this function too powerful but oh well
   * this mf controls forwards and backwards of boats but it stops me from
   * trying to be too smart with pointers and shii so hopefully its more
   * readable
   */
  sensor();
}
