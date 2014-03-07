#!/usr/local/bin/perl

use Fcntl;


# copy.pl

# Perl script 'copy' comment. On Windows the built in "copy" command also
# copies timestamps: this messes up Makefile dependencies.

my $stripcr = 0;

my $arg;

foreach $arg (@ARGV) {
	if ($arg eq "-stripcr")
		{
		$stripcr = 1;
		next;
		}
	$arg =~ s|\\|/|g;	# compensate for bug/feature in cygwin glob...
	foreach (glob $arg)
		{
		push @filelist, $_;
		}
}

$fnum = @filelist;

if ($fnum <= 1)
	{
	die "Need at least two filenames";
	}

$dest = pop @filelist;
	
if ($fnum > 2 && ! -d $dest)
	{
	die "Destination must be a directory";
	}

foreach (@filelist)
	{
	if (-d $dest)
		{
		$dfile = $_;
		$dfile =~ s|^.*[/\\]([^/\\]*)$|$1|;
		$dfile = "$dest/$dfile";
		}
	else
		{
		$dfile = $dest;
		}
	sysopen(IN, $_, O_RDONLY|O_BINARY) || die "Can't Open $_";
	sysopen(OUT, $dfile, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY)
					|| die "Can't Open $dfile";
	while (sysread IN, $buf, 10240)
		{
		if ($stripcr)
			{
			$buf =~ tr/\015//d;
			}
		syswrite(OUT, $buf, length($buf));
		}
	close(IN);
	close(OUT);
	print "Copying: $_ to $dfile\n";
	}
		

