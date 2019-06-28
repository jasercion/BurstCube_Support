
//  Revised by Michael S. Briggs, University of Alabama in Huntsville,
//  copyright 2014 February 15.
//  Revised to used C99 fixed-width integer types to enhance portability;
//  also _Bool type.


//  Michael S. Briggs, 2008 June -- July 7.
//  Work in progress.  Needs comments.  See AAA_DESCRIPTION.txt


#include "HSSDB_Progs_Header.h"

#include <limits.h>


//  local typedefs:

typedef struct  Detector_Buffer_type {
   _Bool  HaveData;
   uint16_t  SpecChan;
   uint64_t  Time_in_OneVariable;
   uint32_t CoarseTime;
   uint16_t FineTime;
} Detector_Buffer_type;


//  local function prototypes:

void ReadSummaryFile (

   int argc,
   char * argv [],
   uint32_t  * First_Time_Coarse_ptr,
   uint16_t  * First_Time_Fine_ptr,
   DetectorFile_type  DetectorFiles [NUM_DET]

);

void Load_Detector_Buffer (

   DetectorFile_type  DetectorFiles [NUM_DET],
   Detector_Buffer_type  Detector_Buffer [NUM_DET]

);


void  SkipLines_v2 (

   //  input arguments:
   FILE * InputFile_ptr,
   uint32_t Num2Skip

);


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


int main ( int argc, char * argv [] ) {


   uint32_t  First_Time_Coarse;
   uint16_t  First_Time_Fine;
   DetectorFile_type  DetectorFiles [NUM_DET];

   Detector_Buffer_type Detector_Buffer[NUM_DET];

   _Bool  ContinueFlag;

   uint64_t FirstTime;
   uint16_t Det_of_FirstTime;

   uint16_t j_det;

   Processed_TTE_type  Processed_TTE;

   size_t  num_written;

   FILE * Output_File_ptr;


   // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


   //  Startup Initializations:

   for ( j_det=0;  j_det<NUM_DET;  j_det++ )  Detector_Buffer [j_det] .HaveData = false;

   //  Find out which detectors had TTE data, the names of the files, etc.

   ReadSummaryFile ( argc, argv, &First_Time_Coarse, &First_Time_Fine, DetectorFiles );


   FirstTime = UINT64_MAX;
   //printf ( "Test: 0x%16llx\n\n", FirstTime );


   Output_File_ptr = fopen ( "Processed_TTE.dat", "w" );
   if ( Output_File_ptr == NULL ) {
      printf ( "\n\nFailed to open output TTE File -- Exiting!\n" );
      exit (1);
   }

   //  End of Startup Initializations.



   //  Now process the TTE data, reading TTE events from the 1 to 14 DetectorFiles
   //  until EOF for all of the DetectorFiles:

   while ( true ) {  // do "forever" -- process data

      //  For any detector with TTE data, if the buffer is empty for that detector,
      //  read from the file for that detector to fill the buffer entry -- if there
      //  is still more data in the file.
      //  (On the first pass, the buffer will be completely empty, and reads will
      //  be done for all detectors.   On later passes, only a single read will be done,
      //  to re-fill the entry that was used on the previous pass.)

      Load_Detector_Buffer ( DetectorFiles, Detector_Buffer );



      //  Is there any data in the Buffer? -- it could be empty if this routine drained the Buffer
      //  on the last pass and Load_Detector_Buffer didn't refill any entries in the Buffer
      //  because all of the input data files are at EOF.
      //  If we find the Buffer empty, we will exit the program, here in the middle of the
      //  apparently infinite loop of reading data.

      ContinueFlag = false;
      for ( j_det=0;  j_det<NUM_DET;  j_det++ ) {
         if ( Detector_Buffer [j_det] .HaveData )  ContinueFlag = true;
      }

      if ( ! ContinueFlag ) {
         printf ( "All of the input data has been processed.  Exiting.\n" );
         return (0);                        // <---   THIS IS THE NORMAL EXIT FROM THIS PROGRAM !!!
      }



      //  Loop through the detectors with data, to determine which entry in the
      //  buffer has the earliest time.    (Ties are implicitly broken by
      //  using the smaller detector number -- we only replace the currently
      //  selected detector if we encounter a smaller time.)
      //  Having found an entry, output it to the output file.

      FirstTime = UINT64_MAX;
      Det_of_FirstTime = UINT16_MAX;

      for  ( j_det=0;  j_det<NUM_DET;  j_det++ ) {

         if ( Detector_Buffer [j_det] .HaveData ) {  // detector has data ?

            if ( Detector_Buffer [j_det] .Time_in_OneVariable < FirstTime ) {
               FirstTime = Detector_Buffer [j_det] .Time_in_OneVariable;
               Det_of_FirstTime = j_det;
            }

         }  // detector has data ?

      }  // j_det



      //  The logic of the program guarantees that there is at least one detector in the buffer,
      //  and that the above code will always find a detector.   Check..
      if ( Det_of_FirstTime >= NUM_DET ) {
         printf ( "\nFailure of program logic.  Detector number %u.  Aborting.\n", Det_of_FirstTime );
         exit (2);
      }



      //  Output the detector with the smallest time & mark its entry as consumed:

      Detector_Buffer [Det_of_FirstTime] .HaveData = false;
      Processed_TTE.Detector = Det_of_FirstTime;
      Processed_TTE.SpecChannel = Detector_Buffer [Det_of_FirstTime] .SpecChan;
      //Processed_TTE.Time_in_OneVariable = Detector_Buffer [Det_of_FirstTime] .Time_in_OneVariable;
      Processed_TTE.CoarseTime = Detector_Buffer [Det_of_FirstTime] .CoarseTime;
      Processed_TTE.FineTime =  Detector_Buffer [Det_of_FirstTime] .FineTime;

      num_written = fwrite ( &Processed_TTE, sizeof ( Processed_TTE ), 1, Output_File_ptr );
      if ( num_written != 1 ) {
         printf ( "\nWrite to output file failed !\n" );
         exit (3);
      }


   }  // do "forever" -- process data


}  // main



// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *


void ReadSummaryFile (

   int argc,
   char * argv [],
   uint32_t  * First_Time_Coarse_ptr,
   uint16_t  * First_Time_Fine_ptr,
   DetectorFile_type  DetectorFiles [NUM_DET]

) {

   uint32_t Detector_Output_Cnt;
   FILE * ptr_to_SummaryFile;
   uint16_t  FileName_Length;

   unsigned int j_det, i;

   //  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

   if ( argc != 2 ) {
      printf ( "Bad number of command line arguments: %d\n", argc - 1 );
      printf ( "Example usage:  ./Merge_TTE  FileName.sum\n" );
      printf ( "The command-line argument is the name of the file to analyze,\n" );
      exit (13);
   }

   FileName_Length = strlen ( argv [1] );
   if ( strcmp ( argv [1] + FileName_Length - 4, ".sum" )  != 0  ) {
      printf ( "\nError: the filetype must be .sum !\n" );
      exit (14);
   }


   ptr_to_SummaryFile = fopen ( argv [1], "r" );
   if ( ptr_to_SummaryFile == NULL ) {
      printf ( "\n\nFailed to open input Summary File '%s' -- Exiting!\n", argv [1] );
      exit (4);
   }


   //  Read the First TTE time: Coarse & Fine
   SkipLines_v2 ( ptr_to_SummaryFile, 9 );

   fscanf ( ptr_to_SummaryFile, "%11u", First_Time_Coarse_ptr );
   fscanf ( ptr_to_SummaryFile, "%*3c" );  // move over 3 spaces
   fscanf ( ptr_to_SummaryFile, "%6hu", First_Time_Fine_ptr );
   printf ( "\nStart Time: %u  %u\n", *First_Time_Coarse_ptr, *First_Time_Fine_ptr );

   //  Read the number of detectors that had TTE data:
   SkipLines_v2 ( ptr_to_SummaryFile, 5 );
   fscanf ( ptr_to_SummaryFile, "%u", &Detector_Output_Cnt );

   printf ( "\nNumber of Detectors with TTE Data: %u\n", Detector_Output_Cnt );

   if ( Detector_Output_Cnt == 0 ) {
      printf ( "\n\n No Detectors have TTE Data -- Aborting !!\n" );
      exit (5);
   }


   //  1) Initialize the structure DetectorFiles,
   //  2) read which detectors have data and the corresponding file names,

   for ( j_det=0;  j_det < NUM_DET;  j_det++ ) {

      DetectorFiles [j_det] .DetectorFile_Opened = false;
      DetectorFiles [j_det] .DetectorFileName_ptr = malloc (120);
      if ( DetectorFiles [j_det] .DetectorFileName_ptr == NULL ) {
         printf ( "\n\nmalloc to string failed. exiting.\n" );
         exit (6);
      }
      DetectorFiles [j_det] .ptr_to_DetectorFile = NULL;

   }  // j_det

   SkipLines_v2 ( ptr_to_SummaryFile, 2 );

   for ( i=0;  i<Detector_Output_Cnt;  i++ ) {  // loop over dets with data

      fscanf ( ptr_to_SummaryFile, "%6u", &j_det );
      if ( j_det >= NUM_DET ) {
         printf ( "\nERROR: bad value of detector number: %u\n", j_det );
         exit (7);
      }

      DetectorFiles [j_det] .DetectorFile_Opened = true;
      fscanf ( ptr_to_SummaryFile, "%*3c" );  // move over 3 spaces
      fscanf ( ptr_to_SummaryFile, "%s", DetectorFiles [j_det] .DetectorFileName_ptr );
      printf ( "Detector %u, filename %s\n", j_det, DetectorFiles [j_det] .DetectorFileName_ptr );

      DetectorFiles [j_det] .ptr_to_DetectorFile = fopen ( DetectorFiles [j_det] .DetectorFileName_ptr, "r" );
      if ( DetectorFiles [j_det] .ptr_to_DetectorFile == NULL ) {
         printf ( "\n\nFailed to open TTE input File for det %u!  Exiting!\n", j_det );
         exit (8);
      }

   }  // loop i -- loop over dets with data



}  // subroutine ReadSummaryFile ()



//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***
//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


//  This function skips Num2Skip lines in the file.
//  A line is defined as a sequence of characters ending in the
//  new line symbol '\n'.   The first line may have already
//  been partially read.

void  SkipLines_v2 (

   //  input arguments:
   FILE * InputFile_ptr,
   uint32_t Num2Skip

) {

   uint32_t i;


   //  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***


   //  Skip the rest of the current line, and all of Num2Skip - 1 additional lines
   //  (possibly zero), by reading characters until we see the newline characters.

   for ( i=0;  i < Num2Skip;  i++ ) {
      while ( getc ( InputFile_ptr )  !=  '\n' );
   }

}  //  SkipLines_v2 ()



//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***
//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


//  Argument is input and output !

void Load_Detector_Buffer (

   DetectorFile_type  DetectorFiles [NUM_DET],
   Detector_Buffer_type  Detector_Buffer [NUM_DET]

) {

   uint16_t j_det;
   TTE_Data_type  TTE_Data;

   size_t  num_read;


   //  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***  ***

   for ( j_det=0;  j_det<NUM_DET;  j_det++ ) {

      //  We need to do a read 1) if this detector has data, i.e., a data file exists for it, and
      //  2) if the entry in the Buffer for this detector is currently empty:

      if ( DetectorFiles [j_det] .DetectorFile_Opened  &&  ! Detector_Buffer [j_det] .HaveData ) {  // buffer entry to fill ?

         num_read = fread ( &TTE_Data, sizeof (TTE_Data_type), 1, DetectorFiles [j_det] .ptr_to_DetectorFile );

         if ( num_read != 1 ) {  // successful read ?

            //  We didn't read one item, as requested -- error or EOF -- we will assume EOF.
            //  All that we do is leave DetectorFiles [j_det] .HaveData as FALSE to inform
            //  the calling program that we didn't obtain data for this detector.

         } else {  // successful read ?

            //  The read was successful, so:
            //  1) flag that entry now contains data,
            //  2) transfer data, changing the format of the time:

            Detector_Buffer [j_det] .HaveData = true;
            Detector_Buffer [j_det] .SpecChan = TTE_Data.SpecChannel;
            Detector_Buffer [j_det] .CoarseTime = TTE_Data.CoarseTime;
            Detector_Buffer [j_det] .FineTime = TTE_Data.FineTime;
            Detector_Buffer [j_det] .Time_in_OneVariable = IntegerTime_from_CoarseFine ( TTE_Data.CoarseTime, TTE_Data.FineTime );

         }  // successful read ?


      }  // buffer entry to fill ?


   }  // j_det


}  // subroutine Load_Detector_Buffer ()

