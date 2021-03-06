#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/flash.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/usart.h>

#include "paper.h"
#include "eeprom_driver.h"

void eeprom_setup(void)
{
	/* Enable clocks for I2C1 and AFIO. */
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_I2C1EN);

	/* Set alternate functions for the SCL and SDA pins of I2C2. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		      GPIO_I2C1_SCL | GPIO_I2C1_SDA);

	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);

	/* APB1 is running at 36MHz. */
	i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);

	/* 400KHz - I2C Fast Mode */
	//i2c_set_fast_mode(I2C1);
	i2c_set_standard_mode(I2C1);
	// 2MHz/5 = 400 kHz
	// 2MHz/20 = 100 khZ
 	i2c_set_ccr(I2C1, 0x5a);

	/*
	 * fclock for I2C is 36MHz -> cycle time 28ns, rise time for
	 * 400kHz => 300ns and 100kHz => 1000ns; 300ns/28ns = 10;
	 * 1000ns/28ns = 0x24
	 * Incremented by 1 -> 11. 0x0b
	 */
 	i2c_set_trise(I2C1, 0x24);

	i2c_enable_ack(I2C1);
	
	/* If everything is configured -> enable the peripheral. */
	i2c_peripheral_enable(I2C1);
}


void eeprom_write(u8 addr, u8 val){
	u32 reg32 __attribute__((unused));

	i2c_send_start(I2C1);
	/* Waiting for START is send and switched to master mode. */
	while (!((I2C_SR1(I2C1) & I2C_SR1_SB)
		& (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

	/* Send destination address. */
	i2c_send_7bit_address(I2C1, 0x50, I2C_WRITE);
	//i2c_send_data(I2C1, 0x50);
	
	/* Waiting for address is transferred. */
	while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR));
	
	/* Cleaning ADDR condition sequence. */
	reg32 = I2C_SR2(I2C1);
	
	i2c_send_data(I2C1, addr);
	/* Waiting for address is transferred. */
	while (!(I2C_SR1(I2C1) & I2C_SR1_BTF));

	/* Cleaning I2C_SR1_ACK. */
	I2C_CR1(I2C1) &= ~I2C_CR1_ACK;
	
	/* Sending the data. */
	i2c_send_data(I2C1, val);
	while (!(I2C_SR1(I2C1) & I2C_SR1_BTF)); /* Await ByteTransferedFlag. */

	/* Send STOP condition. */
	i2c_send_stop(I2C1);
}

u8 eeprom_read(u8 addr){
	u32 reg32 __attribute__((unused));
	u8 data;
	/* Send START condition. */
	i2c_send_start(I2C1);
	
	/* Waiting for START is send and switched to master mode. */
	while (!((I2C_SR1(I2C1) & I2C_SR1_SB)
		& (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))));

	/* Say to what address we want to talk to. */
	i2c_send_7bit_address(I2C1, 0x50, I2C_WRITE); 
	//i2c_send_data(I2C1, 0x51);
	/* Waiting for address is transferred. */
	while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR));
	
	/* Cleaning ADDR condition sequence. */
	reg32 = I2C_SR2(I2C1);
	
	i2c_send_data(I2C1, addr);
	/* Waiting for address is transferred. */
	while (!(I2C_SR1(I2C1) & I2C_SR1_BTF));
	
	i2c_send_start(I2C1);
	
	/* Waiting for START is send and switched to master mode. */
	while (!((I2C_SR1(I2C1) & I2C_SR1_SB)
		& (I2C_SR2(I2C1) & (I2C_SR2_MSL | I2C_SR2_BUSY))));
	i2c_send_7bit_address(I2C1, 0x50, I2C_READ); 
	/* Waiting for address is transferred. */
	while (!(I2C_SR1(I2C1) & I2C_SR1_ADDR));
	
	/* Cleaning ADDR condition sequence. */
	reg32 = I2C_SR2(I2C1);
	
	data = i2c_get_data(I2C1);
	i2c_send_stop(I2C1);
	
	return data;
}