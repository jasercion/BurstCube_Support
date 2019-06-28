
#  Michael S. Briggs, 2007 Sept 18 -- Oct 9, UAH / NSSTC.
#  rev. 2010 May 22 -- more debug compile options.

gcc-mp-7  -O2  -std=c99  -pedantic  -Wall  -Wextra  \
   -Wwrite-strings  -Wcast-qual  -Wpointer-arith    \
   -Wstrict-prototypes  -Wmissing-prototypes  -Wconversion  \
    MAIN_Extract_TTE.c   ByteSwap.c   ReSync.c   \
    Extract_TTE_1packet.c  \
    ReadPacket.c   Output_TTE.c  \
    FloatTime_from_CoarseFine.c   \
    IntegerTime_from_CoarseFine.c  \
    GBM_MET_Time_to_JulianDay.c  JulianDay_to_Calendar_subr.c  \
  -o Extract_TTE.exe
