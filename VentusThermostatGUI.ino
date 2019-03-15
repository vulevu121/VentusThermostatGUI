/***************************************************
  This is our library for the Adafruit 3.5" TFT (HX8357) FeatherWing
  ----> http://www.adafruit.com/products/3651

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/



#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_HX8357.h>
#include <Adafruit_STMPE610.h>
#include "bitmaps.h"
#include <WiFi101.h>
#include "wifi_ssid.h"

#define VBATPIN    A7

#define STMPE_CS 6
#define TFT_CS   9
#define TFT_DC   10
#define SD_CS    5

#define TFT_RST -1

// Color definitions
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define YELLOW   0xFFE0 
#define WHITE    0xFFFF
#define GREY     0x528A

// Meter colour schemes
#define RED2RED 0
#define GREEN2GREEN 1
#define BLUE2BLUE 2
#define BLUE2RED 3
#define GREEN2RED 4
#define RED2GREEN 5

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 8

// gauge coordinates
#define GAUGE_XPOS 0
#define GAUGE_YPOS 0
#define GAUGE_RAD 160
#define GAUGE_FONTSIZE 7
#define GAUGE_NDIGITS 3

#define HUM_GAUGE_XPOS 50
#define HUM_GAUGE_YPOS 50
#define HUM_GAUGE_RAD 130
#define HUM_GAUGE_FONTSIZE 7
#define HUM_GAUGE_NDIGITS 3

// up/down arrow coordinates
#define ARROW_CENTER_X 400
#define ARROW_CENTER_Y 160
#define ARROW_WIDTH 120
#define ARROW_HEIGHT 80
#define ARROW_GAP 20

// room title
#define ROOM_ICON_XPOS 75
#define ROOM_ICON_YPOS 260
#define ROOM_ICON_SIZE 32
#define ROOM_ICON_MARGIN 10
#define ROOM_TITLE_FONTSIZE 3

// fan button
#define FAN_BUTTON_XPOS 335
#define FAN_BUTTON_YPOS 4
#define FAN_BUTTON_WIDTH 128
#define FAN_BUTTON_HEIGHT 64

#define TOGGLE_BUTTON_WIDTH 64
#define TOGGLE_BUTTON_HEIGHT 48

#define BUTTON_PRESS_RADIUS 40

// auto button
#define AUTO_BUTTON_XPOS 335
#define AUTO_BUTTON_YPOS 250
#define AUTO_BUTTON_WIDTH 128
#define AUTO_BUTTON_HEIGHT 64

// calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define TFT_REFRESH_INTERVAL 200
#define PRESSED_INTERVAL 200

int valueXpos = GAUGE_XPOS + GAUGE_RAD - int(5.0*GAUGE_FONTSIZE*GAUGE_NDIGITS/2.0);
int valueYpos = GAUGE_YPOS + GAUGE_RAD - int(8.0*GAUGE_FONTSIZE);
int unitXpos = valueXpos;
int unitYpos = valueYpos + int(8.0*GAUGE_FONTSIZE);

int humValueXpos = HUM_GAUGE_XPOS + HUM_GAUGE_RAD - int(5.0*HUM_GAUGE_FONTSIZE*HUM_GAUGE_NDIGITS/2.0);
int humValueYpos = HUM_GAUGE_YPOS + HUM_GAUGE_RAD - int(8.0*HUM_GAUGE_FONTSIZE);

// Use hardware SPI and the above for CS/DC
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

uint32_t lastTFTUpdateTime = 0;       // time for next update
uint32_t lastPressedTime = 0;
uint32_t onTouchTime = 0;
uint32_t onReleaseTime = 0;

bool fanOn = false;
bool autoOn = false;

int tempF = 72;
int lastTempF = 0;
int humidityPercent = 50;
int lastHumidityPercent = 0;


// Please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
bool ready_to_grasp = false;


// Initialize the WiFi client library
WiFiSSLClient client;

// server address:
//char server[] = "us-central1-fir-authdemo2-13925.cloudfunctions.net";
char server[] = "www.google.com";    // name address for Google (using DNS)

// IPAddress server(64,131,82,241);

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 30L * 1000L; // delay between updates, in milliseconds


void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
  
  //  while (!Serial) delay(10);
  //Serial.flush();
  WiFi.setPins(8, 7, 4, 2);
  WiFi.lowPowerMode();
  Serial.begin(9600);
  
  
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  
  tft.begin(HX8357D);
  tft.setRotation(1);  
//  tft.fillScreen(BLACK);
//tft.drawBitmap(240 - 230/2, 160 - 170/2, loadingBitmap, 230, 170, WHITE);
//  delay(500);


  // clear to black before drawing
  tft.fillScreen(BLACK);

  // draw up/down arrows
//  tft.fillTriangle(ARROW_CENTER_X-ARROW_WIDTH/2, ARROW_CENTER_Y-ARROW_GAP/2, ARROW_CENTER_X+ARROW_WIDTH/2, ARROW_CENTER_Y-ARROW_GAP/2, ARROW_CENTER_X, ARROW_CENTER_Y-ARROW_HEIGHT-ARROW_GAP/2, GREY); 
//  tft.fillTriangle(ARROW_CENTER_X-ARROW_WIDTH/2, ARROW_CENTER_Y+ARROW_GAP/2, ARROW_CENTER_X+ARROW_WIDTH/2, ARROW_CENTER_Y+ARROW_GAP/2, ARROW_CENTER_X, ARROW_CENTER_Y+ARROW_HEIGHT+ARROW_GAP/2, GREY);

  tft.drawBitmap(ARROW_CENTER_X-120/2, ARROW_CENTER_Y-180/2, arrowsBitmap, 120, 180, GREY);

  // draw the +/- on the arrows
  tft.setTextSize(8);
  tft.setTextColor(WHITE);
  tft.setCursor(ARROW_CENTER_X-20, ARROW_CENTER_Y-70);
  tft.print("+");
  tft.setTextColor(WHITE);
  tft.setCursor(ARROW_CENTER_X-20, ARROW_CENTER_Y+10);
  tft.print("-");

  tft.drawBitmap(ROOM_ICON_XPOS, ROOM_ICON_YPOS, homeIcon, 32, 32, WHITE);
  tft.setCursor(ROOM_ICON_XPOS + ROOM_ICON_SIZE + ROOM_ICON_MARGIN, ROOM_ICON_YPOS+4);
  tft.setTextSize(ROOM_TITLE_FONTSIZE);
  tft.setTextColor(WHITE);
  tft.print("Kitchen");

  tft.drawBitmap(valueXpos + CHAR_WIDTH*GAUGE_FONTSIZE*2 + 20, valueYpos, thermoIconF, 35, 56, WHITE);
  tft.drawBitmap(valueXpos + CHAR_WIDTH*GAUGE_FONTSIZE*2 + 16, valueYpos+60, humidityIcon, 35, 56, WHITE);

  tft.setCursor(FAN_BUTTON_XPOS, FAN_BUTTON_YPOS+15);
  tft.print("FAN");

  tft.setCursor(AUTO_BUTTON_XPOS, AUTO_BUTTON_YPOS+15);
  tft.print("AUTO");

  tft.drawBitmap(FAN_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, FAN_BUTTON_YPOS, toggleOff, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, GREY);
  tft.drawBitmap(AUTO_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, AUTO_BUTTON_YPOS, toggleOff, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, GREY);
  
//  tft.drawBitmap(FAN_BUTTON_XPOS, FAN_BUTTON_YPOS, fanButton, FAN_BUTTON_WIDTH, FAN_BUTTON_HEIGHT, GREY);
//  tft.drawBitmap(AUTO_BUTTON_XPOS, AUTO_BUTTON_YPOS, autoButton, AUTO_BUTTON_WIDTH, AUTO_BUTTON_HEIGHT, GREY);

//  tft.drawCircle(ARROW_CENTER_X, ARROW_CENTER_Y-45, BUTTON_PRESS_RADIUS, RED);
//  tft.drawCircle(ARROW_CENTER_X, ARROW_CENTER_Y+45, BUTTON_PRESS_RADIUS, RED);
//  
//  
//  tft.drawCircle(AUTO_BUTTON_XPOS+AUTO_BUTTON_WIDTH/2, AUTO_BUTTON_YPOS+AUTO_BUTTON_HEIGHT/2+10, BUTTON_PRESS_RADIUS, RED);
//  tft.drawCircle(FAN_BUTTON_XPOS+FAN_BUTTON_WIDTH/2, FAN_BUTTON_YPOS+FAN_BUTTON_HEIGHT/2-10, BUTTON_PRESS_RADIUS, RED);
//
//  tft.drawLine(ARROW_CENTER_X-65, 0, ARROW_CENTER_X-65, 320, RED);
//  tft.drawLine(0, FAN_BUTTON_YPOS+AUTO_BUTTON_HEIGHT, 480, FAN_BUTTON_YPOS+FAN_BUTTON_HEIGHT, RED);
//  tft.drawLine(0, ARROW_CENTER_Y, 480, ARROW_CENTER_Y, RED);
//  tft.drawLine(0, AUTO_BUTTON_YPOS, 480, AUTO_BUTTON_YPOS, RED);
  connectWiFi();
}

void loop() {
  uint16_t x, y;
  uint8_t z;
  uint16_t px, py;

  
  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  if (millis() - lastConnectionTime > postingInterval) {
	  long rssi = WiFi.RSSI();
	  Serial.print("RSSI: ");
	  Serial.print(rssi);
	  Serial.println(" dBm");
	  //readBat();
	  httpRequest();
	  lastConnectionTime = millis();
  }

  if (ts.touched()) {
    // read x & y & z;
    while (! ts.bufferEmpty()) {
      ts.readData(&x, &y, &z);
      py = map(x, TS_MINX, TS_MAXX, 320, 0);
      px = map(y, TS_MINY, TS_MAXY, 0, 480);

      //Serial.print(ts.bufferSize()); Serial.print("->("); Serial.print(px); Serial.print(", "); Serial.print(py); Serial.print(", "); Serial.print(z); Serial.println(")");
    }
    ts.writeRegister8(STMPE_INT_STA, 0xFF); // reset all ints

    if (px > ARROW_CENTER_X-65) {
      if (py < FAN_BUTTON_YPOS + AUTO_BUTTON_HEIGHT) {
        if ((millis() - lastPressedTime > PRESSED_INTERVAL)) {
          lastPressedTime = millis();
          toggleFan();
        }
      }
      else if (py < ARROW_CENTER_Y) {
        if ((millis() - lastPressedTime > PRESSED_INTERVAL)) {
          lastPressedTime = millis();
          tempF = tempF < 99 ? ++tempF : 99;
        }
      }
      else if (py < AUTO_BUTTON_YPOS) {
        if ((millis() - lastPressedTime > PRESSED_INTERVAL)) {
          lastPressedTime = millis();
          tempF = tempF >= 0 ? --tempF : 0;
        }
      }
      else if (py < 480) {
        if ((millis() - lastPressedTime > PRESSED_INTERVAL)) {
          lastPressedTime = millis();
          toggleAuto();
       }
      }
    }

    
  }

  
  // update the display on an interval
  if (millis() - lastTFTUpdateTime > TFT_REFRESH_INTERVAL) { 
    lastTFTUpdateTime = millis();

    // draw or update the temp gauge when there is a change
    if(tempF != lastTempF) {
      ringMeter(tempF, 43, 99, GAUGE_XPOS, GAUGE_YPOS, GAUGE_RAD, "", BLUE2RED, valueXpos, valueYpos, 20);
      lastTempF = tempF;
    }
    

    // draw or update the humidity gauge when there is a change
    if (humidityPercent != lastHumidityPercent) {
      ringMeter(humidityPercent, 0, 99, GAUGE_RAD - HUM_GAUGE_RAD, GAUGE_RAD - HUM_GAUGE_RAD, HUM_GAUGE_RAD, "", BLUE2RED, valueXpos, valueYpos+60, 20);
      lastHumidityPercent = humidityPercent;
    }
    

  }
}


// toggle fan on/off
void toggleFan() {
  fanOn = fanOn ? false : true;
  if (fanOn) {
//    tft.drawBitmap(FAN_BUTTON_XPOS, FAN_BUTTON_YPOS, fanButton, FAN_BUTTON_WIDTH, FAN_BUTTON_HEIGHT, WHITE);
    tft.fillRect(FAN_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, FAN_BUTTON_YPOS, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, BLACK);
    tft.drawBitmap(FAN_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, FAN_BUTTON_YPOS, toggleOn, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, WHITE);
  }
  else {
//    tft.drawBitmap(FAN_BUTTON_XPOS, FAN_BUTTON_YPOS, fanButton, FAN_BUTTON_WIDTH, FAN_BUTTON_HEIGHT, GREY);
    tft.fillRect(FAN_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, FAN_BUTTON_YPOS, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, BLACK);
    tft.drawBitmap(FAN_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, FAN_BUTTON_YPOS, toggleOff, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, GREY);
  }
}


// toggle auto on/off
void toggleAuto() {
  autoOn = autoOn ? false : true;
  if (autoOn) {
//    tft.drawBitmap(AUTO_BUTTON_XPOS, AUTO_BUTTON_YPOS, autoButton, AUTO_BUTTON_WIDTH, AUTO_BUTTON_HEIGHT, WHITE);
    tft.fillRect(AUTO_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, AUTO_BUTTON_YPOS, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, BLACK);
    tft.drawBitmap(AUTO_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, AUTO_BUTTON_YPOS, toggleOn, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, WHITE);
  }
  else {
//    tft.drawBitmap(AUTO_BUTTON_XPOS, AUTO_BUTTON_YPOS, autoButton, AUTO_BUTTON_WIDTH, AUTO_BUTTON_HEIGHT, GREY);
    tft.fillRect(AUTO_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, AUTO_BUTTON_YPOS, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, BLACK);
    tft.drawBitmap(AUTO_BUTTON_XPOS + 5*ROOM_TITLE_FONTSIZE*4+20, AUTO_BUTTON_YPOS, toggleOff, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, GREY);
  }
}

//  draw the meter on the screen, returns x coord of righthand side
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *unit, byte scheme, int value_x, int value_y, int w)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring

//  int w = r / 8;    // Width of outer ring is 1/4 of radius
  
  int angle = 130;  // Half the sweep angle of meter (300 degrees)

  int text_colour = 0; // To hold the text colour

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 10; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc) {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme) {
      case 0: colour = RED; break; // Fixed colour
      case 1: colour = GREEN; break; // Fixed colour
      case 2: colour = BLUE; break; // Fixed colour
      case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
      case 4: colour = rainbow(map(i, -angle, angle, 63, 127)); break; // Green to red (high temperature etc)
      case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
      default: colour = BLUE; break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, GREY);
    }
  }

  
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(value_x, value_y);
  tft.setTextSize(GAUGE_FONTSIZE);

  if (value < 10) {
    tft.print("0");
  }
  
  tft.print(value);
  tft.print(unit);

  return x + r;
}


// Return a 16 bit rainbow colour
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

// Return a value in range -1 to +1 for a given phase angle in degrees
float sineWave(int phase) {
  return sin(phase * 0.0174532925);
}


void connectWiFi() {
	// attempt to connect to WiFi network:
	while (status != WL_CONNECTED) {
		Serial.print("Attempting to connect to SSID: ");
		Serial.println(ssid);
		// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
		status = WiFi.begin(ssid, pass);

		// wait for good signal strength
		//    long rssi = 0;

		//    while (rssi == 0) {
		//      rssi = WiFi.RSSI();
		//      Serial.print("RSSI: ");
		//      Serial.print(rssi);
		//      Serial.println(" dBm");
		//      delay(1000);
		//    }

		delay(5000);
	}
	Serial.println("Connected to Wifi!");
}

void httpRequest() {
	Serial.println("Requesting HTTP...");
	float bat;

	bat = readBatPercent();
	String stringBat = String(int(bat));
	Serial.println("bat: " + stringBat);


	//////////////////////GET HUMIDITY////////////////////////////////////////
	//digitalWrite(LED_BUILTIN, HIGH);
	//client.stop();
	//if (client.connect(server, 443)) {
	//	client.println("GET /getTemp?roomId=Kitchen HTTP/1.1");
	//	client.println("Host: us-central1-fir-authdemo2-13925.cloudfunctions.net");
	//	client.println("User-Agent: ArduinoWiFi/1.1");
	//	client.println("Connection: close");
	//	client.println();

	//	// note the time that the connection was made:
	//	lastConnectionTime = millis();
	//}
	//else {
	//	// if you couldn't make a connection:
	//	Serial.println("connection failed");
	//}

	client.stop();

	if (client.connect(server, 443)) {
		Serial.println("connected to server");
		// Make a HTTP request:
		client.println("GET /search?q=arduino HTTP/1.1");
		client.println("Host: www.google.com");
		client.println("Connection: close");
		client.println();
	}
	else {
		Serial.println("connection failed");
	}

}

float readBat() {
	float measuredvbat = analogRead(VBATPIN);
	measuredvbat /= 1024; // convert to voltage
	measuredvbat *= 2;    // we divided by 2, so multiply back
	measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage

	Serial.print("VBat: "); Serial.println(measuredvbat);
	return measuredvbat;
}

float readBatPercent() {
	float measuredvbat = analogRead(VBATPIN);
	measuredvbat /= 1024; // convert to voltage
	measuredvbat *= 2;    // we divided by 2, so multiply back
	measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage


	measuredvbat = (measuredvbat - 2.2) * 100.0 / 2.1;
	if (measuredvbat > 100.0)
		measuredvbat = 100.0;
	else if (measuredvbat < 0.0)
		measuredvbat = 0.0;
	return measuredvbat;
}
