import os
import sys
import os.path
import time
import shutil
import random
from datetime import datetime

startTime = datetime.now()

if len(sys.argv) == 2:
	resDir = sys.argv[1]
else:
	resDir = './results/'
	resDir += str(startTime).split('.')[0].replace(' ','_')
	os.system('svn log --revision HEAD | grep "line" | head -n 1 | cut -d "r" -f2 | cut -d " " -f 1 > .svn_rev')
	rev_file = open('.svn_rev', 'r')
	resDir += '_rev' + rev_file.readline().strip()
	os.remove('.svn_rev')
	os.mkdir(resDir)

cptBenchs = 0

nClientsSet = [1,16,32,64,128,255]
pubIntervalSet = [1,5,15,30,50,0]
pollIntervalSet = [-1,0,1,5,15,30,50]

while True:
	print '--> Iteration: %d' %cptBenchs
	for nClients in nClientsSet:
		#isFlashed = False
		print '--> nClients: %d' %nClients
		
		nClientsDirName = './%s/%d' %(resDir,nClients)
		if not os.path.exists(nClientsDirName):
			os.mkdir(nClientsDirName)

		for pubInterval in pubIntervalSet:
			if pubInterval == 0:
				print '--> pubInterval: random'
			else:
				print '--> pubInterval: %d' %pubInterval
			pubDirName = './%s/%d/%d' %(resDir,nClients,pubInterval)
			if not os.path.exists(pubDirName):
				os.mkdir(pubDirName)

			for pollInterval in pollIntervalSet:
				if pollInterval == 0:
					print '--> Comet'
				elif pollInterval == -1:
					print '--> Long polling'
				else:
					print '--> Pull %d' %pollInterval

				if pollInterval == 0:
					dstFile = './%s/%d/%d/comet_#%d' %(resDir,nClients, pubInterval, cptBenchs)
				elif pollInterval == -1:
					dstFile = './%s/%d/%d/lp_#%d' %(resDir,nClients, pubInterval, cptBenchs)
				else:
					dstFile = './%s/%d/%d/pull%d_#%d' %(resDir,nClients, pubInterval, pollInterval, cptBenchs)

				if os.path.exists(dstFile):
					print 'The file %s already exists' %dstFile
				else:
					#if not isFlashed:
						#print('Flashing Smews...')
						#os.system('sudo msp430-jtag -e ./smewsBins/smews%d.elf 2> /dev/null > /dev/null' %nClients)
						#isFlashed = True
					print 'The file %s does not exist' %dstFile
					cmdArgs = '%d %d %d' %(nClients, pollInterval, pubInterval)
					cptErrors = 0
					while cptErrors < 9:
						os.system('sudo /home/pegomas/myScripts/unConfigSlip 2> /dev/null > /dev/null')
						time.sleep(1)
						os.system('sudo /home/pegomas/myScripts/configSlip 2> /dev/null > /dev/null')
						os.system('sudo msp430-jtag 2> /dev/null > /dev/null')

						print "Starting bench %d %d %d #%d (%s)" %(nClients, pollInterval, pubInterval, cptBenchs,str(datetime.now()))
						time.sleep(25)
						os.system('python ./autobench.py %s > .bench_result' %cmdArgs);
						time.sleep(2)
						result_file = open('.bench_result', 'r')
						result = result_file.readlines()
						if result[len(result)-1].startswith('Results:'):
							print "Bench OK"
							break
						else:
							print "An error occured"
							cptErrors += 1
					result_file = open(dstFile, 'w')
					result.insert(0,'Iteration: %d\n'%cptErrors)
					result_file.writelines(result)
					result_file.close()
					os.remove('.bench_result')
	cptBenchs += 1
