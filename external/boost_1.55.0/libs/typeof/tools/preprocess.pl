# // (C) Copyright Tobias Schwinger
# //
# // Use modification and distribution are subject to the boost Software License
# // Version 1.0. (See http:/\/www.boost.org/LICENSE_1_0.txt).

# // Preprocess and run this script.
# //
# // Invocation example using the GNU preprocessor:
# //
# //   g++ -I$BOOST_ROOT -x c++ preprocess.pl -E |perl
# //
# // or in two steps:
# //
# //   g++ -I$BOOST_ROOT -x c++ preprocess.pl -E >temp.pl
# //   perl temp.pl

#define die(x) 1
die("ERROR: this script has to be preprocessed, stopped");
#undef die

use strict vars;
use File::Spec updir,curdir,catfile,canonpath,splitpath,file_name_is_absolute;

# // --- Settings
my $up = File::Spec->updir();

# // Relative path to the destination directory.
my $path = File::Spec->catdir($up,$up,$up,'boost','typeof');

my $license = qq@
/\/ Copyright (C) 2005 Arkadiy Vertleyb
/\/ Copyright (C) 2005 Peder Holt
/\/ 
/\/ Use modification and distribution are subject to the boost Software License,
/\/ Version 1.0. (See http:/\/www.boost.org/LICENSE_1_0.txt).

/\/ Preprocessed code, do not edit manually !

@;
# //---

# // Find this script's directory if run directly from the shell (not piped)
$path = File::Spec->canonpath
( File::Spec->catfile
  ( File::Spec->file_name_is_absolute($0)
    ? $0 : (File::Spec->curdir(),$0)
  , $up
  , File::Spec->splitpath($path)
  )
) unless ($0 eq '-');
die 
( ($0 eq '-')
  ? "ERROR: please run from this script's directory, stopped" 
  : "ERROR: target directoty not found, stopped" 
) unless (-d $path);

# // Tidy up the contents and write it to a file
sub write_down(name,contents)
{
  my($name,$contents) = @_; 
  my $filename = $name;

  my $fqfname = File::Spec->catfile($path,$filename);
  $contents =~ s"(((\n|^)\s*\#[^\n]+)|(\s*\n)){2,}"\n"g; # "
  print STDERR "Writing file: '$filename'\n";
  open my($file),">$fqfname" 
    or die "ERROR: unable to open file '$filename' for writing, stopped";
  print $file $license;
  print $file $contents;
  close $file;
}

# // Include external components to ensure they don't end up in the recorded
# // output
#define BOOST_TYPEOF_PP_INCLUDE_EXTERNAL
my $sewer = <<'%--%-EOF-%--%'
#include <boost/typeof/vector.hpp>
#undef  BOOST_TYPEOF_VECTOR_HPP_INCLUDED
%--%-EOF-%--%
; $sewer = '';


#define BOOST_TYPEOF_PREPROCESSING_MODE
#define BOOST_TYPEOF_LIMIT_SIZE 50
#define BOOST_TYPEOF_PP_NEXT_SIZE 100

&write_down('vector50.hpp',<<'%--%-EOF-%--%'
#include <boost/typeof/vector.hpp>
%--%-EOF-%--%
);
#undef  BOOST_TYPEOF_VECTOR_HPP_INCLUDED

#undef  BOOST_TYPEOF_LIMIT_SIZE
#define BOOST_TYPEOF_LIMIT_SIZE 100
#define BOOST_TYPEOF_PP_NEXT_SIZE 149

&write_down('vector100.hpp',<<'%--%-EOF-%--%'
#include <boost/typeof/vector.hpp>
%--%-EOF-%--%
);
#undef  BOOST_TYPEOF_VECTOR_HPP_INCLUDED

#undef  BOOST_TYPEOF_LIMIT_SIZE
#define BOOST_TYPEOF_LIMIT_SIZE 150
#define BOOST_TYPEOF_PP_NEXT_SIZE 199


&write_down('vector150.hpp',<<'%--%-EOF-%--%'
#include <boost/typeof/vector.hpp>
%--%-EOF-%--%
);
#undef  BOOST_TYPEOF_VECTOR_HPP_INCLUDED

#undef  BOOST_TYPEOF_LIMIT_SIZE
#define BOOST_TYPEOF_LIMIT_SIZE 200
#define BOOST_TYPEOF_PP_NEXT_SIZE 250

&write_down('vector200.hpp',<<'%--%-EOF-%--%'
#include <boost/typeof/vector.hpp>
%--%-EOF-%--%
);

