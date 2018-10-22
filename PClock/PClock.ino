#include "time.h"
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#define ADAFRUIT_GFX_EXTRA 10
#include <PxMatrix.h>
#include <Fonts/TomThumb.h>

// Settings
#define logging 1
String Hostname = "PClock"; // The network name
String UPDSrvURL = "http://192.168.0.3:8080/espupdate/index.php"; // This is used to update the sketch over the wifi, see readme for info
bool Rotation = false; // This function will switch between modes every 20 seconds
int Brightness = 255; // The default brightness
// Date/Time Settings
const char* ntpServer = "pool.ntp.org"; // Should probably leave this alone and edit the settings below
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
// Weather settings - Uses Openweathermap API
String CityID = "";
String APIKey = "";
String Units = "Metric";
String TempU = "C";
// AutoBrightness Settings
bool AutoBright = true; // Connect a Photo Transistor/Photocell to the A0 pin to use this, See here: https://learn.adafruit.com/photocells/using-a-photocell
float LightLTimeout = 10000; // How long to wait for the light level to stay below the threshold
float LIGHT_THRESHOLD = 50; // This will vary depending on the resistor value used, I used an 100k
float AutoBrightBrightness = 30; // The brightness % to change to when auto brightness has been triggered
String AutoBrightMode = "BC"; // The mode to change to when auto brightness has been triggered
// End of Settings

String SWVer = "1.4";

ESP8266WebServer server(82);   //Web server object. Will be listening in port 82
// I used port 82 as its not the normal web port so makes it a tiny big more secure (from others on your network), obviously actual authentication would be better

#ifdef ESP32
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

#endif

#ifdef ESP8266

#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2

#endif
// Pins for LED MATRIX

PxMATRIX display(32, 16, P_LAT, P_OE, P_A, P_B, P_C, P_D);
//PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
//PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

uint16_t static CallIM[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0010 (16) pixels
0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0020 (32) pixels
0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000,   // 0x0030 (48) pixels
0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,   // 0x0040 (64) pixels
0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0050 (80) pixels
};

uint16_t static NotiIM[] = {
0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000,   // 0x0010 (16) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0020 (32) pixels
0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000,   // 0x0030 (48) pixels
0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF,   // 0x0040 (64) pixels
0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static ClearIM[] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0010 (16) pixels
  0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0xFFE0, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0xFFE0, 0xFFE0,   // 0x0020 (32) pixels
  0xFFE0, 0xFFE0, 0x0000, 0xFFE0, 0xFFE0, 0x0000, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0xFFE0, 0x0000, 0xFFE0, 0x0000, 0x0000, 0x0000,   // 0x0030 (48) pixels
  0xFFE0, 0xFFE0, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFE0, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0000, 0xFFE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static RainIM[] = {
  0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0010 (16) pixels
  0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0020 (32) pixels
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static ThunderstormIM[] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0xFFA0,   // 0x0010 (16) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0xFFA0,   // 0x0020 (32) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0xFFA0, 0xFFA0, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFA0, 0xFFA0, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static SnowIM[] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000,   // 0x0010 (16) pixels
  0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF,   // 0x0020 (32) pixels
  0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000,   // 0x0030 (48) pixels
  0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static DrizzleIM[] = {
  0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0010 (16) pixels
  0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000,   // 0x0020 (32) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0000, 0x07FF, 0x0000, 0x07FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static CloudsIM[] = {
  0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0010 (16) pixels
  0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x0020 (32) pixels
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF,   // 0x0030 (48) pixels
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

uint16_t static AtmosphereIM[] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0638, 0x0638, 0x0000, 0x0000, 0x0000, 0x0638,   // 0x0010 (16) pixels
  0x0638, 0x0000, 0x0638, 0x0000, 0x0000, 0x0638, 0x0638, 0x0638, 0x0000, 0x0000, 0x0638, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0020 (32) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0030 (48) pixels
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0638, 0x0638, 0x0000, 0x0000, 0x0000, 0x0638, 0x0638, 0x0000, 0x0638,   // 0x0040 (64) pixels
  0x0000, 0x0000, 0x0638, 0x0638, 0x0638, 0x0000, 0x0000, 0x0638, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0050 (80) pixels
};

String Status = "ON";
char* Hostname2 = &Hostname[0];
String Mode = "TDW";
String timeNOW;
String temperature;
String condition;
String humidity;
String windsp;
int sunset;
int sunrise;

unsigned long next_weather_update = 0;
unsigned long loopdelay = 0;
unsigned long rotationtm = 0;

enum state {
  OFF, COUNTDOWN, ON
};

state current_state;
unsigned long countdownMs;

#ifdef ESP8266
// ISR for display refresh
void display_updater()
{
  //  display.displayTestPixel(70);
  display.display(50);
}
#endif

#ifdef ESP32
void IRAM_ATTR display_updater() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  //isplay.display(70);
  display.displayTestPattern(70);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif


const char MAIN_page[] PROGMEM = R"=====(
  <!DOCTYPE html>
<html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
  background: #2c3e50ff;
}
.redt {
  color: red;
}
.center {
  text-align: center;
}
</style>
<title>PClock Control</title>
<body onload="BrightSL()">
<div class="center">

<h1 class="redt">PClock Control</h1>

<div class="redt" >
Auto Brightness: <button type="button" onclick="loadDoc('autobri', 'true')">On</button> <button type="button" onclick="loadDoc('autobri', 'false')">Off</button>
<br><br>
Brightness: <a id="sliderv"></a><br>
<input id="bright" type="range" min="1" max="255" value="128">

<button type="button" onclick="loadDoc('bright', document.getElementById('bright').value)">Submit</button>
<br><br>
<button type="button" onclick="loadDoc('chmode', '1')">Change Mode</button>
<br><br>
Display: <button type="button" onclick="loadDoc('status', 'ON')">On</button> <button type="button" onclick="loadDoc('status', 'OFF')">Off</button> 
<br><br><br>
<button type="button" onclick="loadDoc('updw', '1')">Update Weather</button>
<br><br><br>
<button type="button" onclick="loadDoc('rst', '1')">Restart Clock</button>

<p id="response"></p>
</div>
</div>
<script>
var slider = document.getElementById('bright');
slider.addEventListener('input', BrightSL);

function BrightSL() {
 document.getElementById("sliderv").innerHTML = document.getElementById("bright").value;
}

function loadDoc(type, value) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange=function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("response").innerHTML = "Response: " + this.responseText;
    }
  };
  xhttp.open("GET", "/api?" + type +  "=" + value, true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";

// Will eventually allow settings to be changed on the web interface
const char Settings_page[] PROGMEM = R"=====(
  <!DOCTYPE html>
<html>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
body {
  background: #2c3e50ff;
}
.redt {
  color: red;
}
.center {
  text-align: center;
}
</style>
<title>PClock Control - Settings</title>
<body>
<div class="center">
<h1 class="redt">PClock Control - Settings</h1>

<div class="redt" >This page is a work in progress.</div>

</div>
<script>
function loadDoc(type, value) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange=function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("response").innerHTML = "Response: " + this.responseText;
    }
  };
  xhttp.open("GET", "/api?" + type +  "=" + value, true);
  xhttp.send();
}
</script>

</body>
</html>
)=====";

void handleRoot() {
 //Serial.println("You called root page");
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void handleSettings() {
 //Serial.println("You called root page");
 String s = Settings_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}

void handleNotFound() {
 // digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  //digitalWrite(led, 0);
}

bool update_weather()
{

  HTTPClient http;  //Object of class HTTPClient
  http.begin("http://api.openweathermap.org/data/2.5/weather?id=" + CityID + "&appid=" + APIKey + "&units=" + Units);
  int httpCode = http.GET();
  //Check the returning code
  if (httpCode > 0) {
    // Parsing
    const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& root = jsonBuffer.parseObject(http.getString());
    // Parameters
    JsonObject& main = root["main"];
    float main_temp = main["temp"];
    int main_humidity = main["humidity"];
    float wind_speed = root["wind"]["speed"];
    JsonArray& weather = root["weather"];
    JsonObject& weather0 = weather[0];
    int weather0_id = weather0["id"];
    String weather0_main = weather0["main"];
    temperature = String(round(main_temp));
    condition = weather0_main;
    humidity = String(round(main_humidity));
    windsp = String(round(wind_speed));
    JsonObject& sys = root["sys"];
    sunrise = sys["sunrise"];
    sunset = sys["sunset"];
  #ifdef logging
    // Output to serial monitor
    Serial.println("----");
    Serial.println(timeNOW);
    Serial.print("Cond:");
    Serial.println(condition);
    Serial.print("temp:");
    Serial.println(temperature + TempU);
    Serial.print("Humidity:");
    Serial.println(humidity);
    Serial.print("winsp:");
    Serial.println(windsp);
  #endif
  }
  http.end();   //Close connection
}

/*
  %a Abbreviated weekday name
  %A Full weekday name
  %b Abbreviated month name
  %B Full month name
  %c Date and time representation for your locale
  %d Day of month as a decimal number (01-31)
  %H Hour in 24-hour format (00-23)
  %I Hour in 12-hour format (01-12)
  %j Day of year as decimal number (001-366)
  %m Month as decimal number (01-12)
  %M Minute as decimal number (00-59)
  %p Current locale's A.M./P.M. indicator for 12-hour clock
  %S Second as decimal number (00-59)
  %U Week of year as decimal number,  Sunday as first day of week (00-51)
  %w Weekday as decimal number (0-6; Sunday is 0)
  %W Week of year as decimal number, Monday as first day of week (00-51)
  %x Date representation for current locale
  %X Time representation for current locale
  %y Year without century, as decimal number (00-99)
  %Y Year with century, as decimal number
  %z %Z Time-zone name or abbreviation, (no characters if time zone is unknown)
  %% Percent sign
  You can include text literals (such as spaces and colons) to make a neater display or for padding between adjoining columns.
  You can suppress the display of leading zeroes  by using the "#" character  (%#d, %#H, %#I, %#j, %#m, %#M, %#S, %#U, %#w, %#W, %#y, %#Y)
*/

char buffer[80];
char buffer2[80];
char buffer3[80];
char buffer4[80];

String timestring;
String dateDstring;
String dateMstring;

void LocalTimeDate() // Gets the time and date and saves it to strings, used for logging too
{
  time_t rawtime;
  struct tm * timeinfo;
  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, 80, "%d/%m/%y %H:%M:%S", timeinfo);
  timeNOW = buffer;
  strftime (buffer2, 80, " %H:%M ", timeinfo);
  strftime (buffer3, 80, " %d", timeinfo);
  strftime (buffer4, 80, " %m", timeinfo);
  timestring = buffer2;
  timestring.trim();
  dateDstring = buffer3;
  dateDstring.trim();
  dateMstring = buffer4;
  dateMstring.trim();
}

void DisplayWeather()
{
  int imageHeight = 9;
  int imageWidth = 9;
  int counter = 0;
  int x = 2;
  int y = 0;
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    { 
      if (condition == "Clear") {
      display.drawPixel(xx + x , yy + y, ClearIM[counter]);
      } else if(condition == "Rain"){
        display.drawPixel(xx + x , yy + y, RainIM[counter]);
      } else if(condition == "Clouds"){
        display.drawPixel(xx + x , yy + y, CloudsIM[counter]);
      } else if(condition == "Drizzle"){
        display.drawPixel(xx + x , yy + y, DrizzleIM[counter]);
      } else if(condition == "Atmosphere" or condition == "Fog" or condition == "Mist" or condition == "Haze"){
        display.drawPixel(xx + x , yy + y, AtmosphereIM[counter]);
      } else if(condition == "Snow"){
        display.drawPixel(xx + x , yy + y, SnowIM[counter]);
      } else if(condition == "Thunderstorm"){
        display.drawPixel(xx + x , yy + y, ThunderstormIM[counter]);
      }
      counter++;
    }
  }

  display.setTextColor(myYELLOW);
  display.setCursor(1, 15);
  display.print(temperature + TempU);

  display.setTextColor(myGREEN);
  display.setCursor(15, 7);
  display.print(humidity + "%");

  display.setTextColor(myCYAN);
  display.setCursor(15, 15);
  display.print(windsp + "m");

}

void DisplayTimeDateWeather()
{
  display.setFont(&TomThumb);
  display.setTextColor(myWHITE);
  display.setCursor(15, 6);
  display.print(timestring);

  display.setTextColor(myYELLOW);
  display.setCursor(15, 15);
  display.print(dateDstring);

  // Draw date seperation line
  display.drawPixel(23, 9, myYELLOW);
  display.drawPixel(23, 10, myYELLOW);
  display.drawPixel(23, 11, myYELLOW);
  display.drawPixel(23, 12, myYELLOW);
  display.drawPixel(23, 13, myYELLOW);
  display.drawPixel(23, 14, myYELLOW);
  display.drawPixel(23, 15, myYELLOW);

  display.setTextColor(myYELLOW);
  display.setCursor(25, 15);
  display.print(dateMstring);

  //display.setTextColor(myCYAN);
  //display.setCursor(1,6);
  //display.print(condition);

  int imageHeight = 9;
  int imageWidth = 9;
  int counter = 0;
  int x = 2;
  int y = 0;
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    { 
      if (condition == "Clear") {
      display.drawPixel(xx + x , yy + y, ClearIM[counter]);
      } else if(condition == "Rain"){
        display.drawPixel(xx + x , yy + y, RainIM[counter]);
      } else if(condition == "Clouds"){
        display.drawPixel(xx + x , yy + y, CloudsIM[counter]);
      } else if(condition == "Drizzle"){
        display.drawPixel(xx + x , yy + y, DrizzleIM[counter]);
      } else if(condition == "Atmosphere" or condition == "Fog" or condition == "Mist" or condition == "Haze"){
        display.drawPixel(xx + x , yy + y, AtmosphereIM[counter]);
      } else if(condition == "Snow"){
        display.drawPixel(xx + x , yy + y, SnowIM[counter]);
      } else if(condition == "Thunderstorm"){
        display.drawPixel(xx + x , yy + y, ThunderstormIM[counter]);
      }
      counter++;
    }
  }

  display.setTextColor(myCYAN);
  display.setCursor(1, 15);
  display.print(temperature + TempU);

}

void DisplayBigClock()
{
  display.setFont();
  display.setTextColor(myWHITE);
  display.setCursor(1, 4);
  display.print(timestring);

//  display.setFont(&TomThumb);

//  display.setTextColor(myWHITE);
//  display.setCursor(15, 15);
//  display.print(dateDstring);

  // Draw date seperation line
//  display.drawPixel(23, 9, myWHITE);
//  display.drawPixel(23, 10, myWHITE);
//  display.drawPixel(23, 11, myWHITE);
//  display.drawPixel(23, 12, myWHITE);
//  display.drawPixel(23, 13, myWHITE);
//  display.drawPixel(23, 14, myWHITE);
//  display.drawPixel(23, 15, myWHITE);

//  display.setTextColor(myWHITE);
//  display.setCursor(25, 15);
//  display.print(dateMstring);

//  display.setTextColor(myWHITE);
//  display.setCursor(1, 15);
//  display.print(temperature + TempU);
}

void ChangeMode()
{
  if (Mode == "W")
  {
    Mode = "BC";
  } else if (Mode == "BC")
  {
    Mode = "TDW";
  } else if (Mode == "TDW")
  {
    Mode = "W";
  }
}

void ToggleRotation()
{
  if (Rotation == true)
  {
    Rotation = false;
  } else if (Rotation == false)
  {
    Rotation = true;
  }
}

void handleSpecificArg() { 

String message = "";

if (server.arg("chmode")== ""){     //Parameter not found

}else{     //Parameter found
ChangeMode();

message = "Mode Changed";
}
if (server.arg("updw")== ""){     //Parameter not found

}else{     //Parameter found
update_weather();

message = "Weather Updated";
}
if (server.arg("rst")== ""){     //Parameter not found

}else{     //Parameter found
message = "Restarting";
server.send(200, "text/plain", message);          //Returns the HTTP response
delay(2000);
ESP.restart();
}
if (server.arg("mode")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
Mode = server.arg("mode");

message = "Mode changed to ";
message += server.arg("mode");     //Gets the value of the query parameter
}
if (server.arg("status")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
Status = server.arg("status");

message = "Display turned ";
message += server.arg("status");     //Gets the value of the query parameter
}
if (server.arg("bright")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
Brightness = server.arg("bright").toInt();

message = "Brightness changed to ";
message += server.arg("bright");     //Gets the value of the query parameter
}
if (server.arg("vers")== ""){     //Parameter not found

//message = SWVer;     //Gets the value of the query parameter

}else{     //Parameter found
  message = SWVer;
}
if (server.arg("rotation")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
//Rotation = server.arg("rotation");
ToggleRotation();

message = "Rotation toggled ";
message += Rotation;     //Gets the value of the query parameter
}
if (server.arg("autobri")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
  if (server.arg("autobri") == "true")
    AutoBright = true;
  else if (server.arg("autobri") == "false")
    AutoBright = false;

message = "Auto Brightness state: ";
message += server.arg("autobri");     //Gets the value of the query parameter
}
if (server.arg("noti")== ""){     //Parameter not found

//message = "Mode Argument not found";

}else{     //Parameter found
  message = "Received";
  if (Status == "ON" && current_state != ON){
  if (server.arg("noti") == "other"){
    display.clearDisplay();
    display.setFont(&TomThumb);
    display.setTextColor(myWHITE);
    display.setCursor(15,9);
    display.print("noti");
    display.setCursor(2,15);
    display.print("received");
    
  int imageHeight = 9;
  int imageWidth = 9;
  int counter = 0;
  int x = 2;
  int y = 0;
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    {
    display.drawPixel(xx + x , yy + y, NotiIM[counter]);
    counter++;
    }
  }
    server.send(200, "text/plain", message);          //Returns the HTTP response
    delay(10000);
    }
  else if (server.arg("noti") == "call") {
    display.clearDisplay();
    display.setFont(&TomThumb);
    display.setTextColor(myWHITE);
    display.setCursor(15,9);
    display.print("call");
    display.setCursor(3,15);
    display.print("incoming");
    
  int imageHeight = 9;
  int imageWidth = 9;
  int counter = 0;
  int x = 2;
  int y = 0;
  for (int yy = 0; yy < imageHeight; yy++)
  {
    for (int xx = 0; xx < imageWidth; xx++)
    {
    display.drawPixel(xx + x , yy + y, CallIM[counter]);
    counter++;
    }
  }
    server.send(200, "text/plain", message);          //Returns the HTTP response
    delay(28000);
  }
  }
}


server.send(200, "text/plain", message);          //Returns the HTTP response
}

void setup()
{
  Serial.begin(9600);
  WiFi.hostname(Hostname2);
  WiFiManager wifiManager;
  wifiManager.autoConnect(Hostname2);

  String SWVerString = "SW VER: ";
  SWVerString += SWVer;
  Serial.println(SWVerString);

  t_httpUpdate_return ret = ESPhttpUpdate.update(UPDSrvURL, SWVer);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        Serial.println();
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
   }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  while (!time(nullptr))
  {
    Serial.print(".");
    delay(1000);
  }
  delay(1000);

  Serial.println("Time...");

  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.onNotFound(handleNotFound);
  server.on("/api", handleSpecificArg);   //Associate the handler function to the path

  server.begin();                                       //Start the server
  Serial.println("Server listening");   

  String IPString = WiFi.localIP().toString();
  IPString.remove(0, 8);
  String IPString2 = "IP: ";
  IPString2 += IPString;

  display.begin(4);
  display.setMuxPattern(STRAIGHT);
  display.setScanPattern(ZAGGIZ);
  display.clearDisplay();
  display.setFont(&TomThumb);
  display.setTextColor(myWHITE);
  display.setCursor(2,8);
  display.print(IPString2);
  
  display_ticker.attach(0.002, display_updater);
  //attachInterrupt(digitalPinToInterrupt(0),button_pressed,FALLING);
  //dimm=1;
  //yield();
  delay(3000);
}

boolean isLightAboveThreshhold() {
  //Serial.print(analogRead(A0));
  return analogRead(A0) > LIGHT_THRESHOLD;
}

void loop()
{
  server.handleClient();    //Handling of incoming requests

  if (millis() > loopdelay)
  {
    
  display.setBrightness(Brightness);
  
  LocalTimeDate();
  
  // Update weather data every 10 minutes
  if (millis() > next_weather_update)
  {
    update_weather();
    next_weather_update = millis() + 1800000; // Thirty Minutes
    //next_weather_update = millis() + 10000; // Ten Seconds
  }
  
if (AutoBright) {
 switch (current_state) {
    case OFF:
      if (isLightAboveThreshhold()) {
        // do nothing;
      }
      else {
        countdownMs = millis();
        current_state = COUNTDOWN;
      }
      break;

    case COUNTDOWN:
      if (isLightAboveThreshhold()) {
        // don't need to perform any action to stop counting down
        current_state = OFF;
      }
      else if (millis() - countdownMs > LightLTimeout) {
        Brightness = AutoBrightBrightness;
        Mode = AutoBrightMode;
        //Serial.print("dark");
        current_state = ON;
      }
      else {
        // do nothing
      }
      break;

    case ON:
      if (isLightAboveThreshhold()) {
        Brightness = 255;
        Mode = "TDW";
        //Serial.print("bright");
        current_state = OFF;
      }
      else {
        // do nothing
      }      break;
  } 
  }

  if (Rotation)
  {
    if (millis() > rotationtm)
  {
    ChangeMode();
    rotationtm = millis() + 20000; // Twenty Seconds
  }
  } else {
    
  }

if (Status == "ON"){
  if (Mode == "TDW")
  {
    display.clearDisplay();
    DisplayTimeDateWeather();
  } else if (Mode == "BC")
  {
    display.clearDisplay();
    DisplayBigClock();
  } else if (Mode == "W"){
    display.clearDisplay();
    DisplayWeather();
  } else {
    display.clearDisplay();
    display.setFont(&TomThumb);
    display.setTextColor(myWHITE);
    display.setCursor(1,8);
    display.print("No Mode");
  }
} else {
display.clearDisplay();
}
 loopdelay = millis() + 1000;
  }
}
