/*
 * STM32F429 + QMC5883L
 * Brújula completa (hard-iron corregido)
 */

 #include <libopencm3/stm32/rcc.h>
 #include <libopencm3/stm32/gpio.h>
 #include <libopencm3/stm32/i2c.h>
 
 #include <libopencm3-plus/newlib/syscall.h>
 #include <libopencm3-plus/newlib/devices/cdcacm.h>
 
 #include <stdio.h>
 #include <stdint.h>
 #include <math.h>
 
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
 
 static void delay(volatile uint32_t n)
 {
     while (n--) __asm__("nop");
 }
 
 /* ================= USB CDC ================= */
 
 static void system_init(void)
 {
     rcc_clock_setup_pll(
         &rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
 
     devoptab_list[0] = &dotab_cdcacm;
     devoptab_list[1] = &dotab_cdcacm;
     devoptab_list[2] = &dotab_cdcacm;
 
     cdcacm_f429_init();
 }
 
 static void init_console(void)
 {
     setvbuf(stdout, NULL, _IONBF, 0);
 }
 
 /* ================= I2C SETUP ================= */
 
 static void i2c_setup(void)
 {
     rcc_periph_clock_enable(RCC_GPIOB);
     rcc_periph_clock_enable(RCC_I2C1);
 
     /* PB6=SCL, PB7=SDA */
     gpio_mode_setup(GPIOB, GPIO_MODE_AF,
                     GPIO_PUPD_NONE, GPIO6 | GPIO7);
     gpio_set_output_options(GPIOB, GPIO_OTYPE_OD,
                             GPIO_OSPEED_50MHZ, GPIO6 | GPIO7);
     gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);
 
     i2c_reset(I2C1);
     i2c_peripheral_disable(I2C1);
 
     i2c_set_speed(I2C1, i2c_speed_sm_100k,
                   rcc_apb1_frequency / 1000000);
 
     i2c_peripheral_enable(I2C1);
 }
 
 /* ================= I2C WRITE (timeout) ================= */
 
 static int i2c_write_reg_timeout(uint8_t addr, uint8_t reg, uint8_t val)
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
 
 static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
 {
     uint8_t val = 0;
     i2c_transfer7(I2C1, addr, &reg, 1, &val, 1);
     return val;
 }
 
 /* ================= QMC INIT ================= */
 
 static void qmc_init(void)
 {
     printf("Esperando QMC power-up...\n\r");
     delay(15000000);
 
     printf("Reset QMC...\n\r");
     while (i2c_write_reg_timeout(QMC_ADDR,
                                 QMC_REG_SETRESET, 0x01) != 0)
         delay(3000000);
 
     printf("Config QMC...\n\r");
     /* OSR=128, RNG=2G, ODR=10Hz, Continuous */
     while (i2c_write_reg_timeout(QMC_ADDR,
                                  QMC_REG_CONTROL, 0x11) != 0)
         delay(3000000);
 }
 
 /* ================= READ XYZ ================= */
 
 static void qmc_read_xyz(int16_t *x, int16_t *y, int16_t *z)
 {
     uint8_t status;
 
     /* Esperar DRDY */
     do {
         status = i2c_read_reg(QMC_ADDR, QMC_REG_STATUS);
     } while (!(status & 0x01));
 
     uint8_t xl = i2c_read_reg(QMC_ADDR, 0x00);
     uint8_t xh = i2c_read_reg(QMC_ADDR, 0x01);
     uint8_t yl = i2c_read_reg(QMC_ADDR, 0x02);
     uint8_t yh = i2c_read_reg(QMC_ADDR, 0x03);
     uint8_t zl = i2c_read_reg(QMC_ADDR, 0x04);
     uint8_t zh = i2c_read_reg(QMC_ADDR, 0x05);
 
     *x = (int16_t)((xh << 8) | xl);
     *y = (int16_t)((yh << 8) | yl);
     *z = (int16_t)((zh << 8) | zl);
 }
 
 /* ================= MAIN ================= */
 
 int main(void)
 {
     system_init();
     init_console();
     i2c_setup();
 
     printf("\n\r=== BRUJULA QMC5883L ===\n\r");
 
     qmc_init();
     printf("Sensor listo\n\r");
 
     while (1) {
         int16_t x, y, z;
    
 
         qmc_read_xyz(&x, &y, &z);
 
         /* Hard-iron correction */
         x -= OFF_X;
         y -= OFF_Y;
         z -= OFF_Z;

         /* Filtro exponencial sobre X y Z */
fx = fx + ALPHA * ((float)x - fx);
fz = fz + ALPHA * ((float)z - fz);

/* Heading usando X,Z filtrados */
float heading = atan2f(fx, fz) * 180.0f / M_PI;
if (heading < 0) heading += 360.0f;

 
         /* Heading (plano X-Z) */
         heading = atan2((float)fx, (float)fz) * 180.0f / M_PI;
         if (heading < 0) heading += 360.0f;
 
         printf("X=%d  Z=%d  Heading=%.2f°\n\r",
                x, z, heading);
 
         delay(3000000);
     }
 }
 
