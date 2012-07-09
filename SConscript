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
import GenApps

# imports from SConstruct
Import('env libFileName elfFileName binDir coreDir driversDir genDir appBase toolsList chuncksNbits sourcesMap gzipped test')

# returns the list of .c and .s files in dir, prefixed by dstDir
def getAllSourceFiles(dir, dstDir):
	sourceFiles = []
	for file in os.listdir(dir):
		if os.path.isdir(os.path.join(dir,file)):
			sourceFiles.extend(getAllSourceFiles(os.path.join(dir,file), os.path.join(dstDir,file)))
		elif file.endswith('.c'):
			sourceFiles.append(os.path.join(dstDir,file))
		elif file.endswith('.s'):
			sourceFiles.append(os.path.join(dstDir,file))
	return sourceFiles

# builders for web applicative resources creation
# used to generate both static and dynamic resources
def generateResource(target, source, env):
	if propsFilesMap.has_key(str(source[0])):
		GenApps.generateResource(str(source[0]),str(target[0]),chuncksNbits,gzipped,propsFilesMap[str(source[0])])
	else:
		GenApps.generateResource(str(source[0]),str(target[0]),chuncksNbits,gzipped,None)
	return None

# builder used to generate the file index, with the URLs tree
def generateIndex(target, source, env):
	GenApps.generateIndex(genDir,sourcesMap,str(target[0]),chuncksNbits,appBase,propsFilesMap)
	return None

# builder used to generate the channels header file
def generateChannelsH(target, source, env):
	GenApps.generateChannelsH(str(target[0]),propsFilesMap)
	return None

# builder used to generate to synthetized properties of generator files
# a property file is created for each generator file, it synthetizes some of its XML information
# all information used to build the file-index.c file or the channel.h file MUST be included in the props file
def generateResourceProps(target, source, env):
	GenApps.generateResourceProps(str(source[0]),propsFilesMap[str(source[0])])
	return None

def generateDefinesH(target, source, env):
	GenApps.writeDefinesH(str(target[0]))
	return None

def generateBlobsH(target, source, env):
	GenApps.generateBlobsH(str(target[0]))
	return None

def runTest(target, source, env):
	os.system('tools/launch_test')
	return None


env['BUILDERS']['GenBlobsH'] = Builder(action = generateBlobsH)
env['BUILDERS']['GenDefinesH'] = Builder(action = generateDefinesH)
env['BUILDERS']['GenResource'] = Builder(action = generateResource)
env['BUILDERS']['GenIndex'] = Builder(action = generateIndex)
env['BUILDERS']['GenChannelsH'] = Builder(action = generateChannelsH)
env['BUILDERS']['GenResourceProps'] = Builder(action = generateResourceProps)

# build directories settings, with no source duplication
VariantDir(os.path.join(binDir,'core'), coreDir, duplicate=0)
VariantDir(os.path.join(binDir,'drivers'), driversDir, duplicate=0)

# applications files index and channel files settings
resourcesIndexO = os.path.join(binDir,'gen','resources_index')
resourcesIndexC = os.path.join(genDir,'resources_index.c')
channelsH = os.path.join(genDir,'channels.h')
appListName = os.path.join(genDir,'appList')
definesH = os.path.join(genDir,'defines.h')
blobsH = os.path.join(genDir,'blobs.h')
# loop on each web resource in order to generate associated c files
# static resources generate pre-computed c files
# dynamic resources are enriched with new declarations (from their XML)
# other c and h files are simply copied
appFiles = []
genObjects = []
propsFilesList = []
propsFilesMap = {}
for file in sourcesMap.keys():
	fileName = GenApps.getFileName(file,chuncksNbits,gzipped)
	if fileName.startswith(genDir):
		fileName = fileName[len(genDir)+1:]
	if file.endswith('.h'):
		cFile = os.path.join(genDir,fileName) + '.h'
	else:
		cFile = os.path.join(genDir,fileName) + '.c'
	env.GenResource(cFile,file)
	env.Depends(cFile,toolsList)
	if file.endswith('.c'): # this is a generator or a c file to compile
		propsFile = cFile + '.props'
		env.GenResourceProps(propsFile,file)
		env.Depends(propsFile,toolsList)
		propsFilesList.append(propsFile)
		propsFilesMap[file] = {'fileName' : propsFile}
	elif file.endswith('.h'): # header files have not to be embedded
		propsFilesMap[file] = {'fileName' : '', 'hasXml' : False, 'channel' : ''}
	appFiles.append(file)
	if not file.endswith('.h'):
		objFile = os.path.join(binDir,'gen',fileName)
		genObjects.append(env.Object(objFile,cFile))

# files index generated file dependencies management, via a appList file (for SCons dependencies tree)
# appList is a file that contains the list of the web applicative resources embedded with smews
appList = open(appListName,'w')
for source in sourcesMap.keys():
	appList.write(sourcesMap[source] + ' -> ' + source + '\n')
appList.close()

# files index generation
env.GenIndex(resourcesIndexC,[])
env.Depends(resourcesIndexC,appListName)
env.Depends(resourcesIndexC,toolsList)
env.Depends(resourcesIndexC,propsFilesList)
# files index object file
genObjects.append(env.Object(resourcesIndexO, resourcesIndexC))
# channels header file
env.GenChannelsH(channelsH,propsFilesList)
env.Depends(channelsH,toolsList)
env.GenDefinesH(definesH,[])
env.GenBlobsH(blobsH,[])
# engine source code dependencies
coreFiles = getAllSourceFiles(coreDir, os.path.join(binDir,'core'))
# target drivers source code dependencies
targetFiles = getAllSourceFiles(driversDir, os.path.join(binDir,'drivers'))
# create a library from all sources
lib = env.Library(libFileName, targetFiles + coreFiles + genObjects)
# link the library into a elf file
if env['BUILDERS']['Program'] is not None:
	final = env.Program(elfFileName, targetFiles + coreFiles + genObjects)
else:
	final = None
# clean
Clean([lib,final],[binDir,genDir])
