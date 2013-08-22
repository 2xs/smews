#!/usr/bin/python

from xml.dom import minidom

import base64
import re
import hashlib

import GenApps
import GenBlob

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
authenticationType = ''     # 'basic' | 'digest'
realmUrisMap = { }          # Read parseAuthData description for details

# XML Tag Names
rootNodeTag = "http-authentication"
authNodeTag = "authorizations"
restNodeTag = "restrictions"
userNodeTag = "user"
realmNodeTag = "realm"
uriNodeTag = "uri"

# Effectively used usernames list
usedUsernames = [ ]

# parseAuthData(xmlFilename):
# Parse authentication data from the corresponding XML file.
def parseXMLFile(xmlFilename):
    global authenticationType
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
    authenticationType = rootNode.attributes['type'].value.lower()

    if authenticationType != 'basic' and authenticationType != 'digest':
        print '%s: Unknown authentication scheme: %s' %  (xmlFilename, authenticationType)
        exit()    
 
    parseAuthData(restNode, parseUsersList(authNode))

    return authenticationType

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

# Parse the data corresponding to the specified restriction node, and
# build the data structure according to the previously parsed users list.
#
# The returned data structure looks like this :
#
# /!\ HTTP BASIC AUTHENTICATION /!\
# ret = { 'realm' : [ { 'uri' : [credentials] } ] }
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
# /!\ HTTP DIGEST AUTHENTICATION /*\
# ret = { ('users' | 'realms') }
#   where the value associated with users is a dictionnary with username as key
#   and password as value
#   AND
#   where the value associated with realms is the same data structure as for
#   basic authentication, except about the credentials list which becomes now
#   a username list.
# .
# |- users
# |  `- { username : password }
# `- realms
#    |- uri1
#    |  |- user1
#    |  `- user2
#    |- uri2
#    |  `- user1
#    `- uri3
#       |- user1
#       `- user3
#
def parseAuthData(restrictionNode, usersList):
    global realmUrisMap
    realmNodeList = restrictionNode.getElementsByTagName(realmNodeTag)
    mapBuilt = { }

    # Parsing each restriction list (1x list/realm)
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
                        if authenticationType == 'basic':
                            credentials.append(user.toBasicHTTPAuthString())
                        elif authenticationType == 'digest':
                            credentials.append(username)

            uriCredentialsMap[uri] = credentials
                                      
        mapBuilt[realmName] = uriCredentialsMap            
                                      
    if authenticationType == 'digest':
        digestMap = { }
        digestMap['users'] = { }
        for user in usersList:
            digestMap['users'][user.username] = user.password
        digestMap['realms'] = mapBuilt;
        mapBuilt = digestMap;

        # DEBUG PURPOSE
#        print 'USERS:'
#        for user in mapBuilt['users']:
#            print '\t' + user + ' : ' + mapBuilt['users'][user]
#
#        print 'REALMS:'
#        for realm in mapBuilt['realms']:
#            print '\t' + realm + ':'
#            for uri in mapBuilt['realms'][realm]:
#                print '\t\t' + uri
#                for user in mapBuilt['realms'][realm][uri]:
#                    print '\t\t\t' + user

    realmUrisMap = mapBuilt # Saving result for code generation purpose
    return mapBuilt

def genHTTPAuthDataFile(dstFile):
    global realmUrisMap
    global authenticationType
    realmUrisCredentialsDataMap = { }
    uriDatasMap = { }

    uChar = 'unsigned char'

    generatedHeader = ''
    generatedCode = ''

    defHeader = '__HTTP_AUTH_DATA_H__'
    generatedHeader += '#ifndef ' + defHeader + '\n'
    generatedHeader += '#define ' + defHeader + '\n'
    generatedHeader += '\n'
    generatedHeader += '#include "types.h"\n'
    generatedHeader += '#include "auth.h"\n'
    generatedHeader += '\n'

    generatedCode += '#include "http_auth_data.h"\n\n'

    # Basic authentication
    if (authenticationType == 'basic'):
        credentialsList = []      # Ordered list of credentials, app by app
        credentialsVarMap = { }   # Dictionnary where key is a credentials string,
                                  # and value the credentials' C identifier string.
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
        generatedCode += 'CONST_VAR(%s *, http_auth_basic_credentials_data[]) = {\n' %(uChar)
        for credentials in credentialsList:
            generatedCode += '\t(%s *)&%s,\n' % (uChar, credentialsVarMap[credentials])

        generatedHeader += '\n'
        generatedCode += '};\n'

    # Digest authentication
    elif (authenticationType == 'digest'):
        credentialsByUserRealm = { } # Key is (user,realm)

        # Write realm string declarations
        commentary = '/*********** Realm List ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary

        realmVars = { } # Will store the name of the character string for realms
        realmUsers = { } # Will store the usernames used for each realm
        global usedUsernames

        for realm in realmUrisMap['realms']:
            # Building the string identifier
            realmVar = realm.lower()
            for c in realmVar:
                if (re.search('\W', c)):
                    realmVar = realmVar.replace(c, '_' + str(ord(c)) + '_')
            # Generating the string
            generatedHeader += 'extern CONST_VAR(%s, %s[]);\n' % (uChar, realmVar)
            generatedCode += 'CONST_VAR(%s, %s[]) = "%s";\n' % (uChar, realmVar, realm)
            
            realmVars[realm] = realmVar
            
            # Building the list of users per realm
            realmUsers[realm] = [ ]
            for uri in realmUrisMap['realms'][realm]:
                for user in realmUrisMap['realms'][realm][uri]:
                    realmUsers[realm].append(user)
                    if user not in usedUsernames:
                        usedUsernames.append(user)

        # Building the map where (user,realm) is a key corresponding to a credentials digest
        for realm in realmUsers:
            for user in realmUsers[realm]:
                m = hashlib.md5()
                m.update(user + ':' + realm + ':' + realmUrisMap['users'][user])
                credentialsByUserRealm[(user,realm)] = m.hexdigest()

        # Write credentials digest struct table
        commentary = '\n/*********** Credentials digest table ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary

        generatedHeader += '#define USER_DIGEST_COUNT %s\n' % len(credentialsByUserRealm)

        generatedHeader += 'extern CONST_VAR(%s, %s[]);\n' % ('credentials_digest_t', 'http_auth_digest_credentials_data')

        generatedCode += 'CONST_VAR(%s, %s[]) = {\n' % ('credentials_digest_t', 'http_auth_digest_credentials_data')
        
        for key in credentialsByUserRealm:
            generatedCode += '\t{\n'
            generatedCode += '\t\t.username_blob = %s,\n' % str(usedUsernames.index(key[0]))
            generatedCode += '\t\t.realm = (unsigned const char *)&%s,\n' % realmVars[key[1]]
            generatedCode += '\t\t.user_digest = "%s"\n' % credentialsByUserRealm[key]
            generatedCode += '\t},\n'

        generatedCode += '};\n'

        # Write credentials table
        commentary = '\n/*********** Credentials table ***********/\n'
        generatedHeader += commentary
        generatedCode += commentary
        
        generatedHeader += 'extern CONST_VAR(%s, %s[]);\n' % ('uint8_t', 'http_auth_digest_credentials_table')
        
        generatedCode += 'CONST_VAR(%s, %s[]) = {\n' % ('uint8_t', 'http_auth_digest_credentials_table')
        offset = 0;

        for realm in realmUrisMap['realms']:
            uriDatasMap = { }
            for uri in realmUrisMap['realms'][realm]:
                count = 0;
                for user in realmUrisMap['realms'][realm][uri]:
                    generatedCode += '\t%s, /* %s  =>  %s */\n' % (usedUsernames.index(user), uri, user)
                    count += 1

                uriDatasMap[uri] = (offset, count)
                offset += count

            realmUrisCredentialsDataMap[realm] = uriDatasMap

        generatedCode += '};\n'
        
    generatedHeader += '\n/*********** Username blobs ***********/\n'
    for u in usedUsernames:
        user = u.upper()
        generatedHeader += '#define HTTP_AUTH_USERNAME_%s\t%s\n' % (user, usedUsernames.index(u))

    generatedHeader += '\n#endif\n'

    headerFile = open(dstFile + '.h', 'w')
    GenApps.writeHeader(headerFile, 0)
    headerFile.write(generatedHeader)

    cFile = open(dstFile + '.c', 'w')
    GenApps.writeHeader(cFile, 0)
    cFile.write(generatedCode)

    return realmUrisCredentialsDataMap

def getUsedUsernamesList():
    global userUsernames
    return usedUsernames
