#include <Arduino.h>
#include <bluefruit.h>
#include <InternalFileSystem.h>
#include <KeypadShiftIn.h>

#define DEVICE_NAME "Plikter"

// Got from adafruit PDF
#define VBAT_MV_PER_LSB (0.73242188F)

// Keyboard options
#define HID_KEY_BT_PREVIOUS          HID_KEY_F13
#define HID_KEY_BT_NEXT              HID_KEY_F14
#define HID_KEY_FN                   HID_KEY_F15
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
        { HID_KEY_CONTROL_LEFT, HID_KEY_GUI_LEFT, HID_KEY_ALT_LEFT, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_SPACE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_FN, HID_KEY_ALT_RIGHT, HID_KEY_CONTROL_RIGHT, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT, },
};
const char ALT_KBD_MAP[KBD_ROWS][KBD_COLUMNS] = {
        { KEYPAD_NO_KEY, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8, HID_KEY_F9, HID_KEY_F10, HID_KEY_F11, HID_KEY_F12, KEYPAD_NO_KEY, HID_KEY_DELETE, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_PRINT_SCREEN, HID_KEY_SCROLL_LOCK, HID_KEY_PAUSE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_HOME, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_END, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_PLAY_PAUSE, HID_KEY_STOP, HID_KEY_SCAN_PREVIOUS, HID_KEY_SCAN_NEXT, HID_KEY_VOLUME_DECREMENT, HID_KEY_VOLUME_INCREMENT, HID_KEY_MUTE, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_BRIGHTNESS_INCREMENT, KEYPAD_NO_KEY, },
        { KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, KEYPAD_NO_KEY, HID_KEY_FN, HID_KEY_ALT_RIGHT, HID_KEY_CONTROL_RIGHT, HID_KEY_BT_PREVIOUS, HID_KEY_BRIGHTNESS_DECREMENT, HID_KEY_BT_NEXT, },
};
const byte KBD_INPUT_ROWS[KBD_ROWS] = { A5, A1, A2, A3, A4, };

BLEBas bleBas;
BLEDis bleDis;
BLEHidAdafruit bleHid;
KeypadShiftIn keyboard(KBD_INPUT_ROWS, KBD_ROWS, KBD_COLUMNS, 16, 15, 7);

SoftwareTimer batteryTimer;
SoftwareTimer inputTimer;
SoftwareTimer keyboardTimer;

bool consumerPressed = false;
uint8_t clearBonds = 0;

typedef struct Conn {
    int handle;
    Conn* next; // Pointer to next node in DLL
    Conn* prev; // Pointer to previous node in DLL
} Conn;

Conn* connection = NULL;

#define REPORT_BUFSIZ 10

hid_keyboard_report_t* report_buffer[REPORT_BUFSIZ];
int report_index = -1;
uint16_t consumer_buffer[REPORT_BUFSIZ];
int consumer_index = -1;

void conn_push(uint16_t conn_handle) {
    Conn* new_conn = (Conn *) malloc(sizeof(Conn));
    new_conn->handle = conn_handle;
    new_conn->next = NULL;

    if (connection != NULL) {
        // Put the new connection to the end
        while (connection->next != NULL)
            connection = connection->next;

        connection->next = new_conn;
        new_conn->prev = connection;
    }

    connection = new_conn;
}

uint16_t conn_pop(uint16_t conn_handle) {
    if (connection == NULL)
        return BLE_CONN_HANDLE_INVALID;

    while (connection->handle != conn_handle) {
        if (connection->next != NULL)
            connection = connection->next;
        else if (connection->prev != NULL)
            connection = connection->prev;
        else
            return BLE_CONN_HANDLE_INVALID;
    }

    Conn* old_conn = connection;
    uint16_t old_handle = connection->handle;

    if (connection->prev != NULL && connection->next != NULL) {
        // The deleted node is in the middle of the linked list
        connection->prev->next = connection->next;
        connection = connection->prev;
    } else if (connection->prev != NULL) {
        // The connection is in the end of the linked list
        connection = connection->prev;
        connection->next = NULL;
    } else if (connection->next != NULL) {
        // The deleted node is in the start of the linked list
        connection = connection->next;
        connection->prev = NULL;
    } else {
        connection = NULL;
    }

    free(old_conn);

    return old_handle;
}

void handleFnMap(KeypadEvent key, KeyState state) {
    if (state == RELEASED && key == HID_KEY_FN) {
        keyboard.begin(makeKeymap(KBD_MAP));
        clearBonds = 0;
    }

    if (state != PRESSED) return;

    switch (key) {
        case HID_KEY_FN:
            keyboard.begin(makeKeymap(ALT_KBD_MAP));
            clearBonds = 1;
            break;
        case HID_KEY_BT_PREVIOUS:
            if (connection == NULL) break;

            if (connection->prev != NULL)
                connection = connection->prev;
            else {
                while (connection->next != NULL)
                    connection = connection->next;
            }

#ifdef CFG_DEBUG
            Serial.printf("Prev handle: %d\n", connection->handle);
#endif
            break;
        case HID_KEY_BT_NEXT:
            if (connection == NULL) break;

            if (connection->next != NULL)
                connection = connection->next;
            else {
                while (connection->prev != NULL)
                    connection = connection->prev;
            }

#ifdef CFG_DEBUG
            Serial.printf("Next handle: %d\n", connection->handle);
#endif
            break;
        case HID_KEY_DELETE:
            // Use delete key for the clearbonds shortcut but report it too
            if (clearBonds > 0 && clearBonds < 4)
                clearBonds++;
            break;
    }
}

void handleKeys() {
    if (connection == NULL) {
#ifdef CFG_DEBUG
        Serial.println("[keys] Abort!");
#endif
        return;
    }

    hid_keyboard_report_t* report = (hid_keyboard_report_t *) calloc(1, sizeof(hid_keyboard_report_t));
    uint16_t consumer = 0;

    for (byte i=0, j=0; i < 6 && j < 6; i++) {
        // Don't handle dead keys or Fn
        if (keyboard.key[i].kchar == KEYPAD_NO_KEY || keyboard.key[i].kchar == HID_KEY_FN ||
                // Don't handle key releases or IDLE
                (keyboard.key[i].kstate != PRESSED && keyboard.key[i].kstate != HOLD))
            continue;

        // Button translator
        switch (keyboard.key[i].kchar) {
            case HID_KEY_CONTROL_LEFT:
                report->modifier |= KEYBOARD_MODIFIER_LEFTCTRL;
                break;
            case HID_KEY_SHIFT_LEFT:
                report->modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
                break;
            case HID_KEY_ALT_LEFT:
                report->modifier |= KEYBOARD_MODIFIER_LEFTALT;
                break;
            case HID_KEY_GUI_LEFT:
                report->modifier |= KEYBOARD_MODIFIER_LEFTGUI;
                break;
            case HID_KEY_CONTROL_RIGHT:
                report->modifier |= KEYBOARD_MODIFIER_RIGHTCTRL;
                break;
            case HID_KEY_SHIFT_RIGHT:
                report->modifier |= KEYBOARD_MODIFIER_RIGHTSHIFT;
                break;
            case HID_KEY_ALT_RIGHT:
                report->modifier |= KEYBOARD_MODIFIER_RIGHTALT;
                break;
            case HID_KEY_GUI_RIGHT:
                report->modifier |= KEYBOARD_MODIFIER_RIGHTGUI;
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
            case HID_KEY_BRIGHTNESS_INCREMENT:
                consumer = HID_USAGE_CONSUMER_BRIGHTNESS_INCREMENT;
                break;
            case HID_KEY_BRIGHTNESS_DECREMENT:
                consumer = HID_USAGE_CONSUMER_BRIGHTNESS_DECREMENT;
                break;
            default:
                report->keycode[j] = keyboard.key[i].kchar;
                break;
        }

        j++;
    }

    // TODO: Implement out of time sending of consumer keys
    if (consumer > 0) {
        bleHid.consumerKeyPress(connection->handle, consumer);
        consumerPressed = true;
    } else if (consumerPressed) {
        bleHid.consumerKeyRelease(connection->handle);
        consumerPressed = false;
    }

    if (report_index >= REPORT_BUFSIZ) {
#ifdef CFG_DEBUG
        Serial.println("[updateInput] Clearing reports");
#endif
        // Throw the oldest event
        free(report_buffer[0]);

        for (int i = 1; i < report_index; i++) {
            report_buffer[i-1] = report_buffer[i];
        }

        // A +1 will happen afterwards
        report_index = REPORT_BUFSIZ - 2;
    }

    report_index += 1;
    if (report_index < 0)
        report_index = 0;

    report_buffer[report_index] = report;
    // Serial.printf("[updateKeyboard] Added report %d", report_index);
}

void updateKeyboard(TimerHandle_t _handle) {
    for (int i = 0; i <= report_index; i++) {
        hid_keyboard_report_t *report = report_buffer[i];
#ifdef CFG_DEBUG
        uint32_t startTime = millis();
#endif
        bleHid.keyboardReport(connection->handle, report);
#ifdef CFG_DEBUG
        uint32_t deltaTime = millis() - startTime;
        if (deltaTime > 0) Serial.printf("Time to send: %d ", deltaTime);
#endif

        free(report);
    }

    report_index = -1;
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

void updateInput(TimerHandle_t _handle) {
#ifdef CFG_DEBUG
    uint32_t startTime = millis();
#endif

    // Serial.println("Running");
    if (keyboard.getKeys())
        // Run only if state has changed
        handleKeys();


    if (clearBonds >= 4) {
        // Actually happens when Fn is pressed and Delete is pressed 4 times
        clearBonds = 0;
        InternalFS.format();
        Bluefruit.clearBonds();
    }

#ifdef CFG_DEBUG
    uint32_t deltaTime = millis() - startTime;
    if (deltaTime > 0) Serial.printf("Total Time: %d\n", deltaTime);
#endif
}

void updateBattery(TimerHandle_t _handle) {
    if (connection == NULL) return;

    uint32_t vbat = analogRead(PIN_VBAT);
    float mvolts = vbat * VBAT_MV_PER_LSB;
    uint8_t batp = mvToPer(mvolts);

    bleBas.notify(connection->handle, batp);
}

void connect_callback(uint16_t conn_handle) {
    conn_push(conn_handle);

    if (connection != NULL && connection->prev == NULL && connection->next == NULL) {
        BLEConnection* conn = Bluefruit.Connection(conn_handle);
        conn->requestConnectionParameter(6);
        delay(1000);

#ifdef CFG_DEBUG
        Serial.println("Started timers");
#endif
        inputTimer.start();
        batteryTimer.start();
        keyboardTimer.start();
        updateBattery(NULL);
    }

    // Keep advertising
    if (!Bluefruit.Advertising.isRunning())
        Bluefruit.Advertising.start(0);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
    if ((connection != NULL && connection->prev == NULL && connection->next == NULL) || connection == NULL) {
#ifdef CFG_DEBUG
        Serial.println("Stopped timers");
#endif
        inputTimer.stop();
        batteryTimer.stop();
        keyboardTimer.stop();
    }

    conn_pop(conn_handle);

    // Keep advertising
    if (!Bluefruit.Advertising.isRunning())
        Bluefruit.Advertising.start(0);
}

void setupBluetooth() {
    Bluefruit.begin(2, 0);
    Bluefruit.setTxPower(4);
    Bluefruit.setName(DEVICE_NAME);
    Bluefruit.Periph.setConnectCallback(connect_callback);
    Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

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
    // bleHid.setKeyboardLedCallback(handleBtLed);
    bleHid.enableKeyboard(true);
    bleHid.enableMouse(false);

    // Setup BLE Battery Service
    bleBas.begin();

    /* Set connection interval (min, max) to your preferred value.
     * Note: It is already set by BLEHidAdafruit::begin() to 11.25ms - 15ms
     * min = 9*1.25=11.25 ms, max = 12*1.25= 15 ms
     */
    // Bluefruit.Periph.setConnInterval(9, 12);
    Bluefruit.Periph.setConnInterval(6, 12);

    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

    // Include BLE HID service
    Bluefruit.Advertising.addService(bleHid);
    Bluefruit.Advertising.addService(bleBas);

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
    keyboard.addStatedEventListener(handleFnMap);
    keyboard.setHoldTime(-1);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    // 3.0V reference
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);

#ifdef CFG_DEBUG
    Serial.begin(115200);
#endif

    setupBluetooth();
    setupKeyboard();

    inputTimer.begin(20, updateInput);
    inputTimer.stop();
    batteryTimer.begin(60000, updateBattery);
    batteryTimer.stop();
    keyboardTimer.begin(40, updateKeyboard);
    keyboardTimer.stop();

    suspendLoop();
}

void loop() {}
