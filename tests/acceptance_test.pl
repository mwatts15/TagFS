#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use warnings "all";
use strict;
use File::Path qw(make_path);
use File::stat;
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
            my $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --track-origins=yes --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full ../tagfs --drop-db --data-dir=$dataDirName -g 0 -l $TAGFS_LOG -d $testDirName 2> $FUSE_LOG";
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

sub dir_contents
{
    my $dir = shift;
    opendir(my $dh, $dir);
    my @l = readdir($dh);
    closedir $dh;
    return @l;
}

sub mkpth
{
    # For some reason, fuse doesn't give me requests that come at it
    # as fast as they come from this script. This script tries to
    # delay execution of make_path in order that all of the intended
    # requests are received
    my $arg = shift;
    my $limit = 1;
    my $i = 0;
    while ((not make_path($arg)) and ($i < $limit))
    {
        sleep 1;
        $i++;
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

                if (system("grep -E --silent -e 'ERROR|WARN' $TAGFS_LOG") == 0)
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
        mkpth($dir);

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
        mkpth($dir);
        open F, ">", "$dir/file";
        printf F "text\n";
        close F;
        open F, "<", "$dir/file";
        my $s = <F>;
        is($s, "text\n", "Read the stuff back in");
        close F;
    },
    sub {
        # Make a file and delete it. The directory should be empty
        my $dir = $testDirName;
        my $file = "$dir/file";
        new_file($file);
        ok(-f $file, "new file exists");
        unlink $file;
        opendir(my $dh, $dir);
        my @l = readdir($dh);
        closedir $dh;
        ok((scalar(@l) == 0), "Directory is empty");
    },
    sub {
        # Make a couple of directories and then make a file. All three should list
        my $dir = $testDirName;
        my $d1 = "$dir/d1";
        my $d2 = "$dir/d2";
        my $file = "$dir/file";
        mkdir $d1;
        mkdir $d2;
        new_file($file);
        my @l = dir_contents($dir);
        my %fs = map { $_ => 1 } @l;
        ok((scalar(@l) == 3), "Directory list three files");
        ok((defined $fs{"d1"}), "d1 is there");
        ok((defined $fs{"d2"}), "d2 is there");
        ok((defined $fs{"file"}), "file is there");
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
        my $d = $testDirName . "/234#dir";
        ok(not(mkdir $d), "$d mkdir errored");
        ok(not(-d $d), "directory wasn't created anyway");
    },
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = $testDirName . "/not_a_number#dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = $testDirName . "/1b#dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = $testDirName . "/b1#dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    sub {
        # Removing a directory that still has stage contents is allowed
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/a/b/c";
        mkpth($d);
        ok((rmdir $e), "mkdir errored");
        ok(not (-d $d), "contents remain");
    },
    sub {
        # When all tags deleted are for a given file, it should show up at the root.
        my $d = "$testDirName/a/f/g";
        mkpth $d;
        my $f = "$d/file";
        new_file($f);
        rmdir "$testDirName/a";
        rmdir "$testDirName/f";
        rmdir "$testDirName/g";
        ok(not(-f $f), "can't find it at the original location");
        ok(-f "$testDirName/file", "can find it at the root");
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
        mkpth($d);
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
        mkpth($d);
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
        mkpth($d);
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
        my $c = "$testDirName/a";
        my $d = "$testDirName/b";
        my $cd = "$testDirName/a/b";
        rename($c, $cd);
        sleep(1);
        ok((-d $cd), "Directory appears at rename location");
    }
    sub {
        my $d = "$testDirName/a/b";
        my $e = "$testDirName/a/b/c";
        my $f = "$testDirName/a/b/file";
        my $k = "$testDirName/b/a/c";
        mkpth($d);
        mkdir $e;
        new_file $f;
        ok(not (-d $k), "$k doesn't exist");
    },
    sub {
        # A 'staged' directory should disappear if one of its 
        # components is deleted
        my $d = "$testDirName/a/b";
        my $e = "$testDirName/b";
        my $f = "$testDirName/a";
        mkpth($d);
        rmdir $e;
        sleep 1;# The rmdir doesn't complete quickly enough for the tests to work...
        my @cont = dir_contents($f);
        ok(not (-d $d), "$d doesn't exist");
        ok((scalar(@cont) == 0), "$f is empty");
    },
    sub {
        # Another fun test
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/c";
        mkpth($d);
        rmdir $e;
        mkpth($d);
        ok((-d $d), "$d exists");
    },
    sub {
        # Based on incorrect entries in the file_tag table causing entries to hang around
        my $f = "$testDirName/f";
        my $d = "$testDirName/b";
        my $z = "$testDirName/b/f";
        new_file($f);
        mkdir($d);
        ok(rename($f,$z), "rename succeeds");
        my $n = dir_contents($testDirName);
        ok((not -f $f), "File isn't at the root");
        ok(($n == 1), "root contains one entry ($n)");
        ok((-f $z), "File has actually moved");
    },
    sub {
        # Removing a tag from a file
        my $f = "$testDirName/f";
        my $d = "$testDirName/a/b/c";
        my $z = "$testDirName/a/b/c/f";
        my $e = "$testDirName/a/f";
        new_file($f);
        mkpth($d);
        ok(rename($f,$z), "rename (add a/b/c) succeeds");
        ok(rename($z,$e), "rename (remove b/c) succeeds");

        my $n = dir_contents("$testDirName/b");
        my $m = dir_contents("$testDirName/c");

        ok(($n == 0), "b is empty ($n)");
        ok(($n == 0), "c is empty ($n)");
    },
    sub {
        # rename idempotent 1
        my $d = "$testDirName/a";
        my $z = "$testDirName/b";
        my $f = "$d/f";
        my $r = "$z/f";
        mkdir ($d);
        mkdir ($z);
        new_file($f);
        ok(rename($f,$r), "first rename $f to $r succeeds");
        ok(rename($f,$r), "second rename $f to $r succeeds");
        ok((-f $f), "$f still exists");
        ok((-f $r), "$r also exists");
    },
    sub {
        # Ensure that we can set times for a file
        my $f = "$testDirName/f";
        my $time = 23;
        new_file($f);
        utime $time, $time, $f;
        my $stat = stat($f);
        is($time, $stat->atime, "atime is set");
        is($time, $stat->mtime, "mtime is set");
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
