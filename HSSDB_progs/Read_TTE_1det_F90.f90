program Read_Processed


   use, intrinsic :: ISO_FORTRAN_ENV

   use, intrinsic :: ISO_C_BINDING

   implicit none


   ! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

!   typedef  struct   TTE_Data_type {
!      uint32_t CoarseTime;
!      uint16_t FineTime;
!      uint16_t SpecChannel;
!   } TTE_Data_type;


   !  should be unsigned integers, but those are not available in Fortran.
   !  This will need "fixing" for the Coarse time.  The most-significant
   !  bit should not get set for the other items.

   type, bind (C) :: TTE_Data_type

      integer (C_INT32_T) :: CoarseTime
      integer (C_INT16_T) :: FineTime
      integer (C_INT16_T) :: SpecChannel

   end type

   type (TTE_Data_type) :: TTE_data

   integer :: in_LUN
   integer :: i

   integer (int64) :: CoarseTime_64
   integer (int32) :: FineTime_32

   character (len=200) :: FileName

   ! - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -



   call GET_COMMAND_ARGUMENT ( 1, FileName )

   open ( newunit=in_LUN, file=FileName,  status="old", access="stream",   &
             form="unformatted", action="read" )


   do i=1,1000

      read (in_LUN)  TTE_data

      CoarseTime_64 = TTE_Data % CoarseTime
      if ( CoarseTime_64 < 0_int64 )  CoarseTime_64 = CoarseTime_64  +  2_int64 ** 32_int64

      FineTime_32 = TTE_Data % FineTime
      if ( FineTime_32 < 0_int32 )  FineTime_32 = FineTime_32  +  2_int32 ** 16_int32

      write (*, '( I10, 2X, I5, 3X, I3 )' )   &
         CoarseTime_64, FineTime_32, TTE_Data % SpecChannel

   end do


end program Read_Processed
