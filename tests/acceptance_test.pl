#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use strict;
use File::Path qw(make_path rmtree);
use Cwd 'abs_path';
use Test::More;

my $testDirName = abs_path("testDir");
my $dataDirName = abs_path("acceptanceTestData");
my $TAGFS_PID = -1;
my $VALGRIND_OUTPUT = "";
my $TAGFS_LOG = "";
sub setupTestDir
{
    while (`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/)
    {
        print "Mount directory is busy. Sleeping.\n";
        sleep 1;
    }

    `rm -rf $testDirName`;
    if (not (mkdir $testDirName))
    {
        print "Couldn't create test directory\n";
        exit(1);
    }

    # Have to create this before the fork so that it's shared
    $VALGRIND_OUTPUT = `mktemp /tmp/acctest-valgrind.out.XXX`;
    $TAGFS_LOG = `mktemp /tmp/acctest-tagfs-log.out.XXX`;
    chomp $VALGRIND_OUTPUT;
    chomp $TAGFS_LOG;

    my $child_pid = fork();
    if (defined $child_pid)
    {
        if ($child_pid == 0)
        {
            my $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind -v --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full ../tagfs --drop-db --data-dir=$dataDirName -g 0 -s -l $TAGFS_LOG -d $testDirName 2> /dev/null";
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
    while (`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/)
    {
        print "sleeping\n";
        sleep 1;
    }
    waitpid($TAGFS_PID, 0);
    if (-f $VALGRIND_OUTPUT && ! $ENV{TAGFS_NOTESTLOG})
    {
        if (system("grep --silent -e \"ERROR SUMMARY: 0 errors\" $VALGRIND_OUTPUT") != 0)
        {
            cat($VALGRIND_OUTPUT);
            cat($TAGFS_LOG);
        }
    }
    `rm -rf $testDirName`;
    unlink($TAGFS_LOG);
    unlink($VALGRIND_OUTPUT);
}

my @tests = (
    sub {
        my @files = map { "" . $_ } 0..8;
        foreach my $f (@files){
            open F, ">", "$testDirName/$f";
            printf F "text$f\n";
            close F;
        }

        foreach my $f (@files){
            open F, "<", "$testDirName/$f";
            my $s = <F>;
            is($s, "text$f\n");
            close F;
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
        ok(not(new_file($file)));
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
);

foreach my $t (@tests)
{
    &setupTestDir;
    &$t;
    &cleanupTestDir;
    print "\n";
}
done_testing();
