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

import xml.parsers.expat
import GenBlob
import os
import gzip
import StringIO
import glob
import shutil
import time
import datetime

# initialization
propertiesInfos = None
channelInfos = None
handlerInfos = {}
argsList = []
chuncksSize = 0
cOutName = 'pages.c'
mimeListFileName = os.path.join('tools','mimeList')
mimeHash = {}
StaticContent, ServiceContent = range(2)
argsTypesMap = {'str': 'arg_str',
'uint8' : 'arg_ui8',
'uint16' : 'arg_ui16',
'uint32' : 'arg_ui32'
}

# process the Mime file
for line in open(mimeListFileName,'r'):
	split = line.split()
	if len(split) == 2:
		mimeHash[split[0]]=split[1]

# get Mime type from a file extension
def getMime(hash,ext):
	lowExt = ext.lower()
	if hash.has_key(lowExt):
		return hash[lowExt]
	else:
		return 'text/plain'

# write files header
def writeHeader(file,enriched):
	if enriched == 0:
		tmp = 'generated'
	else:
		tmp = 'enriched'
	file.write('/*\n')
	file.write('* This file has been ' + tmp + ' by GenContents, a tool of the smews project\n')
	file.write('* smews home page: http://www2.lifl.fr/~duquenno/Research/smews\n')
	t = datetime.datetime.now()
	t = time.mktime(t.timetuple())
	file.write('* Generation date: ' + str(datetime.datetime.utcfromtimestamp(t).ctime()) + '\n')
	file.write('*/\n\n')

# replaces dot and file separators by underscores
def getCName(fileName):
	return fileName.replace('.','_').replace(os.sep,'_')

# returns the name of the generated file for file fileName
def getFileName(fileName,chuncksNBits,gzipped):
	if getContentType(fileName) == ServiceContent:
		fileName = fileName[0:fileName.rfind('.')]
	else:
		prefix = ('g' if gzipped else 'r') + '_'
		prefix += 'c' + str(chuncksNBits) + '_'
		fileName = os.path.join(os.path.dirname(fileName),prefix + os.path.basename(fileName))
	if fileName.endswith('.embed'):
		fileName = fileName[:fileName.rfind('.')]
	return fileName.replace('.','_')

# used by getWCFiles
def getWCFilesRec(wcPath,path):
	files=[]
	for file in glob.glob(path + '/*'):
		if os.path.basename(file) != 'SConscript':
        		if os.path.isdir(file):
					files = files + (getWCFilesRec(wcPath,file))
        		elif not file.endswith('~'):
					files.append(file[len(wcPath)+1:])
	return files

# returns all files contained in path (with recurssion)
def getWCFiles(path):
	return getWCFilesRec(path,path)

# content type from original content file
def getContentType(path):
	if path.endswith('.c') or path.endswith('.h'):
		return ServiceContent
	else:
		return StaticContent

# extracts c file properties from its xml data
# returns the file contents
def extractPropsFromXml(srcFile,dstFileInfos):
	# open the source file in order to parse the XML and return the file contents
	file = open(srcFile,'r')
	lines = file.readlines()
	if len(lines) > 1:
		fileContents = reduce(lambda x,y: x + y,lines)
	else:
		fileContents = ''
	# prcoess XML only if dstFilInfos do not already contains informations
	if not dstFileInfos.has_key('hasXml'):
		# 3 XML handler functions
		def start_element(name, attrs):
			global argsList
			global handlerInfos
			global propertiesInfos
			global channelInfos
			if name == 'args':
				argsList = []
			elif name == 'arg':
				argsList.append(attrs)
			elif name == 'handlers':
				handlerInfos = attrs
			elif name == 'properties':
				propertiesInfos = attrs
			elif name == 'channel':
				channelInfos = attrs

		def end_element(name):
			return
		def char_data(data):
			return

		# select the XML part of the c file
		xmlRoot = 'generator'		
		xmlContents = fileContents[fileContents.rfind('<' + xmlRoot + '>'):]
		xmlContents = xmlContents[:xmlContents.rfind('</' + xmlRoot + '>') + len(xmlRoot) + 3]
		
		# default value for the channel field
		dstFileInfos['channel'] = ''
		dstFileInfos['isStream'] = False
		# XML data have been found
		if len(xmlContents) > 1:
			global argsList
			global handlerInfos
			global propertiesInfos
			global channelInfos
			
			# init globals used for parsing
			propertiesInfos = None
			channelInfos = None
			handlerInfos = {}
			argsList = []

			# parse the XML
			p = xml.parsers.expat.ParserCreate()
			p.StartElementHandler = start_element
			p.EndElementHandler = end_element
			p.CharacterDataHandler = char_data
			p.Parse(xmlContents, 0)

			# doGet handler
			if handlerInfos == None or not handlerInfos.has_key('doGet'):
				exit('Error: the file ' + srcFile + ' does not describe a doGet handler')
			dstFileInfos['doGet'] = handlerInfos['doGet']
			# init handler
			if handlerInfos.has_key('init'):
				dstFileInfos['init'] = handlerInfos['init']
			# initGet handler
			if handlerInfos.has_key('initGet'):
				dstFileInfos['initGet'] = handlerInfos['initGet']
			# generator arguments
			dstFileInfos['argsList'] = argsList
			# generator type (persistent by default)
			if propertiesInfos != None and propertiesInfos.has_key('type'):
				dstFileInfos['type'] = propertiesInfos['type']
			else:
				dstFileInfos['type'] = 'persistent'
			# comet channel name
			if channelInfos != None and channelInfos.has_key('name'):
				dstFileInfos['channel'] = channelInfos['name']
			# comet channel type
			if channelInfos != None and channelInfos.has_key('type'):
				dstFileInfos['isStream'] = channelInfos['type'] == 'stream'

		# boolean used to know if the c file contains XML data
		dstFileInfos['hasXml'] = len(xmlContents) > 1
	return fileContents

# generates the .props file for a c file
# the .props file contains a subset of the information described in generators XMLs
# all information used to build the file-index.c file or the channel.h file MUST be included in the props file
def generateContentProps(srcFile,dstFileInfos):
	# extract the properties from the XML (if needed)
	extractPropsFromXml(srcFile,dstFileInfos)
	# store the hasXml flag and the comet channel
	pOut = open(dstFileInfos['fileName'],'w')
	pOut.write('1\n' if dstFileInfos['hasXml'] else '0\n')
	pOut.write(dstFileInfos['channel'] + '\n')
	pOut.close()

# launches a content file generation
def generateContent(srcFile,dstFile,chuncksNbits,gzipped,dstFileInfos):
	if getContentType(srcFile) == ServiceContent:
		generateServiceContent(srcFile,dstFile,dstFileInfos)
	else:
		generateStaticContent(srcFile,dstFile,chuncksNbits,gzipped)

# service: the generator file is enriched
def generateServiceContent(srcFile,dstFile,dstFileInfos):
	# extract the properties from the XML (if needed)
	fileContents = extractPropsFromXml(srcFile,dstFileInfos)
	# if the c/h file does not conatin any XML, we simply copy it: this is not a generator
	if not dstFileInfos['hasXml']:
		shutil.copyfile(srcFile,dstFile)
	# here, the c file is a generator. It will be enriched
	else:
		cFuncName = getCName(srcFile[:srcFile.rfind('.c')])

		generatedHeader = '#include "generators.h"\n'
		generatedHeader += '#include "stddef.h"\n\n'

		generatedOutputHandler = '/********** Output handler **********/\n'
		# handler functions declaration
		if dstFileInfos.has_key('init'):
			generatedOutputHandler += 'static init_service_func_t ' + dstFileInfos['init'] + ';\n'
		if dstFileInfos.has_key('initGet'):
			generatedOutputHandler += 'static initget_service_func_t ' + dstFileInfos['initGet'] + ';\n'
		generatedOutputHandler += 'static doget_service_func_t ' + dstFileInfos['doGet'] + ';\n'
		# output_handler structure creation
		generatedOutputHandler += 'CONST_VAR(struct output_handler_t, ' + cFuncName + ') = {\n'
		# handler type
		generatedOutputHandler += '\t.handler_type = type_generator,\n'
		generatedOutputHandler += '\t.handler_comet = %d,\n' %(1 if dstFileInfos['channel'] != '' else 0)
		generatedOutputHandler += '\t.handler_stream = %d,\n' %(1 if dstFileInfos['isStream'] else 0)
		if dstFileInfos['type'] == 'persistent':
			etype = 'prop_persistent'
		elif dstFileInfos['type'] == 'idempotent':
			etype = 'prop_idempotent'
		elif dstFileInfos['type'] == 'volatile':
			etype = 'prop_volatile'
		if etype == None:
			etype = 'prop_persistent'
		# the handler is a service, we fill the function handlers fields
		generatedOutputHandler += '\t.handler_contents = {\n'
		generatedOutputHandler += '\t\t.service = {\n'
		generatedOutputHandler += '\t\t\t.prop = %s,\n' %etype
		# init
		if not dstFileInfos.has_key('init'):
			generatedOutputHandler += '\t\t\t.init = NULL,\n'
		else:
			generatedOutputHandler += '\t\t\t.init = ' + dstFileInfos['init'] + ',\n'
		# initGet
		if dstFileInfos.has_key('initGet'):
			generatedOutputHandler += '\t\t\t.initget = ' + dstFileInfos['initGet'] + ',\n'
		# doGet
		generatedOutputHandler += '\t\t\t.doget = ' + dstFileInfos['doGet'] + ',\n'
		generatedOutputHandler += '\t\t}\n'
		generatedOutputHandler += '\t},\n'
		# generator arguments information are also written in the structure
		generatedOutputHandler += '#ifndef DISABLE_ARGS\n'
		generatedOutputHandler += '\t.handler_args = {\n'
		if len(dstFileInfos['argsList']) > 0:
			generatedOutputHandler += '\t\t.args_tree = args_tree,\n'
			generatedOutputHandler += '\t\t.args_index = args_index,\n'
			generatedOutputHandler += '\t\t.args_size = sizeof(struct args_t)\n'
		else:
			generatedOutputHandler += '\t\t.args_tree = NULL,\n'
			generatedOutputHandler += '\t\t.args_index = NULL,\n'
			generatedOutputHandler += '\t\t.args_size = 0\n'
		generatedOutputHandler += '\t}\n'
		generatedOutputHandler += '#endif\n'
		generatedOutputHandler += '};\n'
		
		# arguments c structure creation (this structure is directly used by the generator functions)
		generatedArgsStruc = '/********** Arguments structure **********/\n'
		generatedArgsStruc += 'struct args_t {\n'
		# for each argument, create a filed in the structure
		for attrs in dstFileInfos['argsList']:
			tmpString = ''
			if attrs['type'] == 'str':
				tmpString = 'char ' + attrs['name'] + '[' + attrs['size'] + ']'
			else:
				tmpString = attrs['type'] + '_t ' + attrs['name']
			generatedArgsStruc += '\t' + tmpString + ';\n'
		generatedArgsStruc += '};\n'
			
		# arguments index creation
		generatedIndex = '/********** Arguments index **********/\n'
		generatedIndex += 'static CONST_VAR(struct arg_ref_t, args_index[]) = {\n'
		# for each argument, type and size informations
		# the offset (in bytes in the structure) is also provided
		for attrs in dstFileInfos['argsList']:
			tmpType = ''
			if attrs['type'] == 'str':
				tmpType = 'char[' + attrs['size'] + ']'
			else:
				tmpType = attrs['type'] + '_t'
			generatedIndex += '\t{arg_type: ' + argsTypesMap[attrs['type']] + ', arg_size: sizeof(' + tmpType + '), arg_offset: offsetof(struct args_t,' + attrs['name'] + ')},\n'
		generatedIndex += '};\n'
	
		# new c file creation
		cOut = open(dstFile,'w')
		writeHeader(cOut,1)
		cOut.write(generatedHeader)
		if len(dstFileInfos['argsList']) > 0:
			cOut.write('#ifndef DISABLE_ARGS\n')
			cOut.write(generatedArgsStruc)
			cOut.write('\n')
			cOut.write(generatedIndex)
			GenBlob.genBlobTree(cOut,map(lambda x: x['name'],dstFileInfos['argsList']),'args_tree',True)
			cOut.write('#endif\n')
		cOut.write('\n')
		cOut.write(generatedOutputHandler)
		# the end of the file contains the original c file
		cOut.write('\n/* End of the enriched part */\n\n')
		cOut.write(fileContents)
		cOut.close()

# static file: the file is completely pre-processed into a c file:
# HTTP header insertion
# chuncks checksums calculation
def generateStaticContent(srcFile,dstFile,chuncksNbits,gzipped):
	# HTTP header generation and concatenation with file contents
	try:
		# open the source file
		file = open(srcFile,'rb')
		lines = file.readlines()
		if len(lines) != 0:
			fileContents = reduce(lambda x,y: x + y,lines)
		else:
			fileContents = ''
		if gzipped:
			# the whole file is gzipped
			sio = StringIO.StringIO()
			gzipper = gzip.GzipFile(mode="wb", fileobj=sio, compresslevel=9)
			gzipper.write(fileContents)
			gzipper.close()
			fileContents = sio.getvalue()
		# HTTP header insertion before the file contents
		tmp = str(len(fileContents)) + '\r\nServer: Smews\r\nContent-Type: ' + getMime(mimeHash,srcFile[srcFile.rfind('.')+1:])
		if gzipped:
			tmp += '\r\nContent-Encoding: gzip'
		#~ headerPadding = 205
		#~ if headerPadding > 4:
			#~ tmp += '\r\na:'
			#~ for i in range(headerPadding-4):
				#~ tmp += '0'
		tmp += '\r\nConnection: Keep-Alive\r\n\r\n' + fileContents
		fileContents = tmp
		if os.path.basename(srcFile) == '404.html':
			fileContents = 'HTTP/1.1 404 Not Found\r\nContent-Length: ' + fileContents
		else:
			fileContents = 'HTTP/1.1 200 OK\r\nContent-Length: ' + fileContents
			
	except IOError:
		fileContents = ('HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n')
	
	# TCP chuncks checksums precalculationon the file contents (HTTP header included)
	chkSum = 0
	chkSumList = []
	chuncksSize = 2 ** chuncksNbits
	for i in range(len(fileContents)):
		if i % 2 == 0:
			chkSum += ord(fileContents[i]) << 8
		else:
			chkSum += ord(fileContents[i])
		if (i % chuncksSize == chuncksSize - 1) | (i == len(fileContents)-1):
			while chkSum & 0xffff0000:
				chkSum = (chkSum & 0xffff) + (chkSum >> 16);
			chkSumList += [chkSum]
			chkSum = 0

	# destination c file creation
	cOut = open(dstFile,'w')
	writeHeader(cOut,0)

	# inclusions
	cOut.write('#include "handlers.h"\n')

	# data structures declaration
	cName = getCName(srcFile)
	cOut.write('\n/********** File ' + srcFile + ' **********/\n')
	cOut.write('const uint16_t chk_' + cName + '[] /*CONST_VAR*/;\n')
	cOut.write('const unsigned char data_' + cName + '[] /*CONST_VAR*/;\n')

	# we fill the output handler structure
	cOut.write('\n/********** File handler **********/\n')
	cOut.write('CONST_VAR(struct output_handler_t, ' + cName + '_handler) = {\n')
	cOut.write('\t.handler_type = type_file,\n'
		+ '\t.handler_contents = {\n'
			+ '\t\t.file = {\n'
			+ '\t\t\t.length = ' + str(len(fileContents)) + ',\n'
			+ '\t\t\t.chk = chk_' + cName + ',\n'
			+ '\t\t\t.data = data_' + cName + '\n'
			+ '\t\t}\n'
			+ '\t},\n')
	cOut.write('};\n')

	# chuncks checksums data generation
	cOut.write('\n/********** Chuncks checksums **********/\n')
	cOut.write('CONST_VAR(uint16_t, chk_' + cName + '[])  = {\n')
	cOut.write(reduce(lambda x,y: x + "," + y,map(lambda x: hex(x),chkSumList)))
	cOut.write('};')
	
	# file data generation
	cOut.write('\n')
	cOut.write('\n/********** File data **********/\n')
	cOut.write('CONST_VAR(unsigned char, data_' + cName + '[]) = {\n')
	cOut.write(reduce(lambda x,y: x + "," + y,map(lambda x: hex(ord(x)),fileContents)))
	cOut.write('};\n')
	
	cOut.close()

# channels h file generation. This file contains channel declarations
def generateChannelsH(dstFile,propsFilesMap):
	# extract XML properties of each file (if needed)
	for fileName in propsFilesMap.keys():
		extractPropsFromXml(fileName,propsFilesMap[fileName])
	# h file creation
	hOut = open(dstFile,'w')
	writeHeader(hOut,0)
	hOut.write('#ifndef __CHANNELS_H__\n')
	hOut.write('#define __CHANNELS_H__\n\n')
	for fileName in propsFilesMap.keys():
		# for each channel: external structure declaration, macro for the channel name
		if propsFilesMap[fileName]['channel'] != '':
			cStructName = getCName(fileName[:fileName.rfind('.c')])
			hOut.write('extern CONST_VAR(struct output_handler_t, ' + cStructName + ');\n')
			hOut.write('#define ' + propsFilesMap[fileName]['channel'] + ' ' + cStructName + '\n')
	hOut.write('\n#endif\n')
	hOut.close()

# file index c file generatin
# the file index file links URLs to output_handler struct, thanks to an URL tree
# the URL tree is generated using genBlob.py scripts
def generateFilesIndex(dstDir,sourcesMap,target,chuncksNbits,wcBase,propsFilesMap):
	# files selection, static files and services separation
	filesNames = sourcesMap.keys()
	staticFilesNames = filter(lambda x: getContentType(x) == StaticContent,filesNames)
	serviceFilesNames = filter(lambda x: getContentType(x) == ServiceContent,filesNames)
	# extract XML information for each service file (if needed)
	for fileName in serviceFilesNames:
		if not propsFilesMap[fileName].has_key('hasXml'):
			extractPropsFromXml(fileName,propsFilesMap[fileName])
	# remove c files that do not contain any XML: they are not generators
	serviceFilesNames = filter(lambda x: propsFilesMap[x]['hasXml'],serviceFilesNames)

	# c file creation
	cOut = open(target,'w')
	writeHeader(cOut,0)
	
	cOut.write('#include "handlers.h"\n')
		
	# external handler references
	cOut.write('\n/********** External references **********/\n')
	for fileName in staticFilesNames:
		cName = getCName(fileName)
		cOut.write('extern CONST_VAR(struct output_handler_t, ' + cName + '_handler);\n')
	for fileName in serviceFilesNames:
		cFuncName = getCName(fileName[:fileName.rfind('.c')])
		cOut.write('extern const struct output_handler_t ' + cFuncName + ';\n')

	# filesRef is a map used to associate URLs to output_handlers
	filesRefs = {}
	# static files
	for fileName in set(staticFilesNames):
		# retrieve the output_handler for this file
		handlerName = getCName(fileName) + '_handler'
		# retrive the URL for this file
		fileName = sourcesMap[fileName]
		# .embed extensions are deleted
		if fileName.endswith('.embed'):
			fileName = fileName[:fileName.rfind('.')]
		# URLs always use '/' as file separator
		if os.sep == '\\':
			fileName = fileName.replace('\\','/')
		# update filesRef
		filesRefs[fileName] = handlerName
		# the case of index.hmtl files
		if os.path.basename(fileName) == 'index.html':
			fileName = os.path.dirname(fileName)
			if fileName != os.sep:
				fileName = fileName + '/'
			filesRefs[fileName] = handlerName
	# services
	for fileName in set(serviceFilesNames):
		# retrieve the output_handler for this file
		handlerName = fileName[:fileName.rfind('.')]
		handlerName = getCName(handlerName)
		# retrive the URL for this file
		fileName = sourcesMap[fileName]
		# .c extension is deleted
		fileName = fileName[:fileName.rfind('.c')]
		# URLs always use '/' as file separator
		if os.sep == '\\':
			fileName = fileName.replace('\\','/')
		#update filesRef
		filesRefs[fileName] = handlerName

	# the ordered list of URLs
	filesList = filesRefs.keys()
	filesList.sort()

	# files index creation (table of ordered output_handlers)
	cOut.write('\n/********** Files index **********/\n')
	cOut.write('CONST_VAR(const struct output_handler_t /*CONST_VAR*/ *, files_index[]) = {\n')
	# insert each handler
	for file in filesList:
		cOut.write('\t&' + filesRefs[file] + ',\n')
	# the final handler is NULL
	cOut.write('\tNULL,\n')
	cOut.write('};\n')

	# generate the URLs tree	
	GenBlob.genBlobTree(cOut,filesList,'urls_tree',False)
	cOut.close()
