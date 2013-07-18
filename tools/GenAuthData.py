#!/usr/bin/python

from xml.dom import minidom

import base64
import re

import GenApps

# -------------------------------------------------- #
# Class: User
# -------------------------------------------------- #
class User:
    def __init__(self, username, password):
        self.username = username
        self.password = password
    
    def toBasicHTTPAuthString(self):
        return base64.b64encode('%s:%s' % (self.username, self.password))
# -------------------------------------------------- #

# Script initialisation
authenticationType = 'basic'     # 'basic' | 'digest'
realmUrisMap = { }               # Read parseAuthData description for details

# XML Tag Names
rootNodeTag = "http-authentication"
authNodeTag = "authorizations"
restNodeTag = "restrictions"
userNodeTag = "user"
realmNodeTag = "realm"
uriNodeTag = "uri"

# parseAuthData(xmlFilename):
# Parse authentication data from the corresponding XML file.
def parseAuthData(xmlFilename):
    # Getting XML document
    xmlDoc = minidom.parse(xmlFilename)

    # Getting root, authorizations, restrictions
    rootNodeList = xmlDoc.getElementsByTagName(rootNodeTag)
    authNodeList = xmlDoc.getElementsByTagName(authNodeTag)
    restNodeList = xmlDoc.getElementsByTagName(restNodeTag)

    if (len(rootNodeList) != 1):
        print '%s: Must contain only one node: %s' % (xmlFilename, rootNodeTag)
        exit()
    if (len(authNodeList) != 1):
        print '%s: Must contain only one node: %s' % (xmlFilename, authNodeTag)
        exit()
    if (len(restNodeList) != 1):
        print '%s: Must contain only one node: %s' % (xmlFilename, restNodeTag)
        exit()
    
    rootNode = rootNodeList[0]
    authNode = authNodeList[0]
    restNode = restNodeList[0]

    # Checking authentication type
    authType = rootNode.attributes['type'].value.lower()
    
    if (authType.lower() == 'basic'): # Basic Authentication
        authenticationType = authType
        parseBasicHTTPAuthData(restNode, parseUsersList(authNode))

    elif (authType.lower() == 'digest'): # Digest authentication
#        authenticationType = authType
        print 'Digest HTTP Authentication Data parsing is not yet supported'

    else: # Not handled
        print 'Unknown HTTP authentication type : %s' % (authType)
        exit()

    return authType

# parseUsersList(authorizationNode):
# Parse the authorization node and return a list of User objects corresponding
# to the XML data.
def parseUsersList(authorizationNode):
    usernames = [] # Twins checking purpose
    usersList = []

    userNodeList = authorizationNode.getElementsByTagName(userNodeTag)

    # Parsing username and passwd for each user
    for userNode in userNodeList:
        login = userNode.attributes['login'].value
        passwd = userNode.attributes['passwd'].value
        
        if login in usernames: # Twins check
            print '%s: Username is duplicated.' % (login)
            exit()
        if passwd == '':
            print '%s: Password field not set or empty.' % (login)
            exit()

        usernames.append(login)                
        usersList.append(User(login, passwd))

    return usersList

# parseBasicHTTPAuthData(realmNodeList, usersList):
# Parse the data corresponding to the specified restriction node, and
# build the data structure according to the previously parsed users list.
#
# The returned data structure looks like this :
#
# val = { 'realm' : [ { 'uri' : [credentials] } ] }
#   where credentials is the base 64 encoding of the string "username:passwd"
# .
# |- realm1
# |  |- uri1
# |  |  |- credentials1
# |  |  `- credentials2
# |  `- uri2
# |     `- credentials3
# `- realm2
#    `- uri3
#       |- credentials4
#       `- credentials5
#
def parseBasicHTTPAuthData(restrictionNode, usersList):
    global realmUrisMap
    realmNodeList = restrictionNode.getElementsByTagName(realmNodeTag)
    mapBuilt = { }

    # Parsing each restriction list (1xlist/realm)
    for realmNode in realmNodeList:
        uriCredentialsMap = { }
        realmName = realmNode.attributes['name'].value

        if realmName in mapBuilt: # Twins check
            print '%s: Realm is duplicated.' % (realmName)
            exit()
        if len(realmName) > 32: # Length check
            print '%s: Realm name is too long.' % (realmName)
            exit()

        uriNodeList = realmNode.getElementsByTagName(uriNodeTag)

        for uriNode in uriNodeList:
            uri = uriNode.attributes['path'].value

            if uri in uriCredentialsMap: # Twins check
                print '%s: Uri is duplicated.' % (uri)
                exit()
                
            # Building allowed users list
            allowedUsers = uriNode.attributes['users'].value.split(';')
            credentials = []

            for username in allowedUsers:
                # Existence check
                if username not in [user.username for user in usersList]:
                    print '%s: User "%s" doesn\'t exist' % (uri, username)
                    exit()

                # Getting corresponding string
                for user in usersList:
                    if user.username == username:
                        credentials.append(user.toBasicHTTPAuthString())

            uriCredentialsMap[uri] = credentials
                                      
        mapBuilt[realmName] = uriCredentialsMap            
                                      
    realmUrisMap = mapBuilt # Saving result for code generation purpose
    return mapBuilt

def genHTTPAuthDataFile(dstFile):
    global realmUrisMap
    realmUrisCredentialsDataMap = { }
    uriDatasMap = { }

    uChar = 'unsigned char'

    generatedHeader = ''
    generatedCode = ''

    defHeader = '__HTTP_AUTH_DATA_H__' # TODO: Automated gen
    generatedHeader += '#ifndef ' + defHeader + '\n'
    generatedHeader += '#define ' + defHeader + '\n'
    generatedHeader += '\n'
    generatedHeader += '#include "types.h"\n'
    generatedHeader += '\n'

    generatedCode += '#include "http_auth_data.h"\n\n'

    # Basic authentication
    if (authenticationType == 'basic'):
        credentialsList = []      # Ordered list of credentials, app by app
        credentialsVarMap = { }   # Dictionnary where key is a credentials,
                                    # and value the credentials C string name.
        credentialsCount = 0

        # Write realm string declarations
        commentary = '/*********** Realm list ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary

        offset = 0
        for realm in realmUrisMap:
            realmVar = realm.lower()
            for c in realmVar:
                if (re.search('\W', c)):
                    realmVar = realmVar.replace(c, '_' + str(ord(c)) + '_')
            
            generatedHeader += 'extern CONST_VAR(%s, %s[]);\n' % (uChar, realmVar)
            generatedCode += 'CONST_VAR(%s, %s[]) = "%s";\n' % (uChar, realmVar, realm)

            for uri in realmUrisMap[realm]:
                count = 0
                for credentials in realmUrisMap[realm][uri]:
                    credentialsList.append(credentials)
                    count += 1

                uriDatasMap[uri] = (offset, count)
                offset += count

            realmUrisCredentialsDataMap[realm] = uriDatasMap

        generatedHeader += '\n'
        generatedCode += '\n'
            
        # Write basic credentials string declarations
        commentary = '/*********** Credentials list ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary

        for credentials in credentialsList:
            credentialsVar = credentials.lower()
            for c in credentialsVar:
                if (re.search('\W', c)):
                    credentialsVar = credentialsVar.replace(c, '_' + str(ord(c)) + '_')

            if credentials not in credentialsVarMap:
                credentialsVarMap[credentials] = credentialsVar
                generatedHeader += 'extern CONST_VAR(%s, %s[]);\n' % (uChar, credentialsVar)
                generatedCode += 'CONST_VAR(%s, %s[]) = "%s";\n' % (uChar, credentialsVar, credentials)

        generatedHeader += '\n'
        generatedCode += '\n'

        # Write association table for handler->credentials
        commentary = '/*********** Credentials string table ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary

        generatedHeader += 'extern CONST_VAR(%s*, http_auth_basic_credentials_data[]);' % (uChar)
        generatedCode += 'CONST_VAR(%s*, http_auth_basic_credentials_data[]) = {\n' %(uChar)
        for credentials in credentialsList:
            generatedCode += '\t(%s*)&%s,\n' % (uChar, credentialsVarMap[credentials])

        generatedHeader += '\n'
        generatedCode += '};\n'

    # Digest authentication
    elif (authenticationType == 'digest'):
        print 'HTTP Digest Authentication data generation is not yet supported.'
        exit()

    generatedHeader += '\n#endif\n'

    headerFile = open(dstFile + '.h', 'w')
    GenApps.writeHeader(headerFile, 0)
    headerFile.write(generatedHeader)

    cFile = open(dstFile + '.c', 'w')
    GenApps.writeHeader(cFile, 0)
    cFile.write(generatedCode)

    return realmUrisCredentialsDataMap
