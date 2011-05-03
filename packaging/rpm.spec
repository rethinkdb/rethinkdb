dnl Important: This is a debian control file template which is preprocessed by m4 from the Makefile.
dnl Please be gentle (and test your changes in all relevant configurations!).
dnl
dnl List of macros to be substituted (please update, if you're adding some):
dnl   SOLO - 0 or 1 (are we packaging RethinkDB Cache (0), or RethinkDB Solo (1)?)
dnl   RPM_PACKAGE_DIR - directory where RPM packaging is done
dnl   SERVER_EXEC_NAME - name of the executable (the one that should be used as the link in the alternatives system)
dnl   SERVER_EXEC_NAME_VERSIONED - name of the executable with version at the end (the actual binary name)
dnl   PACKAGE_NAME - full package name to be used (e.g. rethinkdb or rethinkdb-trial)
dnl   VANILLA_PACKAGE_NAME - package name without modifiers (e.g. rethinkdb, even when TRIAL=1)
dnl   TRIAL_PACKAGE_NAME - name of the trial package (e.g. rethinkdb-trial, even when TRIAL=0)
dnl   PACKAGE_VERSION - version string, dashes are fine
dnl   PACKAGE_FOR_SUSE_10 - 0 or 1 (specifies which library versions we can use)
dnl   LEGACY_LINUX - 0 or 1 (specifies which library versions we can use)
dnl   TRIAL - 0 or 1
dnl
%define _topdir RPM_PACKAGE_DIR
%define packagename PACKAGE_NAME
%define vanilla_packagename PACKAGE_NAME
%define versioned_packagename VERSIONED_PACKAGE_NAME
%define versioned_trial_packagename VERSIONED_TRIAL_PACKAGE_NAME
%define server_exec_name SERVER_EXEC_NAME
%define server_exec_name_versioned SERVER_EXEC_NAME_VERSIONED
%define version patsubst(PACKAGE_VERSION, `-', `_')
%define buildroot %{_topdir}/BUILD

%define bin_dir BIN_DIR
%define doc_dir DOC_DIR
%define man1_dir MAN1_DIR
%define full_server_exec_name %{bin_dir}/%{server_exec_name}
%define full_server_exec_name_versioned %{bin_dir}/%{server_exec_name_versioned}
%define bash_completion_dir BASH_COMPLETION_DIR
%define internal_bash_completion_dir INTERNAL_BASH_COMPLETION_DIR
%define priority PRIORITY

BuildRoot: %{buildroot}
Summary:   RethinkDB - the database for solid-state drives
Name:      %{versioned_packagename}
Version:   %{version}
Release:   1
License:   RethinkDB Beta Test License 0.1
Vendor:    Hexagram 49, Inc.
Packager:  Package Maintainer <support@rethinkdb.com>
URL:       http://rethinkdb.com/
Group:     Productivity/Databases/Servers
Provides:  %{vanilla_packagename}, memcached
Requires:  ifelse(PACKAGE_FOR_SUSE_10, 1,
  `glibc >= 2.4-31, libaio >= 0.3.104, update-alternatives',
  LEGACY_LINUX, 1,
  `glibc >= 2.5, libaio >= 0.3.106, chkconfig >= 1.3.30.2',
  `glibc >= 2.10.1, libaio >= 0.3.106, chkconfig >= 1.3.30.2')
Conflicts: ifelse(TRIAL, 0, `%{versioned_trial_packagename} <= %{version}')


%description
ifelse(SOLO,1,
`RethinkDB is a key-value storage system designed for persistent
data.

It conforms to the memcache text protocol, so any memcached client
can have connectivity with it.',
`FIXME: Put RethinkDB Cache description here')

%pre

%post
# find update-alternatives
ALTERNATIVES=""
for alt in update-alternatives alternatives; do
  for dir in /usr/sbin /sbin; do
    full_alt="$dir/$alt"
    if [ -x "$full_alt" ]; then
      ALTERNATIVES="$full_alt"
      break 2
    fi
  done
done

# not all systems have alternatives, so we use them when we can, or do something simple and reasonable if not
if [ -n "$ALTERNATIVES" ]; then
  $ALTERNATIVES \
    --install %{full_server_exec_name} %{server_exec_name} %{full_server_exec_name_versioned} %{priority} \
    --slave %{man1_dir}/%{server_exec_name}.1.gz %{server_exec_name}.1.gz %{man1_dir}/%{server_exec_name_versioned}.1.gz \
    --slave %{bash_completion_dir}/%{server_exec_name}.bash %{server_exec_name}.bash %{internal_bash_completion_dir}/%{server_exec_name_versioned}.bash
else
  while read link path; do
    if [ ! \( -e "$link" -o -h "$link" \) ]; then
      ln -vs "$path" "$link"
    fi
  done << END
%{full_server_exec_name} %{full_server_exec_name_versioned}
%{man1_dir}/%{server_exec_name}.1.gz %{man1_dir}/%{server_exec_name_versioned}.1.gz
%{bash_completion_dir}/%{server_exec_name}.bash %{internal_bash_completion_dir}/%{server_exec_name_versioned}.bash
END
fi

%preun
# find update-alternatives
ALTERNATIVES=""
for alt in update-alternatives alternatives; do
  for dir in /usr/sbin /sbin; do
    full_alt="$dir/$alt"
    if [ -x "$full_alt" ]; then
      ALTERNATIVES="$full_alt"
      break 2
    fi
  done
done

# even if alternatives are installed it's still possible that we were installed without them, so we should be able to fallback onto the crude system
if [ -n "$ALTERNATIVES"  -a -h %{full_server_exec_name} -a "$(readlink %{full_server_exec_name})" != "%{full_server_exec_name_versioned}" ]; then
  $ALTERNATIVES --remove %{server_exec_name} %{full_server_exec_name_versioned}
else
  while read link path; do
    if [ -h "$link" -a "$(readlink $link)" = "$path" ]; then
      rm -v "$link"
    fi
  done << END
%{full_server_exec_name} %{full_server_exec_name_versioned}
%{man1_dir}/%{server_exec_name}.1.gz %{man1_dir}/%{server_exec_name_versioned}.1.gz
%{bash_completion_dir}/%{server_exec_name}.bash %{internal_bash_completion_dir}/%{server_exec_name_versioned}.bash
END
fi

%postun

%clean

%files
%defattr(-,root,root)
%attr(0755,root,root) %{full_server_exec_name_versioned}

%dir %attr(0755,root,root) %{bash_completion_dir}
%dir %attr(0755,root,root) %{internal_bash_completion_dir}
%attr(0444,root,root) %{internal_bash_completion_dir}/%{server_exec_name}.bash

%doc %attr(0444,root,root) %{man1_dir}/%{server_exec_name_versioned}.1.gz

%dir %attr(0755,root,root) %{doc_dir}
%doc %attr(0444,root,root) %{doc_dir}/copyright

