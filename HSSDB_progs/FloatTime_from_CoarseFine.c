
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.

//  Calculates time as a floating point number from GBM Mission Elapsed
//  Time (MET).   Units of the output value are seconds, and the zero point of
//  the output value is the GLAST Epoch of midnight, January 1, 2001 --
//  assuming that the GBM Time is set to SC Time via the PPS Sync mechanism,
//  and the SC Time is set correctly -- this routine merely converts the two GBM MET
//  counters into a single floating point value.

//  MET is a 4-byte coarse time with units of 0.1 s and
//  a 2-byte fine time with units of 2 microseconds.
//  To retain 2 microsec precision over 13.6 years (the roll-over period
//  of MET time), a long double is used for the floating point result.
//  (P.S.  Precision issue not carefully tested.)


#include "HSSDB_Progs_Header.h"


//   Both arguments are input, output is solely by function return.

long double FloatTime_from_CoarseFine (

   uint32_t   CoarseTime,
   uint16_t  FineTime

) {

   const long double FactorOne = 0.1;
   const long double FactorTwo = 2.E-6;

   return ( FactorOne * CoarseTime + FactorTwo * FineTime );

}  // FloatTime_from_CoarseFine ()
