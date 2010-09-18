import sys
import os
import socket
import time
import signal
from subprocess import *
from datetime import datetime
from datetime import timedelta
import thread
import random
import benchTools

def ratio(a,b):
	return round(a / float(b),2) if b > 0 else 0

def printResults(trafficBytes,trafficPackets):
	toWait = benchTools.toWait
	statsMap = benchTools.statsMap
	nClients = benchTools.nClients
	firstReceiveTime = benchTools.firstReceiveTime
	lattestUnconsistentTime = benchTools.lattestUnconsistentTime
	nPublish = benchTools.nPublish
	cptClients = 0
	keys = statsMap.keys()
	keys.sort()
	ptSum = 0
	ctSum = 0
	rpmSum = 0
	rpmuSum = 0
	rpmPercentSum = 0
	rpmuPercentSum = 0
	for key in keys:
		ctSum += statsMap[key]['tct']
		ptSum += statsMap[key]['tpt']
		rpmSum += statsMap[key]['rpm']
		rpmuSum += statsMap[key]['rpmu']
	if int(100*ratio(rpmuSum,nPublish * nClients)) == 0:
		print 'error: rupm == 0'
	else:
		print 'Results: %d %d %d %d %.1f %.1f %.1f' %(ratio(ctSum,nPublish * nClients), ratio(ptSum,rpmuSum),100*ratio(rpmSum,nPublish * nClients),100*ratio(rpmuSum,nPublish * nClients),trafficBytes,trafficPackets,100*ratio(ctSum,nClients*benchTools.timeDeltaToMillis(lattestUnconsistentTime-firstReceiveTime)))

def main():
	if len(sys.argv) != 4:
		print 'Usage: ' + sys.argv[0] + ' nClients pollInterval pubInterval'
		exit(1)

	dstIP = 'www.wsn430.pan'
	nClients = int(sys.argv[1])
	pollInterval = float(sys.argv[2])
	pubInterval = float(sys.argv[3])

	print "Configuration %d %d %d" %(nClients, pollInterval, pubInterval)
	print "Timestamp %s" %datetime.now()

	if pollInterval == 0:
		if pubInterval == 0:
			dstURL = '/time/cometRand'
		else:
			dstURL = '/time/comet%ds' %pubInterval
	elif pollInterval == -1:
		if pubInterval == 0:
			dstURL = '/time/lpRand'
		else:
			dstURL = '/time/lp%ds' %pubInterval
	else:
		if pubInterval == 0:
			dstURL = '/time/timeRand'
		else:
			dstURL = '/time/time'

	benchTools.start(dstIP,dstURL,nClients,pollInterval,pubInterval)

	if pubInterval == 0:
		timeout = int(200)
	else:
		timeout = int(50 + max(3*max(pubInterval,pollInterval),nClients/2))
	print 'Timeout: %d' %timeout
	maxInitTimer = datetime.now() + timedelta(seconds = timeout)
	while benchTools.toWait > 0 or benchTools.firstReceiveTime == None:
		if datetime.now() > maxInitTimer:
			print 'init error'
			sys.exit(1)
		else:
			time.sleep(1)

	if pubInterval == 0:
		timeToSleep = 500
	else:
		elapsed = benchTools.timeDeltaToMillis(datetime.now() - benchTools.firstReceiveTime) / 1000.
		basicWait = max(150,5 * pubInterval)
		timeToSleep = basicWait + (pubInterval - elapsed % pubInterval - 2)

	os.system('cat /proc/net/dev | grep sl0 > .net_stats')
	devStats = filter(lambda c: c != '',open('.net_stats', 'r').readline().split(' '))
	trafficBytes1 = int(devStats[1]) + int(devStats[9])
	trafficPackets1 = int(devStats[2]) + int(devStats[10])

	startTime = datetime.now()

	print 'Sleep %d' %timeToSleep
	time.sleep(timeToSleep)

	endTime = datetime.now()
	deltaTimeSec = benchTools.timeDeltaToMillis(endTime - startTime) / 1000.

	os.system('cat /proc/net/dev | grep sl0 > .net_stats')
	devStats = filter(lambda c: c != '',open('.net_stats', 'r').readline().split(' '))
	os.remove('.net_stats')
	trafficBytes2 = int(devStats[1]) + int(devStats[9])
	trafficPackets2 = int(devStats[2]) + int(devStats[10])

	printResults((trafficBytes2 - trafficBytes1) / float(deltaTimeSec),(trafficPackets2 - trafficPackets1) / float(deltaTimeSec))

main()
