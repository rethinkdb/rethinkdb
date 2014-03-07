Name: boost-jam
Version: 3.1.19
Summary: Build tool
Release: 1
Source: %{name}-%{version}.tgz

License: Boost Software License, Version 1.0
Group: Development/Tools
URL: http://www.boost.org
Packager: Rene Rivera <grafik@redshift-software.com>
BuildRoot: /var/tmp/%{name}-%{version}.root

%description
Boost Jam is a build tool based on FTJam, which in turn is based on
Perforce Jam. It contains significant improvements made to facilitate
its use in the Boost Build System, but should be backward compatible
with Perforce Jam.

Authors:
    Perforce Jam : Cristopher Seiwald
    FT Jam : David Turner
    Boost Jam : David Abrahams

Copyright:
    /+\
    +\  Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
    \+/
    License is hereby granted to use this software and distribute it
    freely, as long as this copyright notice is retained and modifications
    are clearly marked.
    ALL WARRANTIES ARE HEREBY DISCLAIMED.

Also:
    Copyright 2001-2006 David Abrahams.
    Copyright 2002-2006 Rene Rivera.
    Copyright 2003-2006 Vladimir Prus.

    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

%prep
%setup -n %{name}-%{version}

%build
LOCATE_TARGET=bin ./build.sh $BOOST_JAM_TOOLSET

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}
install -m 755 bin/bjam $RPM_BUILD_ROOT%{_bindir}/bjam-%{version}
ln -sf bjam-%{version} $RPM_BUILD_ROOT%{_bindir}/bjam
cp -R *.html *.png *.css LICENSE*.txt images jam $RPM_BUILD_ROOT%{_docdir}/%{name}-%{version}

find $RPM_BUILD_ROOT -name CVS -type d -exec rm -r {} \;

%files
%defattr(-,root,root)
%attr(755,root,root) /usr/bin/*
%doc %{_docdir}/%{name}-%{version}


%clean
rm -rf $RPM_BUILD_ROOT
