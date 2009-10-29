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
import GenContents

# imports from SConstruct
Import('env libFileName elfFileName binDir coreDir driversDir genDir wcBase toolsList chuncksNbits sourcesMap gzipped')

# returns the list of .c and .s files in dir, prefixed by dstDir
def getAllSourceFiles(dir, dstDir):
	cFiles = []
	sFiles = []
	for file in os.listdir(dir):
		if file.endswith('.c'):
			cFiles.append(os.path.join(dstDir,file))
		elif file.endswith('.s'):
				sFiles.append(os.path.join(dstDir,file))
	return sFiles + cFiles

# content generation builders creation
# builder used to generate both static and dynamic contents
def generateContent(target, source, env):
	if propsFilesMap.has_key(str(source[0])):
		GenContents.generateContent(str(source[0]),str(target[0]),chuncksNbits,gzipped,propsFilesMap[str(source[0])])
	else:
		GenContents.generateContent(str(source[0]),str(target[0]),chuncksNbits,gzipped,None)
	return None

# builder used to generate the file index, with the URLs tree
def generateIndex(target, source, env):
	GenContents.generateFilesIndex(genDir,sourcesMap,str(target[0]),chuncksNbits,wcBase,propsFilesMap)
	return None

# builder used to generate the channels header file
def generateChannelsH(target, source, env):
	GenContents.generateChannelsH(str(target[0]),propsFilesMap)
	return None

# builder used to generate to synthetized properties of generator files
# a property file is created for each generator file, it synthetizes some of its XML information
# all information used to build the file-index.c file or the channel.h file MUST be included in the props file
def generateContentProps(target, source, env):
	GenContents.generateContentProps(str(source[0]),propsFilesMap[str(source[0])])
	return None

env['BUILDERS']['GenContent'] = Builder(action = generateContent)
env['BUILDERS']['GenContentsIndex'] = Builder(action = generateIndex)
env['BUILDERS']['GenChannelsH'] = Builder(action = generateChannelsH)
env['BUILDERS']['GenContentProps'] = Builder(action = generateContentProps)

# build directories settings, with no source duplication
BuildDir(os.path.join(binDir,'core'), coreDir, duplicate=0)
BuildDir(os.path.join(binDir,'drivers'), driversDir, duplicate=0)

# contents files index and channel files settings
contentsIndexO = os.path.join(binDir,'gen','files_index')
contentsIndexC = os.path.join(genDir,'files_index.c')
channelsH = os.path.join(genDir,'channels.h')
wcListName = os.path.join(genDir,'wcList')

# loop on each web content in order to generate associated c files
# static files generate pre-computed c files
# c generators are enriched with new declarations (from their XML)
# other c and h files are simply copied
usedContentsFiles = []
genObjects = []
propsFilesList = []
propsFilesMap = {}
for file in sourcesMap.keys():
	fileName = GenContents.getFileName(file,chuncksNbits,gzipped)
	if fileName.startswith(genDir):
		fileName = fileName[len(genDir)+1:]
	if file.endswith('.h'):
		cFile = os.path.join(genDir,fileName) + '.h'
	else:
		cFile = os.path.join(genDir,fileName) + '.c'
	env.GenContent(cFile,file)
	env.Depends(cFile,toolsList)
	if file.endswith('.c'): # this is a generator or a c file to compile
		propsFile = cFile + '.props'
		env.GenContentProps(propsFile,file)
		env.Depends(propsFile,toolsList)
		propsFilesList.append(propsFile)
		propsFilesMap[file] = {'fileName' : propsFile}
	elif file.endswith('.h'): # header files have not to be embedded
		propsFilesMap[file] = {'fileName' : '', 'hasXml' : False, 'channel' : ''}
	usedContentsFiles.append(file)
	if not file.endswith('.h'):
		objFile = os.path.join(binDir,'gen',fileName)
		genObjects.append(env.Object(objFile,cFile))

# files index generated file dependencies management, via a wcList file (for SCons dependencies tree)
# wcList is a file that contains the list of the web contents embedded with smews
wcList = open(wcListName,'w')
for source in sourcesMap.keys():
	wcList.write(sourcesMap[source] + ' -> ' + source + '\n')
wcList.close()

# files index generation
env.GenContentsIndex(contentsIndexC,[])
env.Depends(contentsIndexC,wcListName)
env.Depends(contentsIndexC,toolsList)
env.Depends(contentsIndexC,propsFilesList)
# files index object file
genObjects.append(env.Object(contentsIndexO, contentsIndexC))
# channels header file
env.GenChannelsH(channelsH,propsFilesList)
env.Depends(channelsH,toolsList)

# engine source code dependencies
coreFiles = getAllSourceFiles(coreDir, os.path.join(binDir,'core'))
# target drivers source code dependencies
targetFiles = getAllSourceFiles(driversDir, os.path.join(binDir,'drivers'))
# create a library from all sources
lib = env.Library(libFileName, targetFiles + coreFiles + genObjects)
# link the library into a elf file
final = env.Program(elfFileName, targetFiles + coreFiles + genObjects)
# clean
Clean([lib,final],[binDir,genDir])
