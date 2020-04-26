#include <Arduino.h>
#include <bluefruit.h>
#include <Wire.h>

// #define SCREEN_ENABLE
#ifdef SCREEN_ENABLE
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>

#include "bitmaps.h"
#endif

#define DEVICE_NAME "Plikter"

#ifdef SCREEN_ENABLE
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_I2C_ADDRESS 0x3C
#endif

// Keyboard options
#include <KeypadShiftIn.h>

#define KBD_ROWS 4
#define KBD_COLUMNS 4
const char KBD_MAP[KBD_ROWS][KBD_COLUMNS] = {
    { '1', '2', '3', 'U', },
    { '4', '5', '6', 'R', },
    { '7', '8', '9', 'L', },
    { '*', '0', '#', 'D', },
};
const byte KBD_INPUT_COLUMNS[KBD_COLUMNS] = { A0, A1, A2, A3, };

BLEDis bleDis;
BLEHidAdafruit bleHid;
#ifdef SCREEN_ENABLE
SSD1306AsciiWireBitmap display;
#endif
KeypadShiftIn keyboard(KBD_INPUT_COLUMNS, KBD_ROWS, KBD_COLUMNS, 16, 15, 7);

byte curRow = 0;
byte curCol = 0;

void handleBtEvent(ble_evt_t *event) {
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
#ifdef SCREEN_ENABLE
            display.setCursor(120, 0);
            display.printBitmapPGM(BITMAP_BLUETOOTH[3]);
#endif
            break;
        default: break;
    }
}

void handleBtInput(KeypadEvent key, KeyState state) {
    if (state != PRESSED) return;

    // TODO: Use keyboardReport to set multiple keys & modifiers
    // https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/src/services/BLEHidAdafruit.cpp#L115
    bleHid.keyPress(key);
    Serial.println(key);
}

void handleBtLed(uint16_t _conn_handle, uint8_t led_bitmap) {
    // TODO: Handle LED events
    // KEYBOARD_LED_KANA, KEYBOARD_LED_COMPOSE, KEYBOARD_LED_SCROLLLOCK,
    // KEYBOARD_LED_CAPSLOCK, KEYBOARD_LED_NUMLOCK
    digitalWrite(LED_BUILTIN, led_bitmap ? HIGH : LOW);
}

void setupBluetooth() {
    Bluefruit.begin();
    Bluefruit.setTxPower(-20);
    Bluefruit.setName(DEVICE_NAME);
    Bluefruit.setEventCallback(handleBtEvent);
#ifdef SCREEN_ENABLE
    Bluefruit.autoConnLed(false);
#endif

    // Configure and Start Device Information Service
    bleDis.setManufacturer("DZervas");
    bleDis.setModel("Plikter v0.1.0");
    bleDis.begin();

    /* Start BLE HID
     * Note: Apple requires BLE device must have min connection interval >= 20m
     * ( The smaller the connection interval the faster we could send data).
     * However for HID and MIDI device, Apple could accept min connection interval
     * up to 11.25 ms. Therefore BLEHidAdafruit::begin() will try to set the min and max
     * connection interval to 11.25  ms and 15 ms respectively for best performance.
     */
    bleHid.begin();

    // Set callback for set LED from central
    bleHid.setKeyboardLedCallback(handleBtLed);

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
    Bluefruit.Advertising.addService(bleHid);

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

#ifdef SCREEN_ENABLE
void setupScreen() {
    Wire.begin();
    Wire.setClock(1000000L);

    display.begin(&Adafruit128x64, SCREEN_I2C_ADDRESS);

    display.setFont(System5x7);
    display.clear();
    display.print("Ready!");

//    display.setCursor(0, 2);
//    for (byte i=0; i < 4; i++)
//        display.printBitmapPGM(BITMAP_BLUETOOTH[i]);
//
//    display.setCursor(0, 3);
//    for (byte i=0; i < 7; i++)
//        display.printBitmapPGM(BITMAP_BATTERY[i]);
//
//    display.setCursor(0, 4);
//    for (byte i=2; i < 5; i++)
//        display.printBitmapPGM(BITMAP_LOCK_LEDS[i]);
}
#endif

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    Serial.begin(115200);
    while(!Serial) delay(10);

    Serial.println("Plikter v0.1.0");

    setupBluetooth();
    setupKeyboard();
#ifdef SCREEN_ENABLE
    setupScreen();
#endif
}

void loop() {
    keyboard.getKeys();

    if (keyboard.getState() == RELEASED)
        bleHid.keyRelease();

    delay(10);
}
