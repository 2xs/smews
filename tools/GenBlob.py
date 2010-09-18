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

import time
import datetime

# treeToString
def treeToStringRec(node,indent):
	string = indent + node['string']
	if node['final']:
		string += '|END'
	for i in range(len(node['string'])):
		indent += '-'
	for child in node['childs']:
		string += '\n' + treeToStringRec(child,indent)
	return string

def treeToString(tree):
	return treeToStringRec(tree,'* ')[1:]

# tertiaryTreeToString
def tertiaryTreeToStringRec(node,indent,key):
	string = indent
	string += key + node['string']
	if node['final']:
		string += '|END'
	for i in range(len(node['string'])):
		indent += '-'
	for key in node['childs'].keys():
		string += '\n' + tertiaryTreeToStringRec(node['childs'][key],indent,key)
	return string

def tertiaryTreeToString(tree):
	return tertiaryTreeToStringRec(tree,'* ','-')

# buildTree
def buildTree(symbols,indexMap):
	tree = {'string': '', 'childs': [], 'final': False, 'root': True}
	currentRef = 0
	
	for symbol in symbols:
		childSet = tree['childs']
		for char in symbol:
			for node in childSet:
				if node['string'] == char:
					childSet = node['childs']
					break
			else:
				newNode = {'string': char, 'childs': [], 'final': False}
				childSet.append(newNode)
				childSet = newNode['childs']
		newNode['final'] = True
		newNode['ref'] = indexMap[currentRef]
		currentRef += 1
	return tree

# compactTree
def compactTreeRec(node,nBrothers):
	for child in node['childs']:
		compactTreeRec(child,len(node['childs'])-1)
	if nBrothers == 0 and not node['final'] and len(node['childs']) == 1:
		node['string'] += node['childs'][0]['string']
		node['final'] = node['childs'][0]['final']
		if node['final']:
			node['ref'] = node['childs'][0]['ref']
		node['childs'] = node['childs'][0]['childs']
		
def compactTree(tree):
	compactTreeRec(tree,0)
	
# compactTernaryTree
def compactTernaryTree(node):
	for key in node['childs'].keys():
		compactTernaryTree(node['childs'][key])
	#if not node['final'] and node['childs'].keys()==['='] and (node['childs']['=']['childs'].keys()==['='] or node['childs']['=']['final']):
	if not node['final'] and node['childs'].keys()==['=']:
		node['string'] += node['childs']['=']['string']
		node['final'] = node['childs']['=']['final']
		if node['final']:
			node['ref'] = node['childs']['=']['ref']
		node['childs'] = node['childs']['=']['childs']

# toTernaryTree
def toTernaryTree(node):
	if type(node['childs']) == list:
		node['childs'] = {'=': node['childs']}
	for key in node['childs'].keys():
		childs = node['childs'][key]
		if len(childs) > 0:
			centralIndex = len(childs) / 2
			infNodes = childs[:centralIndex]
			centralNode = childs[centralIndex]
			supNodes = childs[centralIndex+1:]
			node['childs'][key] = centralNode
			centralNode['childs'] = {'=': centralNode['childs'], '<': infNodes,'>': supNodes}
			toTernaryTree(centralNode)
		else:
			del node['childs'][key]

# understoodableBLOB
def understoodableBLOBRec(node,key):
	offset = 0
	string = ''
	if node['string'] != '':
		string += ',"' + node['string'] + '"'
	offset += len(node['string'])
	if node['final']:
		string += ',ref:' + str(node['ref'])
		offset += 1
	childsList = []
	if node['childs'].has_key('<'):
		childsList.append('<')
	if node['childs'].has_key('='):
		childsList.append('=')
	if node['childs'].has_key('>'):
		childsList.append('>')
	flags = node['childs'].has_key('<') & 1 | ((node['childs'].has_key('=') & 1) << 1)| ((node['childs'].has_key('>') & 1) << 2)
	returned = {}
	flagsStr = ''
	for key in childsList:
		flagsStr += key
		returned[key] = understoodableBLOBRec(node['childs'][key],key)
		offset += returned[key]['offset']
	if flags != 2:
		if flagsStr != '':
			string += ',' + flagsStr
		else:
			string += ',0'
		offset += 1
		for i in range(len(childsList)-1):
			string += ',' + childsList[i+1] + ':' + str(returned[childsList[i]]['offset'])
			offset += 1
	for key in childsList:
		string += returned[key]['blob']
	return {'blob': string, 'offset': offset}

def understoodableBLOB(tree):
	ret  = understoodableBLOBRec(tree,'|')
	return {'string': ret['blob'][1:], 'length': ret['offset']}

# BLOB
def BLOBRec(node,key):
	offset = 0
	string = ''
	for char in node['string']:
		string += ',' + str(ord(char))
	offset += len(node['string'])
	if node['final']:
		string += ',' + str(128 + node['ref'])
		offset += 1
	childsList = []
	if node['childs'].has_key('<'):
		childsList.append('<')
	if node['childs'].has_key('='):
		childsList.append('=')
	if node['childs'].has_key('>'):
		childsList.append('>')
	flags = node['childs'].has_key('<') & 1 | ((node['childs'].has_key('=') & 1) << 1)| ((node['childs'].has_key('>') & 1) << 2)
	returned = {}
	for key in childsList:
		returned[key] = BLOBRec(node['childs'][key],key)
		offset += returned[key]['offset']
	if flags != 2:
		string += ',' + str(flags)
		offset += 1
		for i in range(len(childsList)-1):
			string += ',' + str(returned[childsList[i]]['offset'])
			offset += 1
	for key in childsList:
		string += returned[key]['blob']
	return {'blob': string, 'offset': offset}

def BLOB(tree):
	ret  = BLOBRec(tree,'|')
	return {'string': ret['blob'][1:], 'length': ret['offset']}

# generate the smbols tree
def genBlobTree(cOut,symbols,blobName,isStatic):
	sortedSymbols = symbols[:]
	sortedSymbols.sort()
	indexMap = {}
	for i in range(len(symbols)):
		indexMap[sortedSymbols.index(symbols[i])] = i
	
	symbolsSize = 0
	for symbol in symbols:
		symbolsSize += len(symbol)

	cOut.write('\n/********** Symbols list, total length: ' + str(symbolsSize) + ' bytes **********/\n/*\n')
	for symbol in symbols:
		cOut.write('* ' + symbol + '\n')
	cOut.write('*/\n\n')

	tree = buildTree(sortedSymbols,indexMap)
	compactTree(tree)
	treeString = treeToString(tree)
	
	cOut.write('/********** Generated Ternary Tree **********/\n/*\n')
	toTernaryTree(tree)
	compactTernaryTree(tree)
	treeString = tertiaryTreeToString(tree)
	cOut.write(treeString)
	cOut.write('\n*/\n\n')
	
	blob = understoodableBLOB(tree)
	cOut.write('/********** \"Understoodable\" Generated BLOB, total length: ' + str(blob['length']) + ' bytes **********/\n/*\n')
	cOut.write('* ' + blob['string'])
	cOut.write('\n*/\n\n')
	
	blob = BLOB(tree)
	cOut.write('/********** Finally Generated BLOB, total length: ' + str(blob['length']) + ' bytes **********/\n')
	if isStatic:
		cOut.write('static ')
	cOut.write('CONST_VAR(unsigned char, ' + blobName + '[]) = {' + blob['string'] + '};\n')
