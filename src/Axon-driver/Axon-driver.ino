/*
    Arms for iSENSE
    Copyright (C) 2018 Engaging Computing Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Arduino.h>
#include "Axon.h"

void setup() {
  // Nothing really neccesary here right now...
}

void loop() {
  // Initialize device
  static ECG::Axon device ;

  // Loop through a basic operational loop until the device ends up in an invalid state
  while (device.isValid()) {
    device.connectToWiFi() ;
    device.callAPI() ;
    device.parseJson() ;
    device.updateDisplay() ;
    device.sleep(5000) ;
  }

  // If the device is in an invalid state, make an obvious flashing pattern to alert the user
  // TODO: report where things went wrong. (If this is not already somewhat implemented
  //by various printf's at points of failure
  if (!device.isValid()) {
    Serial.printf("The device config is invalid. Please check config.h.\n"
      "Switching to offline party mode...\n") ;
      device.endlessDebugFlash() ;
  }
}
