dnl  Copyright (C) 2009 Sun Microsystems
dnl This file is free software; Sun Microsystems
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([PANDORA_VERSION],[

  PANDORA_HEX_VERSION=`echo $VERSION | sed 's|[\-a-z0-9]*$||' | \
    awk -F. '{printf "0x%0.2d%0.3d%0.3d", $[]1, $[]2, $[]3}'`
  AC_SUBST([PANDORA_HEX_VERSION])
])
