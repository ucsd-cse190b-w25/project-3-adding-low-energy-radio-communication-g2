#include <stm32l475xx.h>

void i2c_init() {
	if (I2C2->CR1 & I2C_CR1_PE) return;

    // Enable GPIOB and I2C2 clocks
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;

    // Configure PB10 (SCL) and PB11 (SDA) as alternate function
    GPIOB->MODER &= ~GPIO_MODER_MODE10 & ~GPIO_MODER_MODE11; // Clear mode bits
    GPIOB->MODER |= GPIO_MODER_MODE10_1 | GPIO_MODER_MODE11_1;  // Set alternate function mode

    GPIOB->OTYPER |= GPIO_OTYPER_OT10 | GPIO_OTYPER_OT11;  // Open-drain mode (needed for I2C)

    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD10 | GPIO_PUPDR_PUPD11);  // Clear pull-up/down register
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPD10_0 | GPIO_PUPDR_PUPD11_0);  // Pull-up (default 1 from floating)

    GPIOB->AFR[1] &= ~(GPIO_AFRH_AFSEL10 | GPIO_AFRH_AFSEL11);
    GPIOB->AFR[1] |=  GPIO_AFRH_AFSEL10_2 | GPIO_AFRH_AFSEL11_2;

    // Reset I2C2
    I2C2->CR1 &= ~I2C_CR1_PE;
    I2C2->CR1 |= I2C_CR1_SWRST;
    I2C2->CR1 &= ~I2C_CR1_SWRST;

    // SYSCLK is 4 MHz
    // PCLK1 will be 4 MHz if we configure no divides between SYSCLK and HCLK and b/t HCLK and PCLK1

    // configure HCLK and PCLK1 to have no divides
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV1;

    // We want 25 kHz
    // tSCL = 1 / 25 kHz = 40 us
    // We'll make tLow and tHigh symmetrical
    // tLow = 20 us
    // tHigh = 20 us
    // This satisfies the minimum requirements for I2C standard mode
    // (tLow > 4.7 us, tHigh > 4.0 us)
    // tFilter = 260 ns (analog filter only)
    // tI2CCLK = 1 / PCLK1 = 1 / 4 MHz = 250 ns
    // PRESC = 0 (keep at 250 ns)
    // SCLL = (tLow - tFilter) / tI2CCLK = round((20 us - 260 ns) / 250 ns) = 79
    // SCLH = tHigh / tI2CCLK = 20 us / 250 ns = 80
    I2C2->TIMINGR &= 0;
    I2C2->TIMINGR |= 0 << I2C_TIMINGR_PRESC_Pos;
    I2C2->TIMINGR |= 79 << I2C_TIMINGR_SCLL_Pos;
    I2C2->TIMINGR |= 80 << I2C_TIMINGR_SCLH_Pos;
    I2C2->TIMINGR |= 0 << I2C_TIMINGR_SDADEL_Pos;
    I2C2->TIMINGR |= 2 << I2C_TIMINGR_SCLDEL_Pos;

    // Enable I2C2
    I2C2->CR1 |= I2C_CR1_PE;
}

uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len) {
    uint32_t timeout = 1000000;  // Simple timeout mechanism

    // Ensure the bus is free
    while ((I2C2->ISR & I2C_ISR_BUSY) && --timeout);
    if (!timeout) return 1;  // Timeout error

    if (dir) {  // read
    	// Start condition with address
    	I2C2->CR2 = address << 1;  // address, assume 7-bit mode. << 1 b/c LSB is DC
    	I2C2->CR2 |= 0;  // transfer direction. 0 for write, 1 for read
    	I2C2->CR2 |= 1 << I2C_CR2_NBYTES_Pos;  // len = number of bytes to R/W
	    I2C2->CR2 |= I2C_CR2_START;  // set start flag on

	    // Transmit/Receive Data
	    for (uint8_t i = 0; i < len; i++) {
	    	timeout = 1000000;

    	    if (i == 0) {
    	    	while (!(I2C2->ISR & I2C_ISR_TXIS) && --timeout);
    	        if (!timeout) return 2;
    	        I2C2->TXDR = data[i];

    	        while (!(I2C2->ISR & I2C_ISR_TC) && --timeout);
    	       	if (!timeout) return 2;

    	        I2C2->CR2 = address << 1;  // address, assume 7-bit mode. << 1 b/c LSB is DC
    	       	I2C2->CR2 |= I2C_CR2_RD_WRN;  // transfer direction. 0 for write, 1 for read
    	       	I2C2->CR2 |= (len - 1) << I2C_CR2_NBYTES_Pos;  // len = number of bytes to R/W
    	       	I2C2->CR2 |= I2C_CR2_AUTOEND;  // auto transmit STOP after # of bytes
    	       	I2C2->CR2 |= I2C_CR2_START;  // set start flag on

    	       	continue;
    	    }

    	    while (!(I2C2->ISR & I2C_ISR_RXNE) && --timeout);
    	    if (!timeout) return 3;  // Timeout error
    	    data[i] = I2C2->RXDR;
	    }
    } else {  // write
    	// Start condition with address
    	I2C2->CR2 = address << 1;  // address, assume 7-bit mode. << 1 b/c LSB is DC
    	I2C2->CR2 |= 0;  // transfer direction. 0 for write, 1 for read
    	I2C2->CR2 |= len << I2C_CR2_NBYTES_Pos;  // len = number of bytes to R/W
    	I2C2->CR2 |= I2C_CR2_AUTOEND;
	    I2C2->CR2 |= I2C_CR2_START;  // set start flag on

	    // Transmit/Receive Data
	    for (uint8_t i = 0; i < len; i++) {
	    	timeout = 1000000;

            while (!(I2C2->ISR & I2C_ISR_TXIS) && --timeout);
   	        if (!timeout) return 2;  // Timeout error
    	    I2C2->TXDR = data[i];
	    }
    }

    while (!(I2C2->ISR & I2C_ISR_STOPF));  // Wait for STOP condition
    I2C2->ICR |= I2C_ICR_STOPCF;  // Clear STOP flag

    return 0;  // Success
}
