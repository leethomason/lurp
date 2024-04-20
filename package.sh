#!/bin/bash
# writes the binaries and script files to a zippable folder
# Useage, in the root of the project:
# ./deploy.sh lurp_0.5.0

# exit on errors
set -e

# create the folder
mkdir $1

cp *.md $1
cp LICENSE $1

# copy the binaries
cp build/Release/lurp.exe $1
chmod +x $1/lurp.exe

# copy required script files
mkdir $1/script
cp -r script/* $1/script

# copy game files
mkdir $1/game
cp -r game/* $1/game

# zip the folder
# 7z a $1.zip $1


