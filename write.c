#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define SPI_DEVICE "/dev/spidev0.0"  // Replace with your SPI device path
#define SPI_SPEED  500000            // SPI clock speed (500 kHz)
#define SPI_MODE   SPI_MODE_0        // SPI mode (mode 0 here)

// Function to write data to the ADC register
int spi_write_register(int spi_fd, uint8_t write_cmd, uint8_t reg_addr, uint8_t data) {
    uint8_t tx_buf[3] = {write_cmd, reg_addr, data};  // SPI transfer data (3 bytes)
    struct spi_ioc_transfer spi_transfer = {
        .tx_buf = (unsigned long)tx_buf,   // Transmit buffer
        .rx_buf = 0,                       // Receive buffer (we're not receiving data)
        .len = 3,                          // Length of data to transfer (3 bytes)
        .speed_hz = SPI_SPEED,             // SPI clock speed
        .bits_per_word = 8,                // Bits per SPI word (8 bits)
    };

    // Perform SPI transfer
    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi_transfer);
    if (ret < 1) {
        perror("SPI Write Error");
        return -1;
    }

    return 0;  // Success
}

int main() {
    int spi_fd = open(SPI_DEVICE, O_RDWR);  // Open SPI device
    if (spi_fd < 0) {
        perror("Error opening SPI device");
        return -1;
    }

    // Set SPI mode
    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &SPI_MODE) < 0) {
        perror("Can't set SPI mode");
        close(spi_fd);
        return -1;
    }

    // Example: Write to a register
    uint8_t write_cmd = 0x40;   // Write command (example)
    uint8_t reg_addr = 0x01;    // Register address (example)
    uint8_t data = 0x55;        // Data to write

    if (spi_write_register(spi_fd, write_cmd, reg_addr, data) < 0) {
        printf("Failed to write to the ADC register\n");
    } else {
        printf("Successfully wrote to the ADC register\n");
    }

    close(spi_fd);  // Close SPI device
    return 0;
}
