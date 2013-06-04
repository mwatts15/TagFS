#!/usr/bin/perl
# author: Mark Watts <mark.watts@utexas.edu>
# date: Sun Jun  2 19:17:33 CDT 2013

use strict;
use File::Basename;
use List::Util qw/max/;
use feature qw/switch/;

(@ARGV > 0) or exit;
my $file = shift;
my ($g_file_basename, undef, $file_extension) = fileparse($file, qr/\.[^.]*/);

my %format_specs =
( "const char *" => "\"%s\""
    , "__DNP__ char *" => "%p"
    , "__DNP__ const char *" => "%p"
    , "char *" => "\"%s\""
    , "long" => "%ld"
    , "size_t" => "%zd"
    , "off_t" => "%lld"
    , "struct fuse_file_info *" => "0x%08x"
    , "int" => "%d"
    , "uid_t" => "%d"
    , "gid_t" => "%d"
    , "mode_t" => "0%03o"
    , "dev_t" => "%lld"
);

my %oper_headers =
( getattr => "int %s_getattr (const char *%s, struct stat *%s)"
    , readlink => "int %s_readlink (const char *%s, __DNP__ char *%s, size_t %s)"
    , getdir => "int %s_getdir (const char *%s, fuse_dirh_t %s, fuse_dirfil_t %s)"
    , mknod => "int %s_mknod (const char *%s, mode_t %s, dev_t %s)"
    , mkdir => "int %s_mkdir (const char *%s, mode_t %s)"
    , unlink => "int %s_unlink (const char *%s)"
    , rmdir => "int %s_rmdir (const char *%s)"
    , symlink => "int %s_symlink (const char *%s, const char *%s)"
    , rename => "int %s_rename (const char *%s, const char *%s)"
    , link => "int %s_link (const char *%s, const char *%s)"
    , chmod => "int %s_chmod (const char *%s, mode_t %s)"
    , chown => "int %s_chown (const char *%s, uid_t %s, gid_t %s)"
    , truncate => "int %s_truncate (const char *%s, off_t %s)"
    , utime => "int %s_utime (const char *%s, struct utimbuf *%s)"
    , open => "int %s_open (const char *%s, struct fuse_file_info *%s)"
    , read => "int %s_read (const char *%s, __DNP__ char *%s, size_t %s, off_t %s, struct fuse_file_info *%s)"
    , write => "int %s_write (const char *%s, __DNP__ const char *%s, size_t %s, off_t %s, struct fuse_file_info *%s)"
    , statfs => "int %s_statfs (const char *%s, struct statvfs *%s)"
    , flush => "int %s_flush (const char *%s, struct fuse_file_info *%s)"
    , release => "int %s_release (const char *%s, struct fuse_file_info *%s)"
    , fsync => "int %s_fsync (const char *%s, int %s, struct fuse_file_info *%s)"
    , setxattr => "int %s_setxattr (const char *%s, const char *%s, const char *%s, size_t %s, int %s)"
    , getxattr => "int %s_getxattr (const char *%s, const char *%s, char *%s, size_t %s)"
    , listxattr => "int %s_listxattr (const char *%s, char *%s, size_t %s)"
    , removexattr => "int %s_removexattr (const char *%s, const char *%s)"
    , opendir => "int %s_opendir (const char *%s, struct fuse_file_info *%s)"
    , readdir => "int %s_readdir (const char *%s, void *%s, fuse_fill_dir_t %s, off_t %s, struct fuse_file_info *%s)"
    , releasedir => "int %s_releasedir (const char *%s, struct fuse_file_info *%s)"
    , fsyncdir => "int %s_fsyncdir (const char *%s, int %s, struct fuse_file_info *%s)"
    , destroy => " void %s_destroy (void *%s)"
    , access => "int %s_access (const char *%s, int %s)"
    , create => "int %s_create (const char *%s, mode_t %s, struct fuse_file_info *%s)"
    , ftruncate => "int %s_ftruncate (const char *%s, off_t %s, struct fuse_file_info *%s)"
    , fgetattr => "int %s_fgetattr (const char *%s, struct stat *%s, struct fuse_file_info *%s)"
    , lock => "int %s_lock (const char *%s, struct fuse_file_info *%s, int %s, struct flock *%s)"
    , utimens => "int %s_utimens (const char *%s, const struct timespec %s[2])"
);
my @oper_names = keys(%oper_headers);
my $op_alt = join("|", @oper_names);
my @g_stored_operations = ();

my %regex = (
    function_header => '\s*\w+\s*(\w+)\s*\( ([^\)]*) \)\s*\{\s*$', # 1 = name of the function, 2 = argument list as a string
);

sub make_c_array
{
    "{" . join(",", @_) . "}";
}

sub splist
{
    my @r = split(/\s/, $_[0]);
    \@r;
}

sub c_str_esc
{
    $_ = shift;
    s/"/\\"/g;
    $_;
}

sub add_fn_log_msg
{
    my ($name, $args) = @_;
    #print "$orig\n$name\n$args\n$rest\n";
    my @arg_list = split /,\s*/, $args;
    my @arg_formats = ();
    my @arg_names = ();
    foreach (@arg_list)
    {
        # get the type and argument
        if (m/(.*)\b(\w+)$/)
        {
            my ($type, $arg) = ($1, $2);
            $type =~ s/^\s+|\s+$//g;
            my $format = (defined $format_specs{$type})?
            $format_specs{$type} : "%p";
            push @arg_formats, "$arg=" . &c_str_esc($format);
            push @arg_names, $arg;
        }
    }
    "log_msg(\"\\n$name " . join (" ", @arg_formats) . "\", " . 
    join(", ", @arg_names) . ");";
}

sub make_tagfs_op
{
    my ($op_name) = @_;
    sub generate_argument_names_from_format
    {
        my ($arg_format) = @_;
        my @arg_list = ();
        my $i = 0;
        while ($arg_format =~ /%s/g)
        {
            push @arg_list, "a$i";
            $i++;
        }
        @arg_list;
    }
    my @arg_list = ();
    if ($oper_headers{$op_name} =~ /\((.*)\)/)
    {
        @arg_list = &generate_argument_names_from_format($1);
    }
    my $arg_str0 = join(" ", @arg_list);
    my $arg_str1 = join(", ", @arg_list);
    my $path_name = $arg_list[0];
<<HERE;
%(op $op_name $arg_str0)
{
    %(log)
    struct fuse_operations *ops = subfs_get_opstruct($path_name);
    if (ops)
    {
        return ops->$op_name($arg_str1);
    }
    else
    {
        return -1;
    }
}
HERE
}

sub make_struct_initialization
{
    my ($base, @ops) = @_;
    sub make_struct_entry
    {
        my ($base, $op) = @_;
        sprintf(".%s = ${base}_%s", $op, $op);
    }
    my @assignments = map {&make_struct_entry($base,$_)} @ops;
    my $assignment_string = join(",", @assignments);
    "{ $assignment_string }";
}

sub make_operations_name
{
    my $base = shift;
    "${base}_oper";
}

sub make_fuse_oper 
{
    my ($base, @ops) = @_;
    return "struct fuse_operations ". &make_operations_name($base) . " = ".
    &make_struct_initialization($base,@ops) . ";";
}

sub match
{
    my ($phase,$args,$before,$after,$original) = @_;
    my $head = shift @$args;
    my @phases =
    (
        {
            "log" =>
            sub {
                if ($before =~ /$regex{function_header}/sx)
                {
                    &add_fn_log_msg($1, $2);
                }
                else
                {
                    print "log must follow or precede a recognized form\n";
                    "";
                }
            },
            "fuse_operations"=>
            sub {
                &make_fuse_oper($g_file_basename, @g_stored_operations);
            },
            "operations_struct_name"=>
            sub {
                &make_operations_name($g_file_basename);
            }},
        {
            "op"=>
            sub {
                sub op_and_args_to_function_header
                {
                    my ($op, @args) = @_;
                    sprintf($oper_headers{$op}, $g_file_basename, @args);
                }

                sub store_operation_and_return_function_header
                {
                    my ($op, @args) = @_;
                    push(@g_stored_operations, $op);
                    &op_and_args_to_function_header($op, @args);
                }

                &store_operation_and_return_function_header(@$args); 
            }},
        {
            "tagfs_operations"=>
            sub {
                my @ops;
                foreach(@$args)
                {
                    push @ops, &make_tagfs_op($_);
                }
                join("\n", @ops);
            }});
    if ($phase >= @phases)
    {
        return $original
    }
    else
    {
        my $result = $phases[$phase]{$head};
        if ($result)
        {
            &{$result}();
        }
        else
        {
            $original;
        }
    }
}

local( *FH ) ;
open( FH, $file ) or die "Cannot open $file: $!";
my $F = do { local( $/ ) ; <FH> };
close FH;

for (my $phase = 5; $phase >= 0; $phase--)
{
    print "phase = $phase\n";
    $F =~ s{%\(([_[:alpha:][:digit:][:space:]]*)\)}{ printf "matched:: `$&'\n" ; &match($phase, &splist($1), $`, $', $&) }mge;
}

open FH, ">", "$g_file_basename.c";
print FH $F;

