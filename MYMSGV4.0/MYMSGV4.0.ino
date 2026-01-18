#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define SDA_PIN 5
#define SCL_PIN 6
#define BLUE_LED 8  // built-in LED for ESP32-C3

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// WiFi AP
const char* ssid = "world_link2.4";
const char* password = "9706991774";

// Web Server
WebServer server(80);

//Display Modes
enum DisplayMode { FACE, MESSAGE };
DisplayMode mode = MESSAGE;

//Face
String currentFace = ""; // "Happy", "Sad", "Blink"

//Messages
#define MAX_LINES 20
String lines[MAX_LINES];
int lineCount = 0;

// Smooth scroll variables
int scrollY = 0;
const int lineHeight = 10;
const int maxLinesOnScreen = 4;
bool needScroll = false;
unsigned long lastScroll = 0;
unsigned long scrollInterval = 150; // default

//Blue LED Control
enum LEDMode { LED_OFF, LED_ON, LED_BLINK };
LEDMode ledMode = LED_OFF;
unsigned long lastBlink = 0;
bool ledState = false; // for blinking

//HTML Page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 OLED Chat</title>
<style>
body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #74ebd5, #ACB6E5); display: flex; flex-direction: column; align-items: center; justify-content: flex-start; height:100vh; margin:0; padding:0; color:#333;}
h2 { margin-top:40px; color:#fff; text-shadow:1px 1px 3px rgba(0,0,0,0.3);}
.container { background: rgba(255,255,255,0.9); padding:20px 30px; border-radius:15px; box-shadow:0 8px 20px rgba(0,0,0,0.2); text-align:center; width:90%; max-width:400px; margin-top:30px;}
input[type=text] { padding:10px; width:80%; font-size:16px; border-radius:8px; border:1px solid #ccc; margin-bottom:15px; outline:none; transition:0.2s;}
input[type=text]:focus { border-color:#74ebd5; box-shadow:0 0 5px rgba(116,235,213,0.5);}
button { padding:12px 20px; font-size:16px; margin:5px; border-radius:8px; border:none; cursor:pointer; transition:0.3s; color:#fff;}
button:hover { transform: translateY(-2px); box-shadow:0 4px 12px rgba(0,0,0,0.2);}
.send-btn { background-color:#74ebd5; color:#000;}
.send-btn:hover { background-color:#4fd1c5;}
.happy-btn { background-color:#FFD700; color:#000;}
.happy-btn:hover { background-color:#FFC107;}
.sad-btn { background-color:#1E90FF;}
.sad-btn:hover { background-color:#1C86EE;}
.blink-btn { background-color:#FF69B4;}
.blink-btn:hover { background-color:#FF1493;}
.led-btn { background-color:#00CED1;}
.led-btn:hover { background-color:#20B2AA;}
label { font-weight:bold;}
input[type=range] { width:80%;}
</style>
</head>
<body>
<h2>Hi Sir</h2>
<div class="container">
  <form action="/send" method="GET">
    <input type="text" name="msg" placeholder="Type your message">
    <br>
    <button class="send-btn" type="submit">Send</button>
  </form>
  <div style="margin-top:10px;">
    <a href="/button?type=Happy"><button class="happy-btn" type="button">😊 Happy</button></a>
    <a href="/button?type=Sad"><button class="sad-btn" type="button">😢 Sad</button></a>
    <a href="/button?type=Blink"><button class="blink-btn" type="button">😉 Blink</button></a>
  </div>
  <div style="margin-top:20px;">
    <label for="speed">Scroll Speed:</label>
    <input type="range" id="speed" name="speed" min="50" max="1000" step="50" value="150" oninput="updateSpeed(this.value)">
    <span id="speedValue">150</span> ms
  </div>
  <div style="margin-top:20px;">
    <label>Blue LED Control:</label><br>
    <a href="/led?mode=ON"><button class="led-btn" type="button">ON</button></a>
    <a href="/led?mode=OFF"><button class="led-btn" type="button">OFF</button></a>
    <a href="/led?mode=BLINK"><button class="led-btn" type="button">BLINK</button></a>
  </div>
</div>
<script>
function updateSpeed(val){
  document.getElementById('speedValue').innerText = val;
  fetch('/speed?value='+val);
}
</script>
</body>
</html>
)rawliteral";

//Helper Functions

void splitMessage(String msg){
  u8g2.setFont(u8g2_font_6x10_tr);
  int displayWidth = u8g2.getDisplayWidth();
  lineCount = 0;
  int start=0;
  while(start<msg.length() && lineCount<MAX_LINES){
    String line="";
    while(start<msg.length()){
      line+=msg[start];
      if(u8g2.getStrWidth(line.c_str())>displayWidth){
        line.remove(line.length()-1);
        break;
      }
      start++;
    }
    lines[lineCount++]=line;
  }
  scrollY=0;
  needScroll=(lineCount>maxLinesOnScreen);
}

void drawFace(String type){
  int cx=36, cy=20;
  u8g2.drawCircle(cx,cy,15,U8G2_DRAW_ALL);
  if(type=="Happy"){
    u8g2.drawCircle(cx-6,cy-5,2,U8G2_DRAW_ALL);
    u8g2.drawCircle(cx+6,cy-5,2,U8G2_DRAW_ALL);
    for(int i=0;i<=12;i++){
      float theta=map(i,0,12,-90,90)*3.14159/180.0;
      int dx=i;
      int dy=-int(sin(theta)*4);
      u8g2.drawPixel(cx-6+dx,cy+6+dy);
    }
  } else if(type=="Sad"){
    u8g2.drawCircle(cx-6,cy-5,2,U8G2_DRAW_ALL);
    u8g2.drawCircle(cx+6,cy-5,2,U8G2_DRAW_ALL);
    for(int i=0;i<=12;i++){
      float theta=map(i,0,12,-90,90)*3.14159/180.0;
      int dx=i;
      int dy=int(sin(theta)*4);
      u8g2.drawPixel(cx-6+dx,cy+8+dy);
    }
  } else if(type=="Blink"){
    u8g2.drawLine(cx-8,cy-5,cx-4,cy-5);
    u8g2.drawLine(cx+4,cy-5,cx+8,cy-5);
    u8g2.drawPixel(cx,cy+4);
  }
}

void drawMessages(){
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  int y=-scrollY+lineHeight;
  for(int i=0;i<lineCount;i++){
    u8g2.setCursor(0,y);
    u8g2.print(lines[i]);
    y+=lineHeight;
  }
  u8g2.sendBuffer();

  if(needScroll && millis()-lastScroll>=scrollInterval){
    lastScroll=millis();
    scrollY++;
    int maxScroll=(lineCount-maxLinesOnScreen)*lineHeight;
    if(scrollY>maxScroll) scrollY=0;
  }
}

void updateLED(){
  if(ledMode==LED_OFF) digitalWrite(BLUE_LED, HIGH);
  else if(ledMode==LED_ON) digitalWrite(BLUE_LED, LOW);
  else if(ledMode==LED_BLINK){
    if(millis()-lastBlink>=250){
      lastBlink=millis();
      ledState=!ledState;
      digitalWrite(BLUE_LED, ledState);
    }
  }
}

// Web Handlers
void handleRoot(){ server.send(200,"text/html",htmlPage); }
void handleSend(){ if(server.hasArg("msg")){ String msg=server.arg("msg"); if(msg.length()>0){ splitMessage(msg); mode=MESSAGE; currentFace=""; } } server.sendHeader("Location","/"); server.send(303); }
void handleButton(){ if(server.hasArg("type")){ currentFace=server.arg("type"); mode=FACE; lineCount=0; scrollY=0; } server.sendHeader("Location","/"); server.send(303); }
void handleSpeed(){ if(server.hasArg("value")){ scrollInterval=server.arg("value").toInt(); if(scrollInterval<50) scrollInterval=50; if(scrollInterval>1000) scrollInterval=1000; } server.send(200,"text/plain","OK"); }
void handleLED(){ 
  if(server.hasArg("mode")){
    String m = server.arg("mode");
    if(m=="ON") ledMode=LED_ON;
    else if(m=="OFF") ledMode=LED_OFF;
    else if(m=="BLINK") ledMode=LED_BLINK;
  }
  server.sendHeader("Location","/"); server.send(303);
}

//Setup
void setup(){
  pinMode(BLUE_LED, OUTPUT);
  Wire.begin(SDA_PIN,SCL_PIN);
  u8g2.begin();

  WiFi.softAP(ssid,password);
  Serial.print("Connect to IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/send", handleSend);
  server.on("/button", handleButton);
  server.on("/speed", handleSpeed);
  server.on("/led", handleLED);
  server.begin();
  Serial.println("Server started!");
}

//Loop
void loop(){
  server.handleClient();

  if(mode==FACE && currentFace!=""){ u8g2.clearBuffer(); drawFace(currentFace); u8g2.sendBuffer(); }
  else if(mode==MESSAGE && lineCount>0) drawMessages();

  updateLED();

  delay(50);
}
