#!/usr/bin/env python3

import os
import sys

DRY_RUN = False
if len(sys.argv) > 2:
    DIRECTORY=sys.argv[1]
    MOUNT=sys.argv[2]
else:
    DIRECTORY=os.environ["HOME"] + "/documents/"
    MOUNT="mount"

def cut_prefix(prefix, s):
    idx = s.find(prefix)
    if idx >= 0:
        return s[len(prefix):]
    else:
        raise Exception("bad path "+s)

for (dirname,subdirs,files) in os.walk(DIRECTORY):
    relative_directory = cut_prefix(DIRECTORY, dirname)
    dest_directory = MOUNT +"/"+relative_directory
    if DRY_RUN:
        print("mkdir '{}'".format(dest_directory))
    else:
        try:
            os.mkdir(dest_directory)
        except FileExistsError:
            print("Skipping mkdir {}: file exists".format(dest_directory))

    for f in files:
        source = dirname + "/" + f
        dest = dest_directory + "/" + f
        if DRY_RUN:
            print("link -s '{}' '{}'".format(source, dest))
        else:
            try:
                os.symlink(source, dest)
            except FileExistsError:
                print("Skipping symlink {}: file exists".format(dest))
