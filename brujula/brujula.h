#ifndef Brujula_H
#define Brujula_H

#include <stdint.h>

/* Sistema */
void system_init(void);
void init_console(void);
void i2c_setup(void);
void delay(uint32_t n);

/* QMC5883L */
void qmc_init(void);
void qmc_read_xyz(int16_t *x, int16_t *y, int16_t *z);
float qmc_read_heading(void);

#endif /* Brujula_H */
