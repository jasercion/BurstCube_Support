
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


#include <stdio.h>

#include "HSSDB_Progs_Header.h"

int main ( void ) {

   uint64_t  GBM_UnifiedTime;
   long double  JulianDayNumber;

   int32_t y, m, day, hours, min;
   long double sec;

   printf ( "\nInput GBM Unified Time: " );
   scanf ( "%llu", &GBM_UnifiedTime );



   JulianDayNumber = GBM_UnifiedTime_to_JulianDay ( GBM_UnifiedTime );

   JulianDay_to_Calendar_subr ( JulianDayNumber, &y, &m, &day, &hours, &min, &sec );

   printf ( "\n\n%4ld-%2ld-%2ld at %2ld:%2ld:%9.6Lf\n", y, m, day, hours, min, sec );

   return 0;

}  // main ()
