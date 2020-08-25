#![no_std]

mod platformio;
// use platformio::Arduino_h::Hard;

mod serial
use serial;


#[no_mangle]
pub extern "C" fn setup() {
	// Serial
	let Serial =
}

#[no_mangle]
pub extern "C" fn r#loop() {
}

#[panic_handler]
fn my_panic(_info: &core::panic::PanicInfo) -> ! {
    loop {}
}
