#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Mon Dec 23 21:29:54 CST 2013

use strict;
use File::Path qw(make_path rmtree);
use Test::More;

my $testDirName = "testDir";
sub setupTestDir
{
    `fusermount -u $testDirName`; 
    `rm -rf $testDirName`;
    mkdir $testDirName;
    `../tagfs -f test.db $testDirName`;
}

sub cleanupTestDir
{
    `fusermount -u $testDirName`; 
    `rm -rf $testDirName`;
}

{
    &setupTestDir;
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
    &cleanupTestDir;
}

{
    &setupTestDir;
    my @dirs = map { "dir" . $_ } 0..8;
    my $dir = "$testDirName/" . join("/", @dirs);
    make_path($dir);
    &cleanupTestDir;
}

{
    &setupTestDir;
    my $dir = "$testDirName/a/b/c/d/e/f/g/h";
    make_path($dir);
    open F, ">", "$dir/file";
    printf F "text\n";
    close F;
    open F, "<", "$dir/file";
    my $s = <F>;
    is($s, "text\n");
    close F;
    &cleanupTestDir;
}

done_testing();
