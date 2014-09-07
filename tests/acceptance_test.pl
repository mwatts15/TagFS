#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use strict;
use File::Path qw(make_path rmtree);
use Test::More;

my $testDirName = "testDir";
my $dataDirName = "acceptanceTestData";
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
            my $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full ../tagfs --drop-db --data-dir=$dataDirName -g 0 -s -l $TAGFS_LOG -d $testDirName 2> /dev/null";
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
sub cleanupTestDir
{
    while (`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/)
    {
        print "sleeping\n";
        sleep 1;
    }
    waitpid($TAGFS_PID, 0);
    if (-f $VALGRIND_OUTPUT)
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
        my @dirs = map { "dir" . $_ } 0..8;
        my $dir = "$testDirName/" . join("/", @dirs);
        make_path($dir);
    },
    sub {
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
        my $newfile = $file . ".stuff";
        open F, ">", $file;
        close F;
    }
);

foreach my $t (@tests)
{
    &setupTestDir;
    &$t;
    &cleanupTestDir;
    print "\n";
}
done_testing();
