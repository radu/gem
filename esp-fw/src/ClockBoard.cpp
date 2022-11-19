/*******************************************************************
    Allows YouTube live stream chat to draw on a 64x64 LED matrix.

    - "!colour x y" is the draw command
    - supported colour: red, green, blue, black

    For use with my I2S Matrix Shield.

    Parts:
    ESP32 Mini Kit (ESP32 D1 Mini) * - https://s.click.aliexpress.com/e/_AYPehO (pick the CP2104 Drive version)
    ESP32 I2S Matrix Shield (From my Tindie) = https://www.tindie.com/products/brianlough/esp32-i2s-matrix-shield/

 *  * = Affiliate

    If you find what I do useful and would like to support me,
    please consider becoming a sponsor on Github
    https://github.com/sponsors/witnessmenow/


    Written by Brian Lough
    YouTube: https://www.youtube.com/brianlough
    Tindie: https://www.tindie.com/stores/brianlough/
    Twitter: https://twitter.com/witnessmenow
 *******************************************************************/

// ----------------------------
// Standard Libraries
// ----------------------------

#define ESP_DRD_USE_SPIFFS      true

// #include <WiFi.h>
// #include <WiFiClientSecure.h>

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <WiFiManager.h>

// Captive portal for configuring the WiFi

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#define ARDUINOJSON_DECODE_UNICODE 1

#include <ArduinoJson.h>
// Library used for parsing Json from the API responses

// Search for "Arduino Json" in the Arduino Library manager
// https://github.com/bblanchon/ArduinoJson

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA

#include <base64.hpp>

// You need 1 API key per roughly 2 hours of chat you plan to monitor 
// So you can pass in just one:

#define GEMS_TOKEN "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD"

#include <ArduinoWebsockets.h>
using namespace websockets;

// Or you can pass in an array of keys, 2 keys gives 4 hours, 3 == 6 etc (See Github readme for info)

//#define NUM_API_KEYS 2
//const char *keys[NUM_API_KEYS] = {"AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCDDDDDDDDDDD", "AAAAAAAAAABBBBBBBBBBBCCCCCCCCCCCEEEEEEEEEE"};


//#define CHANNEL_ID "UC8rQKO2XhPnvhnyV1eALa6g" //Bitluni's trash
//#define CHANNEL_ID "UCSJ4gkVC6NrvII8umztf0Ow" //Lo-fi beats (basically always live)
//#define CHANNEL_ID "UCu94OHbBYVUXLYPh4NBu10w" // Unexpected Maker

#define HOST_LABEL      "ws_host"
#define REFRESH_LABEL   "refresh_ms"
#define TOKEN_LABEL     "token"
#define MATRIX_64_LABEL "is64"

#define LED_PIN LED_BUILTIN

#define HOST_LEN 100

char ws_host[HOST_LEN] = "ws://192.168.1.112:4000/ws/token/3000/websocket";
int refresh_ms = 1000;

//------- ---------------------- ------

// -------------------------------------
// -------   Matrix Config   ------
// -------------------------------------

const int panelResX = 64;   // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 32;   // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1;  // Total number of panels chained one to another.

int y_offset = panelResY / 2;

MatrixPanel_I2S_DMA *dma_display = nullptr;


// A library for checking if the reset button has been pressed twice
// Can be used to enable config mode
// Can be installed from the library manager (Search for "ESP_DoubleResetDetector")
//https://github.com/khoih-prog/ESP_DoubleResetDetector


// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#include <ESP_DoubleResetDetector.h>
DoubleResetDetector* drd;

#define GEM_CONFIG_JSON "/gem_config.json"

bool matrixIs64 = false;

//flag for saving data
bool shouldSaveConfig = false;

uint16_t myBLACK = dma_display->color565(0, 0, 0);

uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(255, 0, 0);
uint16_t myGREEN = dma_display->color565(0, 255, 0);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

// WiFiClientSecure client;

// Wifi network station credentials
char ssid[] = "SSID";     // your network SSID (name)
char password[] = "password"; // your network key

unsigned long requestDueTime;               //time when request due
unsigned long delayBetweenRequests = 5000; // Time between requests (5 seconds)

unsigned long ledBlinkDueTime;               //time when blink is due
unsigned long delayBetweenBlinks = 1000; // Time between blinks (1 seconds)

bool haveVideoId = false;
bool haveLiveChatId = false;

bool ledState = false;
bool isBlinking = false;

#define PANEL_MSG_LENGTH 64 * 64 * 4 * 3 * 8 / 7 
unsigned char lastMessageReceived[PANEL_MSG_LENGTH];

#define PANEL_DATA_LENGTH 64*64*4
unsigned char panelData[PANEL_DATA_LENGTH];

WebsocketsClient ws;

void onMessageCallback(WebsocketsMessage message) {
  Serial.print("Got Message: ");
  Serial.println(message.rawData().length());

  const unsigned char* msg = (const unsigned char*)message.c_str();
  int length = message.rawData().length();

  for(int i = 0 ; i < length; i+=2 ) {
    uint16_t* color = (uint16_t*)(&msg[i]);
     
    unsigned int pix = i / 2;
    unsigned char y = panelResY - pix / panelResX - 1;
    unsigned char x = pix % panelResX;
    
    dma_display->drawPixel(x,y,*color);
  }

  ws.ping();
  requestDueTime = millis() + delayBetweenRequests;
}

void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened) {
        Serial.println("Connnection Opened");
        dma_display->drawPixel(1,0, myGREEN);
    } else if(event == WebsocketsEvent::ConnectionClosed) {
        Serial.println("Connnection Closed");
        dma_display->drawPixel(1,0, myRED);
    } else if(event == WebsocketsEvent::GotPing) {
        Serial.println("Got a Ping!");
    } else if(event == WebsocketsEvent::GotPong) {
        Serial.println("Got a Pong!");
    }
}

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered Conf Mode");
  dma_display->fillScreen(myBLACK);
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setTextWrap(false);
  dma_display->setTextColor(myBLUE);
  dma_display->setCursor(0, 0);
  dma_display->print(myWiFiManager->getConfigPortalSSID());

  dma_display->setTextWrap(true);
  dma_display->setTextColor(myRED);
  dma_display->setCursor(0, 8);
  dma_display->print(WiFi.softAPIP());

  dma_display->flipDMABuffer();
}

void saveConfigFile() {
  Serial.println(F("Saving config"));
  StaticJsonDocument<512> json;
  json[HOST_LABEL] = ws_host;
  json[REFRESH_LABEL] = refresh_ms;

  JsonObject matrixJson = json.createNestedObject("matrix");
  matrixJson[MATRIX_64_LABEL] = matrixIs64;

  File configFile = SPIFFS.open(GEM_CONFIG_JSON, "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

bool setupSpiffs() {
  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists(GEM_CONFIG_JSON)) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(GEM_CONFIG_JSON, "r");
      if (configFile) {
        Serial.println("opened config file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error) {
          Serial.println("\nparsed json");

          strcpy(ws_host, json[HOST_LABEL]);
          refresh_ms = json[REFRESH_LABEL].as<int>();

          matrixIs64 = json["matrix"][MATRIX_64_LABEL].as<bool>();
          return true;

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  return false;
}


void setup() {
  Serial.begin(115200);
  bool forceConfig = false;
  Serial.println(F("Starting..."));

  drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);

  if (drd->detectDoubleReset()) {
    Serial.println(F("Forcing config mode as there was a Double reset detected"));
    forceConfig = true;
  }

  bool spiffsSetup = setupSpiffs();
  if (!spiffsSetup) {
    Serial.println(F("Forcing config mode as there is no saved config"));
    forceConfig = true;
  }

  HUB75_I2S_CFG mxconfig(
    panelResX,   // Module width
    panelResY,   // Module height
    panel_chain  // Chain length
  );

  if (matrixIs64) {
    mxconfig.mx_height = 64;
    y_offset = 32;
  }

  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(100); //0-255
  dma_display->clearScreen();
  dma_display->fillScreen(myBLACK);     
  

  if (!forceConfig) {
    dma_display->print("Booting");
  } else {
    dma_display->print("Conf Mode");
  }
  dma_display->flipDMABuffer();


  WiFiManager wm;
  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);


  WiFiManagerParameter custom_host(HOST_LABEL, "Websocket Host", ws_host, HOST_LEN);

  char checkBox[] = "type=\"checkbox\"";
  char checkBoxChecked[] = "type=\"checkbox\" checked";
  char* customHtml;

  char convertedValue[6];
  sprintf(convertedValue, "%d", refresh_ms);

  WiFiManagerParameter custom_time_delay(REFRESH_LABEL, "Refresh Time (ms)", convertedValue, 7);

  if (matrixIs64) {
    customHtml = checkBoxChecked;
  } else {
    customHtml = checkBox;
  }
  WiFiManagerParameter custom_is64(MATRIX_64_LABEL, "64x64 panel", "T", 2, "type=\"checkbox\"");

  //add all your parameters here
  wm.addParameter(&custom_host);
  wm.addParameter(&custom_time_delay);
  wm.addParameter(&custom_is64);

  if (forceConfig) {
    if (!wm.startConfigPortal("GEMSWiFi", "gems1234")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }
  } else {
    if (!wm.autoConnect("GEMSWiFi", "gems1234")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      // if we still have not connected restart and try all over again
      ESP.restart();
      delay(5000);
    }
  }

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {

    strncpy(ws_host, custom_host.getValue(), sizeof(ws_host));
    matrixIs64 = (strncmp(custom_is64.getValue(), "T", 1) == 0);
    refresh_ms = atoi(custom_time_delay.getValue());

    saveConfigFile();
    drd->stop();
    ESP.restart();
    delay(5000);
  }


  dma_display->setTextSize(1);
  dma_display->setTextColor(myGREEN);
  dma_display->setCursor(0, 8);
  dma_display->print("Booting");

  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);

  // Set WiFi to 'station' mode and disconnect
  // from the AP if it was previously connected
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  delay(100);

  dma_display->fillScreen(myBLACK);     
  
  dma_display->setTextSize(1);
  dma_display->setTextColor(myGREEN);
  dma_display->setCursor(0, 8);
  dma_display->print("Wifi...");

  // Connect to the WiFi network
  Serial.print("\nConnecting to WiFi: ");
  Serial.println(ssid);

  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //  Serial.print(".");
  //  delay(500);
  //}

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  
  // IPAddress ip = WiFi.localIP();
  // Serial.println(ip);

  /*
  dma_display->fillScreen(myBLACK);     
  dma_display->setTextSize(1);
  dma_display->setTextColor(myGREEN);
  dma_display->setCursor(0, 0);
  dma_display->print("Connected: ");
  dma_display->setCursor(0, 4);
  dma_display->print(ip);
  */
  // client.setInsecure();

  dma_display->fillScreen(myBLACK);     
  dma_display->setTextSize(1);
  dma_display->setTextColor(myGREEN);
  dma_display->setCursor(0, 0);
  dma_display->print("Websocket");

  // Setup Callbacks
  ws.onMessage(onMessageCallback);
  ws.onEvent(onEventsCallback);
  
  ws.connect(ws_host);
  dma_display->setCursor(0, 4);
  dma_display->print("...");

  ws.poll();   
  requestDueTime = millis() + delayBetweenRequests;
}

void loop() {
  drd->loop();

  if (millis() > requestDueTime) {
    ws.connect(ws_host);
    ws.poll();
    requestDueTime = millis() + delayBetweenRequests;
    dma_display->drawPixel(0,0, myGREEN);
  } else {
    ws.poll();
    sleep(1);
  }
}
