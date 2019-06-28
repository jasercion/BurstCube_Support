
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.
//  Some revisions 2007 September 18.


//  Reads in one packet of GBM Science Data and returns the unprocessed packet data, but not
//  the header itself.   Returns some processed information from the header.  Also tests some
//  of the information in the header, e.g., tests for skipped sequence numbers.
//  Optional output (VerboseFlag) to summary file.

//  Before calling this routine, the "read" location must already be correctly positioned
//  before the packet by having read the synchronization word.

//  The data fields of CSPEC and CTIME packets are natively composed of two-byte little-endian words,
//  while the data field of TTE packets is natively composed of four-byte big endian words.
//  Because we read the data in words, and the Intel processors are little-endian, the
//  4-byte TTE words will need to be byte-swapped by the routine that uses them!
//  The other two datatypes don't need to be byte-swapped since they are the correct
//  endianess for the Intel processor.



#include "HSSDB_Progs_Header.h"

#include <limits.h>



//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***

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

) {


   //   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***

   size_t num_read;
   uint16_t Word2;
   uint32_t Word4;
   uint16_t apid;
   uint32_t  Bytes_per_Word;

   uint16_t HeaderPacketLength;

   long double TimeSeconds;

   long double  JulianDay;
   int32_t  year, month, day, hours, minutes;
   long double  seconds;


   //  This two array is indexed by the enum type DataType_type:
   static long double PrevHeaderTime [4]  = { -999.99, -999.99, -999.99, -999.99 };

   size_t  Words2Read;

   _Bool  SequenceCountGood;



   //  static to remember values across calls:
   //  Since the sequence count field in the packet is 14 bits, UINT16_MAX is an illegal value that indicates
   //  that this variable hasn't been initialized.

   static uint16_t LastSequenceCount [4] = { UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX };;



   //   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


   // *** The first word of packet should contain the APID:

   num_read = fread ( &Word2, 2, 1, ptr_to_InputFile );

   if ( feof (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n BAD EOF trying to read APID !\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n File Error reading APID !\n" );
   if ( num_read  !=  1 ) {
      fprintf ( ptr_to_SummaryFile, "\n*** ERROR: Wrong number of items reading APID!\n" );
      return FAIL;
   }

   //  The least-significant 11 bits of this 2-byte word
   //  should be the APID.   Use the APID to identify the DataType and
   //  increment the count of packets by DataType:



   apid = ByteSwap_2 ( Word2 )  &  0x7FF;

   if ( VerboseFlag )  fprintf ( ptr_to_SummaryFile, "\n\n ***********  APID  =  0x%X  =  %d  =  ", apid, apid );

   switch ( apid ) {

      case 0x5A2:
         *DataType_ptr = TTE;
         if ( VerboseFlag )  fprintf ( ptr_to_SummaryFile, " TTE\n" );
      break;

      case 0x5A0:
         *DataType_ptr = CSPEC;
         if ( VerboseFlag )  fprintf ( ptr_to_SummaryFile, "CSPEC\n" );
      break;

      case 0x5A1:
         *DataType_ptr = CTIME;
         if ( VerboseFlag )  fprintf ( ptr_to_SummaryFile, "CTIME\n" );
      break;

      default:
         *DataType_ptr = BAD;
         fprintf ( ptr_to_SummaryFile, " Unrecognized Datatype !!!!!!!!!! \n" );
      break;

   }  // switch on apid

   CountByAPID [ *DataType_ptr ] ++ ;



   // *** The least significant 14 bits of the next 2-byte word is the
   //     sequence count:

   num_read = fread ( SequenceCount_ptr, 2, 1, ptr_to_InputFile );

   if ( feof (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n BAD EOF reading Sequence Count !\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n File Error reading Sequence Count !\n" );
   if ( num_read  !=  1 ) {
      fprintf (ptr_to_SummaryFile, "\n*** ERROR: Wrong number of items reading Sequence Count!\n");
      return FAIL;
   }

   *SequenceCount_ptr = ByteSwap_2 ( *SequenceCount_ptr )   &  0x3FFF;

   if ( VerboseFlag )  fprintf ( ptr_to_SummaryFile, "Sequence Count = %6u,   ", *SequenceCount_ptr );



   //   Test for a gap in the Sequence Count, by data type:
   //   There are two allowed cases:
   //   1) normal: current sequence count is 1 higher than previous,
   ///  2) roll-over: previous sequence count is highest allowed value: 14 bits = 0x3FFF = 16383,
   //   current sequence count is zero.
   //   (But only test if we already have a Sequence Count for this data type,
   //    as indicated by LastSequenceCount not having the illegal value UINT16_MAX.
   //    UINT16_MAX is impossible since the packet field is 14 bits.)

   if ( LastSequenceCount [ *DataType_ptr ] != UINT16_MAX ) {   // LastSequenceCount initialized ?

      SequenceCountGood = false;

      if ( LastSequenceCount [ *DataType_ptr ] + 1  ==  *SequenceCount_ptr )  SequenceCountGood = true;
      if ( LastSequenceCount [ *DataType_ptr ] == 0x3FFF  &&  *SequenceCount_ptr == 0 )  SequenceCountGood = true;

      if ( ! SequenceCountGood ) {  // sequence count gap ?

         fprintf ( ptr_to_SummaryFile, "\n\n*** *** Skip in packet sequence counter: from %u to %u for datatype ",
              LastSequenceCount [ *DataType_ptr ], *SequenceCount_ptr );
         printf ( "\n\n***Skip in packet sequence counter -- see Summary File for more info !!\n\n" );

         switch ( *DataType_ptr ) {  // data type msg

            case TTE:
               fprintf ( ptr_to_SummaryFile, "TTE\n\n" );
            break;

            case CSPEC:
               fprintf ( ptr_to_SummaryFile, "CSPEC\n\n" );
            break;

            case CTIME:
               fprintf ( ptr_to_SummaryFile, "CTIME\n\n" );
            break;

            default:
               fprintf ( ptr_to_SummaryFile, "UNRECOGNIZED DATATYPE !!  ERROR !!\n\n" );
            break;

         }  // data type msg

      }  // sequence count gap ?

   }   // LastSequenceCount initialized ?


   LastSequenceCount [ *DataType_ptr ] = *SequenceCount_ptr;   // latch current sequence count as new last



   // *** The next word is the packet length, which is defined to be
   //     the length in bytes of the application data, less one:

   num_read = fread ( &Word2, 2, 1, ptr_to_InputFile );

   if ( feof (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n BAD EOF at reading packet length !\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf (ptr_to_SummaryFile, "\n File Error reading packet length !\n") ;
   if ( num_read  !=  1 ) {
      fprintf ( ptr_to_SummaryFile, "\n*** ERROR: Wrong number of items reading packet length!\n" );
      return FAIL;
   }


   HeaderPacketLength = ByteSwap_2 ( Word2 );
   // fprintf ( ptr_to_SummaryFile, "Header Packet Length =  %d\n", HeaderPacketLength );



   // *** Read the 4 byte coarse time from start of application data

   num_read = fread ( &Word4, 1, 4, ptr_to_InputFile );

   if ( feof (ptr_to_InputFile) )  fprintf (ptr_to_SummaryFile, "\n BAD EOF at reading CoarseTime !\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf (ptr_to_SummaryFile, "\n File Error reading CoarseTime !\n" );
   if ( num_read  !=  4 ) {
      fprintf ( ptr_to_SummaryFile, "\n*** ERROR: Wrong number of items reading CoarseTime!\n" );
      return FAIL;
   }

   *HeaderCoarseTime_ptr = ByteSwap_4 ( Word4 );



   // *** Read 2 byte fine time

   num_read = fread ( HeaderFineTime_ptr, 2, 1, ptr_to_InputFile );

   if ( feof (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n BAD EOF at reading FineTime !\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n File Error reading FineTime !\n" );
   if ( num_read  != 1  ) {
      fprintf ( ptr_to_SummaryFile, "\n*** ERROR: Wrong number of items reading FineTime!\n" );
      return FAIL;
  }

   *HeaderFineTime_ptr = ByteSwap_2 ( *HeaderFineTime_ptr );


   //  Calculate clock time and report the interval from the previous packet
   //  (if such has occurred) of the same type.

   //  If this is TTE and if the header time gap is more than 1.1 s, then either
   //  we have missing data (very unlikely) or the FSW has made a TTE change,
   //  which is most likely a change between prompt TTE to the HSSDB and TTE FIFO
   //  dump.  This reasoning is based on a minimum time between TTE packets of 1 s
   //  because of the stale counter in the FPGA.    If we identify a TTE mode change,
   //  the value of the most recent TTE header time is no longer correct and so we
   //  reset it to an illegal value.

   TimeSeconds = FloatTime_from_CoarseFine  ( *HeaderCoarseTime_ptr, *HeaderFineTime_ptr );

   if ( VerboseFlag ) {
      fprintf ( ptr_to_SummaryFile, "Header Time = 0x%X : 0x%X  =  float MET %18.6Lf\n",
         *HeaderCoarseTime_ptr, *HeaderFineTime_ptr, TimeSeconds );

      JulianDay = GBM_MET_Time_to_JulianDay ( *HeaderCoarseTime_ptr, *HeaderFineTime_ptr );
      JulianDay_to_Calendar_subr ( JulianDay, &year, &month, &day, &hours, &minutes, &seconds );
      fprintf (ptr_to_SummaryFile, "which is JD %21.11Lf  = %4d:%02d:%02d at %02d:%02d:%9Lf\n\n",
               JulianDay, year, month, day, hours, minutes, seconds );

   }


   if ( VerboseFlag )  if ( PrevHeaderTime [ *DataType_ptr ]  >  0.0 )
      fprintf ( ptr_to_SummaryFile, "Deduced accumulation length: %13.6Lf\n", TimeSeconds - PrevHeaderTime [ *DataType_ptr ] );
   PrevHeaderTime [ *DataType_ptr ] = TimeSeconds;



   // *** Read the words from the application data field of the packet.
   // The header counts the length from 0.
   // We have already read 6 bytes of time from the application data....
   //  +1 - 6 = 5.

   *PacketDataLength_ptr = (uint16_t) ( HeaderPacketLength - 5 );

   if ( * PacketDataLength_ptr  >  MAX_PACKET_ARRAY_BYTES ) {
      fprintf ( ptr_to_SummaryFile, "\n*** ERROR: length is too big: %d\n", *PacketDataLength_ptr );
      return FAIL;
   }


   //  The data fields of CSPEC and CTIME packets are natively composed of two-byte little-endian words,
   //  while the data field of TTE packets is natively composed of four-byte big endian words.
   //  Here we read the data by word, so the fread call needs to know the number of bytes per word.


   Bytes_per_Word = 2;
   if ( *DataType_ptr  ==  TTE )  Bytes_per_Word = 4;

   Words2Read = *PacketDataLength_ptr / Bytes_per_Word;

   num_read = fread ( PacketData, Bytes_per_Word, Words2Read, ptr_to_InputFile );


   if ( feof (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n BAD EOF while reading application data!\n" );
   if ( ferror (ptr_to_InputFile) )  fprintf ( ptr_to_SummaryFile, "\n File Error while read appl data !\n" );

   if ( num_read  !=  (size_t) Words2Read ) {
      fprintf ( ptr_to_SummaryFile,              //  gcc 4.0 wants %zu for type size_t (for c99?); older compilers use %u:
         "\n*** ERROR: Wrong number of items reading application data!  %zu  %zu\n", num_read, Words2Read );
      return FAIL;
   }



   return OK;

}  //  ReadPacket ()
