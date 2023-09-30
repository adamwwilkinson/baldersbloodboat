#include <HTTPClient.h>
#include "camera.h"
#include "camera_utils.h"

String localTime() {
  char ttime[40];
  if (!getLocalTime(&timeinfo)) return "Failed to obtain time";
  strftime(ttime, 40, "%A %B %d %Y %H:%M:%S", &timeinfo);
  return ttime;
}

void sendHeader(WiFiClient &client, char *hTitle) {
  // Start page
  client.write("HTTP/1.1 200 OK\r\n");
  client.write("Content-Type: text/html\r\n");
  client.write("Connection: close\r\n");
  client.write("\r\n");
  client.write("<!DOCTYPE HTML><html lang='en'>\n");
  // HTML / CSS
  client.printf(R"=====(
        <head>
          <meta name='viewport' content='width=device-width, initial-scale=1.0'>
          <title>%s</title>
          <style>
            body {
              color: black;
              background-color: #FFFF00;
              text-align: center;
            }
            input {
              background-color: #FF9900;
              border: 2px #FF9900;
              color: blue;
              padding: 3px 6px;
              text-align: center;
              text-decoration: none;
              display: inline-block;
              font-size: 16px;
              cursor: pointer;
              border-radius: 7px;
            }
            input:hover {
              background-color: #FF4400;
            }
          </style>
        </head>
        <body>
        <h1 style='color:red;'>%s</H1>
      )=====",
                hTitle, hTitle);
}

void sendFooter(WiFiClient &client) {
  client.write("</body></html>\n");
  delay(3);
  client.stop();
}

void sendText(WiFiClient &client, String theText) {
  if (!sendRGBfile) client.print(theText + "<br>\n");
  if (serialDebug || theText.indexOf("error") > 0) Serial.println(theText);  // if text contains "error"
}

void handleRoot() {
  getNTPtime(2);                        // refresh current time from NTP server
  WiFiClient client = server.client();  // open link with client

  // html header
  sendHeader(client, stitle);
  client.write("<FORM action='/' method='post'>\n");  // used by the buttons in the html (action = the web page to send it to

  int noLines = 5;  // number of text lines to be populated by javascript
  for (int i = 0; i < noLines; i++) {
    client.println("<span id='uline" + String(i) + "'></span><br>");
  }

  client.printf(R"=====(
         <script>
            function getData() {
              var xhttp = new XMLHttpRequest();
              xhttp.onreadystatechange = function() {
              if (this.readyState == 4 && this.status == 200) {
                var receivedArr = this.responseText.split(',');
                for (let i = 0; i < receivedArr.length; i++) {
                  document.getElementById('uline' + i).innerHTML = receivedArr[i];
                }
              }
            };
            xhttp.open('GET', 'data', true);
            xhttp.send();}
            getData();
            setInterval(function() { getData(); }, %d);
         </script>
      )=====",
                dataRefresh * 1000);

  // Control buttons
  client.write("<br><br>");
  // client.write("<input style='height: 35px;' name='button1' value='Toggle pin 12' type='submit'> \n");
  // client.write("<input style='height: 35px;' name='button2' value='Cycle illumination LED' type='submit'> \n");
  // client.write("<input style='height: 35px;' name='button3' value='Toggle Flash' type='submit'> \n");
  // client.write("<input style='height: 35px;' name='button4' value='Wipe SPIFFS memory' type='submit'> \n");
  // client.write("<input style='height: 35px;' name='button5' value='Change Resolution' type='submit'><br> \n");

  // Image setting controls
  client.println("<br>CAMERA SETTINGS: ");
  client.printf("Brightness: <input type='number' style='width: 50px' name='bright' title='from -2 to +2' min='-2' max='2' value='%d'>  \n", cameraImageBrightness);
  client.printf("Exposure: <input type='number' style='width: 50px' name='exp' title='from 0 to 1200' min='0' max='1200' value='%d'>  \n", cameraImageExposure);
  client.printf("Gain: <input type='number' style='width: 50px' name='gain' title='from 0 to 30' min='0' max='30' value='%d'>\n", cameraImageGain);
  client.println(" <input type='submit' name='submit' value='Submit change / Refresh Image'>");
  client.println("<br>Set exposure and gain to zero for auto adjust");

  // links to the other pages available
  client.write("<br><br>LINKS: \n");
  client.write("<a href='/photo'>Capture an image</a> - \n");
  client.write("<a href='/rgb'>Capture Image as raw RGB data</a> - \n");

  // capture and show a jpg image
  client.write("<br><a href='/jpg'>");                                           // make it a link
  client.write("<img id='image1' src='/jpg' width='320' height='240' /> </a>");  // show image from http://x.x.x.x/jpg

  // javascript to refresh the image periodically
  client.printf(R"=====(
         <script>
           function refreshImage(){
               var timestamp = new Date().getTime();
               var el = document.getElementById('image1');
               var queryString = '?t=' + timestamp;
               el.src = '/jpg' + queryString;
           }
           setInterval(function() { refreshImage(); }, %d);
         </script>
      )=====",
                imageRefresh * 1013);  // 1013 is just to stop it refreshing at the same time as /data

  client.println("<br><br><a href='https://github.com/alanesq/esp32cam-demo'>Sketch Info</a>");


  // --------------------------------------------------------------------


  sendFooter(client);  // close web page
}

void handleData() {
  String reply = "";

  reply += "Illumination led brightness=" + String(brightLEDbrightness);
  reply += " &ensp; Flash is ";  // Note: '&ensp;' leaves a gap
  reply += (flashRequired) ? "Enabled" : "Off";
  reply += ",";

  reply += "Current time: " + localTime();
  reply += ",";

  reply += "GPIO output pin 12 is: ";
  reply += (digitalRead(iopinB) == 1) ? "ON" : "OFF";
  reply += " &ensp; GPIO input pin 13 is: ";
  reply += (digitalRead(iopinA) == 1) ? "ON" : "OFF";
  reply += ",";

  reply += "Image size: " + imageResDetails;


  server.send(200, "text/plain", reply);  //Send millis value only to client ajax request
}

void handleNotFound() {
  String tReply;

  if (serialDebug) Serial.print("Invalid page requested");

  tReply = "File Not Found\n\n";
  tReply += "URI: ";
  tReply += server.uri();
  tReply += "\nMethod: ";
  tReply += (server.method() == HTTP_GET) ? "GET" : "POST";
  tReply += "\nArguments: ";
  tReply += server.args();
  tReply += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    tReply += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", tReply);
  tReply = "";  // clear variable
}

void readRGBImage() {
  // used for timing operations
  WiFiClient client = server.client();
  uint32_t tTimer;  // used to time tasks
  static float desired = 135 / 255.0;
  static float threshold = 10 / 255.0;

  // read query params
  // for (int i = 0; i < server.args(); i++) {
  //   if (server.argName(i) == "desired") {
  //     desired = server.arg(i).toFloat();
  //   } else if (server.argName(i) == "threshold") {
  //     threshold = server.arg(i).toFloat();
  //   }
  // }

  if (!sendRGBfile) {
    // html header
    sendHeader(client, "Show RGB data");

    // page title including clients IP
    IPAddress cIP = client.remoteIP();
    sendText(client, "Live image as rgb data, requested by " + cIP.toString());  // 'sendText' sends the String to both serial port and web page
  }

  // make sure psram is available
  if (!psramFound()) {
    sendText(client, "error: no psram available to store the RGB data");
    client.write("<br><a href='/'>Return</a>\n");  // link back
    if (!sendRGBfile) sendFooter(client);          // close web page
    return;
  }

  // capture a live image from camera (as a jpg)
  camera_fb_t *fb = NULL;
  tTimer = millis();  // store time that image capture started
  fb = esp_camera_fb_get();
  if (!fb) {
    sendText(client, "error: failed to capture image from camera");
    client.write("<br><a href='/'>Return</a>\n");  // link back
    if (!sendRGBfile) sendFooter(client);          // close web page
    return;
  } else {
    sendText(client, "JPG image capture took " + String(millis() - tTimer) + " milliseconds");  // report time it took to capture an image
    sendText(client, "Image resolution=" + String(fb->width) + "x" + String(fb->height));
    sendText(client, "Image size=" + String(fb->len) + " bytes");
    sendText(client, "Image format=" + String(fb->format));
    sendText(client, "Free memory=" + String(ESP.getFreeHeap()) + " bytes");
  }

  // allocate memory to store the rgb data (in psram, 3 bytes per pixel)
  sendText(client, "<br>Free psram before rgb data allocated = " + String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) + "K");
  void *ptrVal = NULL;                                 // create a pointer for memory location to store the data

  uint32_t IMAGE_WIDTH = fb->width;
  uint32_t IMAGE_HEIGHT = fb->height;
  uint32_t PIXEL_COUNT = IMAGE_WIDTH * IMAGE_HEIGHT;
  uint32_t ARRAY_LENGTH = PIXEL_COUNT * 3;  // calculate memory required to store the RGB data (i.e. number of pixels in the jpg image x 3)

  if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) < ARRAY_LENGTH) {
    sendText(client, "error: not enough free psram to store the rgb data");
    if (!sendRGBfile) {
      client.write("<br><a href='/'>Return</a>\n");  // link back
      sendFooter(client);                            // close web page
    }
    return;
  }
  ptrVal = heap_caps_malloc(ARRAY_LENGTH, MALLOC_CAP_SPIRAM);  // allocate memory space for the rgb data
  uint8_t *rgb = (uint8_t *)ptrVal;                            // create the 'rgb' array pointer to the allocated memory space
  sendText(client, "Free psram after rgb data allocated = " + String(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024) + "K");

  // convert the captured jpg image (fb) to rgb data (store in 'rgb' array)
  tTimer = millis();  // store time that image conversion process started
  bool jpeg_converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, rgb);
  if (!jpeg_converted) {
    sendText(client, "error: failed to convert image to RGB data");
    if (!sendRGBfile) {
      client.write("<br><a href='/'>Return</a>\n");  // link back
      sendFooter(client);                            // close web page
    }
    return;
  }
  sendText(client, "Conversion from jpg to RGB took " + String(millis() - tTimer) + " milliseconds");  // report how long the conversion took

  // if sendRGBfile is set then just send raw RGB data and close
  if (sendRGBfile) {
    client.write(rgb, ARRAY_LENGTH);  // send the raw rgb data
    esp_camera_fb_return(fb);         // camera frame buffer
    delay(3);
    client.stop();
    return;
  }

  // get hues
  bool mask[PIXEL_COUNT] = {0};
  imageToMask(mask, PIXEL_COUNT, rgb, ARRAY_LENGTH, desired, threshold);

  // get histogram
  uint8_t histogram[IMAGE_WIDTH] = {0};
  maskToHistogram(histogram, IMAGE_WIDTH, mask, PIXEL_COUNT);

  // get max
  int maxIndex = findMaxIndex(histogram, IMAGE_WIDTH);

  // get percentage from center
  float percentage = (float)(maxIndex - (IMAGE_WIDTH / 2)) / (float)IMAGE_WIDTH;

  sendText(client, "Desired hue: " + String(desired));
  sendText(client, "Threshold: " + String(threshold));
  sendText(client, "Max index (Detected x): " + String(maxIndex));

  client.write("<br><a href='/'>Return</a>\n");  // link back
  sendFooter(client);                            // close web page

  // finished with the data so free up the memory space used in psram
  esp_camera_fb_return(fb);  // camera frame buffer
  heap_caps_free(ptrVal);    // rgb data
}

bool handleJPG() {
  WiFiClient client = server.client();
  char buf[32];

  // capture the jpg image from camera
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    if (serialDebug) Serial.println("Error: failed to capture image");
    return 0;
  }

  // store image resolution info.
  imageResDetails = String(fb->width) + "x" + String(fb->height);

  // html to send a jpg
  const char HEADER[] = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n";
  const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
  const int hdrLen = strlen(HEADER);
  const int cntLen = strlen(CTNTTYPE);
  client.write(HEADER, hdrLen);
  client.write(CTNTTYPE, cntLen);
  sprintf(buf, "%d\r\n\r\n", fb->len);
  client.write(buf, strlen(buf));

  // send the captured jpg data
  client.write((char *)fb->buf, fb->len);

  // close client connection
  delay(3);
  client.stop();

  // return image frame so memory can be released
  esp_camera_fb_return(fb);

  return 1;
}

int requestWebPage(String *page, String *received, int maxWaitTime = 5000) {

  if (serialDebug) Serial.println("requesting web page: " + *page);

  WiFiClient client;
  HTTPClient http;
  http.setTimeout(maxWaitTime);
  http.begin(client, *page);
  int httpCode = http.GET();
  if (serialDebug) Serial.println("http code: " + String(httpCode));

  if (httpCode > 0) {
    *received = http.getString();
  } else {
    *received = "error:" + String(httpCode);
  }
  if (serialDebug) Serial.println(*received);

  http.end();  //Close connection
  if (serialDebug) Serial.println("Web connection closed");

  return httpCode;
}

void handleJpeg() {
  const int refreshRate = 2000;  // image refresh rate (ms)

  WiFiClient client = server.client();  // open link with client

  client.write("HTTP/1.1 200 OK\r\n");
  client.write("Content-Type: text/html\r\n");
  client.write("Connection: close\r\n");
  client.write("\r\n");
  client.write("<!DOCTYPE HTML><html lang='en'>\n");
  client.write("<head></head><body>");

  client.write("<FORM action='/' method='post'>\n");  // used by the buttons in the html (action = the web page to send it to

  // capture and show a jpg image
  client.write("<img id='image1' src='/jpg'/>");  // show image from http://x.x.x.x/jpg

  // javascript to refresh the image periodically
  client.printf(R"=====(
         <script>
           function refreshImage(){
               var timestamp = new Date().getTime();
               var el = document.getElementById('image1');
               var queryString = '?t=' + timestamp;
               el.src = '/jpg' + queryString;
           }
           setInterval(function() { refreshImage(); }, %d);
         </script>
      )=====",
                refreshRate);

  sendFooter(client);  // close web page
}
