import os
import sys
import os.path
import time
import shutil
import random
from datetime import datetime

def ratio(a,b):
	return round(a / float(b),2) if b > 0 else 0

nWarnings = 0

class StatsSet:
	def __init__(self):
		self.statshash = {}
		for id in ['mct','mpt','rpm','rupm','ntb','ntp']:
			self.statshash[id] = Stat()

	def addResults(self,resFile,results):
		global nWarnings
		for id in ['mct','mpt','rpm','rupm','ntb','ntp']:
			if results[id] <= 0:
				if id == 'mpt':
					print 'Warning: %s is %d in %s [corrected]' %(id,results[id],resFile)
					results[id] = 0
					nWarnings+=1
				else:
					print 'Warning: %s is %d in %s' %(id,results[id],resFile)
					nWarnings+=1
			elif id in ['mct','rupm'] and results[id] > 100:
				print 'Warning: %s is %d in %s [corrected]' %(id,results[id],resFile)
				results[id] = 100
				nWarnings+=1
		for id in ['mct','mpt','rpm','rupm','ntb','ntp']:
			self.statshash[id].addVal(results[id])

	def getCount(self):
		return self.statshash['mct'].getCount()

	def getMin(self,id):
		return self.statshash[id].getMin()

	def getMax(self,id):
		return self.statshash[id].getMax()

	def getAvg(self,id):
		avg = self.statshash[id].getAvg()
		if avg == None:
			return None
		if id in ['mpt']:
			return avg / 1000.
		elif id == 'ntb':
			return avg / 1024.
		else:
			return avg

	def removeMin(self):
		minRupmIdx = self.statshash['rupm'].getMinIdx()
		for id in ['mct','mpt','rpm','rupm','ntb','ntp']:
			self.statshash[id].removeIndex(minRupmIdx)

	def removeMax(self):
		maxRupmIdx = self.statshash['rupm'].getMaxIdx()
		for id in ['mct','mpt','rpm','rupm','ntb','ntp']:
			self.statshash[id].removeIndex(maxRupmIdx)

class Stat:
	def __init__(self):
		self.vals = []

	def removeIndex(self,idx):
		try:
			self.vals.pop(idx)
		except:
			pass

	def addVal(self,val):
		self.vals.append(val)

	def getCount(self):
		return len(self.vals)

	def getAvg(self):
		if len(self.vals) == 0:
			return None
		else:
			sum = 0
			for val in self.vals:
				sum += val
			return float(sum) / len(self.vals)

	def getMinIdx(self):
		if len(self.vals) == 0:
			return None
		minIdx = 0
		for i in range(len(self.vals)):
			if self.vals[i] < self.vals[minIdx]:
				minIdx = i
		return minIdx

	def getMaxIdx(self):
		if len(self.vals) == 0:
			return None
		maxIdx = 0
		for i in range(len(self.vals)):
			if self.vals[i] > self.vals[maxIdx]:
				maxIdx = i
		return maxIdx

	def getMin(self):
		try:
			return self.vals[self.getMinIdx()]
		except:
			return 0

	def getMax(self):
		try:
			return self.vals[self.getMaxIdx()]
		except:
			return 0

maxBenchs = 10
nClientsSet = [1,16,32,64,128,255]
pubIntervalSet = [1,5,15,30,50,0]
pollIntervalSet = [-1,0,1,5,15,30,50]

minCptBenhs = 999
benchsProcessed = 0
startTime = datetime.now()
resDir = sys.argv[1]
if not os.path.exists('plots'):
	os.mkdir('plots')
globalStats = {}
for nClients in nClientsSet:
	globalStats[nClients] = {}
	nClientsDirName = './%s/%d' %(resDir,nClients)

	for pubInterval in pubIntervalSet:
		globalStats[nClients][pubInterval] = {}
		pubDirName = './%s/%d/%d' %(resDir,nClients,pubInterval)

		for pollInterval in pollIntervalSet:
			stats = StatsSet()
			cptBenchs = 0
			#while True:
			for cptBenchs in range(maxBenchs+1):
				if pollInterval == 0:
					resFile = './%s/%d/%d/comet_#%d' %(resDir,nClients, pubInterval, cptBenchs)
				elif pollInterval == -1:
					resFile = './%s/%d/%d/lp_#%d' %(resDir,nClients, pubInterval, cptBenchs)
				else:
					resFile = './%s/%d/%d/pull%d_#%d' %(resDir,nClients, pubInterval, pollInterval, cptBenchs)
				if os.path.exists(resFile):
					res_file = open(resFile, 'r')
					results = res_file.readlines()
					results = results[len(results)-1].strip().split(' ')
					if pubInterval == 0:
						mct = float(results[7])
					else:
						mct = 100 * ratio(int(results[1]),1000*pubInterval)
					stats.addResults(resFile,{'mct': int(mct), 'mpt': int(results[2]), 'rpm': int(results[3]), 'rupm': int(results[4]), 'ntb': int(float(results[5])), 'ntp': float(results[6])})
					#cptBenchs += 1
					benchsProcessed += 1
				else:
					minCptBenhs = min(minCptBenhs,cptBenchs)
					#break
			globalStats[nClients][pubInterval][pollInterval] = stats

for pubInterval in pubIntervalSet:
	for nClients in nClientsSet:
		for pollInterval in pollIntervalSet:
			count = globalStats[nClients][pubInterval][pollInterval].getCount()
			if count >= 5:
				globalStats[nClients][pubInterval][pollInterval].removeMin()
				globalStats[nClients][pubInterval][pollInterval].removeMax()
				if count >= 7:
					globalStats[nClients][pubInterval][pollInterval].removeMin()
					globalStats[nClients][pubInterval][pollInterval].removeMax()
			sMin = globalStats[nClients][pubInterval][pollInterval].getMin('rupm')
			sMax = globalStats[nClients][pubInterval][pollInterval].getMax('rupm')
			if sMin != None:
				if sMin < 0:
					print 'Warning for %s of %d %d %d: %d' %('rupm',nClients,pubInterval,pollInterval,sMin)
					nWarnings+=1
				elif (sMin == 0 or sMax/float(sMin) > 1.05) and sMax - sMin >= 5:
					print 'Warning for %s of %d %d %d: from %d to %d' %('rupm',nClients,pubInterval,pollInterval,sMin,sMax)
					nWarnings+=1

for statName in ['mct','mpt','rpm','rupm','ntb','ntp']:
	for pubInterval in pubIntervalSet:
		file = open('plots/%s_%d'%(statName,pubInterval),'w')
		file.write('nclients\tlp\tcomet\tpull1\tpull5\tpull15\tpull30\tpull50\n')
		file.write('# nClients: ')
		for i in range(len(nClientsSet)):
			file.write('%d -> %d, ' %(i,nClientsSet[i]))
		file.write('\n')
		for i in range(len(nClientsSet)):
			tmpstr = '%d' %(i)
			nClients = nClientsSet[i]
			for pollInterval in pollIntervalSet:
				statVal = globalStats[nClients][pubInterval][pollInterval].getAvg(statName)
				tmpstr += '\t%f' %(statVal if statVal != None else 0)
			file.write(tmpstr + '\n')

if nWarnings > 0:
	print '#warnings: %d' %nWarnings
print '#full iterations: %d' %minCptBenhs
print '#benchs processed: %d' %benchsProcessed
