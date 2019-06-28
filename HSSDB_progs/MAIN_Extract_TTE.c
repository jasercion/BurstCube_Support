
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2004 Oct 8, UAH / NSSTC.
//  Revised 2007 September 28.

//   The filename should be input on the command line:
//   FileScan filename
//   For example:
//   ./Extract_TTE  HSDAQ_BBE3D5A330A.dat

//  This program reads the HSSDB data and extracts the TTE data, writing the TTE events
//  to files, one file for each detector for which TTE data is encountered.
//  The output data is simmplier and cleaner: there are only TTE events, rather than a mixture
//  of data words (events) and time words.

//  Another important action is correcting time errors in the TTE data.
//  Another action is looking for anomalies such as unexpectedly large time jumps and outputting
//  information about them.
//  There is also output of other summary information.


#include "HSSDB_Progs_Header.h"

#include <limits.h>


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


int main ( int argc, char * argv [] ) {

   FILE * ptr_to_InputFile;
   FILE * ptr_to_AnalysisFile;
   FILE * ptr_to_SummaryFile;


   size_t  FileName_Length;
   char * Input_FileName_ptr;
   char * Analysis_FileName_ptr;
   char * Anomaly_FileName_ptr;
   char * Summary_FileName_ptr;


   _Bool SyncWordFound;
   uint32_t ExcessNonZeroCnt;
   uint32_t ExcessBytes;

   uint32_t ReadStatus;


   DataType_type DataType;


   uint32_t PacketData [ MAX_PACKET_ARRAY_BYTES / 4 ];

   uint16_t PacketDataLength;

   uint32_t HeaderCoarseTime;
   uint16_t HeaderFineTime;

   uint16_t SequenceCount;


   //  This array is indexed by the enum type DataType_type:

   uint32_t CountByAPID [4] = { 0, 0, 0, 0 };


   uint16_t Number_TTE_DataWords;
   uint32_t TTE_CoarseTime [MAX_TTE_WORDS_PER_PACKET];
   uint16_t TTE_FineTime [MAX_TTE_WORDS_PER_PACKET];
   uint16_t TTE_Channel [MAX_TTE_WORDS_PER_PACKET];
   uint16_t TTE_Detector [MAX_TTE_WORDS_PER_PACKET];

   uint32_t  ErrorCounts [NUM_ERROR_TYPES];
   uint16_t  j_err;



   // ********************************************************************************************


   // *************************************************************
   //      Setup, get user input, open files .....
   // *************************************************************


   for ( j_err=0;  j_err < NUM_ERROR_TYPES;  j_err++ )  ErrorCounts[j_err] = 0;


   if ( argc != 2 ) {
      printf ( "Bad number of command line arguments: %d\n", argc - 1 );
      printf ( "Example usage:  ./Extract_TTE  FileName.dat\n" );
      printf ( "The command-line argument is the name of the file to analyze,\n" );
      return 1;
   }



   FileName_Length = strlen ( argv [1] );
   Input_FileName_ptr   = malloc ( FileName_Length );
   Analysis_FileName_ptr = malloc ( FileName_Length );
   Summary_FileName_ptr = malloc ( FileName_Length );
   Anomaly_FileName_ptr = malloc ( FileName_Length );

   if ( Input_FileName_ptr == NULL  ||  Analysis_FileName_ptr == NULL  ||
        Summary_FileName_ptr == NULL  ||  Anomaly_FileName_ptr == NULL ) {
      printf ("\nmalloc call failed.\n");
      return 2;
   }

   memcpy ( Input_FileName_ptr,   argv [1], FileName_Length );
   memcpy ( Analysis_FileName_ptr, argv [1], FileName_Length );
   memcpy ( Summary_FileName_ptr, argv [1], FileName_Length );
   memcpy ( Anomaly_FileName_ptr, argv [1], FileName_Length );


   if ( strcmp ( Input_FileName_ptr + FileName_Length - 4, ".dat" )  != 0  ) {
      printf ( "\nError: the filetype must be .dat !\n" );
      return 3;
   }

   strncpy ( Analysis_FileName_ptr + FileName_Length - 4, ".txt", 4 );
   strncpy ( Summary_FileName_ptr + FileName_Length - 4, ".sum", 4 );
   strncpy ( Anomaly_FileName_ptr + FileName_Length - 4, ".err", 4 );

   ptr_to_InputFile =   fopen ( Input_FileName_ptr, "rb" );
   ptr_to_AnalysisFile = fopen ( Analysis_FileName_ptr, "w" );
   ptr_to_SummaryFile = fopen( Summary_FileName_ptr, "w" );


   if ( ptr_to_InputFile == NULL ) {
      printf ( "Failed to open the Input file !\n" );
      return 4;
   }

   if ( ptr_to_AnalysisFile == NULL  ||  ptr_to_SummaryFile == NULL ) {
      printf ( "Failed to open the Output files !\n" );
      return 5;
   }



   fprintf ( ptr_to_AnalysisFile, "\n\nAnalyzing File %s\n\n", Input_FileName_ptr );
   fprintf ( ptr_to_AnalysisFile, "         Time\n  Coarse     Fine  det chan\n\n" );



   // *************************************************************
   //        read and process the data
   // *************************************************************



   SyncWordFound = true;

   while ( SyncWordFound ) {


      // *** Step 1: Skip over any bytes before the next sync word

      SyncWordFound = ReSync ( ptr_to_InputFile, &ExcessBytes, &ExcessNonZeroCnt );

      if ( !SyncWordFound ) {  //  sync word found ?

         printf ( "\n\n Sync Word NOT Found -- EOF?? !!\n" );

      } else {  //  sync word found ?

         if ( ExcessBytes > 0 ) {  //  ExcessBytes > 0 ?

            if ( ExcessNonZeroCnt == 0 ) {  // ExcessNonZeroCnt ?

               printf (                    "\n\n >>> %u EXCESS BYTES BEFORE SYNCH WORD -- all bytes are zero !!!\n", ExcessBytes);
               fprintf (ptr_to_AnalysisFile, "\n\n >>> %u EXCESS BYTES BEFORE SYNCH WORD -- all bytes are zero !!!\n", ExcessBytes);

            } else {  // ExcessNonZeroCnt ?

                printf (
                  "\n\n >>> %u EXCESS BYTES BEFORE SYNCH WORD -- %u bytes are NON-ZERO !!!\n", ExcessBytes, ExcessNonZeroCnt );
               fprintf (ptr_to_AnalysisFile,
                  "\n\n >>> %u EXCESS BYTES BEFORE SYNCH WORD -- %u bytes are NON-ZERO !!!\n", ExcessBytes, ExcessNonZeroCnt );

            }  // ExcessNonZeroCnt ?

         }  //  ExcessBytes > 0 ?



         // *** Step 2: Process the header of the next packet:

         ReadStatus = ReadPacket (
            ptr_to_InputFile,
            ptr_to_AnalysisFile,
            false,
            &DataType,
            CountByAPID,
            &PacketDataLength,
            (short unsigned int *) PacketData,
            &HeaderCoarseTime,
            &HeaderFineTime,
            &SequenceCount );


         if ( ReadStatus != OK ) {

            printf ( "\nReadPacket returns ERROR !\n" );

         } else {  // ReadStatus ?


            //  *** If the packet is a TTE Packet, process it:

            switch ( DataType ) {


               case TTE:

                  Extract_TTE_1packet ( PacketData,
                                        PacketDataLength,
                                        HeaderCoarseTime,
                                        ptr_to_AnalysisFile,
                                        ErrorCounts,
                                       &Number_TTE_DataWords,
                                        TTE_CoarseTime,
                                        TTE_FineTime,
                                        TTE_Channel,
                                        TTE_Detector
                                      );

                  Output_TTE ( Number_TTE_DataWords,
                               TTE_CoarseTime,
                               TTE_FineTime,
                               TTE_Channel,
                               TTE_Detector,
                               Analysis_FileName_ptr,
                               ptr_to_AnalysisFile,
                               ptr_to_SummaryFile
                              );

               break;


               case CSPEC:
               break;


               case CTIME:
               break;


               default:
                   printf ( "\n ***** ERROR: Unrecognized DataType: %d\n\n", DataType );
                  fprintf ( ptr_to_AnalysisFile, "\n ***** ERROR: Unrecognized DataType: %d\n\n", DataType );
               break;


            }  // switch on DataType

         }  //  ReadStatus ?

      }  //  sync word found ?

   }   // loop while sync words available


   fprintf ( ptr_to_AnalysisFile, "\nCount of APIDs found:\n\n" );
   fprintf ( ptr_to_AnalysisFile, "APID 0x5A0: CSPEC: %6u\n", CountByAPID [ CSPEC ] );
   fprintf ( ptr_to_AnalysisFile, "APID 0x5A1: CTIME: %6u\n", CountByAPID [ CTIME ] );
   fprintf ( ptr_to_AnalysisFile, "APID 0x5A2:   TTE: %6u\n", CountByAPID [ TTE ] );
   fprintf ( ptr_to_AnalysisFile, "Unexpected values: %6u\n", CountByAPID [ BAD ] );

   fprintf ( ptr_to_SummaryFile, "\nCount of APIDs found:\n\n" );
   fprintf ( ptr_to_SummaryFile, "APID 0x5A0: CSPEC: %6u\n", CountByAPID [ CSPEC ] );
   fprintf ( ptr_to_SummaryFile, "APID 0x5A1: CTIME: %6u\n", CountByAPID [ CTIME ] );
   fprintf ( ptr_to_SummaryFile, "APID 0x5A2:   TTE: %6u\n", CountByAPID [ TTE ] );
   fprintf ( ptr_to_SummaryFile, "Unexpected values: %6u\n", CountByAPID [ BAD ] );

   printf ( "\n\nCount of APIDs found:\n\n" );
   printf ( "APID 0x5A0: CSPEC: %6u\n", CountByAPID [ CSPEC ] );
   printf ( "APID 0x5A1: CTIME: %6u\n", CountByAPID [ CTIME ] );
   printf ( "APID 0x5A2:   TTE: %6u\n", CountByAPID [ TTE ] );
   printf ( "Unexpected values: %6u\n", CountByAPID [ BAD] );


   //  Inform routine Output_TTE to perform its closeout actions --
   //  this is signaled by the special value UINT16_MAX for the
   //  number of TTE data words.  (This value is impossible for
   //  a real TTE packet.)

   Output_TTE ( UINT16_MAX,
                TTE_CoarseTime,
                TTE_FineTime,
                TTE_Channel,
                TTE_Detector,
                Analysis_FileName_ptr,
                ptr_to_AnalysisFile,
                ptr_to_SummaryFile
               );

   //  Output info about timing errors detected and, for most types, corrected.
   //  Error types are numbered from 1, so we skip array element 0.

   for ( j_err=1;  j_err < NUM_ERROR_TYPES;  j_err++ ) {
      printf ( "%u Timing Errors of Type %u detected.\n", ErrorCounts[j_err], j_err );
      fprintf ( ptr_to_AnalysisFile, "%u Timing Errors of Type %u detected.\n", ErrorCounts[j_err], j_err );
   }

   return 0;


}  //  FileScan ()
