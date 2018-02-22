#include "Oled.h"
#include "config.h"
#include "wificonf.h"

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

// Include the UI lib
#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

#define  SDA  4
#define SCL  15

#define TARGET_FPS 30
#define FRAMECOUNT 2

#define PIN_LEFT 12
#define PIN_RIGHT 13
#define PIN_UP 14
#define PIN_DOWN 26
#define PIN_CENTER 27

//--- 4way(U,D,L,R)　Switch Read  ---
#define   KEY_ID_UP             1
#define   KEY_ID_DOWN           2
#define   KEY_ID_RIGHT          3
#define   KEY_ID_LEFT           4
#define   KEY_ID_MUN            5

#define MODE_MENU 1
#define MODE_SELECT 2
#define MODE_MOVE 3

// Initialize the OLED display using Wire library
SSD1306 display(0x3c, SDA, SCL);

OLEDDisplayUi ui(&display);

int oldButtonState = HIGH;

int key_code = 0;
int prev_key_code = 0;
int long_press = 0;
int ui_mode = MODE_MENU;

// converts the dBm to a range between 0 and 100%
int8_t getWifiQuality() {
	int32_t dbm = WiFi.RSSI();
	if (dbm <= -100) {
		return 0;
	} else if (dbm >= -50) {
		return 100;
	} else {
		return 2 * (dbm + 100);
	}
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
//  display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->setFont(ArialMT_Plain_10);
//  display->drawString(128, 0, String(millis()));
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0, 0, WiFi.localIP().toString().c_str());

	int8_t quality = getWifiQuality();
	for (int8_t i = 0; i < 4; i++) {
		for (int8_t j = 0; j < 2 * (i + 1); j++) {
			if (quality > i * 25 || j == 0) {
				display->setPixel(120 + 2 * i, 7 - j);
			}
		}
	}
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawHorizontalLine(0, 10, 128);
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x,
		int16_t y) {
	// draw an xbm image.
	// Please note that everything that should be transitioned
	// needs to be drawn relative to x and y

	display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height,
			WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x,
		int16_t y) {
	// Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
	// Besides the default fonts there will be a program to convert TrueType fonts into this format
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 10 + y, WiFi.getHostname());

	display->setFont(ArialMT_Plain_10);
	display->drawString(0 + x, 26 + y, WiFi.SSID().c_str());
	display->drawString(0 + x, 36 + y,
			String(wifi_config.getSignal(WiFi.RSSI())).c_str());
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2 };
//int frameCount = FRAMECOUNT;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void initOled(void) {
	Serial.println("start");
	pinMode(16, OUTPUT);
	digitalWrite(16, LOW); // set GPIO16 low to reset OLED
	delay(50);
	digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

	//init the keypad
	//pinMode(0, INPUT);
	pinMode(PIN_UP, INPUT_PULLDOWN);
	pinMode(PIN_DOWN, INPUT_PULLDOWN);
	pinMode(PIN_LEFT, INPUT_PULLDOWN);
	pinMode(PIN_RIGHT, INPUT_PULLDOWN);
	pinMode(PIN_CENTER, INPUT_PULLDOWN);

// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
	ui.setTargetFPS(TARGET_FPS);
	ui.disableAutoTransition();
	// Customize the active and inactive symbol
	ui.setActiveSymbol(activeSymbol);
	ui.setInactiveSymbol(inactiveSymbol);

	// You can change this to
	// TOP, LEFT, BOTTOM, RIGHT
	ui.setIndicatorPosition(BOTTOM);

	// Defines where the first frame is located in the bar.
	ui.setIndicatorDirection(LEFT_RIGHT);

	// You can change the transition that is used
	// SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
	ui.setFrameAnimation(SLIDE_LEFT);

	// Add frames
	ui.setFrames(frames, FRAMECOUNT);

	// Add overlays
	ui.setOverlays(overlays, overlaysCount);

	// Initialising the UI will init the display too.
	ui.init();

	display.flipScreenVertically();

	display.setTextAlignment(TEXT_ALIGN_LEFT);
	display.setFont(ArialMT_Plain_24);
	display.drawString(20, 0, "ESP3D");
	display.drawString(0, 35, "Connecting");

	display.display();
}

//-------------------------------------------
// key Control
//-------------------------------------------
void KeyPolling(void) {
	prev_key_code = key_code;
	if (digitalRead(PIN_UP) == HIGH) {
		key_code = KEY_ID_UP;
	} else if (digitalRead(PIN_DOWN) == HIGH) {
		key_code = KEY_ID_DOWN;
	} else if (digitalRead(PIN_LEFT) == HIGH) {
		key_code = KEY_ID_LEFT;
	} else if (digitalRead(PIN_RIGHT) == HIGH) {
		key_code = KEY_ID_RIGHT;
	} else
		key_code = 0;

	if (prev_key_code == key_code)
		long_press++;
	else
		long_press = 0;

	return;
}

int oLedUpdate(void) {
	int buttonState = digitalRead(0); //GPIO 0
	// check if the pushbutton is pressed.
	// if it is, the buttonState is LOW:
	if (buttonState == LOW && oldButtonState != buttonState) {
		//Serial.println("Button Pushed");
		ui.nextFrame();
		oldButtonState = buttonState;
	}
	if (buttonState == HIGH && oldButtonState != buttonState) {
		oldButtonState = buttonState;
	}

	KeyPolling();

	if (key_code == KEY_ID_LEFT) {
		ui.previousFrame();
	} else if (key_code == KEY_ID_RIGHT) {
		ui.nextFrame();
	} else if (key_code == KEY_ID_UP) {
		ui.enableAutoTransition();
	} else if (key_code == KEY_ID_DOWN) {
		ui.disableAutoTransition();
	} else if (key_code == KEY_ID_MUN) {
		ui_mode = ui.getUiState()->currentFrame;
	}

	return ui.update();
}

void oledTask(void *pvParameters) {
	int taskno = (int) pvParameters;
	int sleeptime;

	while (1) {
		sleeptime = (int) (esp_random() & 0x0F);
		//Serial.println(String("Task ")+String(taskno)+String(" ")+String(sleeptime));
		int remainingTimeBudget = oLedUpdate();
		if (remainingTimeBudget > 0) {
			// You can do some work here
			// Don't do stuff if you are below your
			// time budget.
			vTaskDelay(remainingTimeBudget);
		}
	}
}
