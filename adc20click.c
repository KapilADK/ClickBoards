#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc20click.h"
#include <unistd.h>

#define ADC20_MAX_VALUE_16BIT 65535
#define ADC20_MAX_VALUE_12BIT 4095


void write_register(int spi_fd, uint8_t regAddr, uint8_t regCfg) {

    // proper initialisation of the write buffer
    uint8_t data_buf[3] = {0};
    data_buf[0] = ADC20_CMD_REG_WRITE;
    data_buf[1] =  regAddr;
    data_buf[2] = regCfg;

    if(write(spi_fd, data_buf, sizeof(data_buf)) < 0) {
        printf("Configuring 0x%02X register with value 0x%02X failed\n", regAddr, regCfg);
        exit(EXIT_FAILURE);
    }

}

void read_register (int spi_fd, uint8_t regAddr) {

    //prepare buffer to send read command with register address and dummy data
    uint8_t tx_buf[3] = {0};
    tx_buf[0] = ADC20_CMD_REG_READ;
    tx_buf[1] = regAddr;
    tx_buf[2] = DUMMY;

    uint8_t rx_buf = 0;

    if (write(spi_fd, tx_buf, sizeof(tx_buf)) < 0) {
        printf("Sending read register command to 0x%02X register failed\n", regAddr);
    }

    if (read(spi_fd, &rx_buf, sizeof(rx_buf)) < 0) {
        printf("Reading register value from 0x%02X register failed\n", regAddr);
    }

    printf("Value stored in 0x%02X register is 0x%02X\n", tx_buf[1], rx_buf);

}

void init_adc20(int spi_fd, bool ch_append, int mode) {

    // donot append channel id at the end of output
    write_register(spi_fd, ADC20_REG_DATA_CFG, ADC20_CH_ID_NOT_APPEND);

    //initialise all channels as analog input pins
    write_register(spi_fd, ADC20_REG_PIN_CFG, ADC20_ALL_CHANNELS_AIN);

    //set sampling rate to 1MHz
    write_register(spi_fd, ADC20_REG_OPMODE_CFG, ADC20_1MHZ_SR_CFG);

}

uint16_t read_adc20_data(int spi_fd) {

    uint8_t rx_buf[3] = {DUMMY};
    uint8_t data_buf[ 2 ] = { 0 };

    // write a dummy byte, then read 2 bytes of data
    //write(spi_fd, &rx_buf, sizeof(rx_buf));


    if (read(spi_fd, data_buf, sizeof(data_buf)) < 0) {
        printf("Reading data from SPI Bus failed\n");
    }

    uint16_t raw_data;
    raw_data = ( ( uint16_t ) data_buf[ 0 ] << 8 ) | data_buf[ 1 ];

    printf("Raw data read from ADC 0x%04X\n", raw_data);

    return raw_data;
}

int convert_adc20raw16_to_mV(uint16_t raw_data, int average) {
    int voltage_mV;
    double scaling_factor;

    // Use 12-bit or 16-bit scaling factor based on average parameter
    if (average == 0) {
        scaling_factor = ADC20_MAX_VALUE_12BIT;
    } else {
        scaling_factor = ADC20_MAX_VALUE_16BIT;
    }

    // Calculate voltage in mV
    voltage_mV = (int)(((double)raw_data / scaling_factor) * ADC20_VOLTAGE_RANGE * 1000);
    return voltage_mV;
}

// int convert_adc20raw16_to_mV(uint16_t raw_data) {

//     int voltage_mV;
//     voltage_mV = (int) (((double)raw_data / ADC20_MAX_VALUE) * ADC20_VOLTAGE_RANGE * 1000);
//     return voltage_mV;

// }

int get_voltage_adc20(int spi_fd, int ch, int average) {

    int voltage_mV = 0;
    
    //uint16_t raw_data = read_adc20_data(spi_fd);

    // check if valid channel is given
    if (ch < 0 || ch > 7) {
        printf("**Invalid channel number: %d**\n", ch);
        exit(EXIT_FAILURE);
    }

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        exit(EXIT_FAILURE);
    }

    // configure the oversampling ratio to average the data
    write_register(spi_fd, ADC20_REG_OSR_CFG, (uint8_t)average);

    // select manual mode in SEQUENCE_CFG register
    //write_register( spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_MANUAL_MODE);

    // select channel to sample data in CHANNEL_SEL register
    write_register(spi_fd, ADC20_REG_CHANNEL_SEL, (uint8_t)ch);

    usleep(10000);

    uint16_t raw_data = read_adc20_data(spi_fd);
    //raw_data = read_adc20_data(spi_fd);

    //check if averaging is enabled
    if (average == 0) {

       raw_data = raw_data >> 4;

    }
    voltage_mV = convert_adc20raw16_to_mV(raw_data, average);
    return voltage_mV;
}

int *sample_sequence_mode_adc20(int spi_fd, int stop_ch, int average, int size, bool verbose) {

    int *voltages_mV =(int*) malloc(size * sizeof(int));

    // check if the stop channel is valid
    if (stop_ch < 1 || stop_ch > 7) {
        printf("**Invalid stop channel: %d**\n", stop_ch);
        exit(EXIT_FAILURE);
    }

    // check if valid averaging ratio is given
    if (average < 0 || average > 7 ) {
        printf("**Invalid averaging ratio: %d**\n", average);
        exit(EXIT_FAILURE);
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
        voltages_mV[i] = convert_adc20raw16_to_mV(raw_data, average);
        if (verbose) {
            printf("Voltage measured at channel %d: %dmV\n", i, voltages_mV[i]);
        }

    }
    // stop sequence mode
    write_register(spi_fd, ADC20_REG_SEQUENCE_CFG, ADC20_STOP_SEQUENCE_MODE);
    return voltages_mV;
}

void adc20_debug_communication(int spi_fd) {

    uint8_t rx_buf[2] = {0};

    write_register(spi_fd, ADC20_REG_DATA_CFG, ADC20_SEND_DEBUG_CMD);

    if (read(spi_fd, &rx_buf, sizeof(rx_buf)) < 0) {
        printf("Reading back failed\n");
    }

    uint16_t result = (rx_buf[0] << 8) | rx_buf[1] & 0xFFF0;
    printf("The value is: 0x%02X\n", result);

}