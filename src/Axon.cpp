#include "Axon.h"

using namespace ECG ;

// Note: functions in this file should appear in the same order
// as their declerations appear in their header.

void Axon::setLED(int LEDCode, bool state) {

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

    // The deice has not connecte to WiFi yet, so hasBegunWiFi should be false
    _hasBegunWiFi = false ;

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
    return WiFi.status() == WL_CONNECTED;
}

// Simple wrapper function to minimize library calls and redirect control flow through object framework
void Axon::sleep(unsigned long milliseconds) {
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
    //if (isOnline()) return true ;

    // Case first connection
    if ( _hasBegunWiFi == false) {
        printf("WIFI BEGINSe: %d\n", WiFi.begin(Keys::WiFiSSID.c_str(), Keys::WiFiPassword.c_str())) ;
        _hasBegunWiFi = true ;

        // The device is not properly connected to the network unless the WiFi.status()
        // method returns WL_CONNECTED and the device also has a valid local IP address
        // Also, break out of the connection loop after thirty seconds and let the user
        // know there was an error connecting to WiFi. The counter variable i will be 
        // used for this purpose
        int i = 0 ;

        // This variable stores the number of half seconds (1000ms/2) that the device attempts
        // to connect to WiFi before timing out. The choice of units is due to the half second (500ms)
        // delay between the printing of '.'s
        int halfSecondsTimeout = 60 ;

        Serial.printf("Connecting to WiFi network %s ", Keys::WiFiSSID.c_str()) ;
        while ( WiFi.status() != WL_CONNECTED && getLocalIP() == "0.0.0.0" )
        {
            Serial.printf(".") ;
            // FIXME: remove this after testing
            Serial.printf("%d<60",i) ;

            sleep(500) ;

            // If the following print statement is removed, the device will crash on the subsequent line
            Serial.printf(" ") ;
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
    
    // TODO:
    // If the device is disconnected, the blue LED should switch off.

    // Return the truth value of whether the device is online or not
    return isOnline() ;
}

bool Axon::callAPI() {

    // First, we must establish a connection to iSENSE
    Serial.printf("Connecting to %s on port %d... \n", Config::iSENSEHost.c_str(), Config::httpsPort) ;
    if ( !_client.connect(Config::iSENSEHost, Config::httpsPort ) ) {
        Serial.printf("Connection failed!\n") ;
        return false ;
    }

    // Now we veryify the identity of the server by comparing SHA1 hash fingerprints
    if ( _client.verify(Config::iSENSEFingerprint_SHA1.c_str(), Config::iSENSEHost.c_str()) ) {
        Serial.printf("Server certificate matches locally stored certificate.\nVerification complete\n") ;
    }
    else {
        Serial.printf("Certificate mismatch! Is the host incorrect or is someone impersonating iSENSE?\n") ;
        return false ;
    }

    // Next, we will construct the HTTP get request
    String getRequest = "GET" + Config::iSENSEAPIPath + "HTTP/1.1\r\n" +
                        "Host: " + Config::iSENSEHost + "\r\n" +
                        "Connection: close\r\n\r\n" ;
                    
    // Now, we send this to the server
    _client.print(getRequest) ;

    // The client now reads the response sent by the server
    Serial.printf("RESPONSE FOLLOWS:\n") ;

    //TODO: actually save instead of dumping

    String line ;
    while (_client.connected()) {
        line = _client.readStringUntil('\n') ;
        Serial.printf("%s", line.c_str() ) ;
        if (line == "\r") break ;
    }

    line = _client.readStringUntil('\n') ;
    Serial.printf("%s", line.c_str() ) ;

    return true ;
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
void Axon::debugDance(int speed) {

    // TODO: speed adjustments
    // For now, speed will always be 1 (and hence a meaningless parameter)
    // but this can easily be tweaked in the future
    speed = 1 ;

    // Let the dance begin

    // Alternate blink twice to start
    for(int i = 0; i < 2; i++) {
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
    for(int i = 0; i < 3; i++) {
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
    for(int i = 0; i < 3; i++) {
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
const int DANCE_SPEED_DEFAULT = 1 ;

void Axon::debugDance() {
    // Simply calls the (int) version of itself with default speed
    debugDance(DANCE_SPEED_DEFAULT) ;
}

// This global variable is declared here because it is only relevant to moveServo
const int DEFAULT_SERVO_SPEED = 90 ; // Degrees per second

void Axon::moveServo(int angle, int speed) {

    // This modding by 181 is to ensure that all values written to the servo
    // are between 0 and 180
    int fixedAngle = angle % 181 ;

    // If the speed argument is outside of the range 1 to 360, use the default value defined above
    if (speed > 360 || speed < 1) {
        speed = DEFAULT_SERVO_SPEED ;
    }

    // A second's worth of milliseconds 
    unsigned long msSecond = 1000 ;

    // Turn the speed parameter into a usable delay between angle increments
    // by dividing a second's worth of milliseconds by the integer speed parameter
    // using floating point division, effectively treating speed as 'degrees per second'
    unsigned long adjustedDelay = (unsigned long) ( round ((double) msSecond / speed )) ;

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

void Axon::moveServo(int angle) {
    // This function simply calls the (int,int) version of itself with the default speed value
    moveServo(angle, DEFAULT_SERVO_SPEED) ;
}