#!/usr/bin/env perl
use Data::Dumper;
my %tags = ();
$separator = "\0";
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
        $tname = "tag" . sprintf("%03d", $no);
        $tags{$tname} = 2; # 2 == tagdb_int_t
        push @res, $tname . "${separator}" . sprintf("%03d", $no);
    }
    $real_max = $real_max + 2; # for the file name
    "$real_max${separator}" . join "${separator}", @res;
}

sub numbered_file_with_tags_upto_max
{
    my $num = shift @_;
    my $max_tags = shift @_;
    my $max_tags_per_file = shift @_;
    my $copies_dir = shift;
    my $fname = "file" . sprintf("%03d", $num);
    open(RF, ">", "$copies_dir/$num");
    close(RF);
    "$num${separator}" . random_tags_upto_max($max_tags, $max_tags_per_file) 
        . "${separator}name${separator}$fname";
}

sub make_types_file
{
    open(F, ">", $_[0]);
    for my $key ( keys %tags )
    {
        my $value = $tags{$key};
        print F "$key:$value\n";
    }
    print F "name:3" # very very necessary!
}
my $name = shift;
my $size = shift;
my $max_tags = shift;
my $max_tags_per_file = shift;
my $copies_dir = shift;
my @files = ();
(my $types_name = $name) =~ s/(\..*)$/.types/;
open(FILE, ">", $name);
if (! $copies_dir)
{
    $copies_dir = "copies"
}
for my $f (glob("./" . $copies_dir . "/*"))
{
    print "Unlinking $f\n";
    unlink $f;
}
for my $i (1 .. $size)
{
    push @files, numbered_file_with_tags_upto_max($i, $max_tags,
            $max_tags_per_file, $copies_dir);
}
print FILE join "${separator}", @files;
make_types_file($types_name);
