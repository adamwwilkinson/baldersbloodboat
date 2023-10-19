#include <WiFi.h>
#include <esp_now.h>

#define SERVO 2

#define HBRIDGE_CW 12
#define HBRIDGE_CCW 13

#define SENSOR_ECHO 36
#define SENSOR_TRIG 32

typedef struct struct_message {
  int percent_from_center;
} struct_message;

struct_message camValue;

int percent_from_center;
const int servoChannel = 0;
const int motorChannel = 2;

const int servoFrequency = 50;
const int motorFrequency = 3000;

const int resolution = 8;

void forwards() { ledcWrite(motorChannel, 220); }

/**
 * degrees given is degree of servo
 * if wanna turn right servo goes left
 * value of -90 makes servo go left makes boat go right
 * servo should be from 0.6ms up time to 1.3ms uptime (0.6 makes boat go left)
 **/
void turn(int degrees) {
  int degrees_actual = degrees;
  int pwm_wavelength = 20000;

  static long last = millis();

  int degree_to_100_percent = map(degrees_actual, -90, 90, 300, 650);

  // 1ms pulse is -90 degrees, 1.5 ms pulse is 0 degrees, 2ms pulse is 90
  float percentage = degree_to_100_percent / 100.0;

  int actual = map(percentage, 0, 100, 0, 255);
  ledcWrite(servoChannel, actual);

  // Serial.println(percentage);

  // pwm(SERVO, pwm_wavelength, percentage);
}

// remove forward momentum by blipping reverse for a second
void abrupt_stop() {
  digitalWrite(HBRIDGE_CW, LOW);
  digitalWrite(HBRIDGE_CCW, HIGH);

  delay(500);
  digitalWrite(HBRIDGE_CCW, LOW);
}

void get_distance(int* p_centimeters) {
  long duration;
  digitalWrite(SENSOR_TRIG, LOW);
  delayMicroseconds(5);
  digitalWrite(SENSOR_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(SENSOR_TRIG, LOW);

  duration = pulseIn(SENSOR_ECHO, HIGH);
  *p_centimeters = (duration / 2) / 29.1;
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
    forwards();
  }
}

void test_servo() {
  int dutyCycle;
  for (dutyCycle = 10; dutyCycle <= 18; dutyCycle++) {
    ledcWrite(servoChannel, dutyCycle);
    delay(70);
  }
  for (dutyCycle = 18; dutyCycle >= 10; dutyCycle--) {
    ledcWrite(servoChannel, dutyCycle);
    delay(70);
  }
}

void bang_servo() {
  // Serial.print("percent from center: ");
  // Serial.println(percent_from_center);
  if (percent_from_center == -998) {
    return;
  }

  // 18 and 0 is max right and left
  if (percent_from_center < 0) {
    ledcWrite(servoChannel, 15);
  } else if (percent_from_center > 0) {
    ledcWrite(servoChannel, 13);
  } else {
    return;
  }
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

  pinMode(HBRIDGE_CW, OUTPUT);
  pinMode(HBRIDGE_CCW, OUTPUT);

  pinMode(SENSOR_TRIG, OUTPUT);
  pinMode(SENSOR_ECHO, INPUT);

  ledcSetup(servoChannel, servoFrequency, resolution);
  ledcSetup(motorChannel, motorFrequency, resolution);

  ledcAttachPin(SERVO, servoChannel);
  ledcWrite(servoChannel, 0);

  ledcAttachPin(HBRIDGE_CW, motorChannel);
  ledcWrite(motorChannel, 0);
}

void loop() {
  percent_from_center = camValue.percent_from_center;
  static int centimeters = 0;
  static bool stopped = false;

  get_distance(&centimeters);

  bang_servo();

  if (centimeters > 0 && centimeters < 20) {
    if (!stopped) {
      abrupt_stop();
      stopped = true;
    } else {
      ledcWrite(motorChannel, 0);
    }
  } else {
    stopped = false;
    if (percent_from_center == -998) {
      ledcWrite(motorChannel, 0);
    } else {
      forwards();
    }
  }
}