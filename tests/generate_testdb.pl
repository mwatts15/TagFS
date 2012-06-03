#!/usr/bin/env perl
use Data::Dumper;
my ($name, $table_size, $ntags, $max_tags_per_item, $copies_dir) = @ARGV;
my %tags = ("name" => 3);
my $separator = "\0";
$section_separator = "\0\0";

sub make_tags
{
    for my $i (0 .. $ntags)
    {
        $tags{"tag" . sprintf("%03d", $i)} = 2;
    }
}

sub random_tags_upto_max
{
    my $i = 0;
    my @keyarr = keys %tags;
    my %res = ();
    while ($i < $max_tags_per_item)
    {
        my $n = int(rand($ntags));
        $res{$keyarr[$n]} = int(rand(256));
        $i++;
    }
    \%res;
}

sub numbered_file_with_tags_upto_max
{
    my $num = shift;
    my $fname = "file" . sprintf("%03d", $num);
    my %tags = %{random_tags_upto_max()}; 

    $tags{name} = $fname;
    open(RF, ">", "$copies_dir/$num");
    close(RF);
    
    "$num${separator}" . scalar(keys %tags) . $separator . join ($separator, %tags);
}

sub tag_with_tags
{
    my $tname = shift;
    my %tags = %{random_tags_upto_max()}; 
    delete $tags{$tname};
    "$tname${separator}" . scalar(keys %tags) . $separator . join ($separator, %tags);
}

sub make_types_file
{
    my $F = shift;
    my @keys = keys %tags; 
    print $F scalar(@keys), $separator; 
    for my $key ( keys %tags )
    {
        my $value = $tags{$key};
        print $F "$key${separator}$value${separator}";
    }
}

if (! $copies_dir)
{
    $copies_dir = "copies";
}
if (! -d $copies_dir)
{
    mkdir $copies_dir;
}
for my $f (glob("./$copies_dir/*"))
{
    print "Unlinking $f\n";
    unlink $f;
}

open(FILE, ">", $name);

make_tags();

make_types_file(FILE);

my @files = ();
for my $i (1 .. $table_size)
{
    push @files, numbered_file_with_tags_upto_max($i);
}
print FILE scalar(@files), $separator . join(${separator}, @files) 
    . $separator;

my @tag_entries = ();
for my $i (keys %tags)
{
    push @tag_entries, tag_with_tags($i);
}
print FILE scalar(@tag_entries), $separator . join(${separator}, @tag_entries) 
    . $separator;
