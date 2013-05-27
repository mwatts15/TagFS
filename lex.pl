#!/usr/bin/env perl

my $file = shift;
my %format_specs =
( "const char *" => "\"%s\""
, "__DNP__ char *" => "%p"
, "__DNP__ const char *" => "%p"
, "char *" => "\"%s\""
, "long" => "%ld"
, "size_t" => "%zd"
, "off_t" => "%lld"
, "strut fuse_file_info *" => "0x%08x"
, "int" => "%d"
, "uid_t" => "%d"
, "gid_t" => "%d"
, "mode_t" => "0%03o"
, "dev_t" => "%lld"
);

sub c_str_esc
{
    $_ = shift;
    s/"/\\"/g;
    $_;
}

sub print_file {
    $outfile = $file;
    $outfile =~ s/\.l(.)$/.$1/;
    open( my $fh, ">$outfile" ) or die "can't create $outfile $!";
    print $fh @_;
}

local( *FH ) ;
open( FH, $file ) or die "Cannot open $file: $!";
$_ = do { local( $/ ) ; <FH> };

sub add_fn_log_msg
{
    my ($orig, $name, $args, $rest) = @_;
    #print "$orig\n$name\n$args\n$rest\n";
    my @arg_list = split /, ?/, $args;
    my @arg_formats = ();
    my @arg_names = ();
    foreach (@arg_list)
    {
        # get the type and argument
        if (m/(.*)\b(\w+)$/)
        {
            my $type = $1;
            my $arg = $2;
            my $format = (defined $format_specs{$type})?
            $format_specs{$type} : "%p";
            push @arg_formats, "$arg=" . c_str_esc($format);
            push @arg_names, $arg;
        }
    }
    "${orig}\nlog_msg(\"\\n$name". join (", ", @arg_formats) . "$rest\\n\", " .
    join(", ", @arg_names) . ");\n";
}

s/%LOG//;
s<^\w+ (tagfs_\w+ ?\()([^\)]+)(\))\n{$><add_fn_log_msg($&, $1, $2, $3)>mge;
print_file($_);
