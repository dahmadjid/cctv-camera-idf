import socket
import time

now = time.time()
server_address = ("192.168.43.232", 6205) # The address of the server. its "" because we are in the same machine

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # same as server
client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
client_socket.settimeout(3)
client_socket.connect(server_address) # we initialise the connection. this will not finish excuting until the server accepts.

print("connected")

message = b'12345678' # same as server
client_socket.send(message)  # same as server
print("message sent")
total = bytearray()
while True:
  
    try:
        data = client_socket.recv(400000)  # same as server
        print("message recieved", data, len(data))
        total.extend(data)
    except:
        break


with open("image.jpg", "wb") as f:
    f.write(total)

client_socket.close()
print(time.time() - now)