#include <stdio.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc24click.h"


void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg) {

    // extract the last 12 bits from the configuration
    regCfg &= 0x0FFF;

    // data to be sent (the control register is loaded on the first 12 clocks, therefore the data are MSB aligned)
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

    struct spi_ioc_transfer dummy_transfer[2] = {
    {
        .tx_buf = (unsigned long)&dummy_data,  // First transfer
        .rx_buf = (unsigned long)0,
        .len = 2,
        .bits_per_word = 16,
    },
    {
        .tx_buf = (unsigned long)&dummy_data,  // Second transfer (same settings as first)
        .rx_buf = (unsigned long)0,
        .len = 2,
        .bits_per_word = 16,
    }
};

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), &dummy_transfer) < 0) {
            printf("**Sending dummy data while initialising failed**\n");
        }
}

int16_t read_adc24_data(int spi_fd) {

    uint16_t dummy_data = DUMMY_LOW;

    //buffer for receiving data
    int16_t rx_buf;

    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)&dummy_data,
        .rx_buf = (unsigned long)&rx_buf,
        .len = 2,
        .bits_per_word = 16,
    };

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0) {
        printf("**Reading sampled data failed***\n");
        return;
    }

    // return only the binary representation of voltage (remove the first 4 channel identifier)
    return (rx_buf & 0x0FFF);
    
}

int convert_raw16_to_mV(int16_t raw_data) {

    int voltage_mV = 0;
    voltage_mV = (int) (((double)raw_data / ADC24_MAX_VALUE) * ADC24_VOLTAGE_RANGE * 1000);
    return voltage_mV;
}

int get_voltage_adc24(int spi_fd, int ch) {

    int voltage_mV;

    // check if valid channel is given
    if (ch < 0 || ch > 15 ){
        printf("**Invalid channel: %d**\n", ch);
        return -1;
    }

    // prepare configuration for control register with the given channel
    uint16_t regCfg = ADC24_DEF_MANUAL_MODE | (uint16_t)(ch << 6);

    //write the configuration in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    //read back voltage
    int16_t raw_data;
    raw_data = read_adc24_data(spi_fd);

    // check if the number is negative
    if ((raw_data & (1 << 11)) != 0) {
        raw_data |= 0xF000; 
    }
    
    voltage_mV = convert_raw16_to_mV(raw_data);
    return voltage_mV;
}

void sequence_mode_adc24(int spi_fd, int stop_ch, int *voltages_mV, bool verbose) {

    // check if the stop channel is valid
    if (stop_ch < 1 || stop_ch > 15) {
        printf("**Invalid stop channel: %d**\n");
        return;
    }

    //prepare control register configuration with sequence mode selected 
    uint16_t regCfg = ADC24_DEF_SEQUENCE_MODE | (uint16_t)(stop_ch << 6);

    //write the config in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    // read the voltages in a loop
    int16_t raw_data;
    for (int i = 0; i <= stop_ch; i++) {
        raw_data = read_adc24_data(spi_fd);

        // check if the number is negative
        if ((raw_data & (1 << 11)) != 0) {
            raw_data |= 0xF000;
        }

        voltages_mV[i] = convert_raw16_to_mV(raw_data);
        if (verbose) {
            printf("Voltage measured at channel %d: %dmV\n", i, voltages_mV[i]);
        }
    }
}
