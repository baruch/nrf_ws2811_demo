#include <stdint.h>
#include <stdio.h>

#include "interrupt.h"
#include "pwr_clk_mgmt.h"
#include "gpio.h"
#include "uart.h"
#include "delay.h"

#define LED_PIN GPIO_PIN_ID_P1_4
#define LED_PIN_BIT P1_SB_D4

#define XBUFLEN 32
#define RBUFLEN 4

/* You might want to specify idata, pdata or xdata for the buffers */
static unsigned char __pdata rbuf[RBUFLEN], xbuf[XBUFLEN];
static unsigned char rcnt, xcnt, rpos, xpos;
static __bit busy;

void ser_init(void)
{
   rcnt = xcnt = rpos = xpos = 0;  /* init buffers */
   busy = 0;
   
	// Setup UART pins
	gpio_pin_configure(GPIO_PIN_ID_FUNC_RXD,
			GPIO_PIN_CONFIG_OPTION_DIR_INPUT |
			GPIO_PIN_CONFIG_OPTION_PIN_MODE_INPUT_BUFFER_ON_NO_RESISTORS);

	gpio_pin_configure(GPIO_PIN_ID_FUNC_TXD,
			GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
			GPIO_PIN_CONFIG_OPTION_OUTPUT_VAL_SET |
			GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_NORMAL_DRIVE_STRENGTH);

	uart_configure_8_n_1_38400();
	uart_rx_disable();

}

interrupt_isr_uart()
{
   if (S0CON_SB_RI0) {
           S0CON_SB_RI0 = 0;
           /* don't overwrite chars already in buffer */
           if (rcnt < RBUFLEN)
                   rbuf [(unsigned char)(rpos+rcnt++) % RBUFLEN] = S0BUF;
   }
   if (S0CON_SB_TI0) {
           S0CON_SB_TI0 = 0;
           if (busy = xcnt) {   /* Assignment, _not_ comparison! */
                   xcnt--;
                   S0BUF = xbuf [xpos++];
                   if (xpos >= XBUFLEN)
                           xpos = 0;
           }
   }
}

void
putchar (char c)
{
   while (xcnt >= XBUFLEN) /* wait for room in buffer */
           ;
   interrupt_control_uart_disable();
   if (busy) {
           xbuf[(unsigned char)(xpos+xcnt++) % XBUFLEN] = c;
   } else {
           S0BUF = c;
           busy = 1;
   }
   interrupt_control_uart_enable();
}

char
getchar (void)
{
   unsigned char c;
   while (!rcnt)   /* wait for character */
           ;
   interrupt_control_uart_disable();
   rcnt--;
   c = rbuf [rpos++];
   if (rpos >= RBUFLEN)
           rpos = 0;
   interrupt_control_uart_enable();
   return (c);
}

void putuint8(uint8_t val)
{
	uint8_t tmp_val = val;
	uint8_t factor = 1;
	uint8_t num_digits = 1;

	while (tmp_val > 10) {
		tmp_val /= 10;
		factor *= 10;
		num_digits++;
	}

	for (; num_digits; num_digits--) {
		putchar('0' + tmp_val);
		val -= tmp_val * factor;
		factor /= 10;
		tmp_val = val / factor;
	}
}

inline void rgb_short(void)
{
		nop(); nop();
		nop(); nop();
}

inline void rgb_long(void)
{
	rgb_short();
	rgb_short();
	rgb_short();
	rgb_short();
}

inline void rgb_mid(void)
{
	rgb_short();
	rgb_short();
	nop();
}

inline void rgb_bit(uint8_t bit)
{
	if (bit & 1) {
		gpio_pin_val_sbit_set(LED_PIN_BIT);
		rgb_mid();
		gpio_pin_val_sbit_clear(LED_PIN_BIT);
		rgb_mid();
	} else {
		gpio_pin_val_sbit_set(LED_PIN_BIT);
		rgb_short();
		gpio_pin_val_sbit_clear(LED_PIN_BIT);
		rgb_long();
	}
}

inline void rgb_byte(uint8_t data)
{
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
	data >>= 1;
	rgb_bit(data);
}

inline void rgb_set(uint8_t red, uint8_t green, uint8_t blue)
{
	rgb_byte(red);
	rgb_byte(green);
	rgb_byte(blue);
}

void main()
{
	ser_init();

	// Setup pin P0.0 for IR receiver
	gpio_pin_configure(LED_PIN,
			GPIO_PIN_CONFIG_OPTION_DIR_OUTPUT |
			GPIO_PIN_CONFIG_OPTION_PIN_MODE_OUTPUT_BUFFER_HIGH_DRIVE_STRENGTH);

	pwr_clk_mgmt_clklf_configure(PWR_CLK_MGMT_CLKLF_CONFIG_OPTION_CLK_SRC_RCOSC32K);
	pwr_clk_mgmt_wait_until_clklf_is_ready();

	//puts("Starting up\r\n");

	while(1)
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;

		for (r = 0; r < 200; r += 10) {
			rgb_set(r, 0, 0);
			delay_s(1);
		}
		for (r = 0; r < 200; r += 10) {
			rgb_set(0, r, 0);
			delay_s(1);
		}

		for (r = 0; r < 200; r += 10) {
			rgb_set(0, 0, r);
			delay_s(1);
		}

		for (r = 0; r < 200; r += 10) {
			for (g = 0; g < 200; g += 10) {
				for (b = 0; b < 200; b += 10) {
					rgb_set(r, g, b);
					delay_s(1);
				}
			}
		}

	
	}
}
