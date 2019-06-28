
#include "HSSDB_Progs_Header.h"


//   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***   ***


int main ( ) {


   FILE * ptr_to_DetectorFile;

   Processed_TTE_type  TTE_Data;

   uint32_t  i;
   size_t  num_read;

   // ********************************************************************************************

   ptr_to_DetectorFile = fopen ( "Processed_TTE.dat", "rb" );

   if (ptr_to_DetectorFile == NULL ) {
      printf ( "\n\nFailed to input TTE File.  Exiting!\n" );
      exit (2);
   }


   for ( i=0; i < 1000; i++ ) {

      num_read = fread ( &TTE_Data,
                         sizeof ( TTE_Data ),
                         1,
                         ptr_to_DetectorFile );

      if ( num_read == 1 ) {


         printf ( "%u  %u  %u  %u\n", TTE_Data.CoarseTime,  TTE_Data.FineTime, TTE_Data.Detector, TTE_Data.SpecChannel );
      } else {

         printf ( "\n\nRead failed.  Exiting!\n" );
         exit (3);

      }

   }  // i


   return (0);


}  // main ()
