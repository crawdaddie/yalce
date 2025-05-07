#!/bin/sh

VST3DIR=$1
echo $VST3DIR
if [ "x$1" = "x" ]; then
 VST3DIR=libs/VST3_SDK
fi

rm -rf "${VST3DIR}"
git clone https://github.com/steinbergmedia/vst3_pluginterfaces "${VST3DIR}/pluginterfaces/"
