extern String localTime();
extern void sendHeader(WiFiClient &client, char *hTitle);
extern void sendFooter(WiFiClient &client);
extern void sendText(WiFiClient &client, String theText);
extern void handleRoot();
extern void handleData();
extern void handleNotFound();
extern void readRGBImage();
extern bool handleJPG();
extern int requestWebPage(String *page, String *received, int maxWaitTime = 5000);
extern void handleJpeg();