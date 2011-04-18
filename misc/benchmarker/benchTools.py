import sys
import socket
import time
import signal
from datetime import datetime
from datetime import timedelta
import thread
import random

dstIP = None
pubInterval = None
pollInterval = None
httpRequest = None
timeSync = None
firstPublishId = None
nPublish = 0
toWait = None
firstReceiveTime = None
lattestUnconsistentTime = datetime.now()

statsMap = {}

def timeDeltaToMillis(time):
	return time.days * 24 * 3600 * 1000 + time.seconds * 1000 + time.microseconds / 1000.

initTime = datetime.now()
def millisToTime(time):
	return initTime + timedelta(seconds = time / 1000,microseconds = (1000 * time) % 1000000)

def doConnect(stat,dstIP,port):
	global pubInterval
	connected = False
	while connected == False:
		try:
			sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			sock.settimeout(30)
			sock.connect((dstIP, port))
			if pubInterval == 0:
				sock.settimeout(100)
			else:
				sock.settimeout(max(2 * pubInterval,30))
			connected = True
			stat['sock'] = sock
		except:
			time.sleep(4)

def doSyncRequest(stat,httpRequest):
	while True:
		try:
			stat['sock'].send(httpRequest)
			ret = stat['sock'].recv(1024).split('\n')
			return int(ret[4])
		except:
			stat['sock'].close()
			time.sleep(4)
			doConnect(stat,dstIP,80)

def doPullRequest(stat,httpRequest):
	while True:
		try:
			stat['sock'].send(httpRequest)
			ret = stat['sock'].recv(1024).split('\n')
			if pubInterval > 0:
				publishId = int(int(ret[4]) / pubInterval)
				return (int(publishId * pubInterval),publishId,pubInterval)
			else:
				return (int(ret[4]),int(ret[5]),int(ret[6])*1000)
		except:
			stat['sock'].close()
			time.sleep(4)
			doConnect(stat,dstIP,80)

def doPushRequest1(stat,httpRequest):
	while True:
		try:
			stat['sock'].send(httpRequest)
			return
		except:
			stat['sock'].close()
			time.sleep(4)
			doConnect(stat,dstIP,80)

def doPushRequest2(stat,httpRequest):
	while True:
		try:
			ret = stat['sock'].recv(1024).split('\n')
			if pubInterval > 0:
				if len(ret) == 3:
					return (int(ret[1]),int(int(ret[1]) / pubInterval),pubInterval)
				else:
					return (int(ret[5]),int(int(ret[5]) / pubInterval),pubInterval)
			else:
				if len(ret) == 5:
					return (int(ret[1]),int(ret[2]),int(ret[3])*1000)
				else:
					return (int(ret[5]),int(ret[6]),int(ret[7])*1000)
		except:
			doPushRequest1(stat,httpRequest)

def clientFunc(id):
	global statsMap, timeSync, firstPublishId, nPublish, toWait, firstReceiveTime, lattestUnconsistentTime

	if id != 0:
		while timeSync == None:
			pass
		time.sleep(random.random() * nClients * 0.01)

	statsMap[id] = {'sock': None, 'rpm': 0, 'rpmu': 0, 'tpt': 0, 'tct': 0}
	doConnect(statsMap[id],dstIP,80)

	if id == 0:
		tmpTimeSync = None
		for i in range(16):
			rep = doSyncRequest(statsMap[id],'GET /time/time HTTP/1.1\r\nUser-Agent: custom\r\nHost: localhost\r\n\r\n ')
			timeAfter = datetime.now()
			repTime = millisToTime(rep)
			if tmpTimeSync == None or tmpTimeSync > timeAfter - repTime:
				tmpTimeSync = timeAfter - repTime
		timeSync = tmpTimeSync

	time.sleep(random.random() * nClients * 0.01 + random.random() * pollInterval)

	publishId = -1
	totalPubTripTime = 0
	totalConsistentTime = 0
	isActive = False

	if isComet:
		doPushRequest1(statsMap[id],httpRequest)

	while True:
		timeBefore = datetime.now()
		if isComet:
			rep = doPushRequest2(statsMap[id],httpRequest)
		else:
			rep = doPullRequest(statsMap[id],httpRequest)
		timeAfter = datetime.now()
		if not isActive:
			isActive = True
			toWait -= 1
		lastPublishId = publishId
		(published,publishId,nextPubDelay) = rep
		if toWait == 0 and firstPublishId != None and publishId >= firstPublishId:
			if publishId > lastPublishId:
				if firstReceiveTime == None:
					firstReceiveTime = timeAfter
				lattestUnconsistentTime = max(lattestUnconsistentTime,millisToTime(published)+timeSync+timedelta(microseconds = 1000 * nextPubDelay))
				pubTripTime = timeDeltaToMillis((timeAfter - timeSync) - millisToTime(published))
				totalPubTripTime += pubTripTime
				consistentTime = max(0,nextPubDelay - pubTripTime)
				totalConsistentTime += consistentTime
				statsMap[id]['rpmu'] += 1
				statsMap[id]['tpt'] = totalPubTripTime
				statsMap[id]['tct'] = totalConsistentTime
				nPublish = max(nPublish,publishId - firstPublishId + 1)
			statsMap[id]['rpm'] += 1
		elif firstReceiveTime == None:
			firstPublishId = max(firstPublishId,publishId + 1)
		if isComet:
			pass
		elif isLp:
			if pubInterval == 1000:
				time.sleep(0.1 + random.random() * nClients * 0.01)
			else:
				time.sleep(0.1 + random.random() * nClients * 0.03)
		else:
			randPart = 0.1 * (0.5 - random.random()) * pollInterval
			delta = datetime.now() - timeBefore
			deltaSec = timeDeltaToMillis(delta) / 1000.
			timeToSleep = pollInterval - (deltaSec % pollInterval) + randPart
			time.sleep(max(0,timeToSleep))

def start(argDstIP,argDstURL,argNClients,argPollInterval,argPubInterval):
	global dstIP,dstURL,nClients,pollInterval,pubInterval,isComet,isLp,toWait,httpRequest
	dstIP = argDstIP
	dstURL = argDstURL
	nClients = argNClients
	pollInterval = argPollInterval
	pubInterval = int(argPubInterval * 1000)
	isComet = pollInterval == 0
	isLp = pollInterval == -1
	pollInterval = max(0,pollInterval)

	httpRequest = 'GET ' + dstURL + ' HTTP/1.1\r\nUser-Agent: custom\r\nHost: localhost '
	for i in range(450):
		httpRequest += '.'
	httpRequest += '\r\n\r\n'
	toWait = nClients
	for i in range(nClients):
		thread.start_new_thread(clientFunc,(i,))
