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
  static ECG::Axon test;

  //Serial.printf("Is this a valid object? [%d]\n", test.isValid()) ;
  //test.endlessDebugFlash() ;

  // 90 Degrees per second
  // 2 seconds each way
  //test.moveServo_pub(180) ;
  //test.moveServo_pub(0) ;

  // Test debug dance
  //test.debugDance() ;

  //test.isOnline() ;
  test.connectToWiFi() ;
  Serial.printf("WiFi.status() == %d\n", WiFi.status()) ;
  Serial.printf("Local device IP: %s\n", test.getLocalIP().c_str()) ;

  test.callAPI() ;

  Serial.printf("End loop(). Sleeping 5 seconds...\n") ;

  //Serial.printf("\n\nxxx") ;
  test.sleep(5000) ;


}
