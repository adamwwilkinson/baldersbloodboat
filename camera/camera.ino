// Reference: https://github.com/alanesq/esp32cam-demo/blob/master/ESP32cam-demo.ino

#if !defined ESP32
#error This sketch is only for an ESP32Cam module
#endif

#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "driver/ledc.h"

#include "camera_server.h"
#include "camera.h"

#define SSID_NAME "Telstra1B1452"
#define SSID_PASWORD "dawetwyavxf6"

char *stitle = "ESP32Cam";  // title of this sketch
char *sversion = "1.0";     // Sketch version

bool sendRGBfile = 0;  // if set '/rgb' will just return raw rgb data which can be saved as a file rather than display a HTML pag

uint16_t dataRefresh = 2;   // how often to refresh data on root web page (seconds)
uint16_t imageRefresh = 2;  // how often to refresh the image on root web page (seconds)

const bool serialDebug = 1;  // show debug info. on serial port (1=enabled, disable if using pins 1 and 3 as gpio)

// Camera related
bool flashRequired = 1;                          // If flash to be used when capturing image (1 = yes)
framesize_t FRAME_SIZE_IMAGE = FRAMESIZE_SVGA;  // Resolution
#define PIXFORMAT PIXFORMAT_JPEG;                // TODO: investigate RGB888
int cameraImageExposure = 0;                     // Camera exposure (0 - 1200)   If gain and exposure both set to zero then auto adjust is enabled
int cameraImageGain = 0;                         // Image gain (0 - 30)
int cameraImageBrightness = 0;                   // Image brightness (-2 to +2)

const int timeBetweenStatus = 600;  // speed of flashing system running ok status light (milliseconds)

const int indicatorLED = 33;  // onboard small LED pin (33)

// Bright LED (Flash)
const int brightLED = 4;       // onboard Illumination/flash LED pin (4)
int brightLEDbrightness = 0;   // initial brightness (0 - 255)
const int ledFreq = 5000;      // PWM settings
const int ledChannel = 15;     // camera uses timer1
const int ledRresolution = 8;  // resolution (8 = from 0 to 255)

const int iopinA = 13;  // general io pin 13
const int iopinB = 12;  // general io pin 12 (must not be high at boot)

const int serialSpeed = 115200;  // Serial data speed to use

WebServer server(80);  // serve web pages on port 80

uint32_t lastStatus = millis();      // last time status light changed status (to flash all ok led)
String imageResDetails = "Unknown";  // image resolution info

struct tm timeinfo = {};
const char *ntpServer = "pool.ntp.org";
const char *TZ_INFO = "GMT+8BST-1,M3.5.0/01:00:00,M10.5.0/02:00:00";
long unsigned lastNTPtime;
time_t now;

// Used to disable brownout detection
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Random utility functions that I couldn't figure out how to separate
void flashLED(int reps) {
  for (int x = 0; x < reps; x++) {
    digitalWrite(indicatorLED, LOW);
    delay(1000);
    digitalWrite(indicatorLED, HIGH);
    delay(500);
  }
}

void brightLed(byte ledBrightness) {
  brightLEDbrightness = ledBrightness;   // store setting
  ledcWrite(ledChannel, ledBrightness);  // change LED brightness (0 - 255)
  if (serialDebug) Serial.println("Brightness changed to " + String(ledBrightness));
}

void setupFlashPWM() {
  ledcSetup(ledChannel, ledFreq, ledRresolution);
  ledcAttachPin(brightLED, ledChannel);
  brightLed(brightLEDbrightness);
}

bool getNTPtime(int sec) {
  uint32_t start = millis();  // timeout timer

  do {
    time(&now);
    localtime_r(&now, &timeinfo);
    if (serialDebug) Serial.print(".");
    delay(100);
  } while (((millis() - start) <= (1000 * sec)) && (timeinfo.tm_year < (2016 - 1900)));

  if (timeinfo.tm_year <= (2016 - 1900)) return false;  // the NTP call was not successful
  if (serialDebug) {
    Serial.print("now ");
    Serial.println(now);
  }

  // Display time
  if (serialDebug) {
    char time_output[30];
    strftime(time_output, 30, "%a  %d-%m-%y %T", localtime(&now));
    Serial.println(time_output);
    Serial.println();
  }
  return true;
}







void setup() {
  if (serialDebug) {
    Serial.begin(serialSpeed);  // Start serial communication
    // Serial.setDebugOutput(true);

    Serial.println("\n\n\n");  // line feeds
    Serial.println("-----------------------------------");
    Serial.printf("Starting - %s - %s \n", stitle, sversion);
    Serial.println("-----------------------------------");
  }

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Turn-off the 'brownout detector'

  // small indicator led on rear of esp32cam board
  pinMode(indicatorLED, OUTPUT);
  digitalWrite(indicatorLED, HIGH);

  // Connect to wifi
  digitalWrite(indicatorLED, LOW);  // small indicator led on
  if (serialDebug) {
    Serial.print("\nConnecting to ");
    Serial.print(SSID_NAME);
    Serial.print("\n   ");
  }
  WiFi.begin(SSID_NAME, SSID_PASWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (serialDebug) Serial.print(".");
  }
  if (serialDebug) {
    Serial.print("\nWiFi connected, ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  server.begin();                    // start web server
  digitalWrite(indicatorLED, HIGH);  // small indicator led off

  // define the web pages (i.e. call these procedures when url is requested)
  server.on("/", handleRoot);      // root page
  server.on("/data", handleData);  // suplies data to periodically update root (AJAX)
  server.on("/jpg", handleJPG);
  server.on("/jpeg", handleJpeg);  // show updating image
  server.on("/rgb", readRGBImage);                                 // demo converting image to RGB
  server.onNotFound(handleNotFound);  // invalid url requested

  // NTP - internet time
  if (serialDebug) Serial.println("\nGetting real time (NTP)");
  configTime(0, 0, ntpServer);
  setenv("TZ", TZ_INFO, 1);
  if (getNTPtime(10)) {  // wait up to 10 sec to sync
  } else {
    if (serialDebug) Serial.println("Time not set");
  }
  lastNTPtime = time(&now);

  // set up camera
  if (serialDebug) Serial.print(("\nInitialising camera: "));
  if (initialiseCamera()) {
    if (serialDebug) Serial.println("OK");
  } else {
    if (serialDebug) Serial.println("failed");
  }

  // define i/o pins
  pinMode(indicatorLED, OUTPUT);     // defined again as sd card config can reset it
  digitalWrite(indicatorLED, HIGH);  // led off = High
  pinMode(iopinA, INPUT);            // pin 13 - free io pin, can be used for input or output
  pinMode(iopinB, OUTPUT);           // pin 12 - free io pin, can be used for input or output (must not be high at boot)

  setupFlashPWM();  // configure PWM for the illumination LED

  // startup complete
  if (serialDebug) Serial.println("\nStarted...");
  flashLED(2);    // flash the onboard indicator led
  brightLed(64);  // change bright LED
  delay(200);
  brightLed(0);  // change bright LED
}


void loop() {
  server.handleClient();

  // flash status LED to show sketch is running ok
  if ((unsigned long)(millis() - lastStatus) >= timeBetweenStatus) {
    lastStatus = millis();                                   // reset timer
    digitalWrite(indicatorLED, !digitalRead(indicatorLED));  // flip indicator led status
  }
}

bool initialiseCamera() {

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;        // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  config.pixel_format = PIXFORMAT;       // Options =  YUV422, GRAYSCALE, RGB565, JPEG, RGB888
  config.frame_size = FRAME_SIZE_IMAGE;  // Image sizes: 160x120 (QQVGA), 128x160 (QQVGA2), 176x144 (QCIF), 240x176 (HQVGA), 320x240 (QVGA),
                                         //              400x296 (CIF), 640x480 (VGA, default), 800x600 (SVGA), 1024x768 (XGA), 1280x1024 (SXGA),
                                         //              1600x1200 (UXGA)
  config.jpeg_quality = 12;              // 0-63 lower number means higher quality (can cause failed image capture if set too low at higher resolutions)
  config.fb_count = 1;                   // if more than one, i2s runs in continuous mode. Use only with JPEG

  // check the esp32cam board has a psram chip installed (extra memory used for storing captured images)
  //    Note: if not using "AI thinker esp32 cam" in the Arduino IDE, SPIFFS must be enabled
  if (!psramFound()) {
    if (serialDebug) Serial.println("Warning: No PSRam found so defaulting to image size 'CIF'");
    config.frame_size = FRAMESIZE_CIF;
  }

  esp_err_t camerr = esp_camera_init(&config);  // initialise the camera
  if (camerr != ESP_OK) {
    if (serialDebug) Serial.printf("ERROR: Camera init failed with error 0x%x", camerr);
  }

  cameraImageSettings();  // apply custom camera settings

  return (camerr == ESP_OK);  // return boolean result of camera initialisation
}

bool cameraImageSettings() {

  if (serialDebug) Serial.println("Applying camera settings");

  sensor_t *s = esp_camera_sensor_get();

  if (s == NULL) {
    if (serialDebug) Serial.println("Error: problem reading camera sensor settings");
    return 0;
  }

  // if both set to zero enable auto adjust
  if (cameraImageExposure == 0 && cameraImageGain == 0) {
    // enable auto adjust
    s->set_gain_ctrl(s, 1);                       // auto gain on
    s->set_exposure_ctrl(s, 1);                   // auto exposure on
    s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1)
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
  } else {
    // Apply manual settings
    s->set_gain_ctrl(s, 0);                       // auto gain off
    s->set_awb_gain(s, 1);                        // Auto White Balance enable (0 or 1)
    s->set_exposure_ctrl(s, 0);                   // auto exposure off
    s->set_brightness(s, cameraImageBrightness);  // (-2 to 2) - set brightness
    s->set_agc_gain(s, cameraImageGain);          // set gain manually (0 - 30)
    s->set_aec_value(s, cameraImageExposure);     // set exposure manually  (0-1200)
  }

  return 1;
}

void resetCamera(bool type = 0) {
  if (type == 1) {
    // power cycle the camera module (handy if camera stops responding)
    digitalWrite(PWDN_GPIO_NUM, HIGH);  // turn power off to camera module
    delay(300);
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(300);
    initialiseCamera();
  } else {
    // reset via software
    esp_camera_deinit();
    delay(50);
    initialiseCamera();
  }
}

void changeResolution(framesize_t tRes = FRAMESIZE_96X96) {
  esp_camera_deinit();  // disable camera
  delay(50);
  if (tRes == FRAMESIZE_96X96) {  // taken as none supplied so cycle through several
    if (FRAME_SIZE_IMAGE == FRAMESIZE_QVGA) tRes = FRAMESIZE_VGA;
    else if (FRAME_SIZE_IMAGE == FRAMESIZE_VGA) tRes = FRAMESIZE_SVGA;
    else if (FRAME_SIZE_IMAGE == FRAMESIZE_SVGA) tRes = FRAMESIZE_SXGA;
    else tRes = FRAMESIZE_QVGA;
  }
  FRAME_SIZE_IMAGE = tRes;
  initialiseCamera();
  if (serialDebug) Serial.println("Camera resolution changed to " + String(tRes));
  imageResDetails = "Unknown";  // set next time image captured
}

void handleReboot() {

  String message = "Rebooting....";
  server.send(200, "text/plain", message);  // send reply as plain text

  // rebooting
  delay(500);  // give time to send the above html
  ESP.restart();
  delay(5000);  // restart fails without this delay
}
