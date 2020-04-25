#include "Arduino.h"

#define DEVICE_NAME "Plikter"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_I2C_ADDRESS 0x3C

#define SCREEN_COLUMNS 10
#define SCREEN_ROWS 4

// SSD1306Ascii options
//#define INCLUDE_SCROLLING 0
//#define OPTIMIZE_I2C 1

// Keyboard options
#define KBD_ROWS 4
#define KBD_COLUMNS 4
const char KBD_MAP[KBD_ROWS][KBD_COLUMNS] = {
    { '1', '2', '3', 'U', },
    { '7', '8', '9', 'L', },
    { '4', '5', '6', 'R', },
    { '*', '0', '#', 'D', },
};
byte KBD_INPUT_ROWS[KBD_COLUMNS] = { A0, A1, A2, A3, };
byte KBD_INPUT_COLUMNS[KBD_ROWS] = { 16, 15, 7, 11, };

#include <bluefruit.h>
#include <Keypad.h>
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

BLEDis bledis;
BLEHidAdafruit blehid;
SSD1306AsciiWire display;
Keypad keyboard(makeKeymap(KBD_MAP), KBD_INPUT_ROWS, KBD_INPUT_COLUMNS, sizeof(KBD_INPUT_ROWS), sizeof(KBD_INPUT_COLUMNS));

byte curRow = 0;
byte curCol = 0;

void handleBtEvent(ble_evt_t *event) {
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            digitalWrite(LED_CONN, LOW);
            break;
        default: break;
    }
}

void handleBtInput(KeypadEvent key, KeyState state) {
    if (state != PRESSED) return;

    // TODO: Use keyboardReport to set multiple keys & modifiers
    // https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/src/services/BLEHidAdafruit.cpp#L115
    blehid.keyPress(key);
}

void handleBtLed(uint16_t _conn_handle, uint8_t led_bitmap) {
    // TODO: Handle LED events
    // KEYBOARD_LED_KANA, KEYBOARD_LED_COMPOSE, KEYBOARD_LED_SCROLLLOCK,
    // KEYBOARD_LED_CAPSLOCK, KEYBOARD_LED_NUMLOCK
    digitalWrite(LED_BUILTIN, led_bitmap ? HIGH : LOW);
}

void setupBluetooth() {
    Bluefruit.begin();
    Bluefruit.setTxPower(-40);
    Bluefruit.setName(DEVICE_NAME);
    Bluefruit.setEventCallback(handleBtEvent);

    // Configure and Start Device Information Service
    bledis.setManufacturer("DZervas");
    bledis.setModel("Plikter v0.1.0");
    bledis.begin();

    /* Start BLE HID
     * Note: Apple requires BLE device must have min connection interval >= 20m
     * ( The smaller the connection interval the faster we could send data).
     * However for HID and MIDI device, Apple could accept min connection interval
     * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
     * connection interval to 11.25  ms and 15 ms respectively for best performance.
     */
    blehid.begin();

    // Set callback for set LED from central
    blehid.setKeyboardLedCallback(handleBtLed);

    /* Set connection interval (min, max) to your preferred value.
     * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
     * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
     */
//    Bluefruit.Periph.setConnInterval(9, 12);

    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

    // Include BLE HID service
    Bluefruit.Advertising.addService(blehid);

    // There is enough room for the dev name in the advertising packet
    Bluefruit.Advertising.addName();

    /* Start Advertising
     * - Enable auto advertising if disconnected
     * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     * - Timeout for fast mode is 30 seconds
     * - Start(timeout) with timeout = 0 will advertise forever (until connected)
     *
     * For recommended advertising interval
     * https://developer.apple.com/library/content/qa/qa1931/_index.html
     */
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);  // in unit of 0.625 ms
    Bluefruit.Advertising.setFastTimeout(30);  // number of seconds in fast mode
    Bluefruit.Advertising.start(0);  // 0 = Don't stop advertising after n seconds
}

void setupKeyboard() {
    keyboard.begin(makeKeymap(KBD_MAP));
    keyboard.addStatedEventListener(handleBtInput);
    keyboard.setHoldTime(-1);
}

void setupScreen() {
    Wire.begin();
    Wire.setClock(1000000L);

    display.begin(&Adafruit128x64, SCREEN_I2C_ADDRESS);

    display.setFont(Adafruit5x7);
    display.set2X();
    display.clear();
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while(!Serial) delay(10);

    Serial.println("Plikter v0.1.0");

    setupBluetooth();
    setupKeyboard();
    setupScreen();
}

void loop() {
    keyboard.getKeys();

    if (keyboard.getState() == RELEASED)
        blehid.keyRelease();

    delay(10);
//    display.clearField(0, 0, 3);
//    display.println(buf);
}
