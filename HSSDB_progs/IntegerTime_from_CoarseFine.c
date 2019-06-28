
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 9, UAH / NSSTC.

//  Calculates "unified GBM time" as a single integer from GBM Mission Elapsed
//  Time (MET), rather than two integers.  This combined time allows
//  simpler calculations and comparisons.  Using an integer instead of
//  floating point avoids the possibilty of rouding errors.
//  The units of the combined GBM time returned by this routine are GBM "ticks"
//  of 2 microseconds, i.e., the same units as GBM fine time.

//  The zero point of the output value is the GLAST Epoch of midnight, January 1, 2001 --
//  assuming that the GBM Time is set to SC Time via the PPS Sync mechanism,
//  and the SC Time is set correctly -- this routine merely converts the two GBM MET
//  counters into a single integer value.   There is NO leap second correction.

//  MET is a 4-byte coarse time with units of 0.1 s and
//  a 2-byte fine time with units of 2 microseconds.
//  To retain 2 microsec precision over 13.6 years (the roll-over period
//  of MET time), a long long unsigned integer is used -- this should be
//  able to retain full 2 microsecond resolution even when the Coarse
//  and Fine times are combined into a single quantity.

//  The advantage of using this peculiar time with 2 microsecond units is that
//  integer calculations have no risk of round off errors, etc.


#include "HSSDB_Progs_Header.h"


//   Both arguments are input, output is solely by function return.

uint64_t IntegerTime_from_CoarseFine (

   uint32_t   CoarseTime,
   uint16_t  FineTime

) {

   const uint64_t Ticks_per_Coarse = 50000;

   //  Multiply GBM coarse time by the number of 2 microsecond
   //  "ticks" per coarse time unit of 0.1 s.

   return ( Ticks_per_Coarse * CoarseTime + FineTime );

}  // IntegerTime_from_CoarseFine ()
