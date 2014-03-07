# Note: A number of encodings are handled with purely algorithmic converters,
# without any mapping tables:
# US-ASCII, ISO 8859-1, UTF-7/8/16/32, SCSU

# Listed here:

# * ISO 8859-2..9,10,13,14,15,16
#   - 8859-11 table is not included. It's rather treated as a synonym of
#     Windows-874
# * Windows-125[0-8]
# * Simplified Chinese : GBK(Windows cp936), GB 18030
#   - GB2312 table was removed and 4 aliases for GB2312 were added
#     to GBK in convrtrs.txt to treat GB2312 as a synonym of GBK.
#   - GB-HZ is supported now that it uses the GBK table.
# * Traditional Chinese : Big5 (Windows cp950), Big5HKSCS (no PUA)
# * Japanese : SJIS (Windows cp932), EUC-JP (google-euc_jp_mod)
# * Korean : Windows-949
#  - EUC-KR table was removed. It's different from Windows-949, but
#    in practice EUC-KR and Windows-949 are treated synonymously.
#  - ISO-2022-KR is now supported by with a one-line change
#    in common/ucnv2022.c and other changes in convrtrs.txt to make it
#    use the windows-949 table.
# * Thai : Windows-874
#   - TIS-620 and ISO-8859-11 are treated as synonyms of Windows-874
#     although they're not the same.
# * Mac encodings : MacRoman, MacCyrillic
# * Cyrillic : KOI8-R, KOI8-U
# * WebKit wants PC-Arabic (DOS 720 = IBM-864)
# * Three 'fake' tables to prevent Webkit from falling back to the default
#   encoding when coming across ISO-2022-CN-(Ext).
#
# * Missing
#  - Armenian, Georgian  : extremly rare
#  - Mac encodings (other than Roman and Cyrillic) : extremly rare

UCM_SOURCE_FILES=

UCM_SOURCE_CORE=ibm-912_P100-1995.ucm ibm-913_P100-2000.ucm\
ibm-914_P100-1995.ucm ibm-915_P100-1995.ucm\
ibm-1089_P100-1995.ucm ibm-9005_X110-2007.ucm\
ibm-5012_P100-1999.ucm ibm-920_P100-1995.ucm\
iso-8859_10-1998.ucm\
ibm-921_P100-1995.ucm iso-8859_14-1998.ucm ibm-923_P100-1998.ucm\
iso-8859_16-2001.ucm\
ibm-5346_P100-1998.ucm ibm-5347_P100-1998.ucm ibm-5348_P100-1997.ucm\
ibm-5349_P100-1998.ucm ibm-5350_P100-1998.ucm ibm-9447_P100-2002.ucm\
ibm-9448_X100-2005.ucm ibm-9449_P100-2002.ucm ibm-5354_P100-1998.ucm\
windows-936-2000.ucm gb18030.ucm\
windows-950-2000.ucm ibm-1375_P100-2007.ucm\
ibm-943_P15A-2003.ucm google-euc_jp_mod.ucm\
windows-949-2000.ucm\
windows-874-2000.ucm ibm-874_P100-1995.ucm\
macos-0_2-10.2.ucm macos-7_3-10.2.ucm\
ibm-878_P100-1996.ucm ibm-1168_P100-2002.ucm\
ibm-864_X110-1999.ucm\
noop-cns-11643.ucm\
noop-gb2312_gl.ucm\
noop-iso-ir-165.ucm

# Do not build EBCDIC converters.
# ibm-37 and ibm-1047 are hardcoded in Makefile.in and
# they're removed by modifying the file. It's also hard-coded in makedata.mak for
# Winwodws, but we don't have to touch it because the data dll is generated out of
# icu*.dat file generated on Linux.
UCM_SOURCE_EBCDIC =
