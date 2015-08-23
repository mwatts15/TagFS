[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/mwatts15/TagFS?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
DOWNLOAD
========

You can download TagFS on Github. The master branch will always have a named release, but I recommend downloading one of the stable (non-alpha/beta) [releases](https://github.com/mwatts15/TagFS/releases).

VERSIONS
--------
TagFS has version numbers like `X[.Y[.Z]][(alpha|beta)][R]` where:

- `X` is the Release Number: Changes when a set of features named for a release has been completed and tested altogether.
- `Y` is the Feature Number: Changes when a new release feature has been added or the database changes in a way that could result in data loss -- usually these conditions coincide and the feature number only increases by 1.
- `Z` is the Fix Number: Changes when a bug fix is added post-release or when a feature is added that doesn't change previously existing and documented functionality and that doesn't cause a change in the database that could result in data loss.
- `alpha` or `beta` is added for pre-release states. `alpha` means the feature is there, but untested. `beta` means that the feature has been tested, but I'll probably add more tests, and feature isn't be used by the developer for his own files yet. These tags probably won't be added for 'fix' releases 
- `R` is the Pre-Release Revision Number: This is some change during pre-release.

I may change the scheme at some time in the future, but I'll document the change here.

BUILD
=====

Do:

    $ make tagfs

in the main project directory. Then run:

    $ make tests && make acc-test

to build and run the tests to make sure everything works correctly on your system.

Prerequisites:
 - GCC (4.8.2) or Clang (3.3)
 - Fuse (libs and headers) (2.9.2)
 - GLib (2.40.2)
 - For testing:
   - CUnit (2.1-3)
   - Valgrind for memory checking
   - lcov (1.9)

*The versions listed are those that I use for development. Other versions may or may not work.*

On Debian/Ubuntu:

     $ apt-get install gcc libglib2.0-dev libfuse-dev libcunit1-dev valgrind

USAGE
=====
To mount TagFS

    ./tagfs <mount directory>

Where `<mount directory>` is an empty directory. TagFS will create the files it needs in what it thinks is your user-data directory (at `~/.local/share/tagfs` on Linux). You can add files by moving them to the mount directory. Unmount TagFS properly or you may lose data from changes made while mounted.

As TagFS is still in active development, the database format has changed and may change in the future. There is code to migrate data from an earlier format to the current one. If, in the future, a change to the database would require a loss of data, the database will not be upgraded automatically. In any case, if an upgrade is attempted on your database, it will be backed up first.

If you drop the database by passing the option `--drop-db` to `tagfs`, the database will NOT be backed up or recoverable in any way.

All un-tagged files are shown in the top level. A file can be referenced at any point in the file system by it's id, so a file `movies/Seven_Samurai` with id `12334` can be referenced as `12334#` or `akira_kurosawa/12334#Seven_Samurai` or `some/random/directory/12334#what-even-is-this-file-s-name`. The id of a file can be found by looking at its inode number -- see documentation on the `stat` system call for more information. It is not allowed to create nor to rename to a file that has an id prefix.

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

There are also utilities `ts`, `rt`, and `lt` for adding, removing, and listing tags on a file under TagFS. A [PCManFM](http://wiki.lxde.org/en/PCManFM) extension is also provided which lists the tags on a file when the file is selected in that browser.

Moving a *directory* to a new location (`mv the-tag new-location`) will cause the directory to show up there, but it will also remain in the original location; you could do the same thing by calling `mkdir new-location/the-tag` assuming `the-tag` already exists.

When listing files, there are situations where two files with the same name would be listed together. In this case, one of the files is listed normally, but all of the files (including that first one) are also listed with their prefixed name (e.g., `1#filename`). This allows for accessing the file under the usual name as well as accessing all of the files regardless of where they are accessed from.

Deleting a file deletes the file proper, so that it no longer appears in the file system.

The main advantage of this system is that it allows you to have files stored in more than one logical location at the same time without having to manage soft or hard links between files. I'm sure the advantages of a system like this are well-documented elsewhere so I won't go into it.

  [1]: In fact, `mv /a/b/tag-to-remove/1#file /b/a/new-file-name`, works also. The new directory just has to be composed of tags which are a strict subset of the ones that make up the old directory.

IMPLEMENTATION
==============
A SQLite database of files and their associated tags is loaded on mount and managed by the running file system. The database file is unlocked unless locked by a SQLite operation so that changes caused by specific operations can can be observed.

As stated, the real files are stored in a separate directory outside of the mount point. The names of these files are the id numbers of the associated tagfs file.

MISCELLANEOUS
=============
If you have a volume monitor running (i.e., gvfs-udisks2-volume-monitor), you may see a CPU spike on mount.

With name conflicts between a tag and a file, the file will be listed with its prefixed name.

QUESTIONS
=========
If you have any questions for me or about TagFS, don't hesitate to contact me -- I'd be happy to help.
