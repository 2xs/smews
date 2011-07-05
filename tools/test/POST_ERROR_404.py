#!/usr/bin/python
import socket
import sys
import time 
import os

# create couple HOST (IP address), PORT
HOST,PORT = sys.argv[1],80
# the send request
request = 'POST /post_test/get_test_1 HTTP/1.1 \\r\\nContent-Type: text/plain; \\r\\nContent-Length: 12 \\r\\n\\r\\nHello World!'
error = 'HTTP/1.1 404 Not Found\\r\\n'

# create the socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# connection to the server
sock.connect((HOST,PORT))

length = len(request)
i = 0
# send request to the server Byte per Byte
while length > i:
	sock.send(bytes(request[i],'UTF-8'))
	i += 1

# receive the server answer
data = sock.recv(1024)

# close the socket
sock.close()

if(error in str(data)):
	sys.exit(os.EX_OK)
else:
	sys.exit(os.EX_SOFTWARE)
