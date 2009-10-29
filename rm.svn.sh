#!/bin/sh
echo "recursively removing .svn folders from current directory : "
pwd
rm -rf `find . -type d -name .svn`
