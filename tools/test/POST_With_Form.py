#!/usr/bin/python
import socket
import sys
import time 
import os

# create couple HOST (IP address), PORT
HOST,PORT = sys.argv[1],80
# the send request
request = 'POST /post_test/post_test_1 HTTP/1.1 \r\nContent-Type: application/x-www-form-urlencoded; \r\nContent-Length: 16 \r\n\r\ni1=5&s=toto&i2=3'
correctAck = 'HTTP/1.1 200 OK\\r\\n'
correctAnswer = 'first int : 5\\nstr : toto\\nsecond int : 3'
# create the socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# connection to the server
sock.connect((HOST,PORT))

sock.send(bytes(request,'UTF-8'))

# receive the server answer
data = sock.recv(1024)

# close the socket
sock.close()

if(correctAck in str(data) and correctAnswer in str(data)):
	sys.exit(os.EX_OK)
else:
	sys.exit(os.EX_SOFTWARE)
