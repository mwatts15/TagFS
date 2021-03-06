#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use warnings "all";
use strict;
use File::Path qw(make_path);
use Cwd 'abs_path';
use Test::More;
use Term::ANSIColor;
use Fcntl;
use File::stat;


my $failures_fname = `mktemp /tmp/acctest-valgrind.out.XXXXXXXXXX`;
open my $failures_fh, "+>", $failures_fname;
Test::More->builder->output("/dev/null"); # Hide non-failures
Test::More->builder->failure_output($failures_fh);
Test::More->builder->todo_output($failures_fh);

my $testDirName;
my $dataDirName;
my $TAGFS_PID = -1;
my $VALGRIND_OUTPUT = "";
my $TAGFS_LOG = "";
my $FUSE_LOG = "";
my $SHOW_LOGS = 0;
my @TESTS = ();
my @TESTRANGE = ();

my $TPS = "::"; # tag path separator. This must match the TAG_PATH_SEPARATOR in ../tag.h
my $FIS = "#"; # file id separator. This must match the FILE_ID_SEPARATOR in ../abstract_file.h

if (defined($ENV{TESTS}))
{
    if ($ENV{TESTS} =~ /^(.*)\.\.(.*)$/)
    {
        @TESTRANGE = ($1, $2);
    }
    else
    {
        @TESTS = split(' ', $ENV{TESTS});
    }
}

if (defined($ENV{SHOW_LOGS}))
{
    $SHOW_LOGS = 1;
}

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
            my $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --track-origins=yes --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full ../tagfs -o use_ino,attr_timeout=0 --drop-db --data-dir=$dataDirName -g 0 -l $TAGFS_LOG -d $testDirName 2> $FUSE_LOG";
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
        note("waiting for mount...\n");
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

sub raise_test_caller_level
{
    Test::More->builder->level(Test::More->builder->level()+1);
}
sub lower_test_caller_level
{
    Test::More->builder->level(Test::More->builder->level()-1);
}

no warnings "redefine";
sub ok($;$)
{
    my ($test, $message) = @_;
    raise_test_caller_level();
    Test::More::ok($test, colored("$message", "bright_red"));
    lower_test_caller_level();
}

sub is($$;$)
{
    my ($first, $second, $message) = @_;
    raise_test_caller_level();
    Test::More::is($first, $second, colored($message, "bright_red"));
    lower_test_caller_level();
}

sub note
{
    my $message = shift;
    Test::More::note(colored($message, "blue"));
}

sub diag
{
    my $message = shift;
    Test::More::diag(colored($message, "green"));
}
use warnings "redefine";

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

sub cat_to_stderr
{
    my $file = $_[0];
    my $fh;
    open $fh, "<", $file;
    print STDERR "Contents of $file:\n";
    print STDERR <$fh>;
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
    if (opendir(my $dh, $dir))
    {
        my @l = readdir($dh);
        closedir $dh;
        return @l;
    }
    return ();
}

sub dir_contains
{
    my ($dir, $f) = @_;
    my @l = dir_contents($dir);
    foreach my $g (@l)
    {
        if ($f eq $g)
        {
            return 1;
        }
    }
    return 0;
}

sub mkpth
{
    # XXX: Augments make_path to retry
    my $arg = shift;
    my $limit = 1;
    my $i = 0;
    my $err;
    make_path($arg, {error => \$err});
    while (scalar(@$err))
    {
        if ($i == $limit)
        {
            return 0;
        }
        sleep 1;
        make_path($arg, {error => \$err});
        $i++;
    }
    return 1;
}

sub cleanupTestDir
{
    eval {{
            while (`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/)
            {
                note("waiting to perform clean unmount...\n");
                sleep 1;
            }
            waitpid($TAGFS_PID, 0);
            if (-f $VALGRIND_OUTPUT && ! $ENV{TAGFS_NOTESTLOG})
            {
                if (system("grep --silent -e \"ERROR SUMMARY: 0 errors\" $VALGRIND_OUTPUT") != 0)
                {
                    cat_to_stderr($VALGRIND_OUTPUT);
                }

                if (system("grep -E --silent -e 'ERROR|WARN' $TAGFS_LOG") == 0)
                {
                    cat_to_stderr($TAGFS_LOG);
                }

                if (system("grep --silent -e \"fuse_main returned 0\" $FUSE_LOG") != 0)
                {
                    cat_to_stderr($FUSE_LOG);
                }
            }

            if ($SHOW_LOGS)
            {
                print("SHOWING LOGS\n");
                cat_to_stderr($VALGRIND_OUTPUT);
                cat_to_stderr($TAGFS_LOG);
                cat_to_stderr($FUSE_LOG);
            }
        }};
    `rm -rf $testDirName`;
    `rm -rf $dataDirName`;
    unlink($TAGFS_LOG);
    unlink($VALGRIND_OUTPUT);
    unlink($FUSE_LOG);
}

my @tests_list = (
    0 =>
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
    1 =>
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
    2 =>
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
    files_dissappear_when_deleted =>
    sub {
        # Make a file and delete it. The directory should be empty
        my $dir = $testDirName;
        my $file = "$dir/file";
        new_file($file);
        ok(-f $file, "new file exists");
        unlink $file;
        my @l = dir_contents($dir);
        # XXX: This one sometimes fails. Possible timing issue
        ok((scalar(@l) == 0), "Directory is empty") or diag("This one sometimes fails due to a timing issue");
    },
    4 =>
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
    5 =>
    sub {
        # renaming a file
        my $dir = $testDirName;
        my $file = "$dir/file";
        my $newfile = $file . ".stuff";
        open F, ">", $file;
        close F;
        rename $file, $newfile;
        note("Renamed $file to $newfile\n");
        ok(-f $newfile, "new file exists");
        ok(not (-f $file), "old file doesn't exist");
    },
    6 =>
    sub {
        # Creating a file at a non-existant directory
        # See valgrind output
        my $dir = $testDirName . "/IDontExist";
        my $file = "$dir/file";
        ok(not(new_file($file)), "file not created at non-existant directory");
    },
    7 =>
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
    add_tag_with_id_prefix_bad0 =>
    sub {
        # Adding a tag with an id prefix is disallowed
        my $d = "$testDirName/234${FIS}dir";
        ok(not(mkdir $d), "mkdir $d errored");
        ok(not(-d $d), "directory wasn't created anyway");
    },
    add_tag_with_id_prefix_bad1 =>
    sub {
        # Adding a tag with an id prefix is disallowed
        my $d = "$testDirName/234${FIS}${FIS}dir";
        ok(not(mkdir $d), "mkdir $d errored");
        ok(not(-d $d), "directory wasn't created anyway");
    },
    add_tag_with_id_prefix_bad2 =>
    sub {
            # Adding a tag with an id prefix is disallowed
            my $d = "$testDirName/ 234${FIS}dir";
            ok(mkdir($d), "mkdir $d succeeds");
            ok((-d $d), "directory was created");
    },
    add_tag_with_id_prefix_bad3 =>
    sub {
            # Adding a tag with an id prefix is disallowed
            my $d = "$testDirName/234 ${FIS}dir";
            ok(mkdir($d), "mkdir $d succeeds");
            ok((-d $d), "directory wasn't created anyway");
    },
    add_tag_with_id_prefix_bad4 =>
    sub {
        # Adding a tag with an id prefix is disallowed
        my $d = "$testDirName/123${FIS}234${FIS}dir";
        ok(not(mkdir $d), "$d mkdir errored");
        ok(not(-d $d), "directory wasn't created anyway");
    },
    add_tag_with_id_prefix_good0 =>
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = $testDirName . "/not_a_number${FIS}dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    add_tag_with_id_prefix_good1 =>
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = $testDirName . "/1b${FIS}dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    add_tag_with_id_prefix_good2 =>
    sub {
        # Adding a tag with an non numerical prefix is allowed
        my $d = "$testDirName/b1${FIS}dir";
        ok(mkdir($d), "$d mkdir succeeded");
    },
    rename_tag_with_id_prefix_bad0 =>
    sub {
        my $d = "$testDirName/dir";
        my $e = "$testDirName/123${FIS}dir";
        mkdir($d);
        ok(rename($d, $e), "rename failed");
    },
    add_file_with_id_prefix_bad0 =>
    sub {
        # Adding a file with a numerical prefix is not allowed
        my $f = "$testDirName/2009${FIS}file";
        ok((not new_file($f)), "file create failed");
    },
    add_file_with_id_prefix_bad1 =>
    sub {
        # Adding a file with a numerical prefix is not allowed
        my $f = "$testDirName/12${FIS}2009${FIS}file";
        ok((not new_file($f)), "file create failed");
    },
    add_file_with_id_prefix_bad2 =>
    sub {
        # Adding a file with a numerical prefix is not allowed
        my $f = "$testDirName/1200${FIS}2009${FIS}file";
        ok((not new_file($f)), "file create failed");
    },
    add_file_with_id_prefix_good0 =>
    sub {
        # Adding a file with a non-decimal prefix is allowed
        my $f = "$testDirName/0x23${FIS}file";
        ok(new_file($f), "file create succeeded");
    },
    add_file_with_id_prefix_good1 =>
    sub {
        # Adding a file with a non-numerical prefix is allowed
        my $f = "$testDirName/not_a_number${FIS}23${FIS}file";
        ok(new_file($f), "file create succeeded");
    },
    add_file_with_id_prefix_good2 =>
    sub {
        # Adding a file with a non-numerical prefix is allowed
        my $f = "$testDirName/not_a_number${FIS}file";
        ok(new_file($f), "file create succeeded");
    },
    remove_staged_directory_with_contents =>
    sub {
        # Removing a directory that still has stage contents is allowed
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/a/b/c";
        mkpth($d);
        ok((rmdir $e), "mkdir errored");
        ok(not (-d $d), "contents remain");
    },
    untagged_files_list_at_the_root =>
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
    14 =>
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
    15 =>
    sub {
        # Had this problem where any file in the root directory would cause
        # getattr to return true for any file in the root
        my $f = "$testDirName/a";
        my $d = "$testDirName/adir";

        new_file($f);
        ok(mkdir ($d), "mkdir succeeded");

    },
    16 =>
    sub {
        # when we delete an untagged file, it shouldn't show anymore
        my $f = "$testDirName/a";
        new_file($f);
        ok(unlink ($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
    },
    17 =>
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
    18 =>
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
    associated_tags_disappear_when_files_are_removed =>
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
    20 =>
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
        #XXX: This redirects to /dev/null because there are several expected 
        #errors from directories that get deleted early
        `rm -r $testDirName/* 2>/dev/null`;

        foreach my $x (@dirs)
        {
            ok(not (-d $x), "$x is gone.");
        }
    },
    21 =>
    sub {
        my $c = "$testDirName/a";
        my $d = "$testDirName/b";
        my $cd = "$testDirName/a/b";
        mkdir $c; 
        mkdir $d;
        rename($d, $cd);
        ok((-d $cd), "Directory appears at rename location");
    },
    22 =>
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
    23 =>
    sub {
        # A 'staged' directory should disappear if one of its 
        # components is deleted
        my $d = "$testDirName/a/b";
        my $e = "$testDirName/b";
        my $f = "$testDirName/a";
        mkpth($d);
        rmdir $e;
        my @cont = dir_contents($f);
        ok(not (-d $d), "$d doesn't exist");
        ok((scalar(@cont) == 0), "$f is empty");
    },
    24 =>
    sub {
        SKIP: {
            # Another fun test
            plan skip_all => "This test fails for some bizzarre reason that I'm writing off as a timing error. It works under normal conditions.";
            my $d = "$testDirName/a/b/c/d";
            my $e = "$testDirName/c";
            mkpth($d);
            rmdir $e;
            mkpth($d);
            ok((-d $d), "$d exists");
        }
    },
    25 =>
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
    26 =>
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
    27 =>
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
    28 =>
    sub {
        # Ensure that we can set times for a file
        my $f = "$testDirName/f";
        my $time = 23;
        new_file($f);
        utime $time, $time, $f;
        my $stat = stat($f);
        is($time, $stat->atime, "atime is set");
        is($time, $stat->mtime, "mtime is set");
    },
    29 =>
    sub {
        # Check that a program like sqlite3 can successfully
        # create a database
        my $f = "$testDirName/sqlite.db";
        my $status = system("sqlite3 $f \"create table turble(a,b,c);\"");
        is($status, 0, "table create succeeds");
    },
    30 =>
    sub {
        # Check that we can open a new file for reading
        # sqlite3 does this
        my $f = "$testDirName/f";
        ok(open(my $fh, ">", $f), "file is opened in write mode");
        close($fh);
    },
    31 =>
    sub {
        # Ensure that we can create a tag within a domain that doesn't
        # exist yet
        my $d = "$testDirName/samurai${TPS}soujiro";
        my $d_parent = "$testDirName/samurai";
        mkdir $d;
        ok((-d $d), "$d was created");
        ok((-d $d_parent), "$d_parent was created");
    },
    parent_tag_doesnt_include_child_tag0 =>
    sub {
        # Ensure that the sub-tag lists under the super
        my $d = "$testDirName/alpha${TPS}beta";
        my $d_parent = "$testDirName/alpha";
        mkdir $d_parent;
        mkdir $d;
        ok((not dir_contains($d_parent, "alpha${TPS}beta")), "$d appears under $d_parent listing");
    },
    parent_tag_doesnt_include_child_tag1 =>
    sub {
        # Ensure that the sub-tag lists under the super when
        # both are created in one call
        my $d = "$testDirName/alpha${TPS}beta";
        my $d_parent = "$testDirName/alpha";
        mkdir $d;
        my @content = dir_contents($d_parent);
        ok((not dir_contains($d_parent, "alpha${TPS}beta")), "$d appears under $d_parent listing");
    },
    parent_tag_doesnt_include_childs_files =>
    sub {
        my $d = "$testDirName/alpha${TPS}beta";
        my $d_parent = "$testDirName/alpha";
        my $f = "$testDirName/alpha${TPS}beta/f";
        my $f_in_parent = "$testDirName/alpha/f";
        mkdir $d_parent;
        mkdir $d;
        new_file($f);
        ok((not dir_contains($d_parent, "f")), "f appears under $d_parent listing");
        ok((not -f $f_in_parent), "$f_in_parent exists");
    },
    subtag_files_exist =>
    sub {
        my $d = "$testDirName/alpha${TPS}beta";
        my $f = "$testDirName/alpha${TPS}beta/f";
        mkdir $d;
        new_file($f);
        ok(dir_contains($d, "f"), "The file is listed");
        ok((-f $f), "The file exists");
    },
    delete_tag_with_conflicting_child_name =>
    sub {
        my $d = "$testDirName/alpha${TPS}beta";
        my $d_parent = "$testDirName/alpha";
        my $e = "$testDirName/beta";
        mkdir $d;
        mkdir $e;
        ok((not(rmdir $d_parent)), "Deleting the parent fails ($!)");
    },
    mkdir_with_invalid_name =>
    sub {
        my $d = "$testDirName/alpha${TPS}";
        ok(!(mkdir $d), "mkdir with $d failed");
    },
    change_parent_tag =>
    sub {
        my $d = "$testDirName/alpha${TPS}beta";
        my $e = "$testDirName/gamma${TPS}beta";
        mkdir $d;
        ok((rename $d, $e), "rename succeeds");
        ok(dir_contains($testDirName, "gamma${TPS}beta"), "The new directory is listed");
        ok((-d $e), "The new directory exists");
    },
    external_symlink =>
    sub {
        # Add a symlink from outside the TagFS
        my $fd = make_tempdir("link-source");
        my $f = "$fd/f";
        ok(open(my $fh, ">", $f), "original file at $f opens") or fail("Couldn't open the original file");
        print $fh "HELLO";
        close($fh);
        my $l = "$testDirName/f";

        ok(eval { symlink($f, $l); 1 }, "symlink succeeds") or fail("Couldn't set up the symlink");
        is(readlink($l), $f, "readlink succeeds");
        ok(open(my $lh, "<", $l), "link file opens") or fail("Couldn't open the link file");
        is(read($lh, my $chars_read, 5), 5, "read from link file succeeds");
        is($chars_read, "HELLO", "correct characters are read");
    },
    internal_symlink =>
    sub {
        # Add a symlink from within the TagFS
        my $f = "$testDirName/f";
        ok(open(my $fh, ">", $f), "original file at $f opens") or fail("Couldn't open the original file");
        print $fh "HELLO";
        close($fh);
        my $l = "$testDirName/lf";

        ok(eval { symlink($f, $l); 1 }, "symlink succeeds") or fail("Couldn't set up the symlink");
        is(readlink($l), $f, "readlink succeeds");
        ok(open(my $lh, "<", $l), "link file opens") or fail("Couldn't open the link file");
        is(read($lh, my $chars_read, 5), 5, "read from link file succeeds");
        is($chars_read, "HELLO", "correct characters are read");
    },
    rename_root_tag =>
    sub {
        # added for a regression that came from adding subtags
        my $d = "$testDirName/alpha";
        my $e = "$testDirName/beta";
        mkdir $d;
        ok((rename $d, $e), "rename succeeds");
        ok(dir_contains($testDirName, "beta"), "The new directory is listed");
        ok((-d $e), "The new directory exists");
    },
    rename_to_subtag_and_make_file =>
    sub {
        # Back trace from the segfault:
        #
        # 0x00000000004065ff in add_tag_to_file (db=0x611080, f=f@entry=0xffffffffe8000990, tag_id=tag_id@entry=2, v=v@entry=0x0) at tagdb.c:540
        # 540    if (t == NULL || !retrieve_file(db, file_id(f)))
        # (gdb) bt
        # #0  0x00000000004065ff in add_tag_to_file (db=0x611080, f=f@entry=0xffffffffe8000990, tag_id=tag_id@entry=2, v=v@entry=0x0) at tagdb.c:540
        # #1  0x0000000000409309 in make_a_file_and_return_its_real_path (path=path@entry=0x7fffe8002800 "/a/f", result=result@entry=0x7ffff63acb18) at tagdb_fs.c:450
        # #2  0x0000000000409392 in tagdb_fs_create (path=0x7fffe8002800 "/a/f", mode=33204, fi=0x7ffff63acd10) at tagdb_fs.c:369
        # #3  0x00000000004033eb in tagfs_create (a0=0x7fffe8002800 "/a/f", a1=33204, a2=0x7ffff63acd10) at tagfs.c:79
        # #4  0x00007ffff78a47db in fuse_fs_create () from /lib/x86_64-linux-gnu/libfuse.so.2
        # #5  0x00007ffff78a4910 in ?? () from /lib/x86_64-linux-gnu/libfuse.so.2
        # #6  0x00007ffff78aab5d in ?? () from /lib/x86_64-linux-gnu/libfuse.so.2
        # #7  0x00007ffff78ac25b in ?? () from /lib/x86_64-linux-gnu/libfuse.so.2
        # #8  0x00007ffff78a8e79 in ?? () from /lib/x86_64-linux-gnu/libfuse.so.2
        # #9  0x00007ffff7681182 in start_thread (arg=0x7ffff63ad700) at pthread_create.c:312
        # #10 0x00007ffff70ebfbd in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:111
        #
        # The fault address (argument `f' to add_tag_to_file) is different every time
        #
        # The issue was caused by the addition of tagdb_make_file without a header entry. The return value, a heap address, was cast to int, overflowed and went
        # negative, hence the wacky address.
        
        # To reproduce:
        # mkdir mount/a
        # mv mount/a mount/a::b
        # ls mount
        # touch mount/a/f
        
        my $d = "$testDirName/a";
        my $e = "$testDirName/b${TPS}a";
        my $f = "$testDirName/b/f";
        mkdir $d;
        ok((rename $d, $e), "rename succeeds");
        ok(new_file($f), "file creation succeeds");
    },
    make_subtag_directory_at_non_root_position =>
    sub {
        my $d = "$testDirName/a";
        my $z = "b${TPS}z";
        my $e = "$testDirName/a/$z";
        mkdir $d;
        ok((mkdir $e), "mkdir succeeds");
        ok(dir_contains($d, $z), "new directory is listed");
    },
    remove_subtag_root =>
    sub {
        my $y = "b";
        my $z = "b${TPS}z";
        my $x = "z";
        my $d = "$testDirName/$z";
        my $e = "$testDirName/$y";
        my $c = "$testDirName/$x";

        mkdir $d;
        ok((rmdir $e), "rmdir $y succeeds");
        ok((not dir_contains($testDirName, $z)), "can't find the directory $z");
        ok(dir_contains($testDirName, $x), "directory $x is listed");
        ok((-d $c), "directory $x exists");
    },
    remove_subtag_leaf =>
    sub {
        my $y = "b";
        my $z = "b${TPS}z";
        my $d = "$testDirName/$z";
        my $e = "$testDirName/$y";

        mkdir $d;
        ok((rmdir $d), "rmdir $z succeeds");
        ok((not dir_contains($testDirName, $z)), "can't find the directory $z");
        ok(dir_contains($testDirName, $y), "directory $y is listed");
        ok((-d $e), "directory $y exists");
    },
    remove_subtag_internal =>
    sub {
        my $z = "a${TPS}b${TPS}c";
        my $y = "a${TPS}b";
        my $x = "a${TPS}c";
        my $d = "$testDirName/$z";
        my $e = "$testDirName/$y";
        my $f = "$testDirName/$x";

        mkdir $d;

        ok((rmdir $e), "rmdir $y succeeds");
        ok((not dir_contains($testDirName, $z)), "can't find the directory $z");
        ok((not (-d $d)), "$z does not exist");
        ok((-d $f), "$x exists");
    },
    remove_subtag_with_name_matching_root_tag =>
    sub {
        my $z = "a${TPS}b${TPS}c";
        my $y = "c";
        my $d = "$testDirName/$z";
        my $e = "$testDirName/$y";

        mkdir $d;
        mkdir $e;

        ok((rmdir $d), "rmdir $d succeeds");
        ok((-d $e), "$e exists");
        ok((not (-d $d)), "$d does not exist");
    },
    rename_staged_entry =>
    sub {
        my $d = "$testDirName/a";
        my $e = "$testDirName/a/b";
        my $f = "$testDirName/a/c";
        mkdir $d;
        ok((mkdir $e), "mkdir succeeds");
        ok((rename $e, $f), "rename succeeds");
        ok(dir_contains($d, "c"), "new directory is listed");
    },
    rename_staged_entry_from_root =>
    sub {
        my $d = "$testDirName/a";
        my $e = "$testDirName/a/b";
        my $f = "$testDirName/b";
        my $g = "$testDirName/c";
        mkdir $d;
        ok((mkdir $e), "mkdir succeeds");
        ok((rename $f, $g), "rename succeeds");
        ok(dir_contains($d, "c"), "new directory is listed");
    },
    symlink_directory =>
    sub {
        my $d = "$testDirName/a";
        my $e = "$testDirName/a/b";
        mkdir $d;
        ok((eval { symlink($d, $e); 1 }), "Symlink succeeds");
        ok((-d $e), "Directory link exists");
    },
    file_inode_preservation =>
    sub {
        # Ensure that a file's inode is the same when listed at 
        # different locations
        my $e = "$testDirName/a/b";
        my $f = "$testDirName/a/b/f";
        my $g = "$testDirName/a/f";
        mkpth($e);
        new_file($f);
        my $f_stat = stat($f);
        my $g_stat = stat($g);
        is($f_stat->ino, $g_stat->ino, "Inode numbers are the same");
    },
    creat_on_existing_fails =>
    sub {
        my $f = "$testDirName/f";
        my $fh;
        my $stat1 = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT, 0644);
        close($fh);

        is($stat1, 1, "First creat succeeds (status $stat1)");
        my $stat2 = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT, 0644);
        isnt($stat2, 1, "Second creat fails");
    },
    file_inode_matches_File_id =>
    sub {
        # Ensure that a file's inode matches the file id shown when there
        # are duplicate files
        my $d = "$testDirName/b";
        my $e = "$testDirName/a/b";
        my $f = "$testDirName/b/f";
        my $g = "$testDirName/a/b/f";
        mkpth($e);
        new_file($f);
        new_file($g);
        my @l = dir_contents($d);
        foreach my $dirent (@l)
        {
            my $fname = $dirent;
            $dirent =~ s/^(\d*)${FIS}?.*$/$1/g;
            if (length($dirent) != 0)
            {
                my $id = int($dirent);
                my $status = stat("$d/$fname");
                my $ino = $status->ino;
                is($ino, $id, "Inode ($ino) matches id ($id)");
            }
        }
    },
    rename_file_to_id_prefix_bad0 =>
    sub {
        # Renaming to a name with an id prefix is disallowed
        my $f = "$testDirName/file";
        new_file($f);
        my $stat = stat($f);
        my $ino = $stat->ino;
        my $newid = $ino + 1;
        my $g = "$testDirName/${newid}${FIS}file";
        ok((not rename($f, $g)), "file rename failed");
    },
    rename_file_to_id_prefix_bad1 => # These id prefix tests use the inode=file_id status, so I put them after the relevant test
    sub {
        # Renaming to a name with an id prefix is disallowed
        # even when the id matches
        my $f = "$testDirName/file";
        new_file($f);
        my $stat = stat($f);
        my $ino = $stat->ino;

        my $g = "$testDirName/${ino}${FIS}gile";
        ok((not rename($f, $g)), "file rename failed");
    },
    rename_file_from_id_prefix_bad0 =>
    sub {
        # Renaming from a name with an id prefix is disallowed
        # when the new name has a different id
        my $f = "$testDirName/file";
        new_file($f);
        my $stat = stat($f);
        my $ino = $stat->ino;
        my $newid = $ino + 1;
        my $g = "$testDirName/${ino}${FIS}gile";
        my $h = "$testDirName/${newid}${FIS}hile";

        ok((not rename($g, $h)), "file rename failed");
    },
    rename_file_from_id_prefix_good0 =>
    sub {
        # Renaming from a name with an id prefix is disallowed
        # when the new name has a different id
        my $f = "$testDirName/file";
        new_file($f);
        my $stat = stat($f);
        my $ino = $stat->ino;
        my $newid = $ino + 1;
        my $g = "$testDirName/${ino}${FIS}gile";
        my $h = "$testDirName/${ino}${FIS}hile";
        my $hh = "$testDirName/hile";

        ok(rename($g, $h), "file rename succeeded");
        ok((-f $hh), "renamed file exists");
    },
);
my %tests = @tests_list;

sub explore
{
    # Explore a test by getting a look before it is cleaned up
    my $test = shift;
    &setupTestDir;
    print "Tagfs data directory at ${dataDirName}\n";
    eval{
        &$test;
    };
    system("cd $testDirName && $ENV{'SHELL'}");
    &cleanupTestDir;
}

sub run_test
{
    my ($test_name, $test) = @_;
    &setupTestDir;
    my $res = 0;
    eval{
        $res = subtest $test_name => \&$test;
    };
    &cleanupTestDir;
    return $res;
}

sub run_named_tests
{
    foreach my $test_name (@_)
    {
        if (run_test(colored($test_name, "red"),  $tests{$test_name}))
        {
            print ".";
        }
        else
        {
            print "F";
        }
    }
    print "\n";
    seek $failures_fh, 0, Fcntl::SEEK_SET;
    print <$failures_fh>;
    print "\n";
}

sub natatime ($@)
{
    my $n = shift;
    my @list = @_;

    return sub
    {
        return splice @list, 0, $n;
    }
}
if (scalar(@ARGV) > 0)
{
    my $should_explore = 0;
    my $t = undef;
    for my $f (@ARGV)
    {
        if ($f eq "e")
        {
            $should_explore = 1;
        }
        elsif ($f eq "--show-logs")
        {
            $SHOW_LOGS = 1;
        }
        else
        {
            $t = $f;
        }
    }

    if ( grep($t, keys(%tests)) )
    {
        my $test = $tests{$t};
        if ($should_explore)
        {
            explore($test);
        }
        else
        {
            run_test($t, $test);
        }
    }
}
elsif (scalar(@TESTRANGE) > 0)
{
    no warnings "numeric";
    my @ttr = ();

    my $it = natatime 2, @tests_list;
    my $started = 0;

    if (!scalar($TESTRANGE[0]))
    {
        $started = 1;
    }

    while (my @t = $it->())
    {
        if ($t[0] eq $TESTRANGE[0])
        {
            $started = 1;
        }

        if ($started)
        {
            push @ttr, $t[0];
        }

        if ($t[0] eq $TESTRANGE[1])
        {
            last;
        }
    }
    run_named_tests(@ttr);
}
elsif (scalar(@TESTS) > 0)
{
    run_named_tests(@TESTS);
}
else
{
    no warnings "numeric";
    my @ttr = ();

    my $it = natatime 2, @tests_list;

    while (my @t = $it->())
    {
        push @ttr, $t[0];
    }
    run_named_tests(@ttr);
}

done_testing();
