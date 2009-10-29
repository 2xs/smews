# Copyright or c or Copr. 2008, Simon Duquennoy
# 
# Author e-mail: simon.duquennoy@lifl.fr
# 
# This software is a computer program whose purpose is to design an
# efficient Web server for very-constrained embedded system.
# 
# This software is governed by the CeCILL license under French law and
# abiding by the rules of distribution of free software.  You can  use, 
# modify and/ or redistribute the software under the terms of the CeCILL
# license as circulated by CEA, CNRS and INRIA at the following URL
# "http://www.cecill.info". 
# 
# As a counterpart to the access to the source code and  rights to copy,
# modify and redistribute granted by the license, users are provided only
# with a limited warranty  and the software's author,  the holder of the
# economic rights,  and the successive licensors  have only  limited
# liability. 
# 
# In this respect, the user's attention is drawn to the risks associated
# with loading,  using,  modifying and/or developing or reproducing the
# software by the user in light of its specific status of free software,
# that may mean  that it is complicated to manipulate,  and  that  also
# therefore means  that it is reserved for developers  and  experienced
# professionals having in-depth computer knowledge. Users are therefore
# encouraged to load and test the software's suitability as regards their
# requirements in conditions enabling the security of their systems and/or 
# data to be ensured and,  more generally, to use and operate it in the 
# same conditions as regards security. 
# 
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL license and that you accept its terms.

import os
import sys
sys.path.append('tools')
import GenContents

import sys, string

def strVersion(version):
	str = '.'
	for i in version:
		str += '.' + i
	return str[2:]

currVersion = string.split(string.split(sys.version)[0], ".")
minVersion = ['2', '5', '0']
if currVersion < minVersion:
	print 'Error: python version ' + strVersion(minVersion) + ' is required. Your current version is ' + strVersion(currVersion) + '.'
	exit(1)

# global names
# scons cache file
sconsCache = '.sconsign.dblite'
# base directories
targetBase = 'targets'
genBase = 'gen'
binBase = 'bin'
coreDir = 'core'
toolsDir = 'tools'
tmpBase = 'tmp'
wcBase = 'webContents'
httpCodesDir = 'httpCodes'
# the list of python files used by this SConstruct
toolsList = map(lambda x: os.path.join(toolsDir,x),filter(lambda x: x.endswith('.py'), os.listdir(toolsDir)))
projectName = 'smews'
elfName = projectName + '.elf'
libName = projectName + '.a'

# options management
opts = Variables()
opts.Add(ListVariable('target', 'Set the target', 'none', filter(lambda x: not x.startswith('.'),os.listdir('targets'))))
opts.Add('contents', 'Set the Web contents directories:\na list of directories name in webContents, possibly preceded by a replacement URL\nExample: inclusion of comet, generator (remplaced by gen in the URL), smews (as root of the file system):\ncontents=comet,gen:generator,:smews\n', None)
opts.Add(BoolVariable('gzip', 'Set to 1 to gzip (at compile time) static contents', True))
opts.Add(BoolVariable('debug', 'Set to 1 to build for debug', False))
opts.Add(BoolVariable('sdump', 'Set to 1 to include stack dump', False))
# the list of disableable options
disabledHash = {}
disabledHash['timers'] = 'DISABLE_TIMERS'
disabledHash['comet'] = 'DISABLE_COMET'
disabledHash['arguments'] = 'DISABLE_ARGS'
opts.Add(ListVariable('disable', 'Disable smews functionnalities', 'none', disabledHash.keys()))
opts.Add(ListVariable('endian', 'Force endianness', 'none', ['little','big']))
opts.Add('chuncksNbits', 'Set the checksum chuncks size', 5)

# environment creation, options extraction
globalEnv = Environment(tools = ['gcc','ar','gnulink'], ENV = os.environ, options = opts)
Help(opts.GenerateHelpText(globalEnv))

# arguments are stored into variables
gzipped = globalEnv['gzip']
debug = globalEnv['debug']
sdump = globalEnv['sdump']
chuncksNbits = int(globalEnv['chuncksNbits'])
toDisable = globalEnv['disable']
endian = globalEnv['endian']
if endian:
	endian = endian[0]
targets = map(lambda x: os.path.normpath(str(x)),globalEnv['target'])

# contents to be embedded with smews
if globalEnv.has_key('contents'):
	originalWcDirs = globalEnv['contents']
else:
	originalWcDirs = ':smews'

# clean rule used if no target: clean all
if len(targets) == 0:
	pycFiles = []
	for file in os.listdir(toolsDir):
		if file.endswith('.pyc'):
			pycFiles.append(os.path.join(toolsDir,file))
	Clean('.',[binBase,genBase,sconsCache,pycFiles])

# the wcDirs map contains associations between application URLs and real paths
# ex.: contents = :smews,myApp:myApplication,test
# will generate :
# / -> smews, myApp/ -> myApplication, test/ -> test
wcDirs = originalWcDirs.split(',')
dirsMap = {}
for wcDir in set(wcDirs + [httpCodesDir]):
	if wcDir != '':
		idx = wcDir.find(':')
		if idx != -1:
			dirsMap[wcDir[idx+1:]] = '/' + wcDir[:idx]
		else:
			dirsMap[wcDir] = '/' + wcDir

# association between web contents files and their final URLs
# wcDir did only contain association of embedded applications
# here, we retrieve all the files of each application
sourcesMap = {}
for wcDir in dirsMap.keys():
	wcDirPath = os.path.join(wcBase,wcDir)
	if not os.path.isdir(wcDirPath):
		Exit('Error: directory ' + wcDirPath + ' does not exist')
	wcFiles = GenContents.getWCFiles(wcDirPath)
	for file in wcFiles:
		wcSource = os.path.join(wcBase,wcDir,file)
		sourcesMap[wcSource] = os.path.join(dirsMap[wcDir],file)

# compilation options
globalEnv.Replace(CC = 'gcc')
globalEnv.Replace(AS = 'as')
globalEnv.Replace(AR = 'ar')
globalEnv.Append(CCFLAGS = '-Wall')
if sdump:
	globalEnv.Append(CCFLAGS = '-DSTACK_DUMP')
if debug:
	globalEnv.Append(CCFLAGS = '-O0 -g')
else:
	globalEnv.Append(CCFLAGS =  '-Os')
globalEnv.Append(CPPDEFINES = {'CHUNCKS_NBITS' : str(chuncksNbits)})
for func in toDisable:
	globalEnv.Append(CPPDEFINES = { disabledHash[func] : '1'})
if endian:
	if endian == 'little':
		globalEnv.Append(CPPDEFINES = { 'ENDIANNESS' : 'LITTLE_ENDIAN'})
	if endian == 'big':
		globalEnv.Append(CPPDEFINES = { 'ENDIANNESS' : 'BIG_ENDIAN'})
sconsBasePath = os.path.abspath('.')

# builds each target
for target in targets:
	# target dependent directories / files
	targetDir = os.path.join(targetBase,target)
	driversDir = os.path.join(targetDir,'drivers')
	binDir = os.path.join(binBase,target)
	genDir = os.path.join(genBase,target)
	elfFileName = os.path.join(binDir,elfName)
	libFileName = os.path.join(binDir,libName)

	# target dependent compilation options
	env = globalEnv.Clone()
	env.Replace(CPPPATH = [coreDir,driversDir,genDir])
	
	# required directories creation
	if not env.GetOption('clean'):
		for dir in [genBase,genDir,os.path.join(genDir,tmpBase)]:
			if not os.path.isdir(dir):
				os.mkdir(dir)
			
	# export variables for external SConscript files
	Export('env libFileName elfFileName binDir coreDir driversDir genDir wcBase toolsList chuncksNbits sourcesMap gzipped')
	Export('env targetDir binDir projectName elfName')
	Export('dirsMap sourcesMap target sconsBasePath httpCodesDir tmpBase')
	# target dependent SConscript call
	SConscript(os.path.join(targetDir,'SConscript'),build_dir = binDir,duplicate = 0)
	# possible web applications SConscript calls
	for wcDir in dirsMap.keys():
		wcSconsFile = os.path.join(wcBase,wcDir,'SConscript')
		if os.path.isfile(wcSconsFile):
			ret = SConscript(wcSconsFile)
			sourcesMap.update(ret)
	# main SConscript call
	SConscript('SConscript')
