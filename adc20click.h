#ifndef ADC20CLICK_H
#define ADC20CLICK_H

#define ADC20_CMD_REG_WRITE  0x08
#define ADC20_CMD_REG_READ   0x10


#define DUMMY 0x00 

// registers list of ADC20Click
#define ADC20_REG_SYSTEM_STATUS             0x00
#define ADC20_REG_GENERAL_CFG               0x01
#define ADC20_REG_DATA_CFG                  0x02
#define ADC20_REG_OSR_CFG                   0x03
#define ADC20_REG_OPMODE_CFG                0x04
#define ADC20_REG_PIN_CFG                   0x05
#define ADC20_REG_GPIO_CFG                  0x07
#define ADC20_REG_GPO_DRIVE_CFG             0x09
#define ADC20_REG_GPO_VALUE                 0x0B
#define ADC20_REG_GPI_VALUE                 0x0D
#define ADC20_REG_SEQUENCE_CFG              0x10
#define ADC20_REG_CHANNEL_SEL               0x11
#define ADC20_REG_AUTO_SEQ_CH_SEL           0x12

// config to set sampling rate to 1MHz
#define ADC20_1MHZ_SR_CFG                   0x00      

// config to set all channels as analog inputs
#define ADC20_ALL_CHANNELS_AIN              0x00 

// config to select manual mode
#define ADC20_MANUAL_MODE                   0x00

// config for not appending channel identifier in output_format
#define ADC20_CH_ID_NOT_APPEND              0x00 


#endif