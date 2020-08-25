pub struct Serial {
	pub uart: Uart
}

impl Serial {
	pub fn new() -> Self {
		Self {
			uart: Uart.new(NRF_UARTE0_BASE, IRQn_Type_UARTE0_UART0_IRQn, PIN_SERIAL_RX, PIN_SERIAL_TX)
		}
	}
}
