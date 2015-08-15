#!/bin/bash

if [ ! -e "#LISTEN#" ] ; then
    exit;
fi

name=`zenity --entry --text "New tag name:" --title "Tag Name"`
type=`zenity --list --radiolist --text "Select the tag type from the list below" --column="" --column="Tag Type" 0 Integer 0 String`

case $type in
    Integer)
        mktag $name INT
        ;;
    String)
        mktag $name STRING
        ;;
esac

