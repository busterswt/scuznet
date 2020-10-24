/*
 * Copyright (C) 2019 saybur
 * 
 * This file is part of scuznet.
 * 
 * scuznet is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * scuznet is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with scuznet.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <util/delay.h>
#include "../config.h"

#define led_on()              LED_PORT.DIR |= LED_PIN;
#define led_off()             LED_PORT.DIR &= ~LED_PIN;

#ifdef PHY_PORT_DATA_IN_OE
	#define doe_off()         PHY_PORT_DOE.OUT   |=  PHY_PIN_DOE
	#define doe_on()          PHY_PORT_DOE.OUT   &= ~PHY_PIN_DOE
#else
	#define doe_off()
	#define doe_on()
#endif

#ifdef PHY_PORT_DATA_IN_CLOCK
	#define dclk_rise()       PHY_PORT_DCLK.OUT  |=  PHY_PIN_DCLK
	#define dclk_fall()       PHY_PORT_DCLK.OUT  &= ~PHY_PIN_DCLK
#else
	#define dclk_rise()
	#define dclk_fall()
#endif

static void led_flash(uint8_t l, uint8_t s, uint8_t x)
{
	while (1)
	{
		for (uint8_t i = 0; i < l; i++)
		{
			led_on();
			_delay_ms(500);
			led_off();
			_delay_ms(500);
		}
		_delay_ms(1500);
		for (uint8_t i = 0; i < s; i++)
		{
			led_on();
			_delay_ms(200);
			led_off();
			_delay_ms(200);
		}
		_delay_ms(1500);
		for (uint8_t i = 0; i < x; i++)
		{
			led_on();
			_delay_ms(500);
			led_off();
			_delay_ms(500);
		}
		_delay_ms(4000);
	}
}

/*
 * Returns the first set bit in the given value, LSB first, where '1' is the
 * first, '2' is the second, etc. If none is found 9 is returned.
 */
static uint8_t test_set_bits(uint8_t value)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if ((1 << i) & value)
		{
			return i + 1;
		}
	}
	return 9;
}

static void init_disable_jtag(void)
{
	__asm__ __volatile__(
		"ldi r24, %0"		"\n\t"
		"out %1, r24"		"\n\t"
		"ldi r24, %2"		"\n\t"
		"sts %3, r24"		"\n\t"
		:
		: "M" (CCP_IOREG_gc), "i" (&CCP),
		  "M" (MCU_JTAGD_bm), "i" (&(MCU.MCUCR))
		: "r24"
		);
}

static void phy_init(void)
{
	#ifdef PHY_PORT_DATA_IN_ACKEN
		PHY_PORT_ACKEN.OUT &= ~PHY_PIN_ACKEN;
		PHY_PORT_ACKEN.DIR |=  PHY_PIN_ACKEN;
	#endif
	#ifdef PHY_PORT_DATA_IN_CLOCK
		PHY_PORT_DCLK.OUT  &= ~PHY_PIN_DCLK;
		PHY_PORT_DCLK.DIR  |=  PHY_PIN_DCLK;
	#endif

	#ifdef PHY_PORT_DATA_IN_OE
		doe_off();
		PHY_PORT_DOE.DIR |= PHY_PIN_DOE;
		PHY_PORT_DATA_IN.PIN0CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN1CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN2CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN3CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN4CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN5CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN6CTRL |= PORT_OPC_PULLUP_gc;
		PHY_PORT_DATA_IN.PIN7CTRL |= PORT_OPC_PULLUP_gc;
	#endif

	#ifdef PHY_PORT_DATA_IN_INVERT
		PHY_PORT_DATA_IN.PIN0CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN1CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN2CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN3CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN4CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN5CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN6CTRL |= PORT_INVEN_bm;
		PHY_PORT_DATA_IN.PIN7CTRL |= PORT_INVEN_bm;
	#endif
}

#ifdef PHY_PORT_DATA_IN_REVERSED
static uint8_t phy_reverse_table[256] = {
	0, 128, 64, 192, 32, 160, 96, 224, 16, 144, 80, 208, 48, 176, 112, 240, 8, 
	136, 72, 200, 40, 168, 104, 232, 24, 152, 88, 216, 56, 184, 120, 248, 4, 
	132, 68, 196, 36, 164, 100, 228, 20, 148, 84, 212, 52, 180, 116, 244, 12, 
	140, 76, 204, 44, 172, 108, 236, 28, 156, 92, 220, 60, 188, 124, 252, 2, 
	130, 66, 194, 34, 162, 98, 226, 18, 146, 82, 210, 50, 178, 114, 242, 10, 
	138, 74, 202, 42, 170, 106, 234, 26, 154, 90, 218, 58, 186, 122, 250, 6, 
	134, 70, 198, 38, 166, 102, 230, 22, 150, 86, 214, 54, 182, 118, 246, 14, 
	142, 78, 206, 46, 174, 110, 238, 30, 158, 94, 222, 62, 190, 126, 254, 1, 
	129, 65, 193, 33, 161, 97, 225, 17, 145, 81, 209, 49, 177, 113, 241, 9, 
	137, 73, 201, 41, 169, 105, 233, 25, 153, 89, 217, 57, 185, 121, 249, 5, 
	133, 69, 197, 37, 165, 101, 229, 21, 149, 85, 213, 53, 181, 117, 245, 13, 
	141, 77, 205, 45, 173, 109, 237, 29, 157, 93, 221, 61, 189, 125, 253, 3, 
	131, 67, 195, 35, 163, 99, 227, 19, 147, 83, 211, 51, 179, 115, 243, 11, 
	139, 75, 203, 43, 171, 107, 235, 27, 155, 91, 219, 59, 187, 123, 251, 7, 
	135, 71, 199, 39, 167, 103, 231, 23, 151, 87, 215, 55, 183, 119, 247, 15, 
	143, 79, 207, 47, 175, 111, 239, 31, 159, 95, 223, 63, 191, 127, 255
};
#endif
static inline __attribute__((always_inline)) uint8_t phy_read(void)
{
	uint8_t raw;

	#if defined(PHY_PORT_DATA_IN_OE) || defined(PHY_PORT_DATA_IN_CLOCK)
		dclk_rise();
		doe_on();
		dclk_fall();
		raw = PHY_PORT_DATA_IN.IN;
		doe_off();
	#else
		raw = PHY_PORT_DATA_IN.IN;
	#endif

	#ifdef PHY_PORT_DATA_IN_REVERSED
		return phy_reverse_table[raw];
	#else
		return raw;
	#endif
}

#ifdef ENC_ENABLED
static void enc_init(void)
{
	ENC_PORT.OUTCLR = ENC_PIN_XCK;
	ENC_PORT.OUTSET = ENC_PIN_TX | ENC_PIN_CS | ENC_PIN_RST;
	ENC_PORT.DIRSET = ENC_PIN_XCK | ENC_PIN_TX | ENC_PIN_CS | ENC_PIN_RST;
	ENC_RX_PINCTRL |= PORT_OPC_PULLUP_gc;
	ENC_INT_PINCTRL |= PORT_INVEN_bm;

	_delay_ms(1);
	ENC_PORT.OUTCLR = ENC_PIN_RST;
	_delay_us(50);
	ENC_PORT.OUTSET = ENC_PIN_RST;
	_delay_ms(1);

	ENC_USART.BAUDCTRLA = ENC_USART_BAUDCTRL;
	ENC_USART.BAUDCTRLB = 0;
	ENC_USART.CTRLC = USART_CMODE_MSPI_gc;
	ENC_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}
#endif

#ifdef HDD_ENABLED
static void mem_init(void)
{
	MEM_PORT.OUTCLR = MEM_PIN_XCK;
	MEM_PORT.OUTSET = MEM_PIN_TX | MEM_PIN_CS;
	MEM_PORT.DIRSET = MEM_PIN_XCK | MEM_PIN_TX | MEM_PIN_CS;
	MEM_PINCTRL_RX |= PORT_OPC_PULLUP_gc;
}
#endif

static void phy_check(void)
{
	// verify all PHY output pins are low to start
	uint8_t v = PHY_PORT_DATA_OUT.IN;
	if (v)
	{
		uint8_t m = 1;
		for (uint8_t i = 1; i <= 8; i++)
		{
			if (v & m) led_flash(1, i, 0);
			m = m << 1;
		}
	}

	// check for data line shorts
	PHY_PORT_DATA_OUT.DIR = 0x00;
	PHY_PORT_DATA_OUT.OUT = 0xFF;
	uint8_t mask = 1;
	for (uint8_t i = 1; i <= 8; i++)
	{
		// drive the line, and wait a moment for it to stabilize
		PHY_PORT_DATA_OUT.DIR = mask;
		_delay_us(1);

		// test the output port
		uint8_t read = PHY_PORT_DATA_OUT.IN;
		if (read != mask)
		{
			PHY_PORT_DATA_OUT.DIR = 0x00;
			read &= ~mask;
			led_flash(2, i, test_set_bits(read));
		}

		// now the reading port
		read = phy_read();
		if (read != mask)
		{
			PHY_PORT_DATA_OUT.DIR = 0x00;
			read &= ~mask;
			led_flash(3, i, test_set_bits(read));
		}

		PHY_PORT_DATA_OUT.DIR = 0x00;
		mask = mask << 1;
	}
}

int main(void)
{
	// do bare bones board setup
	init_disable_jtag();
	PORTCFG.VPCTRLA = DEV_VPORT0_CFG | DEV_VPORT1_CFG;
	PORTCFG.VPCTRLB = DEV_VPORT2_CFG | DEV_VPORT3_CFG;
	LED_PORT.OUT &= ~LED_PIN;
	phy_init();
	#ifdef ENC_ENABLED
		enc_init();
	#endif
	#ifdef HDD_ENABLED
		mem_init();
	#endif

	// working board showing errors without this,
	// due to termination power rising too slowly?
	_delay_ms(100);

	phy_check();

	// all tests passed, pulse the LED
	uint8_t x = 0;
	while (1)
	{
		uint8_t y = x;
		if (y > 16) y = 32 - y;
		for (uint16_t i = 0; i < 500; i++)
		{
			led_on();
			for (uint8_t j = 0; j < y; j++) _delay_us(1);
			led_off();
			for (uint8_t j = 0; j < 16 - y; j++) _delay_us(1);
		}
		x++;
		if (x > 32) x = 0;
	}
	return 0;
}