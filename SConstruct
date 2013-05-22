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
import GenApps
from SCons.Errors import BuildError

import sys, string

import IPy
from IPy import IP

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
appBase = 'apps'
httpCodesDir = 'httpCodes'
# the list of python files used by this SConstruct
toolsList = map(lambda x: os.path.join(toolsDir,x),filter(lambda x: x.endswith('.py'), os.listdir(toolsDir)))
projectName = 'smews'
elfName = projectName + '.elf'
libName = projectName + '.a'

# options management
opts = Variables()
opts.Add(ListVariable('target', 'Set the target', 'none', filter(lambda x: not x.startswith('.'),os.listdir('targets'))))
opts.Add('apps', 'Set the Web applications directories:\na list of directories name in %s, possibly preceded by a replacement URL\nExample: inclusion of comet, generator (remplaced by gen in the URL), smews (as root of the file system):\napps=comet,gen:generator,:smews\n' %(appBase), None)
opts.Add(BoolVariable('gzip', 'Set to 1 to gzip (at compile time) static Web resources', True))
opts.Add(BoolVariable('debug', 'Set to 1 to build for debug', False))
opts.Add(BoolVariable('sdump', 'Set to 1 to include stack dump', False))
opts.Add(BoolVariable('test', 'Set to 1 to test the test apps', False))
opts.Add('ipaddr', 'Set the IP address of Smews', None)
# the list of disableable options
disabledHash = {}
disabledHash['timers'] = 'DISABLE_TIMERS'
disabledHash['comet'] = 'DISABLE_COMET'
disabledHash['arguments'] = 'DISABLE_ARGS'
disabledHash['post'] = 'DISABLE_POST'
disabledHash['gpip'] = 'DISABLE_GP_IP_HANDLER'
disabledHash['coroutines'] = 'DISABLE_COROUTINES'
opts.Add(ListVariable('disable', 'Disable smews functionnalities', 'none', disabledHash.keys()))
opts.Add(ListVariable('endian', 'Force endianness', 'none', ['little','big']))
opts.Add('chuncksNbits', 'Set the checksum chuncks size', 5)

# environment creation, options extraction
globalEnv = Environment(tools = ['gcc','as','ar','gnulink'], ENV = os.environ, options = opts)
Help(opts.GenerateHelpText(globalEnv))

# Check for bad options
if opts.UnknownVariables():
        err_msg = "Following options are not supported:"
        for k in opts.UnknownVariables().keys():
                err_msg = err_msg + " {0}".format(k)
        sys.stderr.write("{0}\n".format(err_msg))
        raise BuildError(err_msg)

# arguments are stored into variables
gzipped = globalEnv['gzip']
debug = globalEnv['debug']
sdump = globalEnv['sdump']
chuncksNbits = int(globalEnv['chuncksNbits'])
toDisable = globalEnv['disable']
if 'coroutines' in toDisable and 'post' not in toDisable:
    toDisable.append('post')
endian = globalEnv['endian']
test = globalEnv['test']
if endian:
	endian = endian[0]

if test:
	globalEnv['ipaddr'] = '192.168.1.2'
	globalEnv['apps'] = 'post_test'
	globalEnv['target'].append('linux')

targets = map(lambda x: os.path.normpath(str(x)),globalEnv['target'])

# static configuration of the ip adress
if globalEnv.has_key('ipaddr'):
	ipVersion = IP(globalEnv['ipaddr']).version();

	if (ipVersion == 4):
		parts = str(IP(globalEnv['ipaddr']).strFullsize()).split(".");
		ipAdress = parts[3]+","+parts[2]+","+parts[1]+","+parts[0]
		globalEnv.Append(CPPDEFINES = { 'IP_ADDR' : ipAdress })
	elif (ipVersion == 6):
		hexIPAdress = str(IP(globalEnv['ipaddr']).strHex());
		globalEnv.Append(CPPDEFINES = { 'IPV6' : '1'})
		ipAdress = ''
		low_index = 32
		high_index = 34

		for i in range(15):
			ipAdress += "0x"+hexIPAdress[low_index:high_index]+","
			low_index = low_index - 2
			high_index = high_index - 2

		ipAdress += "0x"+hexIPAdress[low_index:high_index]

	globalEnv.Append(CPPDEFINES = { 'IP_ADDR' : ipAdress })

# applications to be embedded with smews
if globalEnv.has_key('apps'):
	originalAppDirs = globalEnv['apps']
else:
	originalAppDirs = ':welcome'

# clean rule used if no target: clean all
if len(targets) == 0:
	pycFiles = []
	for file in os.listdir(toolsDir):
		if file.endswith('.pyc'):
			pycFiles.append(os.path.join(toolsDir,file))
	Clean('.',[binBase,genBase,sconsCache,pycFiles])

# the appDirs map contains associations between application URLs and real paths
# ex.: apps = :smews,myApp:myApplication,test
# will generate :
# / -> smews, myApp/ -> myApplication, test/ -> test
appDirs = originalAppDirs.split(',')
dirsMap = {}
for appDir in set(appDirs + [httpCodesDir]):
	if appDir != '':
		idx = appDir.find(':')
		if idx != -1:
			dirsMap[appDir[idx+1:]] = '/' + appDir[:idx]
		else:
			dirsMap[appDir] = '/' + appDir

# association between web applicative resources and their final URLs
# appDir did only contain association of embedded applications
# here, we retrieve all the files of each application
sourcesMap = {}
for appDir in dirsMap.keys():
	appDirPath = os.path.join(appBase,appDir)
	if not os.path.isdir(appDirPath):
		Exit('Error: directory ' + appDirPath + ' does not exist')
	appFiles = GenApps.getAppFiles(appDirPath)
	for file in appFiles:
		appSource = os.path.join(appBase,appDir,file)
		sourcesMap[appSource] = os.path.join(dirsMap[appDir],file)

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
	globalEnv.Append(CCFLAGS =  '-Os -ffunction-sections -fdata-sections -fno-strict-aliasing')
	globalEnv.Append(LINKFLAGS = '-Wl,--gc-sections -Wl,--print-gc-sections')
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
	Export('env libFileName elfFileName binDir coreDir driversDir genDir appBase toolsList chuncksNbits sourcesMap gzipped test')
	Export('env targetDir binDir projectName elfName')
	Export('dirsMap sourcesMap target sconsBasePath httpCodesDir tmpBase')
	# target dependent SConscript call
	SConscript(os.path.join(targetDir,'SConscript'),variant_dir = binDir,duplicate = 0)
	# possible web applications SConscript calls
	for appDir in dirsMap.keys():
		appSconsFile = os.path.join(appBase,appDir,'SConscript')
		if os.path.isfile(appSconsFile):
			ret = SConscript(appSconsFile)
			sourcesMap.update(ret)
	# main SConscript call
	SConscript('SConscript')
