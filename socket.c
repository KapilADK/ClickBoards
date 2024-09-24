#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define FLOAT_SIZE 4  // 4 floats * 4 bytes each = 16 bytes
#define BUF_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    float float_array[4] = {-1.1, 2.2, 3.3, -4.4};
    float received_array[4];
    char buffer[BUF_SIZE] = {0};
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set up the address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind the socket to the network address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d...\n", PORT);
    
    // Accept an incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connection accepted\n");
    
    // Send the array of floating-point numbers to the client
    send(new_socket, float_array, sizeof(float_array), 0);
    printf("Array sent to client: %.1f, %.1f, %.1f, %.1f\n", float_array[0], float_array[1], float_array[2], float_array[3]);

    // Read confirmation message from client
    int valread = read(new_socket, buffer, BUF_SIZE);
    buffer[valread] = '\0';  // Ensure buffer is null-terminated
    printf("Client says: %s\n", buffer);

    // Now read exactly 16 bytes (4 floats) from client
    valread = read(new_socket, received_array, sizeof(received_array));
    if (valread == sizeof(received_array)) {
        printf("Array received from client: %.1f, %.1f, %.1f, %.1f\n", received_array[0], received_array[1], received_array[2], received_array[3]);
    } else {
        printf("Error: Expected to receive 16 bytes, but received %d bytes\n", valread);
    }

    // Close the connection
    close(new_socket);
    close(server_fd);
    
    return 0;
}
