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

The sketch can be updated over WiFi by running a web server and setting the address in the settings and then exporting your compiled binaries from the IDE and putting them on the web server with the name pclock-1.0.ino (the 1.0 being a version number higher than the current one on the ESP, Remember to change the version number in the sketch too, otherwise it will go into a update loop.)