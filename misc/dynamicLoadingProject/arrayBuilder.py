import xml.parsers.expat
import os
import gzip
import StringIO
import glob
import shutil
import time
import datetime
import sys

def generateStaticResource(srcFile,dstFile):
	file = open(srcFile,'rb')
	lines = file.readlines()
	if len(lines) != 0:
		fileData = reduce(lambda x,y: x + y,lines)
	else:
		fileData = ''
	
	cOut = open(dstFile,'w')
	
	# file data generation
	cOut.write('\n')
	cOut.write('\n/********** File data **********/\n')
	cOut.write('char smews_to_load[] = {\n')
	cOut.write(reduce(lambda x,y: x + "," + y,map(lambda x: hex(ord(x)),fileData)))
	cOut.write('};\n')
	
	cOut.close()

if len(sys.argv) != 3:
	print("USAGE : arrayBuilder binaryFile outputFile")
	sys.exit(1)

generateStaticResource(sys.argv[1],sys.argv[2]); 

print("--- Array created ---")
