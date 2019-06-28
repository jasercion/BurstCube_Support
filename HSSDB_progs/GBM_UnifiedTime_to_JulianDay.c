
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//   ???  leap second correction of -1 is only correct to 2008 Dec 31.

#include "HSSDB_Progs_Header.h"

//  For the GLAST Epoch and its conversion to Julian Day number, see
//  my memo RollOver_Calc.txt of 2004 Feb 17 -- 19.

#define  JD_OF_GLAST_EPOCH    2451910.5L


//  All arguments are input.   Output is via function return.

long double GBM_UnifiedTime_to_JulianDay (

   long long unsigned int  GBM_UnifiedTime

) {

   long double  Time_in_sec;
   long double  Time_in_days;

   const  long double  Fine_convert = 2.0E-6;
   const  long double  secs_in_day = 86400;


   Time_in_sec = GBM_UnifiedTime * Fine_convert;

   Time_in_sec -= 1.0L;    //  leap seconds

   //printf ( "%Lf\n", Time_in_sec );

   Time_in_days = Time_in_sec / secs_in_day;

   //printf ( "%Lf\n", Time_in_days + JD_OF_GLAST_EPOCH );

   return ( Time_in_days + JD_OF_GLAST_EPOCH );

}  // GBM_UnifiedTime_to_JulianDay ()
