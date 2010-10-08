dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_TEST_VC_DIR],[
  pandora_building_from_vc=no

  if test -d ".bzr" ; then
    pandora_building_from_bzr=yes
    pandora_building_from_vc=yes
  else
    pandora_building_from_bzr=no
  fi

  if test -d ".svn" ; then
    pandora_building_from_svn=yes
    pandora_building_from_vc=yes
  else
    pandora_building_from_svn=no
  fi

  if test -d ".hg" ; then
    pandora_building_from_hg=yes
    pandora_building_from_vc=yes
  else
    pandora_building_from_hg=no
  fi

  if test -d ".git" ; then
    pandora_building_from_git=yes
    pandora_building_from_vc=yes
  else
    pandora_building_from_git=no
  fi
])

AC_DEFUN([PANDORA_BUILDING_FROM_VC],[
  m4_syscmd(PANDORA_TEST_VC_DIR [

    PANDORA_RELEASE_DATE=`date +%Y.%m`
    PANDORA_RELEASE_NODOTS_DATE=`date +%Y%m`

    # Set some defaults
    PANDORA_VC_REVNO="0"
    PANDORA_VC_REVID="unknown"
    PANDORA_VC_BRANCH="bzr-export"

    if test "${pandora_building_from_bzr}" = "yes"; then
      echo "# Grabbing changelog and version information from bzr"
      PANDORA_BZR_REVNO=`bzr revno`
      if test "x$PANDORA_BZR_REVNO" != "x${PANDORA_VC_REVNO}" ; then
        PANDORA_VC_REVNO="${PANDORA_BZR_REVNO}"
        PANDORA_VC_REVID=`bzr log -r-1 --show-ids | grep revision-id | cut -f2 -d' ' | head -1`
        PANDORA_VC_BRANCH=`bzr nick`
      fi
    fi

    if ! test -d config ; then
      mkdir -p config
    fi

    if test "${pandora_building_from_bzr}" = "yes" -o ! -f config/pandora_vc_revinfo ; then 
      cat > config/pandora_vc_revinfo.tmp <<EOF
PANDORA_VC_REVNO=${PANDORA_VC_REVNO}
PANDORA_VC_REVID=${PANDORA_VC_REVID}
PANDORA_VC_BRANCH=${PANDORA_VC_BRANCH}
PANDORA_RELEASE_DATE=${PANDORA_RELEASE_DATE}
PANDORA_RELEASE_NODOTS_DATE=${PANDORA_RELEASE_NODOTS_DATE}
EOF
      if ! diff config/pandora_vc_revinfo.tmp config/pandora_vc_revinfo >/dev/null 2>&1 ; then
        mv config/pandora_vc_revinfo.tmp config/pandora_vc_revinfo
      fi
      rm -f config/pandora_vc_revinfo.tmp
    fi
  ])
])
  
AC_DEFUN([_PANDORA_READ_FROM_FILE],[
  $1=`grep $1 $2 | cut -f2 -d=`
])

AC_DEFUN([PANDORA_VC_VERSION],[
  AC_REQUIRE([PANDORA_BUILDING_FROM_VC])

  PANDORA_TEST_VC_DIR

  AS_IF([test -f ${srcdir}/config/pandora_vc_revinfo],[
    _PANDORA_READ_FROM_FILE([PANDORA_VC_REVNO],${srcdir}/config/pandora_vc_revinfo)
    _PANDORA_READ_FROM_FILE([PANDORA_VC_REVID],${srcdir}/config/pandora_vc_revinfo)
    _PANDORA_READ_FROM_FILE([PANDORA_VC_BRANCH],
                            ${srcdir}/config/pandora_vc_revinfo)
    _PANDORA_READ_FROM_FILE([PANDORA_RELEASE_DATE],
                            ${srcdir}/config/pandora_vc_revinfo)
    _PANDORA_READ_FROM_FILE([PANDORA_RELEASE_NODOTS_DATE],
                            ${srcdir}/config/pandora_vc_revinfo)
  ])
  AS_IF([test "x${PANDORA_VC_BRANCH}" != x"${PACKAGE}"],[
    PANDORA_RELEASE_COMMENT="${PANDORA_VC_BRANCH}"
  ],[
    PANDORA_RELEASE_COMMENT="trunk"
  ])
    
  PANDORA_RELEASE_VERSION="${PANDORA_RELEASE_DATE}.${PANDORA_VC_REVNO}"
  PANDORA_RELEASE_ID="${PANDORA_RELEASE_NODOTS_DATE}${PANDORA_VC_REVNO}"

  VERSION="${PANDORA_RELEASE_VERSION}"
  AC_DEFINE_UNQUOTED([PANDORA_RELEASE_VERSION],["${PANDORA_RELEASE_VERSION}"],
                     [The real version of the software])
  AC_SUBST(PANDORA_VC_REVNO)
  AC_SUBST(PANDORA_VC_REVID)
  AC_SUBST(PANDORA_VC_BRANCH)
  AC_SUBST(PANDORA_RELEASE_DATE)
  AC_SUBST(PANDORA_RELEASE_NODOTS_DATE)
  AC_SUBST(PANDORA_RELEASE_COMMENT)
  AC_SUBST(PANDORA_RELEASE_VERSION)
  AC_SUBST(PANDORA_RELEASE_ID)
])

AC_DEFUN([PANDORA_VC_INFO_HEADER],[
  AC_REQUIRE([PANDORA_VC_VERSION])
  m4_define([PANDORA_VC_PREFIX],m4_toupper(m4_normalize(AC_PACKAGE_NAME))[_])

  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[VC_REVNO], [$PANDORA_VC_REVNO], [Version control revision number])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[VC_REVID], ["$PANDORA_VC_REVID"], [Version control revision ID])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[VC_BRANCH], ["$PANDORA_VC_BRANCH"], [Version control branch name])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[RELEASE_DATE], ["$PANDORA_RELEASE_DATE"], [Release date of version control checkout])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[RELEASE_NODOTS_DATE], [$PANDORA_RELEASE_NODOTS_DATE], [Numeric formatted release date of checkout])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[RELEASE_COMMENT], ["$PANDORA_RELEASE_COMMENT"], [Set to trunk if the branch is the main $PACKAGE branch])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[RELEASE_VERSION], ["$PANDORA_RELEASE_VERSION"], [Release date and revision number of checkout])
  AC_DEFINE_UNQUOTED(PANDORA_VC_PREFIX[RELEASE_ID], [$PANDORA_RELEASE_ID], [Numeric formatted release date and revision number of checkout])
])
