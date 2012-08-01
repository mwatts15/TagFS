#!/usr/bin/env perl
use strict;
use warnings;
use File::Basename;
use List::Util qw/max/;

(@ARGV > 0) or exit;
my $file = shift;
my ($base, undef, $file_extension) = fileparse($file, qr/\.[^.]*/);
print "$base:$file_extension";

my %outfile_extensions =
( ".l" => ".c"
    , ".q" => ".qc"
);

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

sub c_str_esc
{
    $_ = shift;
    s/"/\\"/g;
    $_;
}

sub print_file {
    my $outfile = $file;
    $outfile =~ s/$file_extension/$outfile_extensions{$&}/;
    open( my $fh, ">$outfile" ) or die "can't create $outfile $!";
    print $fh @_;
}

local( *FH ) ;
open( FH, $file ) or die "Cannot open $file: $!";
my $F = do { local( $/ ) ; <FH> };
{ #function headers and component structs
    #turns %%operation <argument names>%% into
    #int <file_name_base>_<operation> (<arguments>)

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

    sub make_fuse_oper 
    {
        my ($base, @ops) = @_;
        return "struct fuse_operations ${base}_oper = {"
        . join (",", map {sprintf(".%s = ${base}_%s", $_, $_)} @ops)
        . "};";
    }
    
    sub make_subfs_component
    {
        "subfs_component ${base}_subfs = { .path_checker = ${base}_handles_path,
        .operations = ${base}_oper };"
    }

    my $op_alt = join("|", @oper_names);
    my @these_opers = ();

    $F =~ s<^\s*op%($op_alt) (.*)%><push(@these_opers, $1); my ($op,$args) = ($1,$2);
    sprintf($oper_headers{$op}, $base, split(/\s*,\s*|\s+/, $args))>mge;
    $F =~ s<^\s*pcheck%(\w+)%><gboolean ${base}_handles_path(const char *$1)>mg;
    $F =~ s<%%%register_component%%%><"%%%fuse_operations%%%\n" 
            . &make_subfs_component($base)>mge;
    $F =~ s<%%%fuse_operations%%%><&make_fuse_oper($base, @these_opers)>mge;
}

QUERIES:
{ #queries
    # TODO: make a qa%% pattern for declaring and naming query arguments
        # like `qa%DIS:argument_dict%`
        # or `qa%I:integer_argument%`
        # or with pattern matching of container types
        # `qa%I:id DSS:new_tags I:update_valuesp%`
        # which would take the first argument in argv as id, take all the arguments
        # up to the last as elements of new_tags (siletly dropping a missing key
        # in the vector due to an odd number of arguments) and taking the last as
        # update_valuesp. An alternative strategy where users can use some limited
        # container syntax may supercede pattern matching.
    # TODO: Auto-include headers hidden behind the patterns
    my $class = $base;
    my $reg_file ="queries.l";

    sub make_query_fn_name
    {
        my ($name, $class) = @_;
        "query_${class}_$name";
    }

    sub make_query_header
    {
        my ($name, $class) = @_;
        "int " . make_query_fn_name($name, $class) 
        . "(TagDB *db, int argc, gchar **argv, gpointer *result, char **type)";
    }

    sub splist
    {
        split(/\s/, $_[0]);
    }

    sub make_c_array
    {
        "{" . join(",", @_) . "}";
    }

    sub make_c_str_array
    {
        &make_c_array(map {"\"$_\""} @_, "NULL");
    }
    
# structs
    if ($file eq $reg_file)
    {
        my %ahash = ();
        $F =~ s<^qq%(\w+)%$ #start block
        \s*
        ([\w\s]+)
        ^%%$
        ><$ahash{$1} = $2;"">mgex;

        print values (%ahash);
        my $n_classes = scalar(keys(%ahash));
        my $max_args = max (map {scalar(splist($_))} values %ahash);

        # string names
        $F =
          join("\n", map {my $class = $_;
                          map {&make_query_header($_, $class) . ";"}
                              splist($ahash{$class})}
                         keys(%ahash)) . "\n"
        . "const char *query_class_names[$n_classes + 1] ="
        . &make_c_str_array(map {uc($_)} keys %ahash) . ";\n"
        . "q_fn query_functions[$n_classes][$max_args] ="
        . &make_c_array(map {
                             my $class = $_;
                             &make_c_array(map {&make_query_fn_name($_, $class)}
                                               splist($ahash{$class}))}
                             keys (%ahash)) . ";\n"
        . "const char *query_cmdstrs[$n_classes][$max_args + 1] =" 
        . &make_c_array(map {&make_c_str_array(splist(uc($_)))} values(%ahash)) . ";\n";

        #truncate $file, 0;
        last QUERIES;
    }
    my (@names, @ret_types);
# HEADERS
    $F =~ s<q%
    \s*
    (\w+) #name
    \s*
    ( \d+ ) #min_number_of_args
    \s*
    ( [a-zA-Z]+ ) #return_type
    \s*
    %
    ([^\{]*) # possibly %log% etc.
    {><push(@names, $1); push(@ret_types, $3);
    &make_query_header($1, $class) . "${4}{check_args($2);";>mgex;

    $F =~ s<^\s*qerr%
    ([^%]+) # just a string
    %><"*result = g_strdup(\"" . c_str_esc($1) . "\");
    *type = g_strdup(\"E\"); return -1;">mgex;

# results
    $F =~ s<^\s*qr%
    ([^%]+) # just a string
    %><"*result = $1; *type = g_strdup(\"" . shift(@ret_types) . "\"); return 0;">mgex;
    open(RF, ">>", $reg_file);
    print RF "\nqq%${class}%\n" . join("\n", @names) . "\n%%\n";
}
{ #logging tagfs operations
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
                push @arg_formats, "$arg=" . c_str_esc($format);
                push @arg_names, $arg;
            }
        }
        "log_msg(\"\\n$name " . join (" ", @arg_formats) . "\", " . 
        join(", ", @arg_names) . ");";
    }
    $F =~ s<^(
    \s* # whitespace
    \w+ # the function return type
    \s*
    (\w+) # the function name 
    \s* #ws
    \(
    ([^\)]*) #arguments
    \)
    \s*
    )
    %log% # log notice
    (\s*\{) #everything else
    ><"$1$4" . &add_fn_log_msg($2, $3)>mgex;
}

&print_file($F);

# call ourselves on any more input files
my $new_arglist = join " ", @ARGV;
qx/perl $0 $new_arglist/;
