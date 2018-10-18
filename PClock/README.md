# PClock Readme - WIP

##### Libraries Used
* [PxMatrix](https://github.com/2dom/PxMatrix)
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [ArduinoJson](https://arduinojson.org/)

## Setup

See the PxMatrix Readme on how to wire the panel up and you might need to change some settings in the setup() to make it work correctly.

Edit the settings in the .ino file.

You will need an [OpenWeatherMap](https://openweathermap.org) API Key.

## OTA Update

The sketch can be updated over WiFi

1. Run a web server and put the espupdate folder on it and edit the index.php with your ESP's MAC Address
2. Set the URL in the settings
3. Export your compiled binaries from the Arduino IDE and put them on the web server in the bin folder with the name pclock-1.0.ino (the 1.0 being a version number higher than the current one on the ESP, Remember to change the version number in the sketch too, otherwise it will go into a update loop.)