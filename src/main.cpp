#include <Arduino.h>
#include <bluefruit.h>
#include <Wire.h>

#define DEVICE_NAME "Plikter"

// Got from adafruit PDF
#define VBAT_MV_PER_LSB (0.73242188F)
#define VBAT_DIVIDER (0.71275837F)
#define VBAT_DIVIDER_COMP (1.403F)

// Keyboard options
#include <KeypadShiftIn.h>

#define HID_KEY_PLAY_PAUSE           HID_KEY_KEYPAD_1
#define HID_KEY_STOP                 HID_KEY_KEYPAD_2
#define HID_KEY_SCAN_PREVIOUS        HID_KEY_KEYPAD_3
#define HID_KEY_SCAN_NEXT            HID_KEY_KEYPAD_4
#define HID_KEY_VOLUME_DECREMENT     HID_KEY_KEYPAD_5
#define HID_KEY_VOLUME_INCREMENT     HID_KEY_KEYPAD_6
#define HID_KEY_MUTE                 HID_KEY_KEYPAD_7
#define HID_KEY_BRIGHTNESS_INCREMENT HID_KEY_KEYPAD_8
#define HID_KEY_BRIGHTNESS_DECREMENT HID_KEY_KEYPAD_9


#define KBD_ROWS 5
#define KBD_COLUMNS 15
const char KBD_MAP[KBD_ROWS][KBD_COLUMNS] = {
        { HID_KEY_GRAVE, HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7, HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_MINUS, HID_KEY_EQUAL, HID_KEY_BACKSPACE, HID_KEY_DELETE, },
        { HID_KEY_TAB, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R, HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_BRACKET_LEFT, HID_KEY_BRACKET_RIGHT, HID_KEY_BACKSLASH, HID_KEY_PAGE_UP, },
        { HID_KEY_ESCAPE, HID_KEY_A, HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K, HID_KEY_L, HID_KEY_SEMICOLON, HID_KEY_APOSTROPHE, KEYPAD_NO_KEY, HID_KEY_RETURN, HID_KEY_PAGE_DOWN, },
        { HID_KEY_SHIFT_LEFT, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B, HID_KEY_N, HID_KEY_M, HID_KEY_COMMA, HID_KEY_PERIOD, HID_KEY_SLASH, KEYPAD_NO_KEY, HID_KEY_SHIFT_RIGHT, HID_KEY_ARROW_UP, HID_KEY_INSERT, },
        { HID_KEY_CONTROL_LEFT, HID_KEY_GUI_LEFT, HID_KEY_ALT_LEFT, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_SPACE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_F15, HID_KEY_ALT_RIGHT, HID_KEY_CONTROL_RIGHT, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT, },
};
const char ALT_KBD_MAP[KBD_ROWS][KBD_COLUMNS] = {
        { KEYPAD_NO_KEY, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12, KEYPAD_NO_KEY, KEYPAD_NO_KEY, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_PRINT_SCREEN, HID_KEY_SCROLL_LOCK, HID_KEY_PAUSE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_HOME, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_END, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_PLAY_PAUSE, HID_KEY_STOP, HID_KEY_SCAN_PREVIOUS, HID_KEY_SCAN_NEXT, HID_KEY_VOLUME_DECREMENT, HID_KEY_VOLUME_INCREMENT, HID_KEY_MUTE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_F13, KEYPAD_NO_KEY, HID_KEY_F14, },
};
const byte KBD_INPUT_ROWS[KBD_ROWS] = { A5, A1, A2, A3, A4, };

BLEBas bleBas;
BLEDis bleDis;
BLEHidAdafruit bleHid;
KeypadShiftIn keyboard(KBD_INPUT_ROWS, KBD_ROWS, KBD_COLUMNS, 16, 15, 7);

bool consumerPressed = false;
uint32_t basTimer = 0;

void handleBtEvent(ble_evt_t *event) {
    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            break;
        default: break;
    }
}

void handleBtInput(KeypadEvent key, KeyState state) {
    if (key == HID_KEY_F15 && state == PRESSED) {
        keyboard.begin(makeKeymap(ALT_KBD_MAP));
    } else if (key == HID_KEY_F15 && state == RELEASED) {
        keyboard.begin(makeKeymap(KBD_MAP));
    }

    uint8_t report[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t modifier = 0;
    uint16_t consumer = 0;

    for (byte i=0, j=0; i < 6 && j < 6; i++) {
        // F15 is handled above as Fn key
        if (keyboard.key[i].kchar == KEYPAD_NO_KEY || keyboard.key[i].kchar == HID_KEY_F15)
            continue;

        if (keyboard.key[i].kstate == PRESSED || keyboard.key[i].kstate == HOLD) {
            switch (keyboard.key[i].kchar) {
                case HID_KEY_CONTROL_LEFT:
                    modifier |= KEYBOARD_MODIFIER_LEFTCTRL;
                    break;
                case HID_KEY_SHIFT_LEFT:
                    modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
                    break;
                case HID_KEY_ALT_LEFT:
                    modifier |= KEYBOARD_MODIFIER_LEFTALT;
                    break;
                case HID_KEY_GUI_LEFT:
                    modifier |= KEYBOARD_MODIFIER_LEFTGUI;
                    break;
                case HID_KEY_CONTROL_RIGHT:
                    modifier |= KEYBOARD_MODIFIER_RIGHTCTRL;
                    break;
                case HID_KEY_SHIFT_RIGHT:
                    modifier |= KEYBOARD_MODIFIER_RIGHTSHIFT;
                    break;
                case HID_KEY_ALT_RIGHT:
                    modifier |= KEYBOARD_MODIFIER_RIGHTALT;
                    break;
                case HID_KEY_GUI_RIGHT:
                    modifier |= KEYBOARD_MODIFIER_RIGHTGUI;
                    break;
                case HID_KEY_F13:
                    // TODO: Previous BT connection
                    break;
                case HID_KEY_F14:
                    // TODO: Next BT connection
                    break;
                case HID_KEY_PLAY_PAUSE:
                    consumer = HID_USAGE_CONSUMER_PLAY_PAUSE;
                    break;
                case HID_KEY_STOP:
                    consumer = HID_USAGE_CONSUMER_STOP;
                    break;
                case HID_KEY_SCAN_PREVIOUS:
                    consumer = HID_USAGE_CONSUMER_SCAN_PREVIOUS;
                    break;
                case HID_KEY_SCAN_NEXT:
                    consumer = HID_USAGE_CONSUMER_SCAN_NEXT;
                    break;
                case HID_KEY_VOLUME_DECREMENT:
                    consumer = HID_USAGE_CONSUMER_VOLUME_DECREMENT;
                    break;
                case HID_KEY_VOLUME_INCREMENT:
                    consumer = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
                    break;
                case HID_KEY_MUTE:
                    consumer = HID_USAGE_CONSUMER_MUTE;
                    break;
                case HID_KEY_BRIGHTNESS_DECREMENT:
                    consumer = HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT;
                    break;
                case HID_KEY_BRIGHTNESS_INCREMENT:
                    consumer = HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT;
                    break;
                default:
                    report[j] = keyboard.key[i].kchar;
                    break;
            }
        }

        j++;
    }

    if (consumer > 0) {
        bleHid.consumerKeyPress(consumer);
        consumerPressed = true;
    }
    else if (consumerPressed) {
        bleHid.consumerKeyRelease();
        consumerPressed = false;
    }

    bleHid.keyboardReport(modifier, report);
}

void handleBtLed(uint16_t _conn_handle, uint8_t led_bitmap) {
    // TODO: Handle LED events
    // KEYBOARD_LED_KANA, KEYBOARD_LED_COMPOSE, KEYBOARD_LED_SCROLLLOCK,
    // KEYBOARD_LED_CAPSLOCK, KEYBOARD_LED_NUMLOCK
    digitalWrite(LED_BUILTIN, led_bitmap ? HIGH : LOW);
}

uint8_t mvToPer(float mvolts) {
    if (mvolts >= 3000)
        return 100;
    else if (mvolts > 2900)
        return 100 - ((3000 - mvolts) * 58) / 100;
    else if (mvolts > 2740)
        return 42 - ((2900 - mvolts) * 24) / 160;
    else if (mvolts > 2440)
        return 18 - ((2740 - mvolts) * 12) / 300;
    else if (mvolts > 2100)
        return 6 - ((2440 - mvolts) * 6) / 340;
    else
        return 0;
}

void updateBattery() {
    uint32_t vbat = analogRead(PIN_VBAT);
    float mvolts = vbat * VBAT_MV_PER_LSB;
    uint8_t batp = mvToPer(mvolts);

    bleBas.write(batp);
}

void setupBluetooth() {
    Bluefruit.begin();
    Bluefruit.setTxPower(-20);
    Bluefruit.setName(DEVICE_NAME);
    Bluefruit.setEventCallback(handleBtEvent);

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
    bleHid.enableKeyboard(true);
    bleHid.enableMouse(false);

    // Setup BLE Battery Service
    bleBas.begin();

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

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    // 3.0V reference
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);

    setupBluetooth();
    setupKeyboard();

    basTimer = millis() - 10000;
}

void loop() {
    keyboard.getKeys();

    if (millis() - basTimer > 10000) {
        updateBattery();
        basTimer = millis();
    }
}
