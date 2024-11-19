#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include "adc24click.h"
#include <errno.h>
#include <unistd.h>


void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg) {

    // data to be sent (the control register is loaded on the first 12 clocks, therefore the data are MSB aligned)
    uint16_t data_to_send = regCfg;
    uint16_t dummy_read;

    struct spi_ioc_transfer spi_transfer;
    memset(&spi_transfer, 0, sizeof(spi_transfer)); 
    
    spi_transfer.tx_buf = (unsigned long)&data_to_send;
    spi_transfer.rx_buf = (unsigned long)&dummy_read;
    spi_transfer.len = sizeof(data_to_send);

    printf("Control register configuration 0x%04X\n", data_to_send);

    //send SPI message
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer) < 0) {
        printf("**Failed to configure control register on ADC24 Click**. %s\n.", strerror(errno));
        exit(EXIT_FAILURE);
    }

}

void init_adc24(int spi_fd) {

    /* send 2 frames of dummy data keeping the DIN line high
     so that the ADC is placed into the correct operating mode
     when control register configuration is sent afterwards.*/

    uint16_t dummy_data = DUMMY_HIGH;

    if (write(spi_fd, &dummy_data, sizeof(dummy_data)) < 2) {
        printf("Sending dummy data failed. %s", strerror(errno));
      }
      usleep(1);

    if (write(spi_fd, &dummy_data, sizeof(dummy_data)) < 2) {
        printf("Sending dummy data failed. %s", strerror(errno));
      }
      usleep(1);

}

int convert_adc24raw16_to_mV(uint16_t raw_data) {

    int voltage_mV = 0;
    voltage_mV = (int) (((double)raw_data / ADC24_MAX_VALUE) * ADC24_VOLTAGE_RANGE * 1000);
    return voltage_mV;
}

int get_voltage_adc24(int spi_fd, int ch) {

    int voltage_mV;

    // check if valid channel is given
    if (ch < 0 || ch > 15 ){
        printf("**Invalid channel: %d**\n", ch);
        exit(EXIT_FAILURE);
    }

    // prepare configuration for control register with the given channel
    uint8_t tx_buf[2] = {0};
    tx_buf[0] = FIRST_BYTE | ((uint8_t) ch << 2);
    tx_buf[1] = SECOND_BYTE;

    uint8_t dummy[2] = {0};


    if (write(spi_fd, tx_buf, sizeof(tx_buf)) < 2) {
        printf("Configuring control register failed. %s\n", strerror(errno));
      }

    usleep(1);
    //read back voltage
    uint8_t raw_data[2] = {0};
    uint16_t raw_value;
    uint8_t ch_id = 0;

    if (read(spi_fd, &raw_data, sizeof(raw_data)) < 2) {
        printf("Reading data from channel failed. %s\n", strerror(errno));
      }


    raw_value = ((uint16_t)raw_data[0]) << 8 | raw_data[1];
    printf("Data read back 0x%04X\n", raw_value);

    ch_id = (raw_value & 0xF000) >> 12; // extract channel identifier bits
    raw_value = raw_value & 0x0FFF;

    voltage_mV = convert_adc24raw16_to_mV(raw_value);

    printf("Voltage measured on channel %d is %d mV\n", ch_id, voltage_mV);
    return voltage_mV;
}

int *sequence_mode_adc24(int spi_fd, int stop_ch, int size, bool verbose) {

    //allocate memory
    int *voltages_mV = (int*) malloc(size * sizeof(int));

    // check if the stop channel is valid
    if (stop_ch < 1 || stop_ch > 15) {
        printf("**Invalid stop channel: %d**\n", stop_ch);
        exit(EXIT_FAILURE);
    }

    //prepare control register configuration with sequence mode selected
    uint16_t regCfg = ADC24_DEF_SEQUENCE_MODE | (uint16_t)(stop_ch << 6);

    //write the config in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    // read the voltages in a loop
    uint8_t raw_data;
    // for (int i = 0; i <= stop_ch; i++) {
    //     raw_data = read_adc24_data(spi_fd);

        // check if the number is negative
        if ((raw_data & (1 << 11)) != 0) {
            raw_data |= 0xF000;
        }

        //voltages_mV[i] = convert_adc24raw16_to_mV(raw_data);
        // if (verbose) {
        //     printf("Voltage measured at channel %d: %dmV\n", i, voltages_mV[i]);
        // }
    }
    //return voltages_mV;
//}
