
#include <stdio.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc20click.h"


void write_register(int spi_fd, uint8_t regAddr, uint8_t regCfg) {

    // proper initialisation of the write buffer
    uint8_t tx_buf[3] = {0};
    tx_buf[0] = ADC20_CMD_REG_WRITE;
    tx_buf[1] =  regAddr;
    tx_buf[2] = regCfg;

    struct spi_ioc_transfer write_register = {
        .tx_buf = (unsigned long)tx_buf,
        .rx_buf = (unsigned long)0,
        .len = 3,
        .bits_per_word = 8,

    };

    //send SPI message 
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &write_register) < 0) {
        printf("**Failed to configure register on ADC20 Click**\n");
    }
}

uint8_t read_register (int spi_fd, uint8_t regAddr) {

    //prepare buffer to send read command with register address and dummy data
    uint8_t tx_buf[3] = {0};
    tx_buf[0] = ADC20_CMD_REG_READ;
    tx_buf[1] = regAddr;
    tx_buf[2] = DUMMY;

    uint8_t rx_buf = 0;

    struct spi_ioc_transfer read_register[2];

    //first transfer setup
    read_register[0].tx_buf = (unsigned long)tx_buf;
    read_register[0].rx_buf =(unsigned long)0;
    read_register[0].len = 3;
    read_register[0].bits_per_word = 8;
    read_register[0].cs_change = 1; 

    //second transfer setup
    read_register[1].tx_buf =(unsigned long)0;
    read_register[1].rx_buf = (unsigned long)&rx_buf;
    read_register[1].len = 1;
    read_register[1].bits_per_word = 8;

    // perform both transfers using ioctl and SPI_IOC_MESSAGE(2)
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(2), &read_register) < 0) {
        printf("**Reading data from address %d failed**\n", regAddr);
        return 0;
    }

    return rx_buf;
}

void init_adc20(int spi_fd) {

    // donot append channel id at the end of output 
    write_register(spi_fd, ADC20_REG_DATA_CFG, ADC20_CH_ID_NOT_APPEND);

    //initialise all channels as analog input pins
    write_register(spi_fd, ADC20_REG_PIN_CFG, ADC20_ALL_CHANNELS_AIN);

    //set sampling rate to 1MHz
    write_register(spi_fd, ADC20_REG_OPMODE_CFG, ADC20_1MHZ_SR_CFG);

}

uint16_t read_adc20_data(int spi_fd) {

    uint16_t raw_data = 0;

    struct spi_ioc_transfer read_data = {
        .tx_buf = (unsigned long)0,
        .rx_buf = (unsigned long)&raw_data,
        .len = 2,
    };

    if(ioctl(spi_fd, SPI_IOC_MESSAGE(1), &read_data) < 0) {
        printf("**Reading sampled data failed**\n");
    }
    return raw_data;
}

int convert_raw16_to_mV(uint16_t raw_data) {

    int voltage_mV;
    voltage_mV = (int) (((double)raw_data / ADC20_MAX_VALUE) * ADC20_VOLTAGE_RANGE * 1000);
    return voltage_mV;

}

int get_voltage_adc20(int spi_fd, int ch, int average) {

    int voltage_mV = 0;

    // check if valid channel is given
    if (ch < 0 || ch > 7) {
        printf("**Invalid channel number: %d**\n", ch);
        return -1;
    }

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        return -1;
    }

    // configure the oversampling ratio to average the data
    write_register(spi_fd, ADC20_REG_OSR_CFG, (uint8_t)average);

    // select manual mode in SEQUENCE_CFG register
    write_register( spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_MANUAL_MODE);

    // select channel to sample data in CHANNEL_SEL register
    write_register(spi_fd, ADC20_REG_CHANNEL_SEL, (uint8_t)ch);

    uint16_t raw_data = read_adc20_data(spi_fd);

    //check if averaging is enabled
    if (average == 0) {

       raw_data = raw_data >> 4;
    
    } 
    voltage_mV = convert_raw16_to_mV(raw_data);
    return voltage_mV;
}

void sample_sequence_mode_adc20(int spi_fd, int stop_ch, int average, int *voltages_mV, bool verbose) {

    // check if the stop channel is valid
    if (stop_ch < 1 || stop_ch > 7) {
        printf("**Invalid stop channel: %d**\n");
        return;
    }

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        return;
    }

    // configure the oversampling ratio to average the data
    write_register(spi_fd, ADC20_REG_OSR_CFG, (uint8_t)average);

    uint8_t regCfg =  (1 << (stop_ch + 1)) - 1;

    // configure the channels upto stop_ch for auto sequencing mode 
    write_register(spi_fd, ADC20_REG_AUTO_SEQ_CH_SEL, regCfg);

    // start auto sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_SEQUENCE_MODE);

    //read voltages in a loop
    uint16_t raw_data;
    for (int i = 0; i <= stop_ch; i++) {
        raw_data = read_adc20_data(spi_fd);

        // check if averaging is enabled
        if (average == 0) {

            raw_data = raw_data >> 4;
        }
        voltages_mV[i] = convert_raw16_to_mV(raw_data); 
        if (verbose) {
            printf("Voltage measured at channel %d: %dmV\n", i, voltages_mV[i]);
        }

    }
    // stop sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_STOP_SEQUENCE_MODE);

}

