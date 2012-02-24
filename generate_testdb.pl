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
    "file" . sprintf("%03d", $num) . " " . random_tags_upto_max($max_tags,
            $max_tags_per_file);
}

my $name = shift @ARGV;
my $size = shift @ARGV;
my $max_tags = shift @ARGV;
my $max_tags_per_file = shift @ARGV;
open(FILE, ">", $name);
for my $i (0 .. $size)
{
    print FILE numbered_file_with_tags_upto_max($i, $max_tags,
            $max_tags_per_file) . " ";
}
