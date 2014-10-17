#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use warnings "all";
use strict;
use File::Path qw(make_path rmtree);
use Cwd 'abs_path';
use Test::More;

my $testDirName;
my $dataDirName;
my $TAGFS_PID = -1;
my $VALGRIND_OUTPUT = "";
my $TAGFS_LOG = "";
my $FUSE_LOG = "";

sub setupTestDir
{
    $testDirName = make_mount_dir();
    $dataDirName = make_data_dir();

    # Have to create this before the fork so that it's shared
    $VALGRIND_OUTPUT = `mktemp /tmp/acctest-valgrind.out.XXXXXXXXXX`;
    $TAGFS_LOG = `mktemp /tmp/acctest-tagfs-log.out.XXXXXXXXXX`;
    $FUSE_LOG = `mktemp /tmp/acctest-fuse-log.out.XXXXXXXXXX`;
    chomp $VALGRIND_OUTPUT;
    chomp $TAGFS_LOG;
    chomp $FUSE_LOG;

    my $child_pid = fork();
    if (defined $child_pid)
    {
        if ($child_pid == 0)
        {
            my $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full ../tagfs --drop-db --data-dir=$dataDirName -g 0 -l $TAGFS_LOG -d $testDirName 2> $FUSE_LOG";
            exec($cmd) or die "Couldn't exec tagfs: $!\n";
        }
        else
        {
            # Don't wait for the child here
            $TAGFS_PID = $child_pid;
            # XXX: Wait for TagFS to, hopefully, be mounted. I know this is lazy, but I don't really care.
            sleep 2
        }
    }
    else
    {
        die "Couldn't fork a child process\n";
    }
    my $i = 0;
    while ($i < 10 && system("mount | grep --silent 'tagfs on $testDirName'") != 0)
    {
        print "waiting for mount...\n";
        sleep 1;
        $i += 1;
    }
    if ($i == 10)
    {
        print "it seems we couldn't mount. Cleaning up and exiting.\n";
        cleanupTestDir();
        exit(1);
    }
}

sub make_mount_dir
{
    make_tempdir("mountdir");
}

sub make_data_dir
{
    make_tempdir("datadir");
}

sub make_tempdir
{
    my $tail = shift;
    my $s = `mktemp -d /tmp/acctest-tagfs-${tail}-XXXXXXXXXX`;
    chomp $s;
    if (not (-d $s))
    {
        print "Couldn't create test directory\n";
        exit(1);
    }
    $s
}

sub cat
{
# Not sure how portable `cat' is ...
    my $file = $_[0];
    my $fh;
    open $fh, "<", $file;
    print "Contents of $file:\n";
    print <$fh>;
    close $fh;
}

sub new_file
{
    my $file = shift;
    if ( open F, ">", $file )
    {
        close F;
        return 1;
    }
    else
    {
        return 0;
    }
}
sub cleanupTestDir
{
    eval {{
            while (`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/)
            {
                print "waiting to perform clean unmount...\n";
                sleep 1;
            }
            waitpid($TAGFS_PID, 0);
            if (-f $VALGRIND_OUTPUT && ! $ENV{TAGFS_NOTESTLOG})
            {
                if (system("grep --silent -e \"ERROR SUMMARY: 0 errors\" $VALGRIND_OUTPUT") != 0)
                {
                    cat($VALGRIND_OUTPUT);
                }

                if (system("grep --silent -e ERROR $TAGFS_LOG") == 0)
                {
                    cat($TAGFS_LOG);
                }

                if (system("grep --silent -e \"fuse_main returned 0\" $FUSE_LOG") != 0)
                {
                    cat($FUSE_LOG);
                }
            }
        }};
    `rm -rf $testDirName`;
    `rm -rf $dataDirName`;
    unlink($TAGFS_LOG);
    unlink($VALGRIND_OUTPUT);
    unlink($FUSE_LOG);
}

my @tests = (
    sub {
        my @files = map { "" . $_ } 0..8;
        foreach my $f (@files){
            if (open F, ">", "$testDirName/$f")
            {
                printf F "text$f";
                close F;
            }
            else
            {
                fail("Couldn't open file $testDirName/$f for writing: $!\n");
            }
        }

        foreach my $f (@files){
            if (open F, "<", "$testDirName/$f")
            {
                my $s = <F>;
                is($s, "text$f", "$f contains text$f");
                close F;
            }
            else
            {
                fail("Couldn't open file $testDirName/$f for reading: $!\n");
            }
        }
    },
    sub {
        # Create nested directories.
        # Should be able to access by the prefix in any order
        # mkdir a/b/c
        # [ -d "a/b/c" ] &&
        # [ -d "b/a/c" ] &&
        # [ ! -d "b/c" ] &&
        # [ ! -d "c/b" ] ...
        my @dirs = qw/a b c/;
        my $dir = "$testDirName/" . join("/", @dirs);
        make_path($dir);

        { # Testing that the tags exist
            foreach my $i (@dirs)
            {
                ok(-d ("$testDirName/$i"), "$i exists");
            }
        }

        { # Testing that you can find c where it should be
            ok(-d "$testDirName/a/b/c", "a/b/c exists");
        }

        { # Testing that you can't find c where it doesn't belong
            ok(not(-d "$testDirName/a/c"), "a/c doesn't exist");
            ok(not(-d "$testDirName/b/c"), "b/c doesn't exist");
        }

        { # Some other permutations that shouldn't be created
            ok(not(-d "$testDirName/c/a"), "c/a doesn't exist");
            ok(not(-d "$testDirName/c/b"), "c/b doesn't exist");
        }
    },
    sub {
        # Just making a file
        my $dir = "$testDirName/a/b/c/d/e/f/g/h";
        make_path($dir);
        open F, ">", "$dir/file";
        printf F "text\n";
        close F;
        open F, "<", "$dir/file";
        my $s = <F>;
        is($s, "text\n", "Read the stuff back in");
        close F;
    },
    sub {
        # renaming a file
        my $dir = $testDirName;
        my $file = "$dir/file";
        my $newfile = $file . ".stuff";
        open F, ">", $file;
        close F;
        rename $file, $newfile;
        print "$file to $newfile\n";
        ok(-f $newfile, "new file exists");
        ok(not (-f $file), "old file doesn't exist");
    },
    sub {
        # Creating a file at a non-existant directory
        # See valgrind output
        my $dir = $testDirName . "/IDontExist";
        my $file = "$dir/file";
        ok(not(new_file($file)), "file not created at non-existant directory");
    },
    sub {
        # Adding a few tags and then deleting one
        my @dirs = qw/dir1 dir2 dir3 dir5 dir23/;
        for (@dirs)
        {
            mkdir $testDirName . "/" . $_;
        }
        my $removed = $testDirName . "/dir23";
        rmdir $removed;
        ok(not(-d $removed), "removed directory doesn't exist");
        ok(-d $testDirName . "/dir1", "not removed(dir1) still exists");
        ok(-d $testDirName . "/dir2", "not removed(dir2) still exists");
        ok(-d $testDirName . "/dir3", "not removed(dir3) still exists");
        ok(-d $testDirName . "/dir5", "not removed(dir5) still exists");
    },
    sub {
        # Adding a tag with an id prefix is disallowed
        my $d = $testDirName . "/234:dir";
        ok(not(mkdir $d), "mkdir errored");
        ok(not(-d $d), "directory wasn't created anyway");
    },
    sub {
        # Removing a directory that still has stage contents is disallowed
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/a/b/c";
        make_path($d);
        ok(not(rmdir $e), "mkdir errored");
        ok((-d $d), "contents remain");
    },
    sub {
        # When all tags are for a given file, it should show up at the root.
        my $d = "$testDirName/a/f/g";
        make_path $d;
        my $f = $d . "/file";
        new_file($f);
        rmdir "$testDirName/a";
        rmdir "$testDirName/f";
        rmdir "$testDirName/g";
        ok(not(-f $f), "can't find it at the original location");
        ok(-f "$testDirName/file", "can find it at the root");
    },
    sub {
        # When all tags are for a given file, it should show up at the root.
        my $d = "$testDirName/a/f/g";
        make_path $d;
        rmdir "$testDirName/a";
        rmdir "$testDirName/f";
        rmdir "$testDirName/g";
        opendir(my $dh, "$testDirName/a") || die "can't opendir $testDirName/a: $!";
        my @dirs = readdir($dh);
        closedir $dh;
        ok((scalar(@dirs) == 0), "all directories are removed");
    },
    sub {
        #adding a tag
        my $c = "$testDirName/a";
        my $cc = "$testDirName/a/b";
        my $d = "$testDirName/b";
        my $dd = "$testDirName/b/a";
        mkdir $c;
        mkdir $d;
        my $f = "$c/file";
        my $g = "$d/file";
        new_file($f);
        # move f to g
        rename $f, $g;
        ok(-f $f, "The file can still be found at the original location");
        ok(-f $g, "The file can be found at the new location");
        ok(-d $cc, "Tag subdir has been added at the original tag");
        ok(-d $dd, "Tag subdir has been added at the new tag");
    },
    sub {
        # Had this problem where any file in the root directory would cause
        # getattr to return true for any file in the root
        my $f = "$testDirName/a";
        my $d = "$testDirName/adir";

        new_file($f);
        ok(mkdir ($d), "mkdir succeeded");

    },
    sub {
        # when we delete an untagged file, it shouldn't show anymore
        my $f = "$testDirName/a";
        new_file($f);
        ok(unlink ($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
    },
    sub {
        # when we delete a tagged file, it shouldn't show anymore
        my $d = "$testDirName/dir";
        my $f = "$d/file";
        mkdir $d;
        new_file($f);
        ok(unlink ($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
        ok(not (-f "$testDirName/file"), "not even as an untagged file");
    },
    sub {
        # when we delete a tagged file, it shouldn't show anymore
        # and it shouldn't show in any of its tag/folders
        my $d = "$testDirName/dir/dur";
        my $f = "$d/file";
        make_path($d);
        new_file($f);
        ok(unlink($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
        ok(not (-f "$testDirName/dir/file"), "not in dir");
        ok(not (-f "$testDirName/dur/file"), "and not in dur");
        ok(not (-f "$testDirName/file"), "not even as an untagged file");
    },
    sub {
        # when we delete a tagged file, the associated tags shouldn't
        # show up any more as subdirectories, unless they are staged 
        # directories also
        my $d = "$testDirName/dir/dur";
        my $f = "$d/file";
        make_path($d);
        new_file($f);

        ok(unlink ($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
        ok(not (-d "$testDirName/dur/dir"), "associated tag subdir doesn't exist");
        ok(-d $d, "but the originally created tree does");
    },
    sub {
        # When we do a rm -r on each tag, we should have no tags left
        #
        # NOTE: This is based on a series of commands that gave me a resource
        # management error (early delete). It can probably be simplified
        my @dirs = qw/a b c d e f g/;
        @dirs = map { "$testDirName/$_"; } @dirs;
        my $d = "$testDirName/a/b/c/d/e/f/g";
        my $f = "$testDirName/a/b/file";
        my $g = "$testDirName/a/b/c/file";
        my $h = "$testDirName/d/file";
        make_path($d);
        new_file($f);
        rename $f, $g;
        rename $f, $h;
        `rm -r $testDirName/*`;

        foreach my $x (@dirs)
        {
            ok(not (-d $x), "$x is gone.");
        }
    },
    sub {
        my $d = "$testDirName/a/b";
        my $e = "$testDirName/a/b/c";
        my $f = "$testDirName/a/b/file";
        my $k = "$testDirName/b/a/c";
        make_path($d);
        mkdir $e;
        new_file $f;
        ok(not (-d $k), "$k doesn't exist.");
    }
);

sub explore
{
    # Explore a test by getting a look before it is cleaned up
    my $test = shift;
    &setupTestDir;
    &$test;
    system("cd $testDirName && $ENV{'SHELL'}");
    &cleanupTestDir;
}

sub run_test
{
    my $test = shift;
    &setupTestDir;
    &$test;
    &cleanupTestDir;
}

if (scalar(@ARGV) > 0)
{
    my $z = 0;
    my $t = -1;
    for my $f (@ARGV)
    {
        if ($f eq "e")
        {
            $z = 1;
        }
        else
        {
            $t = $f;
        }
    }
    if ($t >= 0)
    {
        my $test = $tests[$t];
        if ($z)
        {
            explore($test);
        }
        else
        {
            run_test($test);
        }
    }
}
else
{
    my $test_number = 0;
    foreach my $t (@tests)
    {
        print "Test number $test_number:\n";
        run_test($t);
        print "\n";
        $test_number++;
    }
}
#explore(15);

done_testing();
