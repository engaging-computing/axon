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

#include "Axon.h"

using namespace ECG ;

// Note: functions in this file should appear in the same order
// as their declerations appear in their header.

void Axon::setLED(uint8_t LEDCode, bool state) {

    // Case: LEDCode is invalid
    // Tell user and then return to caller
    if (LEDCode != RED_LED && LEDCode != BLUE_LED) {
        Serial.printf("Attempted to toggle an invalid LED!\n") ;
        return ;
    }

    // Case: LEDCode is valid
    // Toggle relevant LED to desired state
    digitalWrite(LEDCode, state) ;
}

Axon::Axon() {

    // Begin "serial" (really USB) output at 115200 baud
    Serial.begin(115200) ;

    // Connect red and blue LEDs
    pinMode(RED_LED, OUTPUT) ;
    pinMode(BLUE_LED, OUTPUT) ;

    // Set both LEDSs off
    setLED(RED_LED, LED_OFF) ;
    setLED(BLUE_LED, LED_OFF) ;

    // Connect the servo arm
    _servo.attach(SERVO_PIN) ;

    // Set the servo to center (90 degrees)
    _servo.write(90) ;

    // Demonstrate to the user that components are functioning properly
    debugDance() ;

    // The device has not connecte to WiFi yet, so hasBegunWiFi should be false
    _hasBegunWiFi = false ;

    // Set the initial payload to an empty string
    _payload = "" ;

    // If the flag is toggled in Axon.h, enable the output of device debug information
    if (SHOW_WIFI_DIAGNISTICS) {
        Serial.setDebugOutput(true) ;
        WiFi.printDiag(Serial) ;
    }

    // If all the above are successful, the device is now in a valid state.
    // Next, it will attempt to conenct to WiFi
    // TODO actually validate that the above was successful
    _valid = true ;
}

bool Axon::isValid() {
    return _valid ;
}

bool Axon::isOnline() {

    // If the device has never attempted to connect to a newtwork, it is not connected
    if( _hasBegunWiFi == false ) return false ;

    // Otherwise, check that the WiFi status is WL_CONNECTED and that the device has a valid IP address
    return ( WiFi.status() == WL_CONNECTED ) && ( getLocalIP() != "0.0.0.0" ) ;
}

// Simple wrapper function to minimize library calls and redirect control flow through object framework
void Axon::sleep(uint32_t milliseconds) {
    delay(milliseconds) ;
}

/*
* Note on ESP8266 WiFi:
*
* ESP8266 WiFi documentation: http://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html
* For future readers of this code:
*   Debug output from the WiFi class is disabled by default, but it can be toggled using Serial.setDebugOutput(bool yea/nea)
*       This will cause the device to regularly output debug information to the serial port
*
*   Diagnostic information can be printed at any time using WiFi.printDiag(Serial)
*       Where Serial is the class name of the desired output (To the best of my knowledge, it will always be "Serial" in this context)
*/
bool Axon::connectToWiFi() {

    // Not sure if this case is neccessary..
    // Case device already connected to WiFi
    if ( isOnline() ) return true ;

    // Case first connection
    if ( _hasBegunWiFi == false) {
         WiFi.begin(Keys::WiFiSSID.c_str(), Keys::WiFiPassword.c_str()) ;
        _hasBegunWiFi = true ;

        // The device is not properly connected to the network unless the WiFi.status()
        // method returns WL_CONNECTED and the device also has a valid local IP address
        // Also, break out of the connection loop after thirty seconds and let the user
        // know there was an error connecting to WiFi. The counter variable i will be 
        // used for this purpose
        uint32_t i = 0 ;

        // This variable stores the number of half seconds (1000ms/2) that the device attempts
        // to connect to WiFi before timing out. The choice of units is due to the half second (500ms)
        // delay between the printing of '.'s
        uint32_t halfSecondsTimeout = 60 ;

        Serial.printf("Connecting to WiFi network %s ", Keys::WiFiSSID.c_str()) ;
        while ( isOnline() == false )
        {
            // An "progress bar" ticker that shows the user that the connection is in progress
            Serial.printf(".") ;
            // FIXME: remove this after testing
            //Serial.printf("%d<60",i) ;

            sleep(500) ;

            // If the following print statement is removed, the device will crash on the subsequent line
            // The above statment is no longer true for some reason...
            //Serial.printf(" ") ;

            if ( i >= halfSecondsTimeout ) {
                Serial.printf("\nUnable to connect to WiFi network %s. Time out after 30 seconds.\n"
                    "Switching to offline party mode...\n", Keys::WiFiSSID.c_str()) ;
                // This call will never return. It activates "party mode" (basically a screensaver without a screen)
                endlessDebugFlash() ;
            }

            // As our timer variable, i must be incremented at the end of each loop iteration
            i++ ;
        }
        // If the control flow makes it past the preceeding while loop, the device should be connected to WiFi.
        // Make a note of any cases in which control flow reaches this point without a successful WiFi connection

        setLED(NETWORK_LED, LED_ON) ;
        Serial.printf("\nSuccessfully connected to WiFi network %s.\nLocal IP address: %s.\n",
            Keys::WiFiSSID.c_str(), getLocalIP().c_str()) ;
        
        return true ;
    }
    // Case reconnection
    // Should anything else be done?
    
    // If the device is disconnected, the blue LED should switch off.
    if ( isOnline() == false ) {
        setLED(NETWORK_LED, LED_OFF) ;
    }
    else {
        setLED(NETWORK_LED, LED_ON) ;
    }

    // Return the truth value of whether the device is online or not
    return isOnline() ;
}

bool Axon::callAPI() {

    // First, we must establish a connection to iSENSE
    Serial.printf("Connecting to %s on port %d... \n", Config::iSENSEHost.c_str(), Config::iSENSEPort) ;

    if ( !_client.connect(Config::iSENSEHost, Config::iSENSEPort) ) {
        Serial.printf("Connection failed!\n") ;
        return false ;
    }

    // Now we verify the identity of the server by comparing SHA1 hash fingerprints
    /* FIXME: we cannot do this if we are not using https....
    if ( _client.verify(Config::iSENSEFingerprint_SHA1.c_str(), Config::iSENSEHost.c_str()) ) {
        Serial.printf("Server certificate matches locally stored certificate.\nVerification complete\n") ;
    }
    else {
        Serial.printf("Certificate mismatch! Is the host incorrect or is someone impersonating iSENSE?\n") ;
        return false ;
    }
    */

    // Next, we will construct the HTTP get request
    String getRequest = "GET " + Config::iSENSEAPIPath + Config::iSENSEAPIEndpoint + " HTTP/1.1\r\n" +
                        "Host: " + Config::iSENSEHost + "\r\n" +
                        "Connection: close\r\n\r\n" ;

    // Display request being sent for debug purposes if the option has been set in Axon.h
    if (SHOW_HTTP_HEADERS) {
        Serial.printf("Sending the following request:\n%s\n", getRequest.c_str() ) ;
    }
                    
    // Now, we send this to the server
    _client.print(getRequest) ;

    // The client now reads the response sent by the server

    //TODO: actually save instead of dumping

    // Stores each line read from server for each read from the socket
    String line ;

    // Stores the http response code of a given response in order to handle non-200 responses
    String responseCode ;

    // This variable tracks whether the current iteration is the first in order to handle the HTTP response code
    bool isFirstLine = true ;

    // Get the HTTP response from the server, but ignore lines until the end of the header (i.e. "\r\n")
    while (_client.connected()) {
        line = _client.readStringUntil('\n') ;

        // TODO: read the http response code
        // If this is the first line, the HTTP response code is on this line
        if (isFirstLine == true) {
            // It is always in the same place as well, how convienient to have such a standardized protocol at times like these
            
            // The index at which the response code begins is the size of the preceeding substring in the response
            const uint8_t indexOfResponseSubstring = String("HTTP/1.1 ").length() ;

            // An http response code is 3 digits long
            const uint8_t httpResponseCodeLength = 3 ;
            
            // So, the response code is the substring of the first line from its begining index until its begining index plus its length
            responseCode = line.substring(indexOfResponseSubstring, indexOfResponseSubstring + httpResponseCodeLength) ;

            // Take this opportunity to print some debug
            if (SHOW_HTTP_HEADERS) {
                Serial.printf("RESPONSE FOLLOWS\n") ;
            }
        }

        // As above, display headers if the following option has been set
        if (SHOW_HTTP_HEADERS) {
            Serial.printf("%s\n", line.c_str()) ;
        }

        // After a single itertion, this should be perpetually false
        isFirstLine = false ;

        // If the string of characters on a line before the newline is just an '\r', the header is over
        if (line == "\r") break ;
    }

    /*
        The retrieved JSON is preceeded and suceeded by strange three digit hex values and I do not know why yet
        Until then, I will just ignore the first line and save the second, the one with the actual JSON

    */

    // Case: Successful get
    if (responseCode == "200") {

        // Ignore first line
        _client.readStringUntil('\n') ;

        // Save second line
        _payload = _client.readStringUntil('\n') ;

        // Ignore the rest
        _client.readString() ;
        return true ;
    }
    // Case: Resource not found
    else
    if (responseCode == "404") {
        Serial.printf("API endpoint not found on ISENSE! Device config invalid.\n") ;
        _payload = "" ;
        _valid = false ;
        return false ;
    }
    // Case: Unkown error
    else {
        Serial.printf("An unknown error occured. This might not be local.\n") ;
        _payload = "" ;
        return false ;
    }
}

bool Axon::parseJson_manualFallback() {
    
    /*
    const String superTest = "This:is:a:string" ;
    const String subTest = "is" ;
    int indexTest = superTest.indexOf(subTest) ;
    Serial.printf("test index is:%d\n", indexTest) ;
    

    Serial.printf("Searching for key (%s) in data (%s)...\n",
        Config::targetKey.c_str(), _payload.c_str()) ;
        */
    
    uint16_t indexOfKey = _payload.indexOf(Config::targetKey) ;
    if ( indexOfKey == -1 ) {
        Serial.printf("could not find target key manually\n") ;
        return false ;
    }
    Serial.printf("index of key is:%d\n",indexOfKey) ;

    uint16_t lengthOfKey = Config::targetKey.length() ;

    uint16_t firstIndexOfData = indexOfKey + lengthOfKey + 2 ;

    String targetValue = "" ;

    uint8_t indexCounter = 0;
    while(_payload.charAt(firstIndexOfData + indexCounter) != ',') {
        Serial.printf("Scanning over this char: %c\n",_payload.charAt(firstIndexOfData + indexCounter)) ;
        targetValue += _payload.charAt(firstIndexOfData + indexCounter) ;
        indexCounter++ ;
        // if the counter goes past 20 there is probably an error so fail
        if ( indexCounter >= 20) {
            return false ;
        }
    }

    _targetValue = targetValue ;
    return true ;

 }

bool Axon::parseJson() {

    // If the option is toggled in Axon.h, show the payload to be parsed
    if (SHOW_PAYLOAD) {
        Serial.printf(
            "Parsing the following data: %s\n",
            _payload != ""
            ? _payload.c_str()
            : "(invalid or no data)"
        ) ;
    }

    // Case: invalid payload
    // Do nothing
    if (_payload == "") {
        return false ;
    }

    // Case: payload exists
    // We use the ArduinoJson library
    DynamicJsonBuffer jsonBuffer ;
    JsonObject& dataRoot = jsonBuffer.parseObject(_payload) ;

    // Check for parsing failure
    if (dataRoot.success() == false) {
        Serial.printf("There was an error parsing the retrieved JSON\n") ;

        // Try the manual and hastily written manual fallback before failing
        if ( parseJson_manualFallback() == false ) {
            Serial.printf("Manual parse failed!\n") ;
            return false ;
        }
        else {
            Serial.printf("Got value: %s\n", _targetValue.c_str()) ;
            return true ;
        }
    }
    // If there was no error, get the value corresponding to the key specified in config and save it
    // TODO: method to get nested values
    else {
        String temp = dataRoot[Config::targetKey] ;
        _targetValue = temp ;
        Serial.printf("Got value: %s\n", _targetValue.c_str()) ;
        return true ;
    }
}

bool Axon::updateDisplay() {

    // This function converts the distance of the retrieved value between the begining and the end of the specified range to an angle

    double doubleTargetValue = strtod(_targetValue.c_str(),nullptr) ;

    Serial.printf("display value of %lf between %lf and %lf\n", doubleTargetValue,
         Config::displayLowBound, Config::displayHighBound) ;

    // Case: retrieved value is below or at lower bound of display range
    if (doubleTargetValue <= Config::displayLowBound) {
        // Move servo to lowest possible position 
        moveServo(0) ;
    }
    else
    // Case: retrieved values is above or at higher bound of display range
    if (doubleTargetValue >= Config::displayHighBound) {
        // Move servo to highest possible position
        moveServo(180) ;
    }
    // Case: the retrieved value is in bounds
    else {
        // Calculate appropriate angle
        Serial.printf("divide %lf by %lf to get %lf.\n", doubleTargetValue,
            Config::displayHighBound - Config::displayLowBound,
            doubleTargetValue / ( Config::displayHighBound - Config::displayLowBound )) ;
        moveServo( (uint16_t) round( 180.0 * ( ( doubleTargetValue - Config::displayLowBound )/ ( Config::displayHighBound - Config::displayLowBound ) ) ) ) ;
    }
}

// TODO: carefully read arduino WiFi documentation
String Axon::getLocalIP() {

    // If the device has never connected to WiFi, it's IP address is invalid
    if ( _hasBegunWiFi == false ) {
        return "not connected" ;
    }

    // Otherwise, return the stringified IP address
    return WiFi.localIP().toString() ;
}

void Axon::endlessDebugFlash() {

   // This loop never ends
   for(;;) {
       // Red on, blue off
       setLED(RED_LED,LED_ON) ;
       setLED(BLUE_LED,LED_OFF) ;
       sleep(500) ;

       // Blue on, red off
       setLED(RED_LED,LED_OFF) ;
       setLED(BLUE_LED,LED_ON) ;
       sleep(500) ;
   }

   // This function never returns to the caller
}

// I think there should be defined constants speed. Currently, this is TODO
void Axon::debugDance(uint16_t speed) {

    // TODO: speed adjustments
    // For now, speed will always be 1 (and hence a meaningless parameter)
    // but this can easily be tweaked in the future
    speed = 1 ;

    // Let the dance begin

    // Alternate blink twice to start
    for(uint8_t i = 0; i < 2; i++) {
        // Quick red blink
        setLED(RED_LED,LED_ON) ;
        sleep(200 * speed) ;
        setLED(RED_LED,LED_OFF) ;
        sleep(200 * speed) ;

        // Quick blue blink
        setLED(BLUE_LED,LED_ON) ;
        sleep(200 * speed) ;
        setLED(BLUE_LED,LED_OFF) ;
        sleep(200 * speed) ;
    }

    // Small extension of final sleep period
    sleep(200 * speed) ;

    // Triple blink of both LEDs
    for(uint8_t i = 0; i < 3; i++) {
        setLED(BLUE_LED,LED_ON) ;
        setLED(RED_LED,LED_ON) ;
        sleep(200 * speed) ;
        setLED(BLUE_LED,LED_OFF) ;
        setLED(RED_LED,LED_OFF) ;
        sleep(200 * speed) ;
    }

    // Quick servo twist to 0 and 180, then back to 90 (center positoon)
    moveServo(0,180 * speed) ;
    moveServo(180,180 * speed) ;
    moveServo(90,180 * speed) ;
    sleep(200 * speed) ;


    // Another Triple blink of both LEDs
    for(uint8_t i = 0; i < 3; i++) {
        setLED(BLUE_LED,LED_ON) ;
        setLED(RED_LED,LED_ON) ;
        sleep(200 * speed) ;
        setLED(BLUE_LED,LED_OFF) ;
        setLED(RED_LED,LED_OFF) ;
        sleep(200 * speed) ;
    }
}

// This global variabel is declared here because it is only relevant to the parameterless
// call of debugDance()
const uint16_t DANCE_SPEED_DEFAULT = 1 ;

void Axon::debugDance() {
    // Simply calls the (int) version of itself with default speed
    debugDance(DANCE_SPEED_DEFAULT) ;
}

// This global variable is declared here because it is only relevant to moveServo
const uint16_t DEFAULT_SERVO_SPEED = 90 ; // Degrees per second

void Axon::moveServo(uint16_t angle, uint16_t speed) {

    // This modding by 181 is to ensure that all values written to the servo
    // are between 0 and 180
    uint16_t fixedAngle = angle % 181 ;

    // If the speed argument is outside of the range 1 to 360, use the default value defined above
    if (speed > 360 || speed < 1) {
        speed = DEFAULT_SERVO_SPEED ;
    }

    // A second's worth of milliseconds 
    uint32_t msSecond = 1000 ;

    // Turn the speed parameter into a usable delay between angle increments
    // by dividing a second's worth of milliseconds by the integer speed parameter
    // using floating point division, effectively treating speed as 'degrees per second'
    uint32_t adjustedDelay = (uint32_t) ( round ((double) msSecond / speed )) ;

    Serial.printf("Begin move to fixed angle %d\n", fixedAngle) ;

    // Case: requested angle is equal to current angle
    if (fixedAngle == _servo.read()) {
        // TODO: should there be a delay?
        Serial.printf("No Write. Angle is %d\n", _servo.read()) ;
        return ;
    }
    // Case: Requested angle is smaller than current angle
    else
    if (fixedAngle > _servo.read()) {
        for (; fixedAngle > _servo.read() ; _servo.write(_servo.read() + 1)) {
            sleep(adjustedDelay) ;
        }
        return ;
    }
    // Case: Requested angle is larger than current angle
    else
    if (fixedAngle < _servo.read()) {
        for (; fixedAngle < _servo.read() ; _servo.write(_servo.read() - 1)) {
            sleep(adjustedDelay) ;
        }
        return ;
    }
    
    // TODO: refactor into one loop with exit condition bool as loop condition
    // coeffifient of (-1) or (1) to fix increment for different directions
}

void Axon::moveServo(uint16_t angle) {
    // This function simply calls the (int,int) version of itself with the default speed value
    moveServo(angle, DEFAULT_SERVO_SPEED) ;
}