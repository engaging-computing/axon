#include <Arduino.h>
#include <ESP8266HTTPClient.h>
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
  test.sleep(5000) ;


}
