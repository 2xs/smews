#!/usr/bin/python
import socket
import sys
import time 
import os

# create couple HOST (IP address), PORT
HOST,PORT = sys.argv[1],80
# the send request
request = 'POST /post_test/post_test_2 HTTP/1.1 \r\nContent-Length: 12 \r\n\r\nHello World!'
error = 'HTTP/1.1 505 HTTP Version Not Supported\\r\\n'
# create the socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# connection to the server
sock.connect((HOST,PORT))

sock.send(bytes(request,'UTF-8'))

# receive the server answer
data = sock.recv(1024)

# close the socket
sock.close()

if(error in str(data)):
	sys.exit(os.EX_OK)
else:
	sys.exit(os.EX_SOFTWARE)
