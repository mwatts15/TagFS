#!/usr/bin/env perl

sub random_tags_upto_max
{
    my $max = shift @_;
    my $max_per_file = shift @_;
    my $real_max = int(rand($max_per_file));
    my @res = ();
    my @used = ();
    my $in = 0;
    my $no;
    for my $i (0 .. $real_max)
    {
        do
        {
            $no = int(rand($max));
            $in = grep {$_ eq $no} @used;
        } until ($in == 0);
        push @res, "tag" . sprintf("%03d", $no) . ":" . sprintf("%03d", $no);
    }
    join ",", @res;
}

sub numbered_file_with_tags_upto_max
{
    my $num = shift @_;
    my $max_tags = shift @_;
    my $max_tags_per_file = shift @_;
    my $copies_dir = shift;
    my $fname = "file" . sprintf("%03d", $num);
    open(RF, ">", $copies_dir . "/" . $fname);
    close(RF);
    "name:" . $fname . "," . random_tags_upto_max($max_tags,
            $max_tags_per_file);
}

my $name = shift;
my $size = shift;
my $max_tags = shift;
my $max_tags_per_file = shift;
my $copies_dir = shift;
my @files = ();
open(FILE, ">", $name);
for my $i (0 .. $size)
{
    push @files, numbered_file_with_tags_upto_max($i, $max_tags,
            $max_tags_per_file, $copies_dir);
}
print FILE join " ", @files;
