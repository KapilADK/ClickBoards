#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdbool.h>
#include "adc24click.h"


void config_adc24_ctrl_reg(int spi_fd, uint16_t regCfg) {

    regCfg &= ADC24_CTRL_REG_RESOLUTION;

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
        
        printf("**Failed to configure control register on ADC24 Click**\n");
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

    // extract only the last 12 bits, since the first 4 bits are channel identifier
    *data = rx_buf & ADC24_ADC_RESOLUTION;
    
}

int convert_data_to_mV(uint16_t data) {

    int voltage =  (int) data / ADC24_HIGHEST_VAL_TWOS_COMP * ADC24_VREF_2V5 * 1000;
    return voltage;

}

int get_voltage_adc24(int spi_fd, int ch) {

    if (ch < 0 || ch > 15) {
        printf("**Invalid channel number: %d**\n", ch);
        return -1;
    }

    // prepare configuration for control register with the given channel
    ch = ch << 6;
    uint16_t regCfg = ADC24_MANUAL_CONFG | ch;

    //write the configuration in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    //read back voltage
    uint16_t data_read = DUMMY_LOW;
    read_adc24_data(spi_fd, &data_read);

    // check if the 12th bit is set 
    if ((data_read & (1 << 11))!= 0) {
        data_read = data_read - (1 << 12);
    }

    int voltage = convert_data_to_mV(data_read);
    return voltage;

}

void sample_sequence_adc24(int spi_fd,  int stop_ch, int channels, int *voltages) {
    
    //check if channels are valid
    if (channels <=0 || channels > MAX_CHANNELS) {
        printf("**Invalid number of channels: %d**\n", channels);
        return;
    }
    
    // prepare configuration for control register with the sequence mode selected and the given stop channel
    stop_ch = stop_ch << 6;
    uint16_t regCfg = ADC24_SEQUENCE_CONFG | stop_ch;

    // write the configuration in the control register
    config_adc24_ctrl_reg(spi_fd, regCfg);

    // read the voltages in a loop 
    uint16_t data_read = DUMMY_LOW;
    for (int i = 0; i < channels; i++) {
        read_adc24_data(spi_fd, &data_read);

        //check if the 12th bit is set
        if ((data_read & (1 << 11)) != 0) {
            data_read = data_read - (1 << 12);
        }
        /*
        Some delay need to be added ????
        */


        // convert raw ADC data to voltage and store it in the provided array
        voltages[i] = convert_data_to_mV(data_read);
        printf("\t Voltage measured at Channel[%d] of ADC24: %dmV\n", i, voltages[i]);

    }

}