# *   Copyright (C) 1998-2010, International Business Machines
# *   Corporation and others.  All Rights Reserved.
GENRB_CLDR_VERSION = 1.9
# A list of txt's to build
# The downstream packager may not need this file at all if their package is not
# constrained by
# the size (and/or their target OS already has ICU with the full locale data.)
#
# Listed below are locale data files necessary for 40 + 1 + 8 languages Chrome
# is localized to.
#
# In addition to them, 40+ "abridged" locale data files are listed. Chrome is
# localized to them, but
# uses a few categories of data in those locales for IDN handling and language
# name listing (in the UI).
# Aliases which do not have a corresponding xx.xml file (see icu-config.xml &
# build.xml)
GENRB_SYNTHETIC_ALIAS =

# All aliases (to not be included under 'installed'), but not including root.
GENRB_ALIAS_SOURCE = $(GENRB_SYNTHETIC_ALIAS)\
 zh_CN.txt zh_TW.txt zh_HK.txt zh_SG.txt\
 no.txt in.txt iw.txt

# Ordinary resources
GENRB_SOURCE =\
 am.txt\
 ar.txt\
 bg.txt\
 bn.txt\
 ca.txt\
 cs.txt\
 da.txt\
 de.txt\
 el.txt\
 en.txt en_GB.txt en_US.txt\
 es.txt es_ES.txt es_419.txt\
 es_AR.txt es_CO.txt es_EC.txt es_HN.txt es_PA.txt es_PY.txt es_UY.txt\
 es_BO.txt es_CR.txt es_MX.txt es_PE.txt es_SV.txt es_VE.txt es_CL.txt\
 es_DO.txt es_GT.txt es_NI.txt es_PR.txt es_US.txt\
 et.txt\
 fa.txt\
 fi.txt\
 fil.txt\
 fr.txt\
 gu.txt\
 he.txt\
 hi.txt\
 hr.txt\
 hu.txt\
 id.txt\
 it.txt\
 ja.txt\
 kn.txt\
 ko.txt\
 lt.txt\
 lv.txt\
 ml.txt\
 mr.txt\
 nb.txt\
 nl.txt\
 pl.txt\
 pt.txt pt_BR.txt pt_PT.txt\
 ro.txt\
 ru.txt\
 sk.txt\
 sl.txt\
 sr.txt\
 sv.txt\
 sw.txt\
 ta.txt\
 te.txt\
 th.txt\
 tr.txt\
 uk.txt\
 vi.txt\
 zh.txt zh_Hans.txt zh_Hans_CN.txt zh_Hans_SG.txt\
 zh_Hant.txt zh_Hant_TW.txt zh_Hant_HK.txt\
 af.txt\
 ak.txt\
 az.txt\
 bem.txt\
 be.txt\
 bs.txt\
 cy.txt\
 eo.txt\
 eu.txt\
 fo.txt\
 ga.txt\
 gl.txt\
 ha.txt\
 haw.txt\
 hy.txt\
 ig.txt\
 is.txt\
 ka.txt\
 kk.txt\
 km.txt\
 lg.txt\
 mfe.txt\
 mg.txt\
 mk.txt\
 mo.txt\
 ms.txt\
 mt.txt\
 ne.txt\
 nn.txt\
 nyn.txt\
 om.txt\
 or.txt\
 pa.txt\
 ps.txt\
 rm.txt\
 rw.txt\
 si.txt\
 sn.txt\
 so.txt\
 sq.txt\
 ti.txt\
 tl.txt\
 to.txt\
 ur.txt\
 uz.txt\
 yo.txt\
 zu.txt
