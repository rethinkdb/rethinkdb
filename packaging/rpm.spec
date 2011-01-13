%define _topdir TOPDIR
%define buildroot %{_topdir}/BUILD

BuildRoot: %{buildroot}
Summary:   RethinkDB - the database for solid drives
Name:      rethinkdb
Version:   VERSION
Release:   1
License:   RethinkDB Beta Test License 0.1
Packager:  Hexagram 49, Inc.
Group:     Development/Tools
Provides:  memcached,rethinkdb
Requires:  /bin/sh

%description
RethinkDB - the database for solid drives

%pre

%post

%preun

%postun

%clean

%files
%defattr(-,root,root)
/usr/bin/rethinkdb

%doc %attr(0444,root,root) /usr/share/man/man1/rethinkdb.1.gz

