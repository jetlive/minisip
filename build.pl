#!/usr/bin/perl
#  build.pl - a build system management tool
#  Developed originally for the minisip project by Zachary T Welch.
#  Copyright (C) 2005-2006, Superlucidity Services, LLC
#  Released under the GNU GPL v2 license.

use strict;
use warnings;

#######
# script configuration option definitions

use Cwd;
our $topdir = getcwd();	# path to common source directory (svn trunk)
our $builddir = undef;	# path to common build directory
our $installdir = undef; # path to common installation directory
my $bindir = undef;	#   path to installed executables

our $srcdir = undef;	# path to package source directory
our $objdir = undef;	# path to package build directory

our $debug = 0;		# enable debugging build
our $static = 0;	# enable static build (--disable-shared)

my $batch = 0;		# perform actions on dependencies as well as targets
our $toggle_batch = 0;  # reverse meaning of batch flag (default to batch mode)

my $njobs = 0;		# number of parallel jobs during makes (0 = autodetect)
my $ccache = 1;		# enable ccache support
my $force = 0;		# continue despite errors
my $show_env = 0;	# output environment variables this script changes

my $pretend = 0;	# don't actually do anything
my $verbose = 0;	# enable verbose script output
my $quiet = 0;		# enable quiet script output
my $help = 0;
my $man = 0;

my $list_actions = 0;	# show the actions permitted by this script
my $list_targets = 0;	# show the targets known by this script

our $hostdist = 'autodetect';	# This host's distribution (e.g. gentoo).
our $buildspec = 'autodetect';  # This host's compiler specification.
our $hostspec = 'autodetect';	# The target host's compiler specification.
				# e.g. x86-pc-linux-gnu, arm-unknown-linux-gnu

# reset umask; fixes problems from mixing ccache with merge action
umask(0007) or die "unable to set umask to 0007: $!";

#######
# process option arguments

sub usage { 
	print @_, "\n" if @_;
	die <<USAGE;
usage: $0 [<options>] <action>[+<action>...] [<targets>]
Build Options:
    -d|--debug		Enabled debugging in resulting builds
    -s|--static		Enabled static build (uses --disable-shared)

Advanced Build Options:
    -b|--build=...	Build libraries and programs on this platform.
    -t|--host=...	Build libraries and programs that run on this platform.
    -S|--batch		Perform actions on dependencies as well as targets.
    -j|--jobs=n		Set number of parallel jobs (default: $njobs)
    -c|--ccache		Enable ccache support (is on by default; use --noccache)
    -f|--force		Continue building despite any errors along the way
    -E|--show-env	Show environment variables when we update them

Directory Options:
    -T|--topdir=...	Select location of svn repository. 
			(currently: $topdir)
    -B|--builddir=...	Select location for build directories.
			(currently: $topdir/build)
    -I|--installdir=...	Select common install directory (sets --prefix).
	                (currently: $topdir/install)

Distribution Merge Options:
    -D|--distro=...	Sets package manager features (default: $hostdist)

General Options:
    -p|--pretend	Do not actually perform actions
    -v|--verbose	Verbose output mode
    -q|--quiet		Quiet output mode
    -?|--help		Show built-in help
    --list-actions	Show permitted command actions
    --list-targets	Show permitted targets
USAGE
#	--man		Show built-in man page
}

use Getopt::Long qw( :config gnu_getopt );
my $result = GetOptions(
		"topdir|T=s" => \$topdir,

		"builddir|B=s" => \$builddir,
		"installdir|I=s" => \$installdir,
		"distro|D=s" => \$hostdist,

		"debug|d!" => \$debug,
		"static|s!" => \$static,
		"batch|S!" => \$batch,

		"build|b=s" => \$buildspec,
		"host|t=s" => \$hostspec,

		"jobs|j=i" => \$njobs,
		"ccache|c!" => \$ccache,
		"force|f!" => \$force,
		"show-env|E!" => \$show_env,

		"pretend|p!" => \$pretend,
		"verbose|v!" => \$verbose,
		"quiet|q!" => \$quiet,
		'help|?' => \$help, 
		'list-actions' => \$list_actions, 
		'list-targets' => \$list_targets, 
#		man => \$man,
	);
usage() if !$result || $help || $man;

use Pod::Usage;
pod2usage(2) unless $result;
pod2usage(1) if $help;
pod2usage(-exitstatus => 0, -verbose => 2) if $man;

# cross-compiling support

sub cross_compiling { return $buildspec ne $hostspec }
sub autodetect_buildspec { return 'x86-pc-linux-gnu'; }
sub autodetect_hostspec { return autodetect_buildspec(); }

for ($buildspec) {
/^autodetect$/ and do { $buildspec = autodetect_buildspec(); };
}
for ($hostspec) {
/^autodetect$/ and do { $hostspec = autodetect_hostspec(); };
}
die "$hostspec-gcc not found." 
	if cross_compiling() && ! -x "/usr/bin/$hostspec-gcc";

# set-up paths
$builddir = "$topdir/build/$hostspec" unless $builddir;
$installdir = "$topdir/install/$hostspec" unless $installdir;
$bindir = "$installdir/usr/bin";

# set-up common options
$show_env = 1 if $pretend && $verbose;
$verbose = 1 if $pretend;
$verbose = 0 if $quiet;
$quiet = 0 if $pretend;

# guess number of jobs
# XXX: will not work for all platforms; needs revisiting, but okay for now
$njobs = `grep processor /proc/cpuinfo | wc -l` unless $njobs;

# extra arguments to pass to make
my @make_args = ();
push @make_args, '-k' if $force;
push @make_args, "-j$njobs" if $njobs > 1;

#######
# load package, dependency, and configure parameter definitions (see build.conf)

our $default_action;
our $default_target;

our @packages; 		# absolute package build order
our %dependencies; 	# package dependency lists
our %configure_params;	# package configure parameter tables

our @actions = ( qw( 
		bootstrap configure compile 
		dist distcheck install merge run
		clean dclean mclean tarclean
	) );

# load primary definitions
do "$topdir/build.conf" or die "error: unable to load build.conf:\n$@";

sub set_configure_param {
	my ( $package, %params ) = @_;
	if (ref $package) {
		set_configure_param($_, %params) for @$package;
		return;
	}
	die "no package called '$package'" unless grep /^$package$/, @packages;
	for my $key ( keys %params ) {
		die "no parameter called '$key' in $package" 
			unless exists($configure_params{$package}->{$key});
		$configure_params{$package}->{$key} = $params{$key};
	}
	return 1;
}
sub set_global_configure_param {
	my ( %params ) = @_;
	for my $key ( keys %params ) {
		my $value = $params{$key};
		# reassign global variables for build system related params
		for ($key) {
		/^debug$/ and do { $debug = $value; last; };
		/^shared$/ and do { $static = !$value; last; };
		}
		set_configure_param($_, $key, $value) for @packages;
	}
	return 1;
}

# allow overrides using the above pair of accessors
my $localconf = "$topdir/build.local";
if (-f "$localconf") {
	do "$localconf" or die "error: unable to load build.local:\n$@";
}

list_actions() if $list_actions;
list_targets() if $list_targets;

#######
# process action and target arguments

my $action = shift @ARGV || $default_action;
unless ($action) {
	print "error: No action specified! You must provide at least ",
		"one valid action.\n       You may combine actions with ",
		"'+', such as 'bootstrap+configure'.\n";
	list_actions();
	usage();
}

sub add_targets { map { ( $_, add_targets(@{$dependencies{$_}}) ) } @_ };
my @targets = @ARGV;
push @targets, $default_target if !@targets && $default_target;
for ( @targets ) {
	die "'$_' is not a valid target.\n" unless exists $configure_params{$_};
}
$batch = !$batch if $toggle_batch;
@targets = add_targets(@targets) if $batch;
usage("error: no target specified!\nPossible packages are:\n",
	map { "\t$_\n" } @packages, "\n") unless @targets;

print "$action: @targets\n" unless $quiet;

#######
# common action funtions

use File::Spec;
use File::Glob ':glob';
use File::Copy;
use File::Basename;

our $pkg;
my %actions;
my %act_deps;

sub easy_mkdir {
	my $dir = shift;
	unless (-d $dir) {
		my $r = shift;
		easy_mkdir(dirname($dir), $r) if $r-- && ! -d dirname($dir);
		mkdir $dir or die "unable to create '$dir': $!";
	}
	return $dir;
}

sub act {
	my $label = shift;
	print "Running $label in $pkg...", $pretend ? ' (dry run)' : '', "\n" 
		unless $quiet;
	print '+ ', join(' ', @_), "\n" if $verbose;
	return if $pretend;
	system(@_) == 0 or die "system @_ failed: $?";
}
sub callact {
	my $a = shift;
	die "no action '$a'" unless exists $actions{$a};

	$act_deps{$a}->() if exists $act_deps{$a};

	my $tgtdir = $a eq 'bootstrap' ? $srcdir : $objdir;
	if ($tgtdir ne $ENV{PWD}) {
		print "Changing to $tgtdir...\n";
		chdir $tgtdir or die "unable to cd to $tgtdir";
	}
	$actions{$a}->();
}

sub distfiles {
	return bsd_glob("$objdir/$pkg*.tar.gz");
}

sub _is_feature_param { return defined $_[0] && $_[0] =~ /^\d?$/; }
sub _feature_configure_params {
	my $spec = shift;
	my @keys = grep { _is_feature_param($spec->{$_}) } keys %$spec;
	my %spec = map { $_ => $spec->{$_} ? 'en' : 'dis' } @keys;
	return map { join('', '--', $spec{$_}, 'able-', $_) } @keys;
}
sub _is_package_param { return defined $_[0] && $_[0] !~ /^\d?$/; }
sub _package_configure_params {
	my $spec = shift;
	my @keys = grep { _is_package_param($spec->{$_}) } keys %$spec;
	my %spec = map { $_ => $spec->{$_} ? '' : 'out' } @keys;
	return map { join('', '--', 'with', $spec{$_}, '-', $_, 
			$spec->{$_} ? '=' : '', $spec->{$_} ) } @keys;
}
sub configure_params {
	my $spec = $configure_params{$pkg};
	my @spec = _feature_configure_params($spec);
	push @spec, _package_configure_params($spec);
	unshift @spec, "--build=$buildspec", "--host=$hostspec"
		if cross_compiling();
	unshift @spec, "--prefix=$installdir/usr";
	return @spec;
}

########
# XXX: add better distribution support (currently only supports Gentoo)
#  How to do that:  
#   1) factor existing implementation into build.d/dists/gentoo.pl module
#   2) define common set of entry points, change existing references
#   3) add new modules (the details are left as an exercise to reader)
# For bonus points, move configuration files into build.d when you're done.
#
# Finally, these suggestions are not meant to be implemented without review;
# this does not represent the best possible plan, just that plan is possible.

###
# Gentoo support

sub gentoo_distdir {
	my $distdir = `portageq envvar DISTDIR` or die "can't get DISTDIR";
	chomp $distdir;
	print "+portageq envvar DISTDIR=$distdir\n" if $verbose;
	return $distdir;
}

sub gentoo_merge {
	my @distfiles = distfiles();
	die "no distfiles!" unless @distfiles;
	my $distdir = gentoo_distdir();
	for my $p ( @distfiles ) {
		my $tgt = File::Spec->catdir($distdir, basename($p));
		if ( -e $tgt ) {
			print "+removing old $tgt\n";
			unlink($tgt) or die "can't unlink $tgt: $!"; 
		}
		print "+copying $p -> $tgt\n";
		unless (link($p, $tgt) or copy($p, $tgt)) {
			die "copy failed: $p -> $tgt: $!";
		}
	}
	act('system merge', qw( sudo emerge ), $pkg, '--digest');
}

###
# select approriate functions
# XXX: this needs revisiting as other distros become supported

my $mergefunc = undef;

sub autodetect_hostdist {
	$hostdist = 'gentoo';
}

for ($hostdist) {
/^autodetect$/ and do { autodetect_hostdist(); }; # fall through!
/^gentoo$/ and do { $mergefunc = \&gentoo_merge; last };
$mergefunc = sub { die "+BUG: unable to merge packages under '$hostdist'" };
}

######
#  Common action functions

%actions = (
	bootstrap => sub { act('bootstrap', './bootstrap'); },
	configure => sub { 
		act('configure', "$srcdir/configure", configure_params()); 
	},
	compile => sub { act('compile', 'make', @make_args); },
	install => sub { act('install', 'make', @make_args, 'install'); },
	run => sub {
		 # XXX: This needs to be generalized, but it works for now.
		 #  For what it's worth, it is "equivalent" to the old .sh
		return unless $pkg eq 'minisip';
		$ENV{LD_LIBRARY_PATH} = "$installdir/usr/lib";
		act('run', "$bindir/$pkg");
	},
	tarclean => sub {
		for my $p ( distfiles() ) {
			print "+removing $p\n";
			unlink($p) or die "can't unlink $p: $!"; 
		}
	},
	dist => sub { act('distribution', 'make', @make_args, 'dist'); },
	distcheck => sub { act('distcheck', 'make', @make_args, 'distcheck'); },
	merge => $mergefunc,
	clean => sub { act('cleanup', 'make', 'clean'); }, 
	dclean => sub { act('distribution cleanup', 'make', 'distclean'); },
	mclean => sub { act('developer cleanup', 'make', 'maintainer-clean'); },
);

# common checks for preconditions
my $need_bootstrap = sub { callact('bootstrap') unless -e "$srcdir/configure" };
my $need_configure = sub { callact('configure') unless -e "$objdir/Makefile" };
my $need_compile = sub { callact('compile') }; # always rebuild, just in case
my $need_install = sub { callact('install') unless -x "$bindir/minisip"; }; 
my $need_tarclean = sub { callact('tarclean'); $need_compile->() };
my $need_dist = sub { callact('dist') unless scalar(distfiles()) };

%act_deps = (
	configure => $need_bootstrap,
	compile => $need_configure,
	install => $need_compile,
	run => $need_install,
	dist => $need_tarclean,
	distcheck => $need_tarclean,
	merge => $need_dist,
	clean => $need_configure,
	dclean => $need_configure,
	mclean => $need_configure,
);

use Text::Wrap;
sub list_actions {
	print "This script supports the following actions:\n";
	print wrap("\t", "\t", join(", ", @actions)), "\n";
	exit(0);
}

sub list_targets {
	print "This script knows about the following targets:\n";
	print wrap("\t", "\t", join(", ", @packages)), "\n";
	exit(0);
}

########################################################################
#  Main Program Start

# create and possibly report directories
easy_mkdir($builddir, 1);
easy_mkdir($installdir, 1);
if ($verbose) {
	print "+Top directory: $topdir\n";
	print "+Build directory: $builddir\n";
	print "+Install directory: $installdir\n";
}

# setup common environment
$ENV{LDFLAGS} = '';
$ENV{CPPFLAGS} ||= '';
$ENV{CPPFLAGS} .= " -I$installdir/usr/include";
$ENV{CXXFLAGS} ||= '';
$ENV{CXXFLAGS} .= "-Wall";
$ENV{CXXFLAGS} .= " -ggdb" if $debug;

$ENV{PKG_CONFIG_PATH} = "$installdir/usr/lib/pkgconfig";
easy_mkdir($installdir);
$ENV{PKG_CONFIG_PATH} .= ":/usr/lib/pkgconfig" 
	if $installdir ne '/usr' && !cross_compiling();

if ($ccache) {
	$ENV{PATH} = join(':', '/usr/lib/ccache/bin', $ENV{PATH});
	$ENV{CCACHE_DIR} = easy_mkdir(File::Spec->catdir($topdir, '.ccache'))
		unless $ENV{CCACHE_DIR} && -d $ENV{CCACHE_DIR};
}

# XXX: this is screwy and results improperly long g++ command lines, but 
# since it seems to work, it's here for now.  Hacks is as hacks are.
sub append_pkg_flags {
	$ENV{CPPFLAGS} .= " -I$srcdir/include";
}

for $pkg ( @packages ) {
	# XXX: be afraid! dynamic scoping... icky icky icky, but so darn handy
	local $pkg = $pkg;
	local $srcdir = File::Spec->catdir($topdir, $pkg);
	local $objdir = File::Spec->catdir($builddir, $pkg);
	easy_mkdir($objdir);

	print "+Checking for $pkg in targets..." if $verbose;
	unless (grep(/^$pkg$/, @targets)) {
		print " Skipping\n" if $verbose; 
		append_pkg_flags();
		next; 
	} 
	print " continuing.\n", 
		"+Source directory: $srcdir\n",
		"+Object directory: $objdir\n" if $verbose;

	if ($show_env) {
		my @envvars = ( qw( PWD PATH CCACHE_DIR CPPFLAGS CXXFLAGS LDFLAGS ) );
		my @env = map { "\n\t$_=" . $ENV{$_} } @envvars;
		print "Build environment for $pkg: @env\n";
	}

	callact($_) for split(/\+/, $action); 
	append_pkg_flags();
}
