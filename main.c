// Ethan O'Connor - 4/26/25
// ECE 231 Lab 8.2

// For thermometer, TMP36 on ADC0 UART.
// SSD1306 display for temp output to user
// Button on PD5 selects C, else F. 
// Red LED on PB0 on if temp > TOO_HOT_F

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "SSD1306.h"

// Hot threshold in tenths of °F (so here it is 84 F as my room is a sauna)
#define TOO_HOT_F_TENTHS 840

// For input / output data on these pins
#define BUTTON_PIN         PD5    // active-low
#define LED_PIN            PB0 

// use helper to send temp and the unit to the serial monitor
static void uart_print_tenths(int16_t t, char unit) {
    char buf[8];
    int16_t whole = t / 10;
    int16_t frac  = t % 10; // for tenths
    if (frac < 0) frac = -frac;
    // format example: "72.3F\n"
    snprintf(buf, sizeof(buf), "%d.%d%c\n", whole, frac, unit);
    send_string(buf);
}

int main(void) {
    // Define all variables needed
    uint16_t adc; //
    uint32_t mv; //millivolts
    int16_t tempC_t, tempF_t;
    char unit;

    // initialize the peripherals
    adc_init();      // ADC on PC0/A0
    uart_init();     // 9600 baud, 8N1
    OLED_Init();     // SSD1306 via I2C on PC4=SDA, PC5=SCL

    // configure button on PD5 as input, with pull-up
    DDRD  &= ~(1 << BUTTON_PIN);
    PORTD |=  (1 << BUTTON_PIN);

    // configure LED on PB0 as output, start off
    DDRB  |=  (1 << LED_PIN);
    PORTB &= ~(1 << LED_PIN);

    while (1) {
        // - read, convert TMP36 on ADC0 -
        adc = get_adc();
        // convert ADC to millivolts (from 0–5000 mV)
        mv  = (uint32_t)adc * 5000UL / 1024;
        // Celsius*10 = (mV − 500)
        tempC_t = (mv > 500 ? (mv - 500) : 0);
        // Fahrenheit*10 = (C×10)*9/5 + 320
        tempF_t = tempC_t * 9 / 5 + 320;

        // - choose unit based on PD5 (button active-low) -
        unit = (PIND & (1 << BUTTON_PIN)) ? 'F' : 'C';

        // - serial out -
        if (unit == 'C')
            uart_print_tenths(tempC_t, 'C');
        else
            uart_print_tenths(tempF_t, 'F');

        // - For OLED output -
        OLED_Clear();
        OLED_SetCursor(0, 0);
        if (unit == 'C') {
            OLED_DisplayNumber(C_DECIMAL_U8, tempC_t / 10, 3);
            OLED_DisplayChar('.');
            OLED_DisplayNumber(C_DECIMAL_U8, tempC_t % 10, 1);
            OLED_DisplayChar('C');
        } else {
            OLED_DisplayNumber(C_DECIMAL_U8, tempF_t / 10, 3);
            OLED_DisplayChar('.');
            OLED_DisplayNumber(C_DECIMAL_U8, tempF_t % 10, 1);
            OLED_DisplayChar('F');
        }

        // - For too hot LED turn-on -
        if (tempF_t > TOO_HOT_F_TENTHS) {
            PORTB |=  (1 << LED_PIN);
        } else {
            PORTB &= ~(1 << LED_PIN);
        }

        _delay_ms(500); // wait for half a sec per each poll
    }

    return 0;
}
