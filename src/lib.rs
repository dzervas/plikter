#![no_std]

mod platformio;

use platformio::Arduino_h::{pinMode, digitalWrite, delay};

const LED_BUILTIN: u8 = platformio::Arduino_h::PIN_LED1 as u8;

#[no_mangle]
pub extern "C" fn setup() {
    unsafe {
        pinMode(LED_BUILTIN, 1);
    }
}

#[no_mangle]
pub extern "C" fn r#loop() {
    unsafe {
        digitalWrite(LED_BUILTIN, 1);
        delay(2000);
        digitalWrite(LED_BUILTIN, 0);
        delay(100);
    }
}

#[panic_handler]
fn my_panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
