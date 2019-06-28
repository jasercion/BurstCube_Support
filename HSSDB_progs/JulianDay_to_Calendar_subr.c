
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


// ??? precision is probably good enough....
//  More testing?    Range checking on input?

//  Subroutine to convert Julian day number to Calendar date and
//  time of day: hours, minutes and seconds.   Seconds are floating
//  point (specifically double).

//  Converted to subroutine by Michael S. Briggs, UAH/NSSTC, 2008 July 9.
//  Previously was a standalone program that prompted the user for input,
//  and wrote the result to standard output (screen).


//  Original program: jd_to_cal.c: Michael S. Briggs, 2004 Feb 17.

//  From the algorithm on page 8 of Practical Astronomy with Your Calculator,
//  3rd ed., Peter Duffett-Smith.

//  test: 2446113.75 gives 1985 Feb 17.25

//  Note: Julian days being at 12h 00m UT.
//  Output day is UT.


#include "HSSDB_Progs_Header.h"

#include <math.h>

void JulianDay_to_Calendar_subr (

   // Input argument:

   long double  jd,

   // Output arguments:

   int32_t *  y_ptr,
   int32_t *  m_ptr,
   int32_t *  Day_ptr,
   int32_t *  Hours_ptr,
   int32_t *  Minutes_ptr,
   long double *  seconds_ptr

) {

   //  Difference between "*Day_ptr" and "d": both are time in days, but d is
   //  floating point (specifically double) and has fraction of day,
   //  while *Day_ptr is an integer, removing any "remainder".

   long double  jd_2;
   int32_t  I;
   long double  F;
   int32_t  A;
   int32_t  B;
   int32_t  C;
   int32_t  D;
   int32_t  E;
   double  d;
   int32_t  G;


   //  *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***


   //  1)
   jd_2 = jd + 0.5L;
   I = (int32_t) jd_2;
   F = jd_2 - I;

   //  2)
   if ( I > 2299160 ) {
      A =  (int32_t) ( ( I - 1867216.25L ) / 36524.25L );
      B = I + 1 + A - A / 4;
   } else {
      A = 0;  // not used
      B = I;
   }

   //  3)
   C = B + 1524;

   //  4)
   D = (int32_t) ( ( C - 122.1L ) / 365.25L );

   //  5)
   E = (int32_t) ( 365.25L * D );

   //  6)
   G = (int32_t) ( ( C - E ) / 30.6001L );

   //  7)
   d = (double) ( C - E + F - floorl (30.6001L * G ) );

   //  8)
   if ( G <= 13 ) {
      *m_ptr = G - 1;
   } else {
      *m_ptr = G - 13;
   }

   //  9)
   if ( *m_ptr >= 3 ) {
      *y_ptr = D - 4716;
   } else {
      *y_ptr = D - 4715;
   }


   *Day_ptr = (int32_t) d;
   *seconds_ptr = 86400.0L * (d - *Day_ptr);
   *Hours_ptr = (int32_t) ( *seconds_ptr / 3600.0L );
   *seconds_ptr = *seconds_ptr - *Hours_ptr * 3600.0L;
   *Minutes_ptr = (int32_t) ( *seconds_ptr / 60.0L );
   *seconds_ptr = *seconds_ptr - *Minutes_ptr * 60.0L;


   return;

}  // JulianDay_to_Calendar_subr ()
