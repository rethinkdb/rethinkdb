%define _topdir TOPDIR
%define buildroot %{_topdir}/BUILD

BuildRoot: %{buildroot}
Summary:   RethinkDB - the database for solid drives
Name:      rethinkdb
Version:   VERSION
Release:   1
License:   RethinkDB Beta Test License 0.1
Vendor:    Hexagram 49, Inc.
Packager:  Package Maintainer <package-maintainer@mail.rethinkdb.com>
URL:       http://rethinkdb.com/
Group:     Productivity/Databases/Servers
Provides:  memcached
Requires:  glibc >= 2.5, google-perftools >= 1.6, libaio >= 0.3.106 

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

%attr(0444,root,root) /etc/bash_completion.d/rethinkdb
%doc %attr(0444,root,root) /usr/share/man/man1/rethinkdb.1.gz
%doc %attr(0444,root,root) /usr/share/doc/rethinkdb/copyright

