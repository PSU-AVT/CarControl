#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

#include "usb_serial.h"
#include "afproto.h"

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

uint16_t throttle_neutral = 3000;
uint16_t steer_neutral = 2975;

void set_neutral(void) {
	OCR1A = throttle_neutral;
	OCR1B = steer_neutral;
}

void pwm_init(void) {
	// To get 50hz pwm we do fast PWM with TOP = 39999, N = 8
	
	// Set fast pwm mode and prescaling of 8
	TCCR1A = (1 << COM1A1) | (1 << COM1B1) | (1 << WGM11);
	TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11);

	ICR1 = 39999;

	// Valid duty cycles are between 2200 and 3800
	set_neutral();

	DDRB = (1 << PB5) | (1 << PB6);
}

void handle_cmd(unsigned char *cmd, uint8_t len) {
	switch(cmd[0]) {
		case 0:
			set_neutral();
			break;
		case 1:
			if(cmd[1] >= 128)
				OCR1A = throttle_neutral + ((cmd[1] - 128)*4);
			else
				OCR1A = throttle_neutral - ((128 - cmd[1])*4);
			break;
		case 2:
			if(cmd[1] >= 128)
				OCR1B = steer_neutral - ((cmd[1] - 128)*5);
			else
				OCR1B = steer_neutral + ((128 - cmd[1])*5);
			break;
	}
}

void got_cmd(unsigned char *serial_buff, uint8_t len) {
        wdt_reset();
	afproto_get_payload(serial_buff, len, serial_buff, &len);
	handle_cmd(serial_buff, len);
}

int main() {
	unsigned char serial_buff[512];
	unsigned char buff[256];
	uint8_t buff_ndx = 0;
	int16_t getchar_ret;

	// Set CPU to 16Mhz
	CPU_PRESCALE(0);

	//Turn on the Watchdog
	wdt_enable(WDTO_250MS);

	// Start PWM
	pwm_init();

	// Arm esc
	_delay_ms(1000);

	// Init usb serial and wait for connection
	usb_init();
	while(!usb_configured());
	_delay_ms(1000);

	strcpy(buff, "hello");
	uint8_t length;

	// Reset loop (if control program disconnects)
	while(1) {
		// Wait for a control program to connect
		while (!(usb_serial_get_control() & USB_SERIAL_DTR))

		// Reset state 
		buff_ndx = 0;

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
				buff_ndx = 0;
			}
		}
	}
	
	return 0;
}

ISR(WDT_vect)
{
  set_neutral();
}
