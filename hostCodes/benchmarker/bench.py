import sys
import os
import socket
import time
import signal
from datetime import datetime
from datetime import timedelta
import thread
import random
import benchTools

def ratio(a,b):
	return round(a / float(b),2) if b > 0 else 0

def alarm_handler(signo,frame):
	global trafficBytes1,trafficPackets1,startTime
	toWait = benchTools.toWait
	statsMap = benchTools.statsMap
	nClients = benchTools.nClients
	firstReceiveTime = benchTools.firstReceiveTime
	lattestUnconsistentTime = benchTools.lattestUnconsistentTime
	nPublish = benchTools.nPublish
	if toWait > 0:
		print 'Waiting %d...' %toWait
	elif firstReceiveTime == None:
		print 'Waiting for a first synchronized publication'
	else:
		endTime = datetime.now()
		deltaTimeSec = benchTools.timeDeltaToMillis(endTime - startTime) / 1000.
		os.system('cat /proc/net/dev | grep sl0 > .net_stats')
		devStats = filter(lambda c: c != '',open('.net_stats', 'r').readline().split(' '))
		os.remove('.net_stats')
		trafficBytes2 = int(devStats[1]) + int(devStats[9])
		trafficPackets2 = int(devStats[2]) + int(devStats[10])
		trafficBytes = (trafficBytes2 - trafficBytes1) / float(deltaTimeSec)
		trafficPackets = (trafficPackets2 - trafficPackets1) / float(deltaTimeSec)

		cptClients = 0
		print '----------------------------------'
		print 'id, port, mct, mpt, rpm, rpmu'
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
			mct = ratio(statsMap[key]['tct'], nPublish)
			mpt = ratio(statsMap[key]['tpt'], statsMap[key]['rpmu'])
			rpmPercent = 100*ratio(statsMap[key]['rpm'], nPublish)
			rpmuPercent = 100*ratio(statsMap[key]['rpmu'], nPublish)
			try:
				port = statsMap[key]['sock'].getsockname()[1]
			except:
				port = 0
			print '[%d, %d], %d, %d, %d, %d, %d%%, %d%%' %(key,port,mct,mpt,statsMap[key]['rpm'],statsMap[key]['rpmu'],rpmPercent,rpmuPercent)
		print 'id, mct, mpt, rpm, rpmu, nPublish (%d)' %(benchTools.timeDeltaToMillis(datetime.now() - firstReceiveTime) / 1000.)
		print '--- mct, mpt, rpm, rpmu, nPublish, rpm%, rpmu%, ntb, ntp, ct%'
		print '[AVG] %d, %d, %d, %d, %d, %d%%, %d%%, %.1f, %.1f, %d%%' %(ratio(ctSum,nPublish * nClients), ratio(ptSum,rpmuSum),ratio(rpmSum,nClients),ratio(rpmuSum,nClients),nPublish,100*ratio(rpmSum,nPublish * nClients),100*ratio(rpmuSum,nPublish * nClients),trafficBytes,trafficPackets,100*ratio(ctSum,nClients*benchTools.timeDeltaToMillis(lattestUnconsistentTime-firstReceiveTime)))

	signal.alarm(2)

def main():
	global trafficBytes1,trafficPackets1,startTime
	if len(sys.argv) != 4:
		print 'Usage: ' + sys.argv[0] + ' nClients pollInterval pubInterval'
		exit(1)

	dstIP = 'www.wsn430.pan'
	nClients = int(sys.argv[1])
	pollInterval = float(sys.argv[2])
	pubInterval = float(sys.argv[3])

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

	print 'Number of clients: %d' % nClients
	if pollInterval == 0:
		print 'Comet mode'
	elif pollInterval == -1:
		print 'Long polling mode'
	else:
		print 'Polling interval: %.3f seconds' % pollInterval
	if pubInterval == 0:
		print 'Publish interval: random'
	else:
		print 'Publish interval: %.3f seconds' % pubInterval

	os.system('cat /proc/net/dev | grep sl0 > .net_stats')
	devStats = filter(lambda c: c != '',open('.net_stats', 'r').readline().split(' '))
	trafficBytes1 = int(devStats[1]) + int(devStats[9])
	trafficPackets1 = int(devStats[2]) + int(devStats[10])
	startTime = datetime.now()

	benchTools.start(dstIP,dstURL,nClients,pollInterval,pubInterval)
	
	signal.signal(signal.SIGALRM, alarm_handler)
	signal.alarm(2)
	while 1:
		time.sleep(1)

main()
