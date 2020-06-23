#!/usr/bin/env python

# generate version from git, and store it in version.h
#
# author: ZWang
#
# remove tag in both local and remote:
#

import os
import subprocess  # use subprocess.call to replace os.system


s = '''
=================================================================== 

tag git with version number, and store to version.txt

The current local tags are: 
===================================================================
'''
print(s)

subprocess.call("git describe --tags | sed \'s/\\(.*\\)-.*/\\1/\'",shell=True) 

s = input("input new tag, or fetch it from github:\n") 
s = s.strip().lower()

if s=="":
    # get new tag from github
    subprocess.call("git fetch",shell=True) 
else:    
    # git tag -a -m "message"  "tag"
    subprocess.call("git tag -a -m \"only add version number\" " +s,shell=True) 
    subprocess.call("git push origin "+s,shell=True) 

# git describe --tags | sed 's/\(.*\)-.*/\1/'
# use "sed" to remove unwanted string
print("writing to version.h")
getVersion =  subprocess.Popen("git describe --tags | sed \'s/\\(.*\\)-.*/\\1/\'", shell=True, stdout=subprocess.PIPE).stdout
getCommits =  subprocess.Popen("git rev-list HEAD | wc -l", shell=True, stdout=subprocess.PIPE).stdout

version =  getVersion.read()
commits =  getCommits.read()

version =  str(version.decode()).strip()
commits =  str(commits.decode()).strip()

print ("version  =" + version)
print ("commits  =" + commits)

# Opening a file 
file1 = open('version.h', 'w') 
  
# Writing a string to file 
version = "#define homeicu_version \"" + version + "\"\n"
commits = "#define homeicu_commits \"" + commits + "\"\n"

file1.write("// This version is generated from Git\n")
file1.write("// please run version.py again to get the correct version number \n")
file1.write("// before building the binary\n")
file1.write("\n\n")

file1.write(version)
file1.write(commits)
  
# Closing file 
file1.close() 

s = '''
=================================================================== 
    
Done! version number is in both git and version.txt now
    
if you want to delete tag, please run: 
    git tag -d v1.0 
    git push --delete origin v1.0 

===================================================================
'''

print(s)