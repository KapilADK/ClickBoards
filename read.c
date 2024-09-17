// Function to read data from the ADC register
int spi_read_register(int spi_fd, uint8_t read_cmd, uint8_t reg_addr, uint8_t *data) {
    uint8_t tx_buf[2] = {read_cmd, reg_addr};  // Send read command and register address
    uint8_t rx_buf[2] = {0};                   // Buffer for receiving data

    struct spi_ioc_transfer spi_transfer[2] = {
        {
            .tx_buf = (unsigned long)tx_buf,   // Transmit buffer (send read command)
            .rx_buf = 0,                       // No data to receive in the first transfer
            .len = 2,                          // 2 bytes (command + address)
            .speed_hz = SPI_SPEED,             // SPI clock speed
            .bits_per_word = 8,                // Bits per word (8 bits)
        },
        {
            .tx_buf = 0,                       // No data to transmit in the second transfer
            .rx_buf = (unsigned long)rx_buf,   // Receive buffer (to read response)
            .len = 1,                          // Expecting 1 byte of data
            .speed_hz = SPI_SPEED,             // SPI clock speed
            .bits_per_word = 8,                // Bits per word (8 bits)
        }
    };

    // Perform SPI transfer (2 stages)
    int ret = ioctl(spi_fd, SPI_IOC_MESSAGE(2), spi_transfer);
    if (ret < 1) {
        perror("SPI Read Error");
        return -1;
    }

    // The data is in rx_buf[0]
    *data = rx_buf[0];

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

    // Example: Read from a register
    uint8_t read_cmd = 0x80;   // Read command (example)
    uint8_t reg_addr = 0x01;   // Register address (example)
    uint8_t data = 0;

    if (spi_read_register(spi_fd, read_cmd, reg_addr, &data) < 0) {
        printf("Failed to read from the ADC register\n");
    } else {
        printf("Successfully read from the ADC register, data: 0x%02X\n", data);
    }

    close(spi_fd);  // Close SPI device
    return 0;
}
