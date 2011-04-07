dnl Important: This is a debian control file template which is preprocessed by m4 from the Makefile.
dnl Please be gentle (and test your changes in all relevant configurations!).
dnl
dnl List of macros to be substituted (please update, if you're adding some):
dnl   SOLO - 0 or 1 (are we packaging RethinkDB Cache (0), or RethinkDB Solo (1)?)
dnl   RPM_PACKAGE_DIR - directory where RPM packaging is done
dnl   SERVER_EXEC_NAME - name of the executable
dnl   PACKAGE_NAME - full package name to be used (e.g. rethinkdb or rethinkdb-trial)
dnl   VANILLA_PACKAGE_NAME - package name without modifiers (e.g. rethinkdb, even when TRIAL=1)
dnl   TRIAL_PACKAGE_NAME - name of the trial package (e.g. rethinkdb-trial, even when TRIAL=0)
dnl   PACKAGE_VERSION - version string, dashes are fine
dnl   LEGACY_LINUX - 0 or 1 (specifies which library versions we can use)
dnl   TRIAL - 0 or 1
dnl
%define _topdir RPM_PACKAGE_DIR
%define serverexecname SERVER_EXEC_NAME
%define packagename PACKAGE_NAME
%define version patsubst(PACKAGE_VERSION, `-', `_')
%define buildroot %{_topdir}/BUILD

BuildRoot: %{buildroot}
Summary:   RethinkDB - the database for solid drives
Name:      %{packagename}
Version:   %{version}
Release:   1
License:   RethinkDB Beta Test License 0.1
Vendor:    Hexagram 49, Inc.
Packager:  Package Maintainer <support@rethinkdb.com>
URL:       http://rethinkdb.com/
Group:     Productivity/Databases/Servers
Provides:  memcached
Requires:  glibc >= 2.4-31, libaio >= 0.3.104
Conflicts: ifelse(TRIAL, 0, `TRIAL_PACKAGE_NAME', `VANILLA_PACKAGE_NAME')

%description
ifelse(SOLO,1,
`FIXME: Persistent storage engine using the memcache protocol
RethinkDB Solo is a key-value storage system designed for persistent
data.

It conforms to the memcache text protocol, so any memcached client
can have connectivity with it.',
`FIXME: Put RethinkDB Cache description here')

%pre

%post

%preun

%postun

%clean

%files
%defattr(-,root,root)
%attr(0755,root,root) /usr/bin/%{serverexecname}

%attr(0444,root,root) /etc/bash_completion.d/rethinkdb
%doc %attr(0444,root,root) /usr/share/man/man1/rethinkdb.1.gz
%doc %attr(0444,root,root) /usr/share/doc/%{packagename}/copyright

