
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC
//  Revised 2007 Sept 28 -- Oct 10.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

//   >>>>   ASSUMPTIONS  -- could define types, and test the assumptions .... <<<<

//  short (unsigned) int is 2 bytes,
//  long (unsigned) int to be 4 bytes,
//  long long (unsigned) int is 8 bytes


//   >>>>  CONSTANTS, SYMBOLS   <<<<

#define  OK    0
#define  FAIL  1


#define  NUM_TIME_CHAN    8U
#define  NUM_SPEC_CHAN  128U
#define  NUM_DET         14U
#define  NUM_NAI_DET     12U


//    Must be divisble by 4:
#define  MAX_PACKET_ARRAY_BYTES    10000U


//  Really 4 are known, but they are numbered 1 to 4, so we are
//  skipping array element 0:
//   2014 April 17: now 5: 0 not used, 1 to 5.  Which is 6 elements.
#define  NUM_ERROR_TYPES   6U


//   Largest number of TTE words in a packet is 1020, based upon the largest
//   argument to det_tte_threshold, 0xFE, setting threshold of 4080 bytes.
//   See the SwRI SUM, Table 188.
//   i_tte=1022 was seen in data, which means 1023 elements since counting from zero,
//   so increase from 1022 to 1024 on 2008 July 11.   File GLAST_2008192_043300_VC09_GBTTE
//   Previously increased from 1020 to 1022 on 2008 July 9.
#define  MAX_TTE_WORDS_PER_PACKET  1024U



//   >>>>   TYPEDEFS   <<<<


// This enum variable is used to index arrays that record information
// about specific datatypes:
typedef enum { CSPEC, CTIME, TTE, BAD } DataType_type;


typedef  struct  DetectorFile_type {
   char * DetectorFileName_ptr;        // name of the output file
   FILE * ptr_to_DetectorFile;         // pointer to the file
   _Bool  DetectorFile_Opened;  // whether the file has been created
} DetectorFile_type;



//  No need to include detector number since each detector is written to
//  a dedicated file or held in an array indexed by detector number.

typedef  struct   TTE_Data_type {
   uint32_t CoarseTime;
   uint16_t FineTime;
   uint16_t SpecChannel;
} TTE_Data_type;


//  re choice of time units.  combining into one variable is
//  more convenient for science.  The two variables are more
//  native and better for debugging.
typedef struct   Processed_TTE_type {
   uint32_t CoarseTime;
   uint16_t FineTime;
   uint16_t  Detector;
   uint16_t  SpecChannel;
   uint16_t PADDING;   // required for compatibility with other languages
   //uint64_t  Time_in_OneVariable;
}  Processed_TTE_type;




//   >>>>   GLOBAL VARIABLES   <<<<



//   >>>>   SUBROUTINE & FUNCTION PROTOTYPES   <<<<

long long unsigned int GBM_UnifiedTime_from_TTE_Data (

   uint32_t HeaderCoarseTime,
   uint32_t TTE_TimeWord,
   uint32_t TTE_DataWord

);


long double GBM_UnifiedTime_to_JulianDay (

   long long unsigned int GBM_UnifiedTime

);


long double GBM_MET_Time_to_JulianDay (

   uint32_t  GBM_Coarse_Time,
   uint16_t  GBM_Fine_Time

);



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

);


_Bool  ReSync (
   FILE * file_pointer,
   uint32_t * ExcessBytes,
   uint32_t * ExcessNonZeroCnt
);


//  The argument is solely input; output is solely by function return:
uint16_t  ByteSwap_2  ( uint16_t  Word_2 );

//  The argument is solely input; output is solely by function return:
uint32_t  ByteSwap_4  ( unsigned  int  Word_4 );


uint32_t  ReadPacket (

// input arguments:
   FILE * ptr_to_InputFile,
   FILE * ptr_to_SummaryFile,
   _Bool  VerboseFlag,

// output arguments:
   DataType_type  * DataType_ptr,
   uint32_t CountByAPID [],
   uint16_t * PacketDataLength_ptr,
   uint16_t PacketData [],
   uint32_t * HeaderCoarseTime_ptr,
   uint16_t * HeaderFineTime_ptr,
   uint16_t * SequenceCount_ptr

);


void ProcessCSPEC (
   uint16_t PacketData [],
   uint16_t PacketDataLength,
   FILE * ptr_to_SummaryFile
);


void ProcessCTIME (
   uint16_t PacketData [],
   uint16_t PacketDataLength,
   FILE * ptr_to_SummaryFile
);


void ProcessTTE (    // All arguments are input; output by writing to standard out & the 2 files.

   uint32_t PacketData [],
   uint16_t PacketDataLength,
   uint32_t HeaderCoarseTime,
   uint16_t HeaderFineTime,
   uint16_t SequenceCount,
   FILE * ptr_to_SummaryFile,
   FILE * ptr_to_TTE_File

);


  // Both arguments are input, output solely by function return.
long double FloatTime_from_CoarseFine (

   uint32_t   CoarseTime,
   uint16_t  FineTime

);


  // Both arguments are input, output solely by function return.
uint64_t IntegerTime_from_CoarseFine (

   uint32_t   CoarseTime,
   uint16_t  FineTime

);


long double  Time_from_TTE_Data (
   uint32_t HeaderCoarseTime,
   uint32_t TTE_TimeWord,
   uint32_t TTE_DataWord
);


void Extract_TTE_1packet (

   // Input arguments:
   uint32_t PacketData [],
   uint16_t PacketDataLength,
   uint32_t HeaderCoarseTime,
   FILE * ptr_to_SummaryFile,

   // Input/Ouput arguments:

   uint32_t  ErrorCounts [NUM_ERROR_TYPES],

   // Output arguments:
   uint16_t * TTE_DataWords_Output_ptr,
   uint32_t TTE_CoarseTime [],
   uint16_t TTE_FineTime [],
   uint16_t TTE_Channel [],
   uint16_t TTE_Detector []
);


void Output_TTE (

   // Input arguments:
   uint16_t Number_TTE_DataWords,
   uint32_t TTE_CoarseTime [],
   uint16_t TTE_FineTime [],
   uint16_t TTE_Channel [],
   uint16_t TTE_Detector [],
   char * Analysis_FileName_ptr,
   FILE * ptr_to_AnalysisFile,
   FILE * ptr_to_SummaryFile

);
