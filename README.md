BUILD
=====

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mwatts15/TagFS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Do:

    $ make tagfs

in the main project directory. Then run:

    $ make tests && make acc-test

to build and run the tests to make sure everything works correctly on your system.

Prerequisites:
 - GCC (4.8.2) or Clang (3.3)
 - Fuse (libs and headers) (2.8.7)
 - GLib (Works with glib versions 2.24.1, 2.30.2, 2.32.3, 2.40.2)
 - CUnit for tests (2.1-3)
 - Valgrind for testing and development

*Other versions may or may not work.*

On Debian/Ubuntu:

     $ apt-get install libglib2.0-dev libfuse-dev libcunit1-dev valgrind


USAGE
=====
To mount TagFS

    ./tagfs <mount directory>

Where `<mount directory>` is an empty directory. TagFS will create the files it needs in what it thinks is your user-data directory (at `~/.local/share/tagfs` on Linux). You can add files by moving them to the mount directory. Be careful to unmount TagFS properly or you WILL lose data from changes made while mounted.

**CAUTION**:I'm not writing database migration scripts. If you use tagfs, please let me know so that I will start writing them to help you migrate your data if the database format changes.

All un-tagged files are shown in the top level. A file can be referenced at any point in the file system by it's id, so a file `movies/Seven_Samurai` with id `12334` can be referenced as `12334#` or `akira_kurosawa/12334#Seven_Samurai` or `some/random/directory/12334#what-even-is-this-file-s-name`.

Tags are created by making a directory under the mounted TagFS (`mkdir tagname`). In general, directories appear in any location where you could open the directory and find more files. Tags can be renamed and made to appear at additional locations by moving the directory (`mv tag a/b/c/tag`). 

Tags can be put in a hierarchy for purposes of organization. If you make a tag `a::b`, TagFS will first create the `a` tag and then the `a::b` tag as its child. An `a::b` directory can then be found under `a` and any files that belong to the `a::b` tag will also appear under `a`. An arbitrarily deep (but resource-limited!) nesting of tags can be made. Renaming tags can detach them from their parent tags, but child tags are only detached from their parents if the child-tags themselves are renamed or the parent tag is deleted.

When you "copy" a file to the mounted tagfs, the file is tagged with the directory name it falls under and thus appears where it would in a normal  hierarchical file system. The actual file content is stored in your tagfs data directory which, by default, is in your tagfs user-data directory. You can set the location of your data directory with the `--data-dir` option to tagfs.

Moving a file already within the tagfs to another directory in the tagfs will add the tags that comprise the path except in the special case for removing tags. To remove tags from a file, move the file to a directory above the one you are moving from (`mv a/b/tag-to-remove/1#file a/b`)[1]. Note that the set of tags removed depends ONLY on the starting and ending locations rather than on all of the tags associated with the file: 
    
    $ ls a/b/c/d
    file
    $ mv a/b/file a/
    $ ls a/b/c/d
    $ ls a/c/d
    file

If the destination location isn't an ancestor of the starting location, no tags will be removed, but tags besides those already attached to the file will be added.

Moving a *directory* to a new location (`mv the-tag new-location`) will cause the directory to show up there, but it will also remain in the original location; you could do the same thing by calling `mkdir new-location/the-tag` assuming `the-tag` already exists.

When listing files, there are situations where two files with the same name would be listed together. In this case, one of the files is listed normally, but all of the files (including that first one) are also listed with their prefixed name (e.g., `1#filename`). This allows for accessing the file under the usual name as well as accessing all of the files regardless of where they are accessed from.

Deleting a file deletes the file proper, so that it no longer appears in the file system.

The main advantage of this system is that it allows you to have files stored in more than one logical location at the same time without having to manage soft or hard links between files. I'm sure the advantages of a system like this are well-documented elsewhere so I won't go into it.

  [1]: In fact, `mv /a/b/tag-to-remove/1#file /b/a/new-file-name`, works also. The new directory just has to be composed of tags which are a strict subset of the ones that make up the old directory.

CAVEATS
-------

Copying a file from one point in the file system to another where the file already appears will succeed, but it will fill the file with NULL bytes. I don't yet know how to deal with this. Be sure to *rename* the file rather than copying it to avoid this affect.

IMPLEMENTATION
==============
A SQLite database of files and their associated tags is loaded on mount and managed by the running file system. The database file is unlocked unless locked by a SQLite operation so that changes caused by specific operations can can be observed.

As stated, the real files are stored in a separate directory outside of the mount point. The names of these files are the id numbers of the associated tagfs file.

QUESTIONS
=========
If you have any questions for me or about TagFS, don't hesitate to contact me -- I'd be happy to help.
