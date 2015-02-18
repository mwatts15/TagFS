#!/bin/sh

commit=`git show-ref --head | grep " HEAD$" | egrep -o "^[^ ]+"`
tag=`git tag --points-at $commit | head -n 1`
if [ $tag ] ; then
    echo $tag
else
    for hist in `git log --format=%H`; do
        tag=`git tag --points-at $hist | head -n 1`
        if [ $tag ] ; then
            break
        fi
    done
    abbr_head=`echo $commit|head -c 7`
    echo $tag+$abbr_head
fi
