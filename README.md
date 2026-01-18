# ESP32 OLED Web Chat & Face Display (ESP32‑C3)

This project turns an ESP32‑C3 into a WiFi access point that hosts a web interface to control an OLED screen and onboard blue LED. Users can send text messages to be displayed with smooth scrolling or trigger simple animated faces (Happy, Sad, Blink) directly from a browser.

The OLED is driven using U8g2 over I2C, while the ESP32 runs an embedded web server to receive commands and update the display in real time.

# Hardware Used

* ESP32‑C3
* SSD1306 72x40 OLED Display (I2C)
* Onboard Blue LED

# Wiring (I2C OLED)

| OLED Pin | ESP32‑C3 Pin |
| -------- | ------------ |
| SDA      | GPIO 5       |
| SCL      | GPIO 6       |
| VCC      | 3.3V         |
| GND      | GND          |

# Features

* ESP32 SoftAP Web Interface
* Send messages from browser to OLED
* Auto line wrapping & smooth vertical scrolling
* Animated facial expressions (Happy / Sad / Blink)
* Adjustable scroll speed via slider
* Blue LED control (ON / OFF / BLINK)
* Fully offline local network control

# How It Works

1. ESP32 creates its own WiFi network.
2. Web UI allows sending messages and selecting face expressions.
3. Incoming text is auto‑split based on OLED width.
4. Display scrolls smoothly when lines exceed screen height.
5. LED modes are handled using non‑blocking timing.

---

# Required Libraries

Install via Arduino Library Manager:

* U8g2
* WiFi (ESP32 core)
* WebServer (ESP32 core)
* Wire

# Usage

1. Upload code to ESP32‑C3.
2. Connect phone/PC to WiFi: `world_link2.4`
3. Open browser → go to `192.168.4.1`
4. Send messages or control faces & LED from UI.

# Applications

* Smart mini display
* IoT notification screen
* Robot face controller
* Embedded web + display learning project

# Author

Prince Raut
