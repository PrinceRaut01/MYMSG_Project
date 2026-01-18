#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

#define SDA_PIN 5
#define SCL_PIN 6

U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// WiFi AP
const char* ssid = "ESP32_Book";
const char* password = "12345678";

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
int scrollLine = 0;
const int lineHeight = 10;
const int maxLinesOnScreen = 4; // 72x40 OLED, font 6x10
bool needScroll = false;

//HTML Page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<style>
body { font-family: Arial; text-align:center; background:#f0f0f0; margin-top:50px;}
h2 { color:#333; }
input[type=text] { padding:5px; width:150px; font-size:16px; margin-bottom:10px;}
button { padding:10px 20px; font-size:16px; margin:5px;}
</style>
</head>
<body>
<h2>ESP32 OLED Display</h2>
<form action="/send" method="GET">
  <input type="text" name="msg" placeholder="Type message"><br>
  <input type="submit" value="Send">
</form>
<br>
<a href="/button?type=Happy"><button>Happy</button></a>
<a href="/button?type=Sad"><button>Sad</button></a>
<a href="/button?type=Blink"><button>Blink</button></a>
</body>
</html>
)rawliteral";

//Helper Functions

// Split message into lines that fit screen width
void splitMessage(String msg) {
  u8g2.setFont(u8g2_font_6x10_tr);
  int displayWidth = u8g2.getDisplayWidth();
  lineCount = 0;

  int start = 0;
  while (start < msg.length() && lineCount < MAX_LINES) {
    String line = "";
    while (start < msg.length()) {
      line += msg[start];
      if (u8g2.getStrWidth(line.c_str()) > displayWidth) {
        line.remove(line.length() - 1);
        break;
      }
      start++;
    }
    lines[lineCount++] = line;
  }

  scrollLine = 0;
  needScroll = (lineCount > maxLinesOnScreen);
}

// Draw face in center
void drawFace(String type) {
  int cx = 36, cy = 20;
  u8g2.drawCircle(cx, cy, 15, U8G2_DRAW_ALL); // face outline

  if (type == "Happy") {
    u8g2.drawCircle(cx - 6, cy - 5, 2, U8G2_DRAW_ALL);
    u8g2.drawCircle(cx + 6, cy - 5, 2, U8G2_DRAW_ALL);
    // smile
    for (int i = 0; i <= 12; i++) {
      float theta = map(i, 0, 12, -90, 90) * 3.14159 / 180.0;
      int dx = i;
      int dy = -int(sin(theta) * 4);
      u8g2.drawPixel(cx - 6 + dx, cy + 6 + dy);
    }
  } 
  else if (type == "Sad") {
    u8g2.drawCircle(cx - 6, cy - 5, 2, U8G2_DRAW_ALL);
    u8g2.drawCircle(cx + 6, cy - 5, 2, U8G2_DRAW_ALL);
    // frown
    for (int i = 0; i <= 12; i++) {
      float theta = map(i, 0, 12, -90, 90) * 3.14159 / 180.0;
      int dx = i;
      int dy = int(sin(theta) * 4);
      u8g2.drawPixel(cx - 6 + dx, cy + 8 + dy);
    }
  } 
  else if (type == "Blink") {
    u8g2.drawLine(cx - 8, cy - 5, cx - 4, cy - 5);
    u8g2.drawLine(cx + 4, cy - 5, cx + 8, cy - 5);
    u8g2.drawPixel(cx, cy + 4);
  }
}

// Draw messages line by line with optional scrolling
void drawMessages() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);

  int startLine = needScroll ? scrollLine : 0;
  int endLine = startLine + maxLinesOnScreen - 1;
  if (endLine >= lineCount) endLine = lineCount - 1;

  int y = 10;
  for (int i = startLine; i <= endLine; i++) {
    u8g2.setCursor(0, y);
    u8g2.print(lines[i]);
    y += lineHeight;
  }

  u8g2.sendBuffer();

  if (needScroll) {
    scrollLine++;
    if (scrollLine + maxLinesOnScreen > lineCount) scrollLine = 0;
  }
}

// Web Handlers
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Text input
void handleSend() {
  if (server.hasArg("msg")) {
    String msg = server.arg("msg");
    if (msg.length() > 0) {
      splitMessage(msg);
      mode = MESSAGE;  // switch to message mode
      currentFace = "";
    }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

// Button click
void handleButton() {
  if (server.hasArg("type")) {
    currentFace = server.arg("type");
    mode = FACE;     // switch to face mode
    lineCount = 0;   // clear any messages
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

//Setup
void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();

  WiFi.softAP(ssid, password);
  Serial.print("Connect to IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/send", handleSend);
  server.on("/button", handleButton);
  server.begin();
  Serial.println("Server started!");
}

// Loop
void loop() {
  server.handleClient();

  if (mode == FACE && currentFace != "") {
    u8g2.clearBuffer();
    drawFace(currentFace);
    u8g2.sendBuffer();
  }
  else if (mode == MESSAGE && lineCount > 0) {
    drawMessages();
  }

  delay(300); // scroll speed
}
