BUILD
-----

Do:

    $ make tagfs

in the main project directory. Then run:

    $ make tests

to build and run the tests to make sure everything works correctly on your system.

Prerequisites
 - fuse (libs and headers) (>= 2.8.7)
 - glib (Works with glib versions 2.24.1, 2.30.2, 2.32.3)

 On Debian/Ubuntu:

     $ apt-get install libglib2.0-dev libfuse-dev


USAGE
-----
*Note that this is usage based on R1, so some of these interactions aren't supported in beta*

To mount TagFS

    ./tagfs <mount directory>

Where  <mount directory> is an empty directory. TagFS will create the files it needs in what it thinks is your user-data directory (~/.local/share on linux). You can add files by moving them to the mount directory. Be careful to unmount TagFS properly or you WILL lose data from changes made while mounted.

All un-tagged files are shown in the top level -- this is the default behaviour. One could also have *all* managed files appear in the top-level of the tagfs with a special "unsorted" tag applied to all untagged files that are however managed. The idea is that the tag filesystem is to be used in conjunction with a hierarchical file system, but it could be used on its own.

When you "copy" a file to the mounted tagfs, the file is tagged with the associated directory and thus appears there in the hierarchy. Moving a file already within the tagfs to another directory in the tagfs will simply add the tags that comprise the path. Deleting a file deletes the file proper, so that it no longer appears in the filesystem. A set of special file handles described in params.h allow for other actions, like removing tags from a file (`mv file [mountpoint]/tags/to/remove/:X`) or doing a search for files in the filesystem (`ls [mountpoint]/:S"tag1 AND tag2 OR tag3"`).

The main advantage of this system is that it allows you to have files stored in more than one logical location at the same time without having to manage soft or hard links between files. I'm sure the advantages of a system like this are well-documented elsewhere so I won't go into it.

CAVEATS
~~~~~~~
Copying a file from one point in the file system to another where the file already appears will succeeed, but it will fill the file with NULL bytes. I don't yet know how to deal with this. Be sure to *rename* the file rather than copying it to avoid this affect.

File names are are prefixed with an ID number that serves to distinguish them when otherwise there would be a naming conflict. The file can still be accessed by its un-prefixed name, however if there are multiple files with the same name (sans-prefix) you should not depend on a particular one being returned. In this case, you should either reference the file relative to a more specific directory where there is no conflict or else use the name with the id.

IMPLEMENTATION
--------------
A database of files and their associated tags is loaded on mount, managaed by the running file system, and saved on unmount. Tags have simple types like String and Integer associated with them, which users can modify by writing to a special filehandle in the file system or using special utilities that write this filehandle.

The real files (i.e. stored on disk) are stored in a separate directory outside of the mount point (or possibly the mount directory itself in the future) which is specified on mount. They are given id numbers that remain with them for the life of the file. This is also the reason why you must be sure to unmount the filesystem properly since there is currently no means of restoring actions performed while mounted if the database is not saved.

QUESTIONS
---------
If you have any questions for me or about TagFS, don't hesitate to contact me -- I'd be happy to help.
