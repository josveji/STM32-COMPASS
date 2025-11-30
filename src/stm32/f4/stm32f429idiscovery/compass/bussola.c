/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <stdint.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

#include <libopencm3-plus/utils/misc.h>

#include <libopencm3-plus/hw-accesories/cm3/clock.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/i2c-lcd-touch.h>
#include <libopencm3-plus/hw-accesories/tft_lcd/lcd-serial-touch.h>
#include <libopencm3-plus/hw-accesories/lcd/lcd-spi.h>
#include <libopencm3-plus/hw-accesories/sdram_stm32f429idiscovery.h>

#define SLEEP_TIME 2000

// Draw arrow
void draw_arrow_center(int16_t ax, int16_t ay, int16_t bx, int16_t by, int16_t cx, int16_t cy, uint16_t color){
  // Draw triangle0
  gfx_fillTriangle(ax, ay, cx-30, cy, cx+30, cy, color);
  // Draw triangle1
  gfx_fillTriangle(bx, by, cx-30, cy, cx, cy, color);
  // Draw triangle2
  gfx_fillTriangle(bx+84, by, cx+30, cy, cx, cy, color);

  // Draw left line
  gfx_drawLine(ax, ay, bx, by, LCD_BLACK);
  gfx_drawLine(ax-1, ay, bx-1, by+1, LCD_BLACK);
  gfx_drawLine(ax-2, ay, bx-2, by+1, LCD_BLACK);
  gfx_drawLine(ax-3, ay, bx-3, by+1, LCD_BLACK);

  // Draw middle line
  gfx_drawLine(ax, ay-2, cx, cy, LCD_BLACK);
  gfx_drawLine(ax-1, ay-1, cx-1, cy, LCD_BLACK);
  gfx_drawLine(ax+1, ay-1, cx+1, cy, LCD_BLACK);
 
  // Draw right line
  gfx_drawLine(ax, ay, bx+84, by, LCD_BLACK);
  gfx_drawLine(ax+1, ay, bx+85, by+1, LCD_BLACK);
  gfx_drawLine(ax+2, ay, bx+86, by+1, LCD_BLACK);
  gfx_drawLine(ax+3, ay, bx+87, by+1, LCD_BLACK);

  // Draw down left line
  gfx_drawLine(bx, by, cx, cy, LCD_BLACK);
  gfx_drawLine(bx+1, by, cx, cy+1, LCD_BLACK);
  gfx_drawLine(bx+2, by, cx, cy+2, LCD_BLACK);
  gfx_drawLine(bx+3, by, cx, cy+3, LCD_BLACK);

  // Draw down right line
  gfx_drawLine(bx+84, by, cx, cy,   LCD_BLACK);
  gfx_drawLine(bx+83, by, cx, cy+1, LCD_BLACK);
  gfx_drawLine(bx+82, by, cx, cy+2, LCD_BLACK);
  gfx_drawLine(bx+81, by, cx, cy+3, LCD_BLACK);
}


void draw_compass_UI(void){
  // Set display color
  gfx_fillScreen(LCD_WHITE);

  // BUSSOLA!!
  gfx_setTextColor(LCD_BLACK, LCD_WHITE);
  gfx_setCursor(60, 10);
  gfx_puts("BUSSOLA!");

  // Made by Josue & Gabriel
  gfx_setTextColor(LCD_YELLOW, LCD_WHITE);
  gfx_setCursor(20, 40);
  gfx_setTextSize(1);
  gfx_puts("Fatto da Josue & Gabriel");

  gfx_setTextSize(2);

  // Draw centered cross
  gfx_drawFastVLine(120, 55, 210, LCD_GREEN);
  gfx_drawFastHLine(15, 160, 210, LCD_GREEN);

  // Draw centered circles
  gfx_drawCircle(120, 160, 85, LCD_BLACK);
  gfx_drawCircle(120, 160, 100, LCD_BLACK);

  // Draw arrows
  draw_arrow_center(120, 105, 78, 200, 120, 170, LCD_GREEN);

  // Draw angle symbol
  gfx_drawCircle(150, 290, 2, LCD_GREEN);
  

}

void clock_setup(void) {
  const uint32_t one_milisecond_rate = 168000;
  /* Base board frequency, set to 168Mhz */
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);

  /* clock rate / 168000 to get 1mS interrupt rate */
  systick_set_reload(one_milisecond_rate);
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_counter_enable();

  /* this done last */
  systick_interrupt_enable();
}

/*
 * This is our example, the heavy lifing is actually in lcd-spi.c but
 * this drives that code.
 */
int main(void) {
  clock_setup();
  sdram_init();
  lcd_spi_init();
  msleep(SLEEP_TIME);
  gfx_init(lcd_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
  gfx_fillScreen(LCD_GREY);
  //msleep(SLEEP_TIME);
  
  // original cardinal point degrees
  int north_deg = 90; 
  int south_deg = 270;
  int east_deg = 0;
  int west_deg = 180;

  gfx_setCursor(0, 0);
  gfx_setTextColor(LCD_BLACK, LCD_WHITE);
  gfx_setTextSize(2);

  while (1) {
    
    draw_compass_UI();

    // gfx_drawChar(x, y, "N", LCD_YELLOW, LCD_BLACK, 2);
    // North
    gfx_drawChar(120 + (cos(degrees_to_radians(north_deg)) * 100),
                 160 - (sin(degrees_to_radians(north_deg)) * 100), 78, LCD_GREEN, LCD_WHITE, 2);

    // South
    gfx_drawChar(120 + (cos(degrees_to_radians(south_deg)) * 100),
                 160 - (sin(degrees_to_radians(south_deg)) * 100), 83, LCD_BLACK, LCD_WHITE, 2);

    // East
    gfx_drawChar(120 + (cos(degrees_to_radians(east_deg)) * 100),
                 160 - (sin(degrees_to_radians(east_deg)) * 100), 69, LCD_BLACK, LCD_WHITE, 2);

    // West
    gfx_drawChar(120 + (cos(degrees_to_radians(west_deg)) * 100),
                 160 - (sin(degrees_to_radians(west_deg)) * 100), 87, LCD_BLACK, LCD_WHITE, 2);


    // Recalculating new positon for cardinals
    north_deg = (north_deg + 3) % 360;
    south_deg = (south_deg + 3) % 360;
    east_deg = (east_deg + 3) % 360;
    west_deg = (west_deg + 3) % 360;

    //gfx_drawCircle(150, 290, 2, LCD_GREEN);
    gfx_setCursor(95, 290);
    gfx_setTextColor(LCD_GREEN, LCD_WHITE);
    gfx_puts("360");

    lcd_show_frame();
  }
}

