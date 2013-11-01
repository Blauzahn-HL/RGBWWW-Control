#include <SD.h>

// RGB LED Lighting System
// Dan Nixon 2013
// dan-nixon.com

//Includes
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <EEPROM.h>

#include "rgb.h"
#include "daylight.h"

File myFile;

//Pins
static const int RED_PIN = 5;
static const int GREEN_PIN = 6;
static const int BLUE_PIN = 7;

static const int WHITE_PIN = 9;
static const int WWHITE_PIN = 8;

static const int BUTTON_PIN = 12;

//Color Component Enum
static const int R = 0;
static const int G = 1;
static const int B = 2;

static const int W = 3;
static const int WW = 4;


//Transition Mode Enum
static const int NO_EXEC = -1;
static const int INSTANT = 0;
static const int FADE_DIRECT = 1;
static const int FADE_BLACK = 2;

//Hardware Button Mode Enum
static const int LIGHT_OFF = 0;
static const int LIGHT_WEB = 1;
static const int LIGHT_ON = 2;

//Default Values
static const int DEFAULT_TRANSITION = FADE_DIRECT;
static const int DEFAULT_TIME = 200;

//Standard Light Modes
static int OFF[] = {0, 0, 0, 0, 0};
static int FULL_WHITE[] = {255, 255, 255, 255, 255};

//Webserver
static uint8_t MAC[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
static uint8_t IP[] = {172, 31, 142, 159};
//static uint8_t IP[] = {10, 24, 10, 200};
WebServer webserver("", 80);

//Variables
int currentColor[5];
int lastWebColor[5];
int buttonLast;
int lightMode;
int lastUsedTransition = DEFAULT_TRANSITION;
int lastUsedTime = DEFAULT_TIME;




  
//
//void image(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete){
//  loadImage(server, "logo.png");
//}
//
//
//void loadImage(WebServer &server, char* image){
//  myFile = SD.open(image, FILE_READ);
//  int16_t c;
//  while ((c = myFile.read()) >= 0) {
//    server.print((char)c);
//  }
//  myFile.close();
//}




//Serves web front end to control light from a web browser
void webUI(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type == WebServer::GET) {
    server.printP(rgbHTML);
  }
}

//Provides back end to control lights from front end and other apps
void webBackend(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  char name[16];
  char value[16];
  server.httpSuccess();
  int transition = NO_EXEC;
  if(type == WebServer::POST) {
    int color[5];
    int time = DEFAULT_TIME;
    while (server.readPOSTparam(name, 16, value, 16)) {
      if(strcmp(name, "r") == 0) color[R] = atoi(value);
      if(strcmp(name, "g") == 0) color[G] = atoi(value);
      if(strcmp(name, "b") == 0) color[B] = atoi(value);
      
      if(strcmp(name, "w") == 0) color[W] = atoi(value);
      if(strcmp(name, "ww") == 0) color[WW] = atoi(value);
      
      if(strcmp(name, "trans") == 0) transition = atoi(value);
      if(strcmp(name, "time") == 0) time = atoi(value);
    }
    if(transition != NO_EXEC) {
      lightMode = LIGHT_WEB;
      
      Serial.println (lastWebColor[R]);
      lastWebColor[R] = color[R];
      lastWebColor[G] = color[G];
      lastWebColor[B] = color[B];
      
      lastWebColor[W] = color[W];
      lastWebColor[WW] = color[WW];
      
      lightChange(color, transition, time);
    }
  }
    if((type == WebServer::POST) || (type == WebServer::GET)) {
    server.println("<xml>");
    server.println("<currentColor>");
    server.print("<r>");
    server.print(currentColor[R]);
    server.println("</r>");
    server.print("<g>");
    server.print(currentColor[G]);   
    server.println("</g>");
    server.print("<b>");
    server.print(currentColor[B]);
    server.println("</b>");
    
    server.print("<w>");
    server.print(currentColor[W]);
    server.println("</w>");
    server.print("<ww>");
    server.print(currentColor[WW]);
    server.println("</ww>");  
    
    server.println("</currentColor>");
    server.print("<mode>");
    server.print(lightMode);
    server.println("</mode>");
    server.print("<lastTime>");
    server.print(lastUsedTime);
    server.println("</lastTime>");
    server.print("<lastTransition>");
    server.print(lastUsedTransition);
    server.println("</lastTransition>");
    server.print("<exec>");
    if(transition == NO_EXEC) server.print("FALSE");
    else server.print("TRUE");
    server.println("</exec>");
    server.println("</xml>");
  }
}



//############################################# Daylight-Control#######################
//Provides back end to control lights from front end and other apps
//Serves web front end to control light from a web browser
void webDaylight(WebServer &server, WebServer::ConnectionType type, char *, bool) {
  server.httpSuccess();
  if (type == WebServer::GET) {
    server.printP(daylightHTML);
  }
}


//Used to handle a change in lighting
void lightChange(int color[], int transition, int time) {
  if(transition == NO_EXEC) return;
  int oldColor[5];
  oldColor[R] = currentColor[R];
  oldColor[G] = currentColor[G];
  oldColor[B] = currentColor[B];
  
  oldColor[W] = currentColor[W];
  oldColor[WW] = currentColor[WW];
  switch(transition) {
    case INSTANT:
      setRGB(color);
      break;
    case FADE_DIRECT:
      fade(oldColor, color, time);
      break;
    case FADE_BLACK:
      int timeHalf = time / 2;
      fade(oldColor, OFF, timeHalf);
      fade(OFF, color, timeHalf);
      break;
  }
  lastUsedTime = time;
  lastUsedTransition = transition;
}

//Fading über Webseite
//Controls a smooth lighting fade
void fade(int startColor[], int endColor[], int fadeTime) {
  for(int t = 0; t < fadeTime; t++) {
    int color[5];
    color[R] = map(t, 0, fadeTime, startColor[R], endColor[R]);
    color[G] = map(t, 0, fadeTime, startColor[G], endColor[G]);
    color[B] = map(t, 0, fadeTime, startColor[B], endColor[B]);
    
    color[W] = map(t, 0, fadeTime, startColor[W], endColor[W]);
    color[WW] = map(t, 0, fadeTime, startColor[WW], endColor[WW]);
    setRGB(color);
    delay(1);
  }
  setRGB(endColor);
}

//Sets an RGB color
void setRGB(int color[3]) {
  if(color[R] < 0) color[R] = 0;
  if(color[R] > 255) color[R] = 255;
  if(color[G] < 0) color[G] = 0;
  if(color[G] > 255) color[G] = 255;
  if(color[B] < 0) color[B] = 0;
  if(color[B] > 255) color[B] = 255;

  if(color[W] < 0) color[W] = 0;
  if(color[W] > 255) color[W] = 255;
  if(color[WW] < 0) color[WW] = 0;
  if(color[WW] > 255) color[WW] = 255;
  

//  Serial.print("R: ");
//  Serial.print(color[R]);
//  Serial.print("   G: ");
//  Serial.print(color[G]);
//  Serial.print("   B: ");
//  Serial.print(color[B]);
//  Serial.print("   W: ");
//  Serial.print(color[W]);
//  Serial.print("   WW: ");
//  Serial.println(color[WW]);
  
  analogWrite(RED_PIN, color[R]);
  analogWrite(GREEN_PIN, color[G]);
  analogWrite(BLUE_PIN, color[B]);

  analogWrite(WHITE_PIN, color[W]);
  analogWrite(WWHITE_PIN, color[WW]);

  currentColor[R] = color[R];
  currentColor[G] = color[G];
  currentColor[B] = color[B];

  currentColor[W] = color[W];
  currentColor[WW] = color[WW];
}

void buttonHandler() {
  switch(lightMode) {
    case LIGHT_OFF:
      lightMode = LIGHT_WEB;
      lightChange(lastWebColor, lastUsedTransition, lastUsedTime);
      break;
    case LIGHT_WEB:
      lightMode = LIGHT_ON;
      lightChange(FULL_WHITE, lastUsedTransition, lastUsedTime);
      break;
    case LIGHT_ON:
      lightMode = LIGHT_OFF;
      lightChange(OFF, lastUsedTransition, lastUsedTime);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Server gestartet");

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, HIGH);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
 
  pinMode(WHITE_PIN, OUTPUT);
  pinMode(WWHITE_PIN, OUTPUT);
 
  Ethernet.begin(MAC, IP);
  webserver.setDefaultCommand(&webDaylight);
  
  webserver.addCommand("index", &webDaylight);

  webserver.addCommand("rgb", &webUI);
  webserver.addCommand("service", &webBackend);
  
  
 // webserver.addCommand("logo.png", &image);
  
  webserver.begin();
  buttonLast = digitalRead(BUTTON_PIN);
  lightMode = LIGHT_OFF;
}

void loop() {
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
  int buttonState = digitalRead(BUTTON_PIN);
  if(buttonState != buttonLast) {
    if(buttonState == LOW) {
      buttonHandler();
    }
    buttonLast = buttonState;
  }
}
