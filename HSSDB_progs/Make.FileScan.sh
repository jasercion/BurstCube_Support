
#  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.
#  rev.  2008 Oct 2
#  rev. 2010 May 22 -- more debug compile options.

gcc -O2  -std=c99  -pedantic  -Wall  -Wextra  \
   -Wwrite-strings  -Wcast-qual  -Wpointer-arith    \
   -Wstrict-prototypes  -Wmissing-prototypes  -Wconversion  -lm \
    MAIN_FileScan.c   ByteSwap.c   ReSync.c   \
    ProcessCSPEC.c   ProcessCTIME.c   ProcessTTE.c  \
    ReadPacket.c     Time_from_TTE_Data.c   \
    FloatTime_from_CoarseFine.c    \
    IntegerTime_from_CoarseFine.c  \
    GBM_MET_Time_to_JulianDay.c  JulianDay_to_Calendar_subr.c  \
    GBM_UnifiedTime_from_TTE_Data.c  \
    GBM_UnifiedTime_to_JulianDay.c  \
  -o FileScan.exe
