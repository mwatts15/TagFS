#!/usr/bin/env perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

# XXX: Write new tests assuming the current directory is $testDirName

use warnings "all";
use strict;
use File::Path qw(make_path);
use Cwd 'abs_path';
use Cwd;
use Test::More;
use Term::ANSIColor;
use Fcntl;
use File::stat;
use Util qw(natatime random_string);
use POSIX ':sys_wait_h';
use Time::HiRes qw(sleep);

my $failures_fname = `mktemp /tmp/acctest-valgrind.out.XXXXXXXXXX`;
open my $failures_fh, "+>", $failures_fname;
Test::More->builder->output("/dev/null"); # Hide non-failures
Test::More->builder->failure_output($failures_fh);
Test::More->builder->todo_output($failures_fh);

my $testDirName;
my $dataDirName;
# XXX: Read the "tagfs.pid" file for the PID of tagfs in tests
my $TAGFS_PID = -1;
my $VALGRIND_OUTPUT = "";
my $TAGFS_LOG = "";
my $FUSE_LOG = "";
my $SHOW_LOGS = 0;
my @TESTS = ();
my @TESTRANGE = ();
my $TEST_PATTERN = undef;
my $STARTING_DIRECTORY = getcwd;
my $TPS = "::"; # tag path separator. This must match the TAG_PATH_SEPARATOR in ../tag.h
my $FIS = "#"; # file id separator. This must match the FILE_ID_SEPARATOR in ../abstract_file.h
my $XATTR_PREFIX = "user.tagfs."; # The prefix in xattr tag listings
my $MAX_FILE_NAME_LENGTH = 255; # The maximum length of a file name created or returned by readdir. NOTE: this is one less than the internal constant of 256
my $ID_PREFIX_PATTERN = "^\\d+${FIS}.+\$";

if (defined($ENV{TESTS}))
{
    if ($ENV{TESTS} =~ /\*/)
    {
        $TEST_PATTERN = $ENV{TESTS};
    }
    elsif ($ENV{TESTS} =~ /^(.*)\.\.(.*)$/)
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

    my $tagfs_args = "-o use_ino,attr_timeout=0 --drop-db --data-dir=$dataDirName -g 0 -l $TAGFS_LOG -d $testDirName 2> $FUSE_LOG";
    my $cmd = "../tagfs";

    my $child_pid = fork();
    if (defined $child_pid)
    {
        if ($child_pid == 0)
        {
            if (defined($ENV{NO_VALGRIND}))
            {
                $cmd = "G_SLICE=always-malloc $cmd $tagfs_args";
            }
            else
            {
                $cmd = "G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --track-origins=yes --log-file=$VALGRIND_OUTPUT --suppressions=valgrind-suppressions --leak-check=full $cmd $tagfs_args";
            }
            exec($cmd) or die "Couldn't exec tagfs: $!\n";
        }
        else
        {
            # Don't wait for the child here
            $TAGFS_PID = $child_pid;
        }
    }
    else
    {
        die "Couldn't fork a child process\n";
    }
    my $i = 0;
    my $max_mount_seconds = 1.0;
    my $max_mount_samples = 1000.0;
    while ($i < $max_mount_samples && system("mount | grep --silent 'tagfs on $testDirName'") != 0)
    {
        sleep ($max_mount_seconds / $max_mount_samples);
        $i += 1;
    }

    if ($i == $max_mount_samples)
    {
        print "It seems we couldn't mount. Cleaning up and exiting.\n";
        &cleanupTestDir();
        print "Cleaned up.\n";
        exit(1);
    }
    chdir($testDirName);
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
    my ($actual, $expected, $message) = @_;
    raise_test_caller_level();
    Test::More::is($actual, $expected, colored($message, "bright_red"));
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
    my $s = `mktemp -d /tmp/.acctest-tagfs-${tail}-XXXXXXXXXX`;
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

sub file_id
{
    my $file = shift;
    my $stat = stat($file);
    my $ino = $stat->ino;
    return $ino;
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

sub wait_for_file_closure
{
    my ($file_name, $wait_time, $samples) = @_;

    while (($samples > 0) && system("fuser $file_name > /dev/null 2>&1") == 0)
    { 
        sleep ($wait_time / $samples);
        $samples--;
    }
    $samples
}

sub mkpth
{
    # XXX: Augments make_path to retry
    my $arg = shift;
    my $limit = 10;
    my $i = 0;
    my $err;
    make_path($arg, {error => \$err});
    while (scalar(@$err))
    {
        if ($i == $limit)
        {
            return 0;
        }
        sleep .01;
        make_path($arg, {error => \$err});
        $i++;
    }
    return 1;
}

sub cleanupTestDir
{
    chdir($STARTING_DIRECTORY);
    eval {{
            my $max_unmount = 500; # ms
            while ((`fusermount -u $testDirName 2>&1` =~ /[Bb]usy/) && ($max_unmount > 0))
            {
                sleep .01;
                $max_unmount--;
            }
            ($max_unmount > 0) or die "It seems that we were unable to unmount TagFS";

            my $max_wait = 5000; # ms
            my $kid = 17;
            while (($max_wait > 0) && ($kid > 0))
            {
                sleep .001;
                $kid = waitpid(-1, WNOHANG);
                $max_wait--;
            }

            ($max_wait > 0) or die "It seems that we've been unable to reap our TagFS process.\n";

            my %logs = (
                $VALGRIND_OUTPUT => 0,
                $TAGFS_LOG => 0,
                $FUSE_LOG => 0,
            );

            if (not defined($ENV{NO_VALGRIND}))
            {
                &wait_for_file_closure($VALGRIND_OUTPUT, 1, 10000) or warn "$FUSE_LOG still open";
                if (system("grep --silent -e \"ERROR SUMMARY: 0 errors\" $VALGRIND_OUTPUT") != 0)
                {
                    $logs{$VALGRIND_OUTPUT} = 1;
                }
            }

            if (not defined($ENV{TAGFS_NOTESTLOG}))
            {
                &wait_for_file_closure($TAGFS_LOG, 1, 10000) or warn "$TAGFS_LOG still open";
                if (system("grep -E --silent -e 'ERROR|WARN' $TAGFS_LOG") == 0)
                {
                    $logs{$TAGFS_LOG} = 1;
                }
            }

            &wait_for_file_closure($FUSE_LOG, 1, 10000) or warn "$FUSE_LOG still open";

            if (system("grep --silent -e \"fuse_main returned 0\" $FUSE_LOG") != 0)
            {
                $logs{$FUSE_LOG} = 1;
            }

            if ($SHOW_LOGS)
            {
                print("SHOWING LOGS\n");
                foreach my $k (keys %logs)
                {
                    $logs{$k} = 1;
                }
            }

            foreach my $log (keys %logs)
            {
                if ($logs{$log})
                {
                    cat_to_stderr($log);
                }
            }
        }};
    `rm -rf $testDirName`;
    `rm -rf $dataDirName`;
    unlink($TAGFS_LOG);
    unlink($VALGRIND_OUTPUT);
    unlink($FUSE_LOG);
}

sub resolve_key_kind
{
    my ($kind_or_key, $key_or_undef) = @_;
    my ($kind, $key);
    if (not (defined $key_or_undef))
    {
        $kind = undef;
        $key = $kind_or_key;
    }
    else
    {
        $kind = $kind_or_key;
        $key = $key_or_undef
    }
    ($kind, $key);
}

# If kind is undef, then the default kind is used
sub cmd_name
{
    my ($kind_or_key, $key_or_undef) = @_;
    my $fname;
    my ($kind, $key) = &resolve_key_kind($kind_or_key, $key_or_undef);
    if (defined $kind)
    {
        $fname = ".__cmd:$kind:$key";
    }
    else
    {
        $fname = ".__cmd:$key";
    }
    $fname;
}

# get a cmd_name, generating a new key if none is given 
sub generate_cmd_name
{

    my ($kind, $key) = @_;
    if (not defined $key)
    {
        $key = &random_string(15);
    }
    my $fname = &cmd_name($kind, $key);
    ($fname, $key);
}

# Open a file to a write a command 
sub opencmd
{
    my ($kind, $key) = @_;
    my ($fname, $rkey) = &generate_cmd_name($kind, $key);
    open my $cmdfile, ">", $fname;
    ($cmdfile, $fname, $rkey);
}

# Get the name for a command response
sub resp_name
{
    my ($kind_or_key, $key_or_undef) = @_;
    my $res_fname;
    my ($kind, $key) = &resolve_key_kind($kind_or_key, $key_or_undef);
    if (not defined $kind)
    {
        $res_fname = ".__res:${key}"
    }
    else
    {
        $res_fname = ".__res:${kind}:${key}"
    }
    $res_fname;
}

sub err_name
{
    my ($kind_or_key, $key_or_undef) = @_;
    my $res_fname;
    my ($kind, $key) = &resolve_key_kind($kind_or_key, $key_or_undef);
    if (not defined $kind)
    {
        $res_fname = ".__err:${key}"
    }
    else
    {
        $res_fname = ".__err:${kind}:${key}"
    }
    $res_fname;
}

# Commit a command given the file where the command's written 
# or the kind and key for the command
sub commit_cmd
{
    my ($kind_or_filename, $key) = @_;
    my $key_provided = defined $key;
    my $filename;
    if ($key_provided)
    {
        my $kind = $kind_or_filename;
        $filename = &cmd_name($kind, $key);
    }
    else
    {
        $filename = $kind_or_filename;
    }

    if (not -f $filename)
    {
        diag("There is no such command file $filename");
        return 1;
    }
    else
    {
        while (not (unlink $filename))
        {
            sleep .001;
        }
        return 0;
    }
}

# Send a command to tagfs and get the file name for the response
sub tagfs_cmd
{
    my $cmd = shift;
    my ($kind, $key) = @_;
    my ($fh, $rkey);
    eval {
        ($fh, undef, $rkey) = &opencmd($kind, $key);
    };

    if ($@) {
        fail("Failure in opening the command: " . $@);
    }

    print $fh $cmd;
    close $fh;
    eval {
        &commit_cmd($kind, $rkey);
    };

    if ($@)
    {
        fail("Failure in commiting the command: " . $@);
    }

    (&resp_name($kind, $rkey), $rkey);
}

# Send a command to tagfs
# 
# Returns either the normal output or error output
sub tagfs_cmd_complete
{
    my ($res_name, $key) = &tagfs_cmd(@_);
    my $err_name = &err_name(undef, $key);
    my $lim = 100;
    my $success = 0;
    my $res = undef;

    my $i = 0;
    while ($i < $lim)
    {
        if (-f $res_name)
        {
            $success = 1;
            $res = &read_file($res_name);
            unlink $res_name;
            last;
        }
        else
        {
            sleep .001;
        }
        $i++;
    }

    if (not $success)
    {
        $i = 0;
        while ($i < $lim)
        {
            if (-f $err_name)
            {
                $res = &read_file($err_name);
                unlink $err_name;
                last;
            }
            $i++;
        }
    }
    $res;
}

sub read_file
{
    my ($file) = @_;
    local $/ = undef;
    open(my $fh, "<", $file) or (return "");
    binmode $fh;
    my $res = <$fh>;
    close $fh;
    return $res;
}

# Please describe the test throuugh the test name and in a comment within the
# body of the test function
#
# The current directory is the root of the TagFS mount for each test and is the
# same as $testDirName. New tests should avoid explicit use of the $testDirName
# variable.
my @tests_list = (
    read_and_write_regular_files =>
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
    staged_directories_only_create_one_path =>
    sub {
        # Create nested directories.
        # Should not be able to access by the prefix in any order
        # mkdir a/b/c
        # [ -d "a/b/c" ] &&
        # [ ! -d "b/a/c" ] &&
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
            ok(not(-d "$testDirName/b/c/c"), "a/b/c exists");
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
    basic_create_files_and_tags =>
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
    rename_file_basic_succeeds =>
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
    create_file_in_nonexisting_directory =>
    sub {
        # Creating a file at a non-existant directory
        # See valgrind output
        my $dir = $testDirName . "/IDontExist";
        my $file = "$dir/file";
        ok(not(new_file($file)), "file not created at non-existant directory");
    },
    simple_directory_delete =>
    sub {
        # Adding a few tags and then deleting one
        my @dirs1 = qw/dir1 dir2 dir3 dir5 dir23/;
        my @dirs2 = qw/dir1 dir2 dir3 dir5/;
        for (@dirs1)
        {
            mkdir $testDirName . "/" . $_;
        }
        my $removed = $testDirName . "/dir23";
        rmdir $removed;
        ok(not(-d $removed), "removed directory doesn't exist");
        for (@dirs2)
        {
            ok((-d "$testDirName/$_"), "not removed($_) still exists");
        }
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
    remove_file_from_staged_directory_retains_stage =>
    sub {
        # Removing a directory that still has stage contents is allowed
        my $e = "$testDirName/a/b/c";
        my $f = "$testDirName/a/b/c/f";
        mkpth($e);
        new_file($f);
        unlink($f);
        ok((-d $e), "directory remains");
    },
    remove_staged_directory_with_file_contents_fails =>
    sub {
        TODO: {
            local $TODO = "May not be implemented";
            # Removing a directory that still has file contents is not allowed
            my $e = "$testDirName/a/b/c";
            my $f = "$testDirName/a/b/c/f";
            mkpth($e);
            new_file($f);
            ok((not (rmdir $e)), "mkdir errored");
            ok((-d $e), "directory remains");
            ok((-f $f), "contents remain");
        }
    },
    remove_staged_directory_with_directory_contents_succeeds =>
    sub {
        # Removing a directory that still has stage contents is allowed
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/a/b/c";
        mkpth($d);
        ok((rmdir $e), "mkdir did not error");
        ok(not (-d $d), "contents do not remain");
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
    associated_tags_appear_when_files_are_renamed =>
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
    delete_file_at_root_succeeds =>
    sub {
        # when we delete an untagged file, it shouldn't show anymore
        my $f = "$testDirName/a";
        new_file($f);
        ok(unlink ($f), "delete of untagged file succeeded");
        ok(not (-f $f), "and the file doesn't list anymore");
    },
    unlink_removes_file =>
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
    unlink_removes_file_in_all_directories =>
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
    rm_dash_rf_removes_all =>
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
    rename_to_existing_location_stages_directory =>
    sub {
        my $c = "$testDirName/a";
        my $d = "$testDirName/b";
        my $cd = "$testDirName/a/b";
        mkdir $c; 
        mkdir $d;
        rename($d, $cd);
        ok((-d $cd), "Directory appears at rename location");
    },
    rename_to_nonexisting_location_does_not_stage_directory =>
    sub {
        my $c = "$testDirName/a";
        my $d = "$testDirName/b";
        my $cd = "$testDirName/a/b";
        mkdir $c; 
        mkdir $d;
        rename($d, $cd);
        ok((-d $cd), "Directory appears at rename location");
    },
    reordered_staged_directory_path_does_not_exist =>
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
    invalidate_path_through_indirect_deletion =>
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
    recreate_path_partially_invalidated_by_indirect_deletion =>
    sub {
        # Test that when we delete a tag whose corresponding directory is 
        # part of the path to an empty directory and then recreate that path
        # is the path created successfully?
        plan skip_all => "This test fails for some bizzarre reason that I'm writing off as a timing error. It works under normal conditions.";
        my $d = "$testDirName/a/b/c/d";
        my $e = "$testDirName/c";
        mkpth($d);
        rmdir $e;
        mkpth($d);
        ok((-d $d), "$d exists");
    },
    move_file_from_root =>
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
    remove_tag_by_rename =>
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
    rename_twice_idempotency =>
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
    set_atime =>
    sub {
        # Ensure that we can set times for a file
        my $f = "$testDirName/f";
        my $time = 23;
        new_file($f);
        utime $time, $time, $f;
        my $stat = stat($f);
        is($time, $stat->atime, "atime is set");
    },
    set_mtime =>
    sub {
        # Ensure that we can set times for a file
        my $f = "$testDirName/f";
        my $time = 23;
        new_file($f);
        utime $time, $time, $f;
        my $stat = stat($f);
        is($time, $stat->mtime, "mtime is set");
    },
    create_sqlite_table =>
    sub {
        # Check that a program like sqlite3 can successfully
        # create a database
        my $f = "$testDirName/sqlite.db";
        my $status = system("sqlite3 $f \"create table turble(a,b,c);\"");
        is($status, 0, "table create succeeds");
    },
    open_file_in_write_mode =>
    sub {
        # Check that we can open a new file for reading
        # sqlite3 does this
        my $f = "$testDirName/f";
        ok(open(my $fh, ">", $f), "file is opened in write mode");
        close($fh);
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
    creat_on_existing_doesnt_create_any_new_file =>
    sub {
        my $f = "$testDirName/f";
        my $fh;
        my $stat1 = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT, 0644);
        close($fh);
        is($stat1, 1, "First creat succeeds (status $stat1)");
        my $size1 = scalar(dir_contents("."));
        my $stat2 = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT, 0644);
        isnt($stat2, 1, "Second creat fails");
        my $size2 = scalar(dir_contents("."));
        is($size1, $size2, "No extra files were created");
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
    move_directories_beneath_each_other =>
    sub {
        # See issue #34
        my $f = "$testDirName/a";
        my $g = "$testDirName/b";
        my $fg = "$testDirName/a/b";
        my $gf = "$testDirName/b/a";
        mkdir($f);
        mkdir($g);
        ok(rename($f, $gf), "Rename $f to $gf succeeds");
        ok(rename($g, $fg), "Rename $g to $fg succeeds");
        ok((-d $fg), "$fg exists");
        ok((-d $gf), "$gf exists");
    },
    tag_file_name_conflict =>
    sub {
        mkpth("a/b/c");
        new_file("a/c");
        my $cid = file_id("a/c");
        new_file("a/f");
        rename("a/f", "a/b/c/f");
        sleep 1;
        ok((-d "a/c"), "The plain entry is a directory");
        ok(dir_contains("a", "${cid}${FIS}c"), "The prefixed entry is listed");
    },
    mkdir_overlong_name =>
    sub {
        # XXX: This test doesn't actually work
        my $str = "a"x($MAX_FILE_NAME_LENGTH + 1);
        ok((not(mkdir $str)), "Creating with overlong directory name fails");
        ok((not dir_contains(".", $str)), "The directory is not listed");
    },
    rename_to_overlong_name =>
    sub {
        # XXX: This test doesn't actually work
        my $dest = "b"x($MAX_FILE_NAME_LENGTH + 1);
        my $src = "a";
        mkdir $src;
        ok((not(rename $src, $dest)), "Renaming to overlong name fails");
        ok((not dir_contains(".", $dest)), "New name is not listed");
        ok((dir_contains(".", $src)), "Old name remains");
    },
    check_file_descs_are_closed =>
    sub {
        open(my $pidfile, "<", $dataDirName . "/tagfs.pid");
        # 21 covers log_10(2**64) + 1
        my $readcount = read($pidfile, my $real_pid, 21);
        if ((not defined $readcount) || ($readcount == 0))
        {
            fail("error reading from the pid file");
        }
        my $fd_dir = "/proc/$real_pid/fd";
        my @fds_init = dir_contents($fd_dir);
        open(my $fh, ">", "blah") or fail("Couldn't open the file");
        my @fds_opened = dir_contents($fd_dir);
        close($fh) or fail("Couldn't close the file");
        sleep 1.0; # The fd list can take a few milliseconds to update
        my @fds_fin = dir_contents($fd_dir);
        ok(scalar(@fds_init) < scalar(@fds_opened), "TagFS open fds count increases after an open");
        is(scalar(@fds_fin), scalar(@fds_init), "TagFS open fds count is the same as init at end");
    },
);

my @subtag_tests = (
    make_tag_with_new_namespace =>
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
        my $d = "$testDirName/alpha${TPS}beta";
        my $d_parent = "$testDirName/alpha";
        mkdir $d_parent;
        mkdir $d;
        ok((not dir_contains($d_parent, "alpha${TPS}beta")), "$d appears under $d_parent listing");
    },
    parent_tag_doesnt_include_child_tag1 =>
    sub {
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
    rename_to_existing_overwrites =>
    sub {
        my $d = "a/b/c";
        my $c = "a/b";
        my $f = "a/b/c/f";
        my $h = "a/b/h";
        make_path($d);
        new_file($f);
        new_file($h);
        rename $h, $f;
        foreach my $ent (dir_contents($c))
        {
            if ($ent =~ /$ID_PREFIX_PATTERN/)
            {
                fail("prefixed file listed");
            }
        }
        pass("no prefixed files list");
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
    mkdir_overlong_name_with_subtags =>
    sub {
        # XXX: This test doesn't actually work
        my $str = "a${TPS}"x(($MAX_FILE_NAME_LENGTH / 3) - 2);
        ok((not(mkdir $str)), "Creating with overlong directory name fails");
        ok((not dir_contains(".", $str)), "The directory is not listed");
    },
    rename_to_overlong_name_with_subtags =>
    sub {
        my $src = "a";
        my $dest = "a"x($MAX_FILE_NAME_LENGTH - 2);
        my $srcsub = "a${TPS}b";
        my $badsub = "${dest}${TPS}b";
        mkdir $srcsub;
        ok((not(rename $src, $dest)), "Renaming to overlong name fails");
        ok((not dir_contains(".", $dest)), "New name is not listed");
        ok((dir_contains(".", $src)), "Old name remains");
        ok((dir_contains(".", $srcsub)), "Old subtag name is visible");
        ok((not (-d $badsub)), "Renamed subtag does not exist");
        ok((not dir_contains(".", $badsub)), "Renamed subtag is not visible");
    },
);

my @command_tests = (
    command_fs_write_read_1 =>
    sub {
        my $fname = &cmd_name(undef, "key");
        open my $cmdfile, ">", $fname;
        my $str = "blah blah";
        print $cmdfile $str; 
        close $cmdfile;

        open $cmdfile, "<", $fname;
        my $read_str;
        read $cmdfile, $read_str, length($str); 
        is($str, $read_str, "The string written to the start of the command file is read back in");
        close $cmdfile;
    },
    command_fs_write_read_offset =>
    sub {
        my $fname = &cmd_name(undef, "key");
        my $offset = 100;
        my $str = "blah blah";
        my $read_str;

        open my $cmdfile, ">", $fname;
        seek $cmdfile, $offset, Fcntl::SEEK_SET;
        print $cmdfile $str; 
        close $cmdfile;

        open $cmdfile, "<", $fname;
        seek $cmdfile, $offset, Fcntl::SEEK_SET;
        read $cmdfile, $read_str, length($str); 

        is($str, $read_str, "The string written to the start of the command file is read back in");
        close $cmdfile;
    },
    rename_from_tagdb_to_command_sub_filesystem_is_prohibited =>
    sub {
        my ($n, undef) = &generate_cmd_name;
        my $f = "floopyfloop";
        ok((not(rename $f, $n)), "rename fails");
    },
    rename_from_command_to_tagdb_sub_filesystem_is_prohibited =>
    sub {
        my ($n, undef) = &generate_cmd_name;
        my $f = "floopyfloop";
        ok((not(rename $n, $f)), "rename fails");
    },
    create_of_response_file_fails =>
    sub {
        my $fh;
        my $f = &resp_name(undef, "key");
        my $res = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT, 0644);
        isnt($res, 1, "creat fails");
    },
    unlink_command_file_while_opened_still_commits =>
    sub {
        my $fh;
        my $f = &cmd_name(undef, "key");
        my $res = sysopen($fh, $f, O_TRUNC|O_EXCL|O_CREAT|O_WRONLY, 0644);
        is($res, 1, "creat succeeds");
        unlink $f;
        print $fh "not a command"; 
        close $fh;
        my $tries = 0;
        my $max_tries = 20;
        my $success = 0;
        my $res_name = &resp_name(undef, "key");
        my $err_name = &err_name(undef, "key");
        while ($tries < $max_tries)
        {
            $success = (-f $res_name) || (-f $err_name);
            if ($success)
            {
                last;
            }
            sleep 0.05;
            $tries++;
        }
        ok($success, "Some response is created");
    },
    make_overlong_command_name_fails =>
    sub {
        my $f = &cmd_name(undef, 'a' x ($MAX_FILE_NAME_LENGTH - length(".__cmd:") - 1));
        my $res = open my $cmdfile, ">", $f;
        if ($res)
        {
            close $cmdfile;
            fail("create fails");
        }
        else
        {
            pass("create fails");
        }
    },
    listing_error =>
    sub {
        my $f = &cmd_name(undef, 'key');
    }
);

my @alias_tests = (
    orig_tag_has_alias_files =>
    sub {
        mkdir "tag";
        tagfs_cmd_complete("alias_tag tag alias");
        ok((-d "alias"), "a new tag directory is created");
        new_file("alias/f");
        ok((-f "tag/f"), "a file with the original tag appears in the aliased one");
    },
    alias_and_associated_tag_list_once =>
    sub {
        mkdir "tag";
        tagfs_cmd_complete("alias_tag tag alias");
        ok((-d "alias"), "a new tag directory is created");
        new_file("alias/f");
        my @d = dir_contents('.');
        my $c = 0;
        for my $x (@d)
        {
            if ($x eq 'alias')
            {
                $c++;
            }
        }
        is($c, 1, "Only one listing of the alias appears");
    },
    aliased_tag_has_orig_files =>
    {
        proc => sub {
            mkdir "tag";
            tagfs_cmd_complete("alias_tag tag alias");
            ok((-d "alias"), "a new tag directory is created");
            new_file("tag/f");
            ok((-f "alias/f"), "a file with the original tag appears in the aliased one");
        },
        tries => 3
    },
    aliased_tag_lists_with_staged_tag =>
    sub {
        mkpth "pth/tag";
        tagfs_cmd_complete("alias_tag tag alias");
        ok((-d "alias"), "a new tag directory is created");
        ok((-d "pth/alias"), "the alias appears at the stage location");
    },
    aliased_tag_lists_with_original_tag =>
    sub {
        mkdir "tag";
        mkdir "b";
        new_file("tag/f");
        rename("tag/f", "b/f");
        tagfs_cmd_complete("alias_tag tag alias");
        ok((-d "alias"), "a new tag directory is created");
        ok(((-d "b/tag") && (-d "b/alias")), "the aliased tag and its alias list together");
    },
    alias_on_subtag_works =>
    sub {
        my $d = "tag${TPS}a${TPS}c";
        mkdir $d;
        tagfs_cmd_complete("alias_tag $d alias");
        ok((-d "alias"), "a new tag directory is created");
    },
    alias_the_alias =>
    sub {
        my $d = "a";
        mkdir $d;
        tagfs_cmd_complete("alias_tag $d alias");
        tagfs_cmd_complete("alias_tag alias b");
        ok((-d "alias"), "alias tag directory is created");
        ok((-d "b"), "alias-alias tag directory is created");
    },
    alias_to_self =>
    sub {
        my $d = "a";
        mkdir $d;
        tagfs_cmd_complete("alias_tag $d $d");
        ok((-d $d), "The original tag still exists");
    },
    alias_to_subtag_name_fails =>
    sub {
        my $d = "a${TPS}b${TPS}alias";
        mkdir "tag";
        tagfs_cmd_complete("alias_tag tag $d");
        ok((not (-d $d)), "a new tag directory isn't created");
    },
    alias_unlink_orig =>
    sub {
        mkdir "tag";
        tagfs_cmd_complete("alias_tag tag alias");
        rmdir "tag";
        ok((not (-d "tag")), "the original tag name is gone");
        ok((-d "alias"), "the alias remains");
    },
    alias_unlink_alias =>
    sub {
        my $d = "alias";
        mkdir "tag";
        tagfs_cmd_complete("alias_tag tag $d");
        rmdir $d;
        ok((-d "tag"), "the original tag name remains");
        ok((not (-d $d)), "the alias is gone");
    },
);

push @tests_list, @command_tests;
push @tests_list, @alias_tests;
push @tests_list, @subtag_tests;

my %tests = @tests_list;

sub print_failures
{
    print "\n";
    seek $failures_fh, 0, Fcntl::SEEK_SET;
    print <$failures_fh>;
    print "\n";
}

sub explore
{
    # Explore a test by getting a look before it is cleaned up
    my $test = shift;
    &setupTestDir;
    print "Tagfs data directory at ${dataDirName}\n";
    my $res = 0;
    eval{
        $res = &$test;
    };
    system("cd $testDirName && $ENV{'SHELL'}");

    &print_failures;

    &cleanupTestDir;
    return $res;
}

sub run_test
{
    my ($test_name, $test) = @_;
    &setupTestDir;
    my $res = 0;
    if (ref($test) eq 'HASH')
    {
        my %test_data = %{$test};
        my $tries = 1;
        if (defined $test_data{tries})
        {
            $tries = $test_data{tries};
        }
        my $test_proc = $test_data{proc};
        for (my $try = 0; ($res == 0) && ($try < $tries); $try++)
        {
            eval {
                $res = subtest $test_name => \&$test_proc;
            };
        }
    }
    elsif (ref($test) eq 'CODE')
    {
        eval {
            $res = subtest $test_name => \&$test;
        };
    }
    &cleanupTestDir;
    return $res;
}

sub run_named_tests
{
    my @test_names = @_;
    plan tests => scalar(@test_names);
    my $t0 = time;
    foreach my $test_name (@_)
    {
        my $test = $tests{$test_name};
        if (defined $test)
        {
            if (run_test(colored($test_name, "red"), $test))
            {
                print ".";
            }
            else
            {
                print "F";
            }
        }
        else
        {
            diag("$test_name is not a test");
        }
    }
    &print_failures;
    my $tottime = time - $t0;
    print "Tests took $tottime seconds.\n";
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
        if (defined($test))
        {
            if ($should_explore)
            {
                explore($test);
            }
            else
            {
                run_test($t, $test);
                &print_failures;
            }
        }
        else
        {
            print "$t is not a test\n";
        }
    }
}
elsif (defined($TEST_PATTERN))
{
    my @test_names = keys(%tests);
    $TEST_PATTERN =~ s/\*/.*/g;
    my @selected_tests = grep { /$TEST_PATTERN/ } @test_names;
    run_named_tests(@selected_tests);
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
