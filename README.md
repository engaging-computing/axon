# Axon

This is a program for the Adafruit Feather Huzzah ESP8266 board.

It will allow the board to retrieve data using an API.

Currently, it is very much a work in progress.

The Axon object must be declared with the static storage class specifier at the top of the loop() function

To specify WiFi credentials, make a copy of Keys.h.template and rename it to Keys.h. Then, input your
WiFi credentials into the specified area.

Keys.h is not included in the repository to avoid pushing sensitive information to publicly accessible servers.

Dependencies:
    ArduinoJson v5.13.1 or later (included as submodule)

To download submodules, run:

```bash
git submodule init
git submodule update
```
