#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "usb_serial.h"
#include "afproto.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define DRIVE_PWM(n)
#define STEER_PWM(n)

void pwm_init(void) {
	// To get 50hz pwm we do fast PWM with TOP = 39999, N = 8
	
	// Set fast pwm mode and prescaling of 8
	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11);

	ICR1 = 39999;

	// Valid duty cycles are between 2200 and 3800
	OCR1A = 2200;
	OCR1B = 2200;

	DDRB = (1 << PB5) | (1 << PB6);
}

void handle_cmd(unsigned char *cmd, uint8_t len) {
	switch(cmd[0]) {
	}
}

void got_cmd(unsigned char *serial_buff, uint8_t len) {
	afproto_get_payload(serial_buff, len, serial_buff, &len);
	handle_cmd(serial_buff, len);
}

int main() {
	unsigned char serial_buff[512];
	uint8_t buff_ndx = 0;
	int16_t getchar_ret;

	// Set CPU to 16Mhz
	CPU_PRESCALE(0);

	// Start PWM
	pwm_init();

	// Init usb serial and wait for connection
	usb_init();
	while(!usb_configured());
	_delay_ms(1000);


	// Reset loop (if control program disconnects)
	while(1) {
		// Wait for a control program to connect
		while (!(usb_serial_get_control() & USB_SERIAL_DTR))

		// Reset state 
		buff_ndx = 0;
		DRIVE_PWM(0);
		STEER_PWM(0);

		while(1) {
			getchar_ret = usb_serial_getchar();

			// check if disconnect
			if(getchar_ret == -1) {
				if(!usb_configured() ||
				   !(usb_serial_get_control() & USB_SERIAL_DTR))
					break;
			}

			serial_buff[buff_ndx++] = (uint8_t)getchar_ret;

			if(buff_ndx >= 2 && serial_buff[buff_ndx-1] == AFPROTO_FRAME_END_BYTE &&  serial_buff[buff_ndx-2] != AFPROTO_FRAME_ESCAPE_BYTE) {
				got_cmd(serial_buff, buff_ndx);
			}
		}
	}
	
	return 0;
}
