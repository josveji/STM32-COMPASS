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
 #include <stdio.h>


 #include <libopencm3/cm3/nvic.h>
 #include <libopencm3/cm3/systick.h>
 #include <libopencm3/stm32/rcc.h>
 
 #include <libopencm3-plus/utils/misc.h>
 
 #include <libopencm3-plus/hw-accesories/cm3/clock.h>
 #include <libopencm3-plus/hw-accesories/tft_lcd/i2c-lcd-touch.h>
 #include <libopencm3-plus/hw-accesories/tft_lcd/lcd-serial-touch.h>
 #include <libopencm3-plus/hw-accesories/lcd/lcd-spi.h>
 #include <libopencm3-plus/hw-accesories/sdram_stm32f429idiscovery.h>

 #include <libopencm3/stm32/gpio.h>
 #include <libopencm3/stm32/i2c.h>
 
 #include <libopencm3-plus/newlib/syscall.h>
 #include <libopencm3-plus/newlib/devices/cdcacm.h>


//----- I2C------
 /* ================= CONFIG ================= */
 
 #define QMC_ADDR 0x0D
 
 #define QMC_REG_X_LSB     0x00
 #define QMC_REG_STATUS    0x06
 #define QMC_REG_CONTROL   0x09
 #define QMC_REG_SETRESET  0x0B
 
 /* Hard-iron offsets (tus datos reales) */
 #define OFF_X  400
 #define OFF_Y   66
 #define OFF_Z  100
 #define M_PI 3.14159265358979323846

 #define TIMEOUT 1000000

 static float fx = 0.0f;   // X filtrado
static float fz = 0.0f;   // Z filtrado
static const float ALPHA = 0.01f;  // 0<ALPHA<=1 (más pequeño = más suave)

 
 /* ================= DELAY ================= */
 
  void delay( uint32_t n)
 {
     while (n--) __asm__("nop");
 }
 
 /* ================= USB CDC ================= */
 
  void system_init(void)
 {
     rcc_clock_setup_pll(
         &rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
 
 }
 
  void init_console(void)
 {
     setvbuf(stdout, NULL, _IONBF, 0);
 }
 
 /* ================= I2C SETUP ================= */

  void i2c_setup(void)
 {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_I2C1);
 
     /* PB6=SCL, PB7=SDA */
     gpio_mode_setup(GPIOB, GPIO_MODE_AF,
                     GPIO_PUPD_NONE, GPIO8 | GPIO9);
     gpio_set_output_options(GPIOB, GPIO_OTYPE_OD,
                             GPIO_OSPEED_50MHZ, GPIO8 | GPIO9);
     gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);
 
     i2c_reset(I2C1);
     i2c_peripheral_disable(I2C1);
 
     i2c_set_speed(I2C1, i2c_speed_sm_100k,
                   rcc_apb1_frequency / 1000000);
 
     i2c_peripheral_enable(I2C1);
 }
 
 /* ================= I2C WRITE (timeout) ================= */
 
  int i2c_write_reg_timeout(uint8_t addr, uint8_t reg, uint8_t val)
 {
     uint32_t t;
 
     i2c_send_start(I2C1);
     for (t = TIMEOUT; t; t--)
         if (I2C_SR1(I2C1) & I2C_SR1_SB) break;
     if (!t) goto err;
 
     i2c_send_7bit_address(I2C1, addr, I2C_WRITE);
     for (t = TIMEOUT; t; t--)
         if (I2C_SR1(I2C1) & I2C_SR1_ADDR) break;
     if (!t) goto err;
     (void)I2C_SR2(I2C1);
 
     i2c_send_data(I2C1, reg);
     for (t = TIMEOUT; t; t--)
         if (I2C_SR1(I2C1) & I2C_SR1_BTF) break;
     if (!t) goto err;
 
     i2c_send_data(I2C1, val);
     for (t = TIMEOUT; t; t--)
         if (I2C_SR1(I2C1) & I2C_SR1_BTF) break;
     if (!t) goto err;
 
     i2c_send_stop(I2C1);
     return 0;
 
 err:
     i2c_send_stop(I2C1);
     return -1;
 }
 
 /* ================= I2C READ ================= */
 
  uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
 {
     uint8_t val = 0;
     i2c_transfer7(I2C1, addr, &reg, 1, &val, 1);
     return val;
 }
 
 /* ================= QMC INIT ================= */
 
  void qmc_init(void)
 {
     
     delay(15000000);
 

     while (i2c_write_reg_timeout(QMC_ADDR,
                                 QMC_REG_SETRESET, 0x01) != 0)
         delay(3000000);
 
    
     while (i2c_write_reg_timeout(QMC_ADDR,
                                  QMC_REG_CONTROL, 0x11) != 0)
         delay(3000000);
 }
 
 /* ================= READ XYZ ================= */
 
 

 int qmc_read_xyz(int16_t *x, int16_t *y, int16_t *z)
 {
     uint8_t status = i2c_read_reg(QMC_ADDR, QMC_REG_STATUS);
 
     if (!(status & 0x01))
         return 0; // No hay dato nuevo
 
     uint8_t xl = i2c_read_reg(QMC_ADDR, 0x00);
     uint8_t xh = i2c_read_reg(QMC_ADDR, 0x01);
     uint8_t yl = i2c_read_reg(QMC_ADDR, 0x02);
     uint8_t yh = i2c_read_reg(QMC_ADDR, 0x03);
     uint8_t zl = i2c_read_reg(QMC_ADDR, 0x04);
     uint8_t zh = i2c_read_reg(QMC_ADDR, 0x05);
 
     *x = (int16_t)((xh << 8) | xl);
     *y = (int16_t)((yh << 8) | yl);
     *z = (int16_t)((zh << 8) | zl);
 
     return 1;
 }

 qmc_read_heading(int *heading)
{
    int16_t x, y, z;
    static int initialized = 0;

    if (!qmc_read_xyz(&x, &y, &z))
        return 0;   // Sin dato nuevo

    x -= OFF_X;
    y -= OFF_Y;
    z -= OFF_Z;

    if (!initialized) {
        fx = x;
        fz = z;
        initialized = 1;
    } else {
        fx += ALPHA * ((float)x - fx);
        fz += ALPHA * ((float)z - fz);
    }

    float h = atan2f(fx, fz) * 180.0f / M_PI;
    if (h < 0) h += 360.0f;

    *heading = (int)h;
    return 1;
}
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
 
 void draw_cardinal_points(int north_deg_value){
   int north_deg = north_deg_value; 
   int south_deg = north_deg + 180;
   int east_deg = north_deg - 90;
   int west_deg = north_deg + 90;
 
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
 
   // Drawing angle
   gfx_setCursor(95, 290);
   gfx_setTextColor(LCD_GREEN, LCD_WHITE);
   char buffer[16];
   snprintf(buffer, sizeof(buffer), "%03d", north_deg);
   gfx_puts(buffer);
 
 
 }
 
 
 void clock_setup(void) {
   const uint32_t one_milisecond_rate = 168000;
   /* Base board frequency, set to 168Mhz */
   //rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
 
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

    system_init();
    //init_console();
    i2c_setup();

    qmc_init();


   clock_setup();
  // printf("Antes de sdram_init()\n\r");
   sdram_init();
  // delay(3000000);

   //printf("Despues de sdram_init()\n\r");
   lcd_spi_init();
   //msleep(SLEEP_TIME);
   gfx_init(lcd_draw_pixel, LCD_WIDTH, LCD_HEIGHT);
   gfx_fillScreen(LCD_GREY);
   //msleep(SLEEP_TIME);
   
   // original cardinal point degrees
   //int north_deg = 90; 
   //int south_deg = 270;
   //int east_deg = 0;
   //int west_deg = 180;
 
   gfx_setCursor(0, 0);
   gfx_setTextColor(LCD_BLACK, LCD_WHITE);
   gfx_setTextSize(2);

   int heading;
   int prev_heading = -999;
   int fail_count = 0;


   draw_compass_UI();  // DIBUJAR FONDO UNA SOLA VEZ
   lcd_show_frame();
   
   while (1)
   
   {draw_compass_UI();


      
       if (qmc_read_heading(&heading)) {

           fail_count = 0;

        
           if (heading != prev_heading) {
               draw_cardinal_points(heading);
               lcd_show_frame(); 
               prev_heading = heading;
           }

       } else {
           
           fail_count++;
           if (fail_count > 20) {   
               qmc_init();
               fail_count = 0;
           }
       }
   }
 } 