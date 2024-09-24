import socket
import struct

# Define the server address and port
server_address = '127.0.0.1'
server_port = 8080

# Create a TCP/IP socket
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect to the server
client_socket.connect((server_address, server_port))
print("Connected to server.")

# Receive the array of floating-point numbers from the server
data = client_socket.recv(16)  # 4 floats, 4 bytes each
if len(data) == 16:
    float_array = struct.unpack('ffff', data)
    print(f"Array received from server: {float_array}")
else:
    print(f"Error: Expected 16 bytes, but received {len(data)} bytes")

# Send confirmation message to the server
confirmation_message = "Array received successfully"
client_socket.sendall(confirmation_message.encode('utf-8'))

# Send the same array back to the server
client_socket.sendall(data)  # Sending back the exact same 16 bytes
print(f"Array sent back to server: {float_array}")

# Close the connection
client_socket.close()
