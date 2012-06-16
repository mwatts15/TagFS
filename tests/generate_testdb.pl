#!perl
use Data::Dumper;
my ($name, $table_size, $ntags, $max_tags_per_item, $copies_dir) = @ARGV;
my %tags = ();
my $separator = "\0";

sub make_tags
{
    for my $i (0 .. $ntags)
    {
        $tags{"tag" . sprintf("%03d", $i)} = $i + 1;
    }
}

sub random_tags_upto_max
{
    my $i = 0;
    my @keyarr = values %tags;
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

    open(RF, ">", "$copies_dir/$num");
    close(RF);
    
    "$num${separator}$fname${separator}" . scalar(keys %tags) . $separator . join($separator, %tags);
}

sub make_types_file
{
    my $F = shift;
    my @keys = keys %tags; 
    print $F scalar(@keys), $separator; 
    for my $key ( sort keys %tags )
    {
        my $type = 2;
        my $defval = 0;
        print $F "$tags{$key}${separator}$key${separator}$type${separator}$defval${separator}";
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
