#!/bin/bash

if [ ! -e "#LISTEN#" ] ; then
    exit;
fi

name=$1
tname=`zenity --entry --text "Tag name:" --title "Tag Name"`
value=`zenity --entry --text "Tag value:" --title "Tag Value"`

addtags $name $tname:$value
