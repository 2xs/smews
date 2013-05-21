# -*- coding: utf-8 -*-
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
import re

# initialization
propertiesInfos = None
handlerInfos = {}
argsList = []
mimeListPostFileName = os.path.join('tools','mimeListPost')
mimesListPost = []
requestListFileName = os.path.join('tools','supportedRequestList')
requestList = []
attributListFileName = os.path.join('tools','supportedPostAttributList')
attributList = []
contentTypeList = []
postList = []
# doGet:1 doPost:2 doPostIn:4 doPostOut:8 args:16 content-types:32 doPacketIn:64 doPacketOut: 128
controlByte = 0
chuncksSize = 0
cOutName = 'pages.c'
mimeListGetFileName = os.path.join('tools','mimeRessources')
mimesHashGet = {}
StaticResource, DynamicResource = range(2)
argsTypesMap = {'str': 'arg_str',
'uint8' : 'arg_ui8',
'uint16' : 'arg_ui16',
'uint32' : 'arg_ui32'
}

# process the Mime file
for line in open(mimeListGetFileName,'r'):
	indice = line.find('#')
	line = line[:indice]
	split = line.split()
	if len(split) == 2:
		mimesHashGet[split[0]]=split[1]

for line in open(mimeListPostFileName,'r'):
	indice = line.find('#')
	line = line[:indice]
	split = line.split()
	if(len(split) > 0):
		mimesListPost.append(split[0])

for line in open(requestListFileName,'r'):
	indice = line.find('#')
	line = line[:indice]
	split = line.split()
	if(len(split) > 0):
		requestList.append(split[0] + chr(32))

for line in open(attributListFileName,'r'):
	indice = line.find('#')
	line = line[:indice]
	split = line.split()
	if(len(split) > 0):
		attributList.append(split[0])

def generateBlobsH(dstFile):
	hOut = open(dstFile,'w')
	writeHeader(hOut,0)
	hOut.write('#ifndef __BLOBS_H__\n')
	hOut.write('#define __BLOBS_H__\n\n')
	GenBlob.genBlobTree(hOut,requestList,'blob_http_rqt',False)
	GenBlob.genBlobTree(hOut,mimesListPost,'mimes_tree',False)
	GenBlob.genBlobTree(hOut,attributList,'blob_http_header_content',False)
	hOut.write('\n#endif\n')
	hOut.close()

def writeDefinesH(dstFile):
	hOut = open(dstFile,'w')
	writeHeader(hOut,0)
	hOut.write('#ifndef __DEFINES_H__\n')
	hOut.write('#define __DEFINES_H__\n\n')

	# write mimes.h file referencing content-types supported by smews
	for m in mimesListPost:
		mime = m.upper()
		for c in mime:
			if(re.search('\W',c)):
				mime =	mime.replace(c,'_'+str(ord(c))+'_')
		hOut.write('#define \t CONTENT_TYPE_' + mime + '\t' + str(mimesListPost.index(m)) + '\n')

	for m in requestList:
		req = m.upper()
		for c in req:
			if(re.search('\W',c)):
				req = req.replace(c,'_'+str(ord(c))+'_')
		hOut.write('#define \t REQUEST_' + req + '\t' + str(requestList.index(m)) + '\n')

	for m in attributList:
		att = m.upper()
		for c in att:
			if(re.search('\W',c)):
				att =	att.replace(c,'_'+str(ord(c))+'_')
		hOut.write('#define \t ATTRIBUT_' + att + '\t' + str(attributList.index(m)) + '\n')

	hOut.write('\n#endif\n')
	hOut.close()

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
	file.write('* This file has been ' + tmp + ' by GenApps, a tool of the smews project\n')
	file.write('* smews home page: http://www.lifl.fr/2XS/smews\n')
	t = datetime.datetime.now()
	t = time.mktime(t.timetuple())
	file.write('* Generation date: ' + str(datetime.datetime.utcfromtimestamp(t).ctime()) + '\n')
	file.write('*/\n\n')

# replaces dot and file separators by underscores
def getCName(fileName):
	return fileName.replace('.','_').replace(os.sep,'_')

# returns the name of the generated file for file fileName
def getFileName(fileName,chuncksNBits,gzipped):
	if getResourceType(fileName) == DynamicResource:
		fileName = fileName[0:fileName.rfind('.')]
	else:
		prefix = ('g' if gzipped else 'r') + '_'
		prefix += 'c' + str(chuncksNBits) + '_'
		fileName = os.path.join(os.path.dirname(fileName),prefix + os.path.basename(fileName))
	if fileName.endswith('.embed'):
		fileName = fileName[:fileName.rfind('.')]
	return fileName.replace('.','_')

# used by getAppFiles
def getAppFilesRec(appPath,path):
	files=[]
	for file in glob.glob(path + '/*'):
		if os.path.basename(file) != 'SConscript':
        		if os.path.isdir(file):
					files = files + (getAppFilesRec(appPath,file))
        		elif not file.endswith('~'):
					files.append(file[len(appPath)+1:])
	return files

# returns all files contained in path (with recurssion)
def getAppFiles(path):
	return getAppFilesRec(path,path)

# Web resource type from original applicative file
def getResourceType(path):
	if path.endswith('.c') or path.endswith('.h'):
		return DynamicResource
	else:
		return StaticResource

# extracts c file properties from its xml data
# returns the file data
def extractPropsFromXml(srcFile,dstFileInfos):
	# open the source file in order to parse the XML and return the file data
	file = open(srcFile,'r')
	lines = file.readlines()
	if len(lines) > 1:
		fileData = reduce(lambda x,y: x + y,lines)
	else:
		fileData = ''
	# process XML only if dstFileInfos do not already contains informations
	if not dstFileInfos.has_key('hasXml'):
		# 3 XML handler functions
		def start_element(name, attrs):
			global argsList
			global handlerInfos
			global propertiesInfos
			global contentTypeList
			global controlByte
			if name == 'args':
				contentTypeList.append('application/x-www-form-urlencoded')
				controlByte += 16
				argsList = []
			elif name == 'arg':
				argsList.append(attrs)
			elif name == 'handlers':
				handlerInfos = attrs
			elif name == 'properties':
				propertiesInfos = attrs
			elif name == 'content-types':
				controlByte += 32
				contentTypeList = []
			elif name == 'content-type':
				contentTypeList.append(attrs)
		def end_element(name):
			return
		def char_data(data):
			return

		# select the XML part of the c file
		xmlRoot = 'generator'
		xmlData = fileData[fileData.rfind('<' + xmlRoot + '>'):]
		xmlData = xmlData[:xmlData.rfind('</' + xmlRoot + '>') + len(xmlRoot) + 3]

		# default value for the generator properties
		dstFileInfos['persistence'] = 'persistent'
		dstFileInfos['interaction'] = 'rest'
		dstFileInfos['channel'] = ''
		# XML data have been found
		if len(xmlData) > 1:
			global argsList
			global handlerInfos
			global propertiesInfos
			global contentTypeList
			global controlByte
			# init globals used for parsing
			propertiesInfos = None
			handlerInfos = {}
			argsList = []
			contentTypeList = []
			controlByte = 0
			# parse the XML
			p = xml.parsers.expat.ParserCreate()
			p.StartElementHandler = start_element
			p.EndElementHandler = end_element
			p.CharacterDataHandler = char_data
			p.Parse(xmlData, 0)

			if handlerInfos.has_key('doGet'):
				controlByte += 1
				dstFileInfos['doGet'] = handlerInfos['doGet']

			if handlerInfos.has_key('doPostIn'):
				controlByte += 4
				dstFileInfos['doPostIn'] = handlerInfos['doPostIn']
				postList.append(srcFile)

			if handlerInfos.has_key('doPostOut'):
				controlByte += 8
				dstFileInfos['doPostOut'] = handlerInfos['doPostOut']
				postList.append(srcFile)

			if handlerInfos.has_key('doPost'):
				controlByte += 2
				dstFileInfos['doPost'] = handlerInfos['doPost']
				postList.append(srcFile)

			if handlerInfos.has_key('doPacketIn'):
				controlByte += 64
				dstFileInfos['doPacketIn'] = handlerInfos['doPacketIn']

			if handlerInfos.has_key('doPacketOut'):
				controlByte += 128
				dstFileInfos['doPacketOut'] = handlerInfos['doPacketOut']

			# 1 -> get without args
			# 17 -> get with args
			# 18 -> doPost with args
			# 44 -> doPostIn, doPostOut, content-type
			# 64 -> doPacketIn only
			# 192 -> doPacketIn and doPacketOut
			# 0 -> nothing
			# 16 -> only args
			# 32 -> only content-types
			# 48 -> args and content-types only
			if(controlByte != 1 and controlByte != 17 and controlByte != 18 and controlByte != 44 and controlByte != 64 and controlByte != 192):
				if(controlByte == 0 or controlByte == 16 or controlByte == 32 or controlByte == 48):
					exit('Error: the file ' + srcFile + ' does not describe a doGet or doPost handler or doPostIn and doPostOut handlers')
				if (controlByte == 128):
					exit('Error: the file ' + srcFile + ' only defines a doPacketOut without a doPacketIn handler')

				exit('Error: the file ' + srcFile + 'has an incompatible decription (' +
						((controlByte & 1 == 1 and 'doGet,') or '') +
						((controlByte & 2 == 2 and 'doPost,') or '') +
						((controlByte & 4 == 4 and 'doPostIn,') or '') +
						((controlByte & 8 == 8 and 'doPostOut,') or '') +
						((controlByte & 16 == 16 and 'args,') or '') +
						((controlByte & 32 == 32 and 'content-types,') or '') +
						((controlByte & 64 == 64 and 'doPacketIn,') or '') +
						((controlByte & 128 == 128 and 'doPacketOut,') or '') +
						')')

			# init handler
			if handlerInfos.has_key('init'):
				dstFileInfos['init'] = handlerInfos['init']
			# initGet handler
			if handlerInfos.has_key('initGet'):
				dstFileInfos['initGet'] = handlerInfos['initGet']
			# generator arguments
			dstFileInfos['argsList'] = argsList
			dstFileInfos['contentTypeList'] = contentTypeList

			if propertiesInfos != None:
			          # generator persistence
				if propertiesInfos.has_key('persistence'):
				        dstFileInfos['persistence'] = propertiesInfos['persistence']
				# generator interaction mode
				# generator channel name
				if propertiesInfos.has_key('channel'):
				        dstFileInfos['channel'] = propertiesInfos['channel']
				        dstFileInfos['interaction'] = 'alert' # default mode when a channel is set
				if propertiesInfos.has_key('interaction'):
				        dstFileInfos['interaction'] = propertiesInfos['interaction']
				        if(dstFileInfos['channel'] == ''):
					      defaultChannel = os.path.basename(srcFile)
					      defaultChannel = defaultChannel[:defaultChannel.rfind('.c')]
					      defaultChannel = getCName(defaultChannel)
					      dstFileInfos['channel'] = defaultChannel
				if propertiesInfos.has_key('protocol'): # protocol number for above IP handlers
					dstFileInfos['protocol'] = propertiesInfos['protocol']

		# boolean used to know if the c file contains XML data
		dstFileInfos['hasXml'] = len(xmlData) > 1
	return fileData

# generates the .props file for a c file
# the .props file contains a subset of the information described in generators XMLs
# all information used to build the file-index.c file or the channel.h file MUST be included in the props file
def generateResourceProps(srcFile,dstFileInfos):
	# extract the properties from the XML (if needed)
	extractPropsFromXml(srcFile,dstFileInfos)
	# store the hasXml flag and the comet channel
	pOut = open(dstFileInfos['fileName'],'w')
	pOut.write('1\n' if dstFileInfos['hasXml'] else '0\n')
	pOut.write(dstFileInfos['channel'] + '\n')
	pOut.close()

# launches a Web applicative resource file generation
def generateResource(srcFile,dstFile,chuncksNbits,gzipped,dstFileInfos):
	if getResourceType(srcFile) == DynamicResource:
		generateDynamicResource(srcFile,dstFile,dstFileInfos)
	else:
		generateStaticResource(srcFile,dstFile,chuncksNbits,gzipped)

# dynamic resources: the generator file is enriched
def generateDynamicResource(srcFile,dstFile,dstFileInfos):
	# extract the properties from the XML (if needed)
	fileData = extractPropsFromXml(srcFile,dstFileInfos)
	# if the c/h file does not conatin any XML, we simply copy it: this is not a generator
	if not dstFileInfos['hasXml']:
		shutil.copyfile(srcFile,dstFile)
	# here, the c file is a generator. It will be enriched
	else:
		generatedMimesArray = ''

		cFuncName = getCName(srcFile[:srcFile.rfind('.c')])

		generatedHeader = '#include "generators.h"\n'
		generatedHeader += '#include "stddef.h"\n\n'
		generatedHeader += '#include "defines.h"\n\n'

		generatedOutputHandler = '/********** Output handler **********/\n'
		# handler functions declaration
		if dstFileInfos.has_key('init'):
			generatedOutputHandler += 'static generator_init_func_t ' + dstFileInfos['init'] + ';\n'
		if dstFileInfos.has_key('initGet'):
			generatedOutputHandler += 'static generator_initget_func_t ' + dstFileInfos['initGet'] + ';\n'

		if dstFileInfos.has_key('doGet'):
			generatedOutputHandler += 'static generator_doget_func_t ' + dstFileInfos['doGet'] + ';\n'
		if dstFileInfos.has_key('doPostOut'):
			generatedOutputHandler += 'static generator_dopost_out_func_t ' + dstFileInfos['doPostOut'] + ';\n'
		if dstFileInfos.has_key('doPostIn'):
			generatedOutputHandler += 'static generator_dopost_in_func_t ' + dstFileInfos['doPostIn'] + ';\n'
		if dstFileInfos.has_key('doPost'):
			generatedOutputHandler += 'static generator_doget_func_t ' + dstFileInfos['doPost'] + ';\n'

		# other protocols than TCP handlers
		if dstFileInfos.has_key('doPacketIn'):
			generatedOutputHandler += '#ifndef DISABLE_GP_IP_HANDLER\n'
			generatedOutputHandler += 'static generator_dopacket_in_func_t ' + dstFileInfos['doPacketIn'] + ';\n'
			if dstFileInfos.has_key('doPacketOut'):
				generatedOutputHandler += 'static generator_dopacket_out_func_t ' + dstFileInfos['doPacketOut'] + ';\n'
			generatedOutputHandler += '#endif\n'

		# output_handler structure creation
		generatedOutputHandler += 'CONST_VAR(struct output_handler_t, ' + cFuncName + ') = {\n'
		# handler type
		if dstFileInfos.has_key('doPacketIn'):
			generatedOutputHandler += '\t.handler_type = type_general_ip_handler,\n'
		else:
			generatedOutputHandler += '\t.handler_type = type_generator,\n'
		generatedOutputHandler += '\t.handler_comet = %d,\n' %(1 if dstFileInfos['channel'] != '' else 0)
		generatedOutputHandler += '\t.handler_stream = %d,\n' %(1 if dstFileInfos['interaction'] == 'streaming' else 0)
		if dstFileInfos['persistence'] == 'persistent':
			etype = 'prop_persistent'
		elif dstFileInfos['persistence'] == 'idempotent':
			etype = 'prop_idempotent'
		elif dstFileInfos['persistence'] == 'volatile':
			etype = 'prop_volatile'
		if etype == None:
			etype = 'prop_persistent'
		# the handler is a dynamic resource (generator), we fill the function handlers fields
		generatedOutputHandler += '\t.handler_data = {\n'
		generatedOutputHandler += '\t\t.generator = {\n'
		generatedOutputHandler += '\t\t\t.prop = %s,\n' %etype
		# init
		if not dstFileInfos.has_key('init'):
			generatedOutputHandler += '\t\t\t.init = NULL,\n'
		else:
			generatedOutputHandler += '\t\t\t.init = ' + dstFileInfos['init'] + ',\n'

		generatedOutputHandler += '\t\t\t.handlers = {\n'

		if dstFileInfos.has_key('initGet') or dstFileInfos.has_key('doGet') or dstFileInfos.has_key('doPost'):
			generatedOutputHandler += '\t\t\t\t.get = {\n'
			# initGet
			if dstFileInfos.has_key('initGet'):
				generatedOutputHandler += '\t\t\t\t.initget = ' + dstFileInfos['initGet'] + ',\n'
			# doGet
			if dstFileInfos.has_key('doGet'):
				generatedOutputHandler += '\t\t\t\t\t.doget = ' + dstFileInfos['doGet'] + ',\n'
			# doPost
			if dstFileInfos.has_key('doPost'):
				generatedOutputHandler += '\t\t\t\t.doget = ' + dstFileInfos['doPost'] + ',\n'
			generatedOutputHandler += '\t\t\t\t},\n'



		#doPostIn/doPostOut
		if dstFileInfos.has_key('doPostIn'):
			generatedOutputHandler += '\t\t\t\t.post = {\n'
			generatedOutputHandler += '\t\t\t\t\t.dopostin = ' + dstFileInfos['doPostIn'] + ',\n'

		if dstFileInfos.has_key('doPostOut'):
			generatedOutputHandler += '\t\t\t\t\t.dopostout = ' + dstFileInfos['doPostOut'] + ',\n'
			generatedOutputHandler += '\t\t\t\t},\n'

		#doPacketIn, doPacketOut
		if dstFileInfos.has_key('doPacketIn'):
			generatedOutputHandler += '#ifndef DISABLE_GP_IP_HANDLER\n'
			generatedOutputHandler += '\t\t\t\t.gp_ip = {\n'
			generatedOutputHandler += '\t\t\t\t.dopacketin = ' + dstFileInfos['doPacketIn'] + ',\n'
			if dstFileInfos.has_key('doPacketOut'):
				generatedOutputHandler += '\t\t\t\t.dopacketout = ' + dstFileInfos['doPacketOut'] + ',\n'
			if dstFileInfos.has_key('protocol'):
				generatedOutputHandler += '\t\t\t\t.protocol = ' + dstFileInfos['protocol'] + ',\n'
			generatedOutputHandler += '\t\t\t\t},\n'
			generatedOutputHandler += '#endif\n'


		generatedOutputHandler += '\t\t\t},\n'
		generatedOutputHandler += '\t\t},\n'
		generatedOutputHandler += '\t},\n'
		# generator arguments information are also written in the structure
		generatedOutputHandler += '#ifndef DISABLE_ARGS\n'
		generatedOutputHandler += '\t.handler_args = {\n'
		if len(dstFileInfos['argsList']) > 0:
			generatedOutputHandler += '\t\t.args_tree = args_tree,\n'
			generatedOutputHandler += '\t\t.args_index = args_index,\n'
			generatedOutputHandler += '\t\t.args_size = sizeof(struct args_t)\n'
		else :
			generatedOutputHandler += '\t\t.args_tree = NULL,\n'
			generatedOutputHandler += '\t\t.args_index = NULL,\n'
			generatedOutputHandler += '\t\t.args_size = 0\n'

		generatedOutputHandler += '\t},\n'
		generatedOutputHandler += '#endif\n'

		generatedOutputHandler += '#ifndef DISABLE_POST\n'
		generatedOutputHandler += '\t.handler_mimes = {\n'

		if (dstFileInfos.has_key('doPostIn') and len(dstFileInfos['contentTypeList']) > 0) :
			generatedOutputHandler += '\t\t.mimes_index = mimes_index,\n'
			generatedOutputHandler += '\t\t.mimes_size = '+ str(len(dstFileInfos['contentTypeList'])) +',\n'
		elif(dstFileInfos.has_key('doPost') and len(dstFileInfos['argsList']) > 0) :
			generatedOutputHandler += '\t\t.mimes_index = mimes_index,\n'
			generatedOutputHandler += '\t\t.mimes_size = 1,\n'
		else :
			generatedOutputHandler += '\t\t.mimes_index = NULL,\n'
			generatedOutputHandler += '\t\t.mimes_size = 0,\n'

		generatedOutputHandler += '\t},\n'
		generatedOutputHandler += '#endif\n'
		generatedOutputHandler += '};\n'

		if (dstFileInfos.has_key('doPost') or dstFileInfos.has_key('doPostIn')):
			generatedMimesArray = '/********* Content-types array **********/\n'
			generatedMimesArray += 'static CONST_VAR(unsigned char, mimes_index[]) = {'

			countAttr =  len(dstFileInfos['contentTypeList'])

			if(len(dstFileInfos['argsList']) > 0):
				if 'application/x-www-form-urlencoded' in mimesListPost:
					generatedMimesArray += str(mimesListPost.index('application/x-www-form-urlencoded'))
				else:
					exit('the mime type application/x-www-form-urlencoded don\'t exist in the mimeListPost file')

			elif len(dstFileInfos['contentTypeList']) > 0:
				for attrs in dstFileInfos['contentTypeList']:
					if attrs['type'] in mimesListPost:
						generatedMimesArray +=  str(mimesListPost.index(attrs['type']))
					else:
						exit('the mime type ' + attrs['type'] + ' don\'t exist in the mimeListPost file')
					if countAttr > 1:
						generatedMimesArray += ','
					countAttr -= 1
			else:
				generatedMimesArray += 'NULL'


			generatedMimesArray += '};\n'

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
		if (dstFileInfos.has_key('doPost') or dstFileInfos.has_key('doPostIn')):
			cOut.write('#ifndef DISABLE_POST\n')
			cOut.write(generatedMimesArray)
			cOut.write('\n')
			cOut.write('#endif\n')
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
		cOut.write(fileData)
		cOut.close()

# static file: the file is completely pre-processed into a c file:
# HTTP header insertion
# chuncks checksums calculation
def generateStaticResource(srcFile,dstFile,chuncksNbits,gzipped):
	# HTTP header generation and concatenation with file data
	try:
		# open the source file
		file = open(srcFile,'rb')
		lines = file.readlines()
		if len(lines) != 0:
			fileData = reduce(lambda x,y: x + y,lines)
		else:
			fileData = ''
		if gzipped:
			# the whole file is gzipped
			sio = StringIO.StringIO()
			gzipper = gzip.GzipFile(mode="wb", fileobj=sio, compresslevel=9)
			gzipper.write(fileData)
			gzipper.close()
			fileData = sio.getvalue()
		# HTTP header insertion before the file data
		tmp = str(len(fileData)) + '\r\nServer: Smews\r\nContent-Type: ' + getMime(mimesHashGet,srcFile[srcFile.rfind('.')+1:])
		if gzipped:
			tmp += '\r\nContent-Encoding: gzip'
		#~ headerPadding = 205
		#~ if headerPadding > 4:
			#~ tmp += '\r\na:'
			#~ for i in range(headerPadding-4):
				#~ tmp += '0'
		tmp += '\r\nConnection: Keep-Alive\r\n\r\n' + fileData
		fileData = tmp
		if os.path.basename(srcFile) == '404.html':
			fileData = 'HTTP/1.1 404 Not Found\r\nContent-Length: ' + fileData
		elif os.path.basename(srcFile) == '505.html':
			fileData = 'HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: ' + fileData
		else:
			fileData = 'HTTP/1.1 200 OK\r\nContent-Length: ' + fileData

	except IOError:
		fileData = ('HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n')

	# TCP chuncks checksums precalculationon the file data (HTTP header included)
	chkSum = 0
	chkSumList = []
	chuncksSize = 2 ** chuncksNbits
	for i in range(len(fileData)):
		if i % 2 == 0:
			chkSum += ord(fileData[i]) << 8
		else:
			chkSum += ord(fileData[i])
		if (i % chuncksSize == chuncksSize - 1) | (i == len(fileData)-1):
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
		+ '\t.handler_data = {\n'
			+ '\t\t.file = {\n'
			+ '\t\t\t.length = ' + str(len(fileData)) + ',\n'
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
	cOut.write(reduce(lambda x,y: x + "," + y,map(lambda x: hex(ord(x)),fileData)))
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
def generateIndex(dstDir,sourcesMap,target,chuncksNbits,appBase,propsFilesMap):
	# files selection, static files and geenrators separation
	filesNames = sourcesMap.keys()
	staticFilesNames = filter(lambda x: getResourceType(x) == StaticResource,filesNames)
	generatorFilesNames = filter(lambda x: getResourceType(x) == DynamicResource,filesNames)
	# extract XML information for each generetor file (if needed)
	for fileName in generatorFilesNames:
		if not propsFilesMap[fileName].has_key('hasXml'):
			extractPropsFromXml(fileName,propsFilesMap[fileName])
	# remove c files that do not contain any XML: they are not generators
	generatorFilesNames = filter(lambda x: propsFilesMap[x]['hasXml'],generatorFilesNames)

	# c file creation
	cOut = open(target,'w')
	writeHeader(cOut,0)

	cOut.write('#include "handlers.h"\n')

	# external handler references
	cOut.write('\n/********** External references **********/\n')
	for fileName in staticFilesNames:
		cName = getCName(fileName)
		cOut.write('extern CONST_VAR(struct output_handler_t, ' + cName + '_handler);\n')
	for fileName in generatorFilesNames:
		cFuncName = getCName(fileName[:fileName.rfind('.c')])
		cOut.write('extern CONST_VAR(struct output_handler_t, ' + cFuncName + ');\n')

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
	# generator
	# fp is the file post list
	fp = []
	for fileName in set(generatorFilesNames):
		# retrieve the output_handler for this file
		handlerName = fileName[:fileName.rfind('.')]
		handlerName = getCName(handlerName)
		# save fileName
		fn = fileName
		# retrive the URL for this file
		fileName = sourcesMap[fileName]
		# .c extension is deleted
		fileName = fileName[:fileName.rfind('.c')]
		# URLs always use '/' as file separator
		if os.sep == '\\':
			fileName = fileName.replace('\\','/')
		#update filesRef
		filesRefs[fileName] = handlerName
		# search if it's POST URL
		if fn in postList:
			fp.append(fileName)

	# the ordered list of URLs
	filesList = filesRefs.keys()
	filesList.sort()

	# files index creation (table of ordered output_handlers)
	cOut.write('\n/********** Files index **********/\n')
	cOut.write('CONST_VAR(const struct output_handler_t /*CONST_VAR*/ *, resources_index[]) = {\n')
	# insert each handler
	for file in filesList:
		cOut.write('\t&' + filesRefs[file] + ',\n')
	# the final handler is NULL
	cOut.write('\tNULL,\n')
	cOut.write('};\n')

	# construct filesList with end caracter for post urls
	filesListNew = []
	for fileName in filesList:
		if fileName in fp:
			filesListNew.append(fileName + chr(255))
		else:
			filesListNew.append(fileName)

	# generate the URLs tree
	GenBlob.genBlobTree(cOut,filesListNew,'urls_tree',False)
	cOut.close()
