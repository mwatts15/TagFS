#!/bin/sh

commit=`git show-ref --head | grep " HEAD$" | egrep -o "^[^ ]+"`
tag=`git tag --points-at $commit | head -n 1`
if [ $tag ] ; then
    echo $tag
else
    branch=`git branch --list | grep '*' | tail -c +3`

    for hist in `git log --format=%H`; do
        tag=`git tag --points-at $hist | head -n 1`
        if [ $tag ] ; then
            break
        fi
    done
    abbr_head=`echo $commit|head -c 7`
    if [ $branch != "master" ] ; then
        echo $tag-$branch+$abbr_head
    else
        echo $tag+$abbr_head
    fi
fi
