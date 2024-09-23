#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc24.h"


void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg) {

    regCfg &= 0x0FFF;

    // data to be sent
    uint16_t data_to_send = (regCfg << 4);

    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)&data_to_send,
        .rx_buf = 0,
        .len = 2,
        .bits_per_word = 16,
    };

    //send SPI message
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0) {
        
        printf("**Failed to configure control register on ADC24 Click**\n.");
    }
}

void init_adc24(int spi_fd) {

    /* send 2 frames of dummy data keeping the DIN line high
     so that the ADC is placed into the correct operating mode 
     when control register configuration is sent afterwards.*/
    

    uint16_t dummy_data = DUMMY_HIGH;

    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)&dummy_data,
        .rx_buf = 0,
        .len = 2,
        .bits_per_word = 16,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0) {
            printf("**Sending dummy data while initialising failed**\n");
        }
}

void read_adc24_data(int spi_fd, uint16_t *data) {

    uint16_t dummy_data = DUMMY_LOW;

    //buffer for receiving data
    uint16_t rx_buf;

    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)&dummy_data,
        .rx_buf = (unsigned long)&rx_buf,
        .len = 2,
        .bits_per_word = 16,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), spi_transfer) < 0) {
        printf("**Reading sampled data failed***\n");
        return;
    }

    *data = rx_buf;
    
}


uint16_t get_voltage_adc24(int spi_fd, int ch) {

    // prepare configuration for control register with the given channel
    uint16_t ch = (uint16_t) ch;
    ch = ch << 6;
    uint16_t regCfg = 0xbf4 | ch;

    //write the configuration in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    //read back voltage
    uint16_t data_read;
    read_adc24_data(spi_fd, &data_read);

}
