/**
 *		LC898128 Flash update
 *
 *		Copyright (C) 2017, ON Semiconductor, all right reserved.
 *
 **/



//**************************
//	Include Header File
//**************************
#include	"PhoneUpdate.h"

//#include	<stdlib.h>
//#include	<math.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include	"FromCode_01_02_01_00.h"
#include	"FromCode_01_02_02_01.h"
//OPLUS ois firmware
#include	"FromCode_01_01_01_02.h"

/* Burst Length for updating to PMEM */
#define BURST_LENGTH_UC 		( 3*20 ) 	// 60 Total:63Byte Burst
//#define BURST_LENGTH_UC 		( 6*20 ) 	// 120 Total:123Byte Burst
/* Burst Length for updating to Flash */
#define BURST_LENGTH_FC 		( 32 )	 	// 32 Total: 35Byte Burst
//#define BURST_LENGTH_FC 		( 64 )	 	// 64 Total: 67Byte Burst

//****************************************************
//	CUSTOMER NECESSARY CREATING FUNCTION LIST
//****************************************************
/* for I2C communication */
extern	void RamWrite32A( UINT_16, UINT_32 );
extern 	INT_32 RamRead32A( UINT_16, void * );
/* for I2C Multi Translation : Burst Mode*/
extern 	void CntWrt( void *, UINT_16) ;
extern	void CntRd( UINT_32, void *, UINT_16 ) ;

/* for Wait timer [Need to adjust for your system] */
extern void	WitTim( UINT_16 );

//**************************
//	Table of download file
//**************************

const DOWNLOAD_TBL_EXT DTbl[] = {
	{0x010102, CcUpdataCode128_01_01_01_02, UpDataCodeSize_01_01_01_02,  UpDataCodeCheckSum_01_01_01_02, CcFromCode128_01_01_01_02, sizeof(CcFromCode128_01_01_01_02), FromCheckSum_01_01_01_02, FromCheckSumSize_01_01_01_02 },
	{0xFFFFFF, (void*)0,                         0,                            0,                         (void*)0,                          0,                             0,                        0}
};



INT_32	Sl_Hall_X_by_AF[ 9 ] ;
INT_32	Sl_Hall_Y_by_AF[ 9 ] ;

//********************************************************************************
// Function Name 	: IOWrite32A
//********************************************************************************
void IORead32A( UINT_32 IOadrs, UINT_32 *IOdata )
{
	RamWrite32A( CMD_IO_ADR_ACCESS, IOadrs ) ;
	RamRead32A ( CMD_IO_DAT_ACCESS, IOdata ) ;
}

//********************************************************************************
// Function Name 	: IOWrite32A
//********************************************************************************
void IOWrite32A( UINT_32 IOadrs, UINT_32 IOdata )
{
	RamWrite32A( CMD_IO_ADR_ACCESS, IOadrs ) ;
	RamWrite32A( CMD_IO_DAT_ACCESS, IOdata ) ;
}

//********************************************************************************
// Function Name 	: UnlockCodeSet
//********************************************************************************
UINT_8 UnlockCodeSet( void )
{
	UINT_32 UlReadVal, UlCnt=0;

	do {
		IOWrite32A( 0xE07554, 0xAAAAAAAA );
		IOWrite32A( 0xE07AA8, 0x55555555 );
		IORead32A( 0xE07014, &UlReadVal );
		if( (UlReadVal & 0x00000080) != 0 )	return ( 0 ) ;
		WitTim( 1 );
	} while( UlCnt++ < 10 );
	return ( 1 );
}

//********************************************************************************
// Function Name 	: UnlockCodeClear
//********************************************************************************
UINT_8 UnlockCodeClear(void)
{
	UINT_32 UlDataVal, UlCnt=0;

	do {
		IOWrite32A( 0xE07014, 0x00000010 );
		IORead32A( 0xE07014, &UlDataVal );
		if( (UlDataVal & 0x00000080) == 0 )	return ( 0 ) ;
		WitTim( 1 );
	} while( UlCnt++ < 10 );
	return ( 3 );
}
//********************************************************************************
// Function Name 	: WritePermission
//********************************************************************************
void WritePermission( void )
{
	IOWrite32A( 0xE074CC, 0x00000001 );
	IOWrite32A( 0xE07664, 0x00000010 );
}

//********************************************************************************
// Function Name 	: AddtionalUnlockCodeSet
//********************************************************************************
void AddtionalUnlockCodeSet( void )
{
	IOWrite32A( 0xE07CCC, 0x0000ACD5 );
}
//********************************************************************************
// Function Name 	: CoreResetwithoutMC128
//********************************************************************************
UINT_8 CoreResetwithoutMC128( void )
{
	UINT_32	UlReadVal ;

	IOWrite32A( 0xE07554, 0xAAAAAAAA);
	IOWrite32A( 0xE07AA8, 0x55555555);

	IOWrite32A( 0xE074CC, 0x00000001);
	IOWrite32A( 0xE07664, 0x00000010);
	IOWrite32A( 0xE07CCC, 0x0000ACD5);
	IOWrite32A( 0xE0700C, 0x00000000);
	IOWrite32A( 0xE0701C, 0x00000000);
	IOWrite32A( 0xE07010, 0x00000004);

	WitTim(100);

	IOWrite32A( 0xE0701C, 0x00000002);
	IOWrite32A( 0xE07014, 0x00000010);

	IOWrite32A( 0xD00060, 0x00000001 ) ;
	WitTim( 15 ) ;

	IORead32A( ROMINFO,				(UINT_32 *)&UlReadVal ) ;
	switch ( (UINT_8)UlReadVal ){
	case 0x08:
	case 0x0D:
		break;

	default:
		return( 0xE0  | (UINT_8)UlReadVal );
	}

	return( 0 );
}

//********************************************************************************
// Function Name 	: PmemUpdate128
//********************************************************************************
UINT_8 PmemUpdate128( DOWNLOAD_TBL_EXT* ptr )
{
	UINT_8	data[BURST_LENGTH_UC +2 ];
	UINT_16	Remainder;
	const UINT_8 *NcDataVal = ptr->UpdataCode;
	UINT_8	ReadData[8];
	long long CheckSumCode = ptr->SizeUpdataCodeCksm;
	UINT_8 *p = (UINT_8 *)&CheckSumCode;
	UINT_32 i, j;
	UINT_32	UlReadVal, UlCnt , UlNum ;
//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
	IOWrite32A( 0xE0701C, 0x00000000);
	RamWrite32A( 0x3000, 0x00080000 );


	data[0] = 0x40;
	data[1] = 0x00;


	Remainder = ( (ptr->SizeUpdataCode*5) / BURST_LENGTH_UC );
	for(i=0 ; i< Remainder ; i++)
	{
		UlNum = 2;
		for(j=0 ; j < BURST_LENGTH_UC; j++){
			data[UlNum] =  *NcDataVal++;
			if( ( j % 5) == 4)	TRACE("\n");
			UlNum++;
		}

		CntWrt( data, BURST_LENGTH_UC+2 );
	}
	Remainder = ( (ptr->SizeUpdataCode*5) % BURST_LENGTH_UC);
	if (Remainder != 0 )
	{
		UlNum = 2;
		for(j=0 ; j < Remainder; j++){
			data[UlNum++] = *NcDataVal++;
		}
		CntWrt( data, Remainder+2 );
	}

//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------


	data[0] = 0xF0;
	data[1] = 0x0E;
	data[2] = (unsigned char)((ptr->SizeUpdataCode >> 8) & 0x000000FF);
	data[3] = (unsigned char)(ptr->SizeUpdataCode & 0x000000FF);
	data[4] = 0x00;
	data[5] = 0x00;

	CntWrt( data, 6 ) ;


	UlCnt = 0;
	do{
		WitTim( 1 );
		if( UlCnt++ > 10 ) {
			IOWrite32A( 0xE0701C, 0x00000002);
			return (0x21) ;
		}
		RamRead32A( 0x0088, &UlReadVal );
	}while ( UlReadVal != 0 );

	CntRd( 0xF00E, ReadData , 8 );

	IOWrite32A( 0xE0701C, 0x00000002);
	for( i=0; i<8; i++) {
		if(ReadData[7-i] != *p++ ) {
			return (0x22) ;
		}
	}

	return( 0 );
}

//********************************************************************************
// Function Name 	: EraseUserMat128
//********************************************************************************
UINT_8 EraseUserMat128(UINT_8 StartBlock, UINT_8 EndBlock )
{
	UINT_32 i;
	UINT_32	UlReadVal, UlCnt ;

	IOWrite32A( 0xE0701C, 0x00000000);
	RamWrite32A( 0xF007, 0x00000000 );


	for( i=StartBlock ; i<EndBlock ; i++) {
		RamWrite32A( 0xF00A, ( i << 10 ) );
		RamWrite32A( 0xF00C, 0x00000020 );


		WitTim( 5 );
		UlCnt = 0;
		do{

			WitTim( 1 );
			if( UlCnt++ > 10 ){
				IOWrite32A( 0xE0701C, 0x00000002);
				return (0x31) ;
			}
			RamRead32A( 0xF00C, &UlReadVal );
		}while ( UlReadVal != 0 );
	}
	IOWrite32A( 0xE0701C, 0x00000002);
	return(0);

}

//********************************************************************************
// Function Name 	: ProgramFlash128_Standard
//********************************************************************************
UINT_8 ProgramFlash128_Standard( DOWNLOAD_TBL_EXT* ptr )
{
	UINT_32	UlReadVal, UlCnt , UlNum ;
	UINT_8	data[(BURST_LENGTH_FC + 3)];
	UINT_32 i, j;

	const UINT_8 *NcFromVal = ptr->FromCode + 64;
	const UINT_8 *NcFromVal1st = ptr->FromCode;
	UINT_8 UcOddEvn;

	IOWrite32A( 0xE0701C, 0x00000000);
	RamWrite32A( 0xF007, 0x00000000 );
	RamWrite32A( 0xF00A, 0x00000010 );
	data[0] = 0xF0;
	data[1] = 0x08;
	data[2] = 0x00;

	for(i=1 ; i< ( ptr->SizeFromCode / 64 ) ; i++)
	{
		if( ++UcOddEvn >1 )  	UcOddEvn = 0;
		if (UcOddEvn == 0) data[1] = 0x08;
		else 			   data[1] = 0x09;

#if (BURST_LENGTH_FC == 32)
		data[2] = 0x00;
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
	  	data[2] = 0x20;
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
#elif (BURST_LENGTH_FC == 64)
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
#endif

		RamWrite32A( 0xF00B, 0x00000010 );
		UlCnt = 0;
		if (UcOddEvn == 0){
			do{
				RamRead32A( 0xF00C, &UlReadVal );
				if( UlCnt++ > 250 ) {
					IOWrite32A( 0xE0701C, 0x00000002);
					return (0x41) ;
				}
			}while ( UlReadVal  != 0 );
		 	RamWrite32A( 0xF00C, 0x00000004 );
		}else{
			do{
				RamRead32A( 0xF00C, &UlReadVal );
				if( UlCnt++ > 250 ) {
					IOWrite32A( 0xE0701C, 0x00000002);
					return (0x41) ;
				}
			}while ( UlReadVal  != 0 );
			RamWrite32A( 0xF00C, 0x00000008 );
		}
	}
	UlCnt = 0;
	do{
		WitTim( 1 );
		RamRead32A( 0xF00C, &UlReadVal );
		if( UlCnt++ > 250 ) {
			IOWrite32A( 0xE0701C, 0x00000002);
			return (0x41) ;
		}
	}while ( (UlReadVal & 0x0000000C) != 0 );

	{
		RamWrite32A( 0xF00A, 0x00000000 );
		data[1] = 0x08;

#if (BURST_LENGTH_FC == 32)
		data[2] = 0x00;
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal1st++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
	  	data[2] = 0x20;
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal1st++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
#elif (BURST_LENGTH_FC == 64)
		data[2] = 0x00;
		UlNum = 3;
		for(j=0 ; j < BURST_LENGTH_FC; j++){
			data[UlNum++] = *NcFromVal1st++;
		}
		CntWrt( data, BURST_LENGTH_FC+3 );
#endif

		RamWrite32A( 0xF00B, 0x00000010 );
		UlCnt = 0;
		do{
			RamRead32A( 0xF00C, &UlReadVal );
			if( UlCnt++ > 250 ) {
				IOWrite32A( 0xE0701C , 0x00000002);
				return (0x41) ;
			}
		}while ( UlReadVal != 0 );
	 	RamWrite32A( 0xF00C, 0x00000004 );
	}

	UlCnt = 0;
	do{
		WitTim( 1 );
		RamRead32A( 0xF00C, &UlReadVal );
		if( UlCnt++ > 250 ) {
			IOWrite32A( 0xE0701C , 0x00000002);
			return (0x41) ;
		}
	}while ( (UlReadVal & 0x0000000C) != 0 );

	IOWrite32A( 0xE0701C, 0x00000002);
	return( 0 );
}


//********************************************************************************
// Function Name 	: FlashMultiRead
//********************************************************************************
UINT_8	FlashMultiRead( UINT_8 SelMat, UINT_32 UlAddress, UINT_32 *PulData , UINT_8 UcLength )
{
	UINT_8	i	 ;



	if( SelMat != USER_MAT && SelMat != INF_MAT0 && SelMat != INF_MAT1 && SelMat != INF_MAT2  )	return 10;

	if( UlAddress > 0x000003FF )											return 9;

	IOWrite32A( 0xE07008 , 0x00000000 | (UINT_32)(UcLength-1) );
	IOWrite32A( 0xE0700C , ((UINT_32)SelMat << 16) | ( UlAddress & 0x00003FFF ) );

	IOWrite32A( 0xE0701C , 0x00000000);
	IOWrite32A( 0xE07010 , 0x00000001 );
	for( i=0 ; i < UcLength ; i++ ){
		IORead32A( 0xE07000 , &PulData[i] ) ;
	}

	IOWrite32A( 0xE0701C , 0x00000002);
	return( 0 ) ;
}

//********************************************************************************
// Function Name 	: FlashBlockErase
//********************************************************************************
UINT_8	FlashBlockErase( UINT_8 SelMat , UINT_32 SetAddress )
{
	UINT_32	UlReadVal, UlCnt;
	UINT_8	ans	= 0 ;



	if( SelMat != USER_MAT && SelMat != INF_MAT0 && SelMat != INF_MAT1 && SelMat != INF_MAT2  )	return 10;

	if( SetAddress > 0x000003FF )											return 9;


	ans	= UnlockCodeSet();
	if( ans != 0 )	return( ans ) ;

	WritePermission();
	if( SelMat != USER_MAT ){
		if( SelMat == INF_MAT2 )	IOWrite32A( 0xE07CCC, 0x00006A4B );
		else						IOWrite32A( 0xE07CCC, 0x0000C5AD );
	}
	AddtionalUnlockCodeSet();

	IOWrite32A( 0xE0700C , ((UINT_32)SelMat << 16) | ( SetAddress & 0x00003C00 )) ;

	IOWrite32A( 0xE0701C , 0x00000000);
	IOWrite32A( 0xE07010 , 4 ) ;

	WitTim( 5 ) ;

	UlCnt	= 0 ;

	do {
		if( UlCnt++ > 100 ){	ans = 2;	break;	} ;

		IORead32A( FLASHROM_FLAINT, &UlReadVal ) ;
	} while( ( UlReadVal & 0x00000080 ) != 0 ) ;

	IOWrite32A( 0xE0701C , 0x00000002);
	ans	= UnlockCodeClear();
	if( ans != 0 )	return( ans ) ;

	return( ans ) ;
}
//********************************************************************************
// Function Name 	: FlashBlockWrite
//********************************************************************************
UINT_8	FlashBlockWrite( UINT_8 SelMat , UINT_32 SetAddress , UINT_32 *PulData)
{
	UINT_32	UlReadVal, UlCnt;
	UINT_8	ans	= 0 ;
	UINT_8	i	 ;

	if( SelMat != INF_MAT0 && SelMat != INF_MAT1 && SelMat != INF_MAT2  )			return 10;
	//
	if( SetAddress > 0x000003FF )							return 9;

	ans	= UnlockCodeSet();
	if( ans != 0 )	return( ans ) ;

	WritePermission();
	if( SelMat != USER_MAT ){
		if( SelMat == INF_MAT2 )	IOWrite32A( 0xE07CCC, 0x00006A4B );
		else						IOWrite32A( 0xE07CCC, 0x0000C5AD );
	}
	AddtionalUnlockCodeSet();

	IOWrite32A( 0xE0700C , ((UINT_32)SelMat << 16) | ( SetAddress & 0x000010 )) ;

	IOWrite32A( 0xE0701C , 0x00000000);
	IOWrite32A( 0xE07010 , 2 ) ;


	UlCnt	= 0 ;

	for( i=0 ; i< 16 ; i++ ){
		IOWrite32A( 0xE07004 , PulData[i]  );
	}
	do {
		if( UlCnt++ > 100 ){	ans = 2;	break;	} ;

		IORead32A( 0xE07018 , &UlReadVal ) ;
	} while( ( UlReadVal & 0x00000080 ) != 0 ) ;

	IOWrite32A( 0xE07010 , 8  );

	do {
		if( UlCnt++ > 100 ){	ans = 2;	break;	} ;

		IORead32A( 0xE07018 , &UlReadVal ) ;
	} while( ( UlReadVal & 0x00000080 ) != 0 ) ;

	IOWrite32A( 0xE0701C , 0x00000002);
	ans	= UnlockCodeClear();
	return( ans ) ;

}

//********************************************************************************
// Function Name 	: Mat2ReWrite
//********************************************************************************
UINT_8 Mat2ReWrite( void )
{
	UINT_32	UlMAT2[32];
	UINT_32	UlCKSUM=0;
	UINT_8	ans , i ;
	UINT_32	UlCkVal ,UlCkVal_Bk;

	ans = FlashMultiRead( INF_MAT2, 0, UlMAT2, 32 );
	if(ans)	return( 0xA0 );

	if( UlMAT2[FT_REPRG] == PRDCT_WR || UlMAT2[FT_REPRG] == USER_WR ){
		return( 0x00 );
	}

	if( UlMAT2[CHECKCODE1] != CHECK_CODE1 )	return( 0xA1 );
	if( UlMAT2[CHECKCODE2] != CHECK_CODE2 )	return( 0xA2 );

	for( i=16 ; i<MAT2_CKSM ; i++){
		UlCKSUM += UlMAT2[i];
	}
	if(UlCKSUM != UlMAT2[MAT2_CKSM])		return( 0xA3 );

	UlMAT2[FT_REPRG] = USER_WR;

	UlCkVal_Bk = 0;
	for( i=0; i < 32; i++ ){
		UlCkVal_Bk +=  UlMAT2[i];
	}

	ans = FlashBlockErase( INF_MAT2 , 0 );
	if( ans != 0 )	return( 0xA4 ) ;

	ans = FlashBlockWrite( INF_MAT2 , 0 , UlMAT2 );
	if( ans != 0 )	return( 0xA5 ) ;
	ans = FlashBlockWrite( INF_MAT2 , (UINT_32)0x10 , &UlMAT2[0x10] );
	if( ans != 0 )	return( 0xA5 ) ;

	ans =FlashMultiRead( INF_MAT2, 0, UlMAT2, 32 );
	if( ans )	return( 0xA0 );

	UlCkVal = 0;
	for( i=0; i < 32; i++ ){
		UlCkVal +=  UlMAT2[i];
	}

	if( UlCkVal != UlCkVal_Bk )		return( 0xA6 );

	return( 0x01 );
}

//********************************************************************************
// Function Name 	: FlashUpdate128
//********************************************************************************
UINT_8 FlashUpdate128( DOWNLOAD_TBL_EXT* ptr )
{
	UINT_8 ans=0;
	UINT_32	UlReadVal, UlCnt ;

 	ans = CoreResetwithoutMC128();
 	if(ans != 0) return( ans );

	ans = Mat2ReWrite();
 	if(ans != 0 && ans != 1) return( ans );

 	ans = PmemUpdate128( ptr );
	if(ans != 0) return( ans );

//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------
	if( UnlockCodeSet() != 0 ) 		return (0x33) ;
	WritePermission();
	AddtionalUnlockCodeSet();

 #if	0
 	ans = EraseUserMat128(0, 10); // Full Block.
#else
 	ans = EraseUserMat128(0, 7);  // 0-6 Block for use user area.
#endif
	if(ans != 0){
		if( UnlockCodeClear() != 0 ) 	return (0x32) ;
		else					 		return( ans );
	}

 	ans = ProgramFlash128_Standard( ptr );
	if(ans != 0){
		if( UnlockCodeClear() != 0 ) 	return (0x43) ;
		else					 		return( ans );
	}

	if( UnlockCodeClear() != 0 ) 	return (0x43) ;

//--------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------

	IOWrite32A( 0xE0701C , 0x00000000);
	RamWrite32A( 0xF00A, 0x00000000 );
	RamWrite32A( 0xF00D, ptr->SizeFromCodeValid );

	RamWrite32A( 0xF00C, 0x00000100 );
	WitTim( 6 );
	UlCnt = 0;
	do{
		RamRead32A( 0xF00C, &UlReadVal );
		if( UlCnt++ > 10 ) {
			IOWrite32A( 0xE0701C , 0x00000002);
			return (0x51) ;
		}
		WitTim( 1 );
	}while ( UlReadVal != 0 );

	RamRead32A( 0xF00D, &UlReadVal );

	if( UlReadVal != ptr->SizeFromCodeCksm ) {
		IOWrite32A( 0xE0701C , 0x00000002);
		return( 0x52 );
	}

	IOWrite32A( SYSDSP_REMAP,				0x00001000 ) ;
	WitTim( 15 ) ;
	IORead32A( ROMINFO,				(UINT_32 *)&UlReadVal ) ;
	if( UlReadVal != 0x0A)		return( 0x53 );

	return( 0 );
}

//********************************************************************************
// Function Name 	: FlashDownload_128
//********************************************************************************
UINT_8 FlashDownload128( UINT_8 ModuleVendor, UINT_8 ActVer, UINT_8 MasterSlave, UINT_8 FWType)
{
	DOWNLOAD_TBL_EXT* ptr = NULL;
	UINT_32	data1 = 0;
	UINT_32	data2 = 0;

	ptr = ( DOWNLOAD_TBL_EXT * )DTbl ;

	do {
		if(1) {

			// UploadFileが64Byte刻みにPaddingされていないならば、Error。
			if( ( ptr->SizeFromCode % 64 ) != 0 )	return (0xF1) ;

			if(!RamRead32A(0x8000, &data1)) {
				if(!RamRead32A(0x8004, &data2)) {
					if ((data1 == (ptr->FromCode[153] << 24 |
								   ptr->FromCode[154] << 16 |
								   ptr->FromCode[155] << 8 |
								   ptr->FromCode[156])) &&
						(data2 == (ptr->FromCode[158] << 24 |
								   ptr->FromCode[159] << 16 |
								   ptr->FromCode[160] << 8 |
								   ptr->FromCode[161]))) {
							TRACE("The FW 0x%x:0x%x is the latest, no need to upload\n", data1, data2);
							return 0;
					} else {
							TRACE("0x8000 = 0x%x 0x8004 = 0x%x is not the latest 0x%x:0x%x, will upload\n", data1, data2,
									(ptr->FromCode[153] << 24 |
									 ptr->FromCode[154] << 16 |
									 ptr->FromCode[155] << 8 |
									 ptr->FromCode[156]),
									(ptr->FromCode[158] << 24 |
									 ptr->FromCode[159] << 16 |
									 ptr->FromCode[160] << 8 |
									 ptr->FromCode[161]));
					}
				} else {
					TRACE("Read 0x8004 failed\n");
					return 0xF2;
				}
			} else {
				TRACE("Read 0x8000 failed\n");
				return 0xF2;
			}

			return FlashUpdate128( ptr );
		}
		ptr++ ;
	} while (ptr->Index != 0xFFFFFF ) ;

	return 0xF0 ;
}


UINT_8 LoadUserAreaToPM( void )
{
	DOWNLOAD_TBL_EXT* ptr ;
	UINT_32 UlReadVernum;
	UINT_8 ModuleVendor;
	UINT_8 ActVer;
	UINT_8 MasterSlave ;

	ptr = ( DOWNLOAD_TBL_EXT * )DTbl ;

	RamRead32A( 0x8000 , &UlReadVernum );
	ModuleVendor = (UINT_8)((UlReadVernum & 0xFF000000) >> 24);
	RamRead32A( 0x8004 , &UlReadVernum );
	ActVer = (UINT_8)((UlReadVernum & 0x0000FF00) >> 8);
	MasterSlave = ( UINT_8 )( ( UlReadVernum & 0xFF000000) >> 24 );

	TRACE(" VENDER = %02x , ACT = %02x, MasterSlave = %02x \n", ModuleVendor , ActVer, MasterSlave );
	do {
		if( ptr->Index == ( ((UINT_32)ModuleVendor<<16) + ((UINT_32)ActVer<<8) + MasterSlave) ) {

			if( ( ptr->SizeFromCode % 64 ) != 0 )	return (0xF1) ;

			return LoadUareToPM( ptr , 0 );
		}
		ptr++ ;
	} while (ptr->Index != 0xFFFF ) ;

	return 0xF0 ;
}
UINT_8 LoadUareToPM( DOWNLOAD_TBL_EXT* ptr , UINT_8 mode )
{
	UINT_8 ans=0;
	UINT_32 UlReadVal=0;
	UINT_32 UlReadVer;
	UINT_32 UlCnt=0;

	if( !mode ){
		RamRead32A( 0x8000 , &UlReadVer );
		if( (UlReadVer & 0xFF000000) == 0x01000000 ){
			RamWrite32A( 0xE000 , 0x00000000 ); 	// to boot
			WitTim( 5 ) ;												// Bootプログラムを回すのに15msec必要。
			//neo: need to reduce this time
			IORead32A( ROMINFO, 			(UINT_32 *)&UlReadVal ) ;
		}else{
			return( 0x01 ); // NG
		}
		if( UlReadVal != 0x0B ){
			IOWrite32A( SYSDSP_REMAP,				0x00001400 ) ;		// CORE_RST[12], MC_IGNORE2[10] = 1
			WitTim( 5 ) ;												// Bootプログラムを回すのに15msec必要。
			//neo: need to reduce this time
			IORead32A( ROMINFO, 			(UINT_32 *)&UlReadVal ) ;
			if( UlReadVal != 0x0B) {
				return( 0x02 );
			}
		}

		ans = PmemUpdate128( ptr ); 		// Update the special program for updating the flash memory.
		if(ans != 0) return( ans );
	}

	IOWrite32A( 0xE0701C , 0x00000000);
	RamWrite32A( 0xF007, 0x00000000 );		// boot command Access Setup
	RamWrite32A( 0x5004 , 0x00000000 ); 	// Trans user data from flash memory to pm

	do{
		WitTim( 1 );
		if( UlCnt++ > 10 ) {
			IOWrite32A( 0xE0701C , 0x00000002);
			return (0x10) ; 								// trans ng
		}
		RamRead32A( 0x5004, &UlReadVal );					// PmCheck.ExecFlagの読み出し
	}while ( UlReadVal != 0 );
	IOWrite32A( 0xE0701C , 0x00000002);

	return( 0 );

}
UINT_8	RdBurstUareaFromPm( UINT_32 UlAddress, UINT_8 *PucData , UINT_8 UcLength , UINT_8 mode )
{
	UINT_8 i = 0;
	if(!UcLength)	return(0xff);
	if( !mode ){
		RamWrite32A( 0x5000 , UlAddress );
	}
	TRACE("UlAddress:%0x, UcLength:%u", UlAddress, UcLength);

	RamWrite32A( 0x5002 , (UINT_32)UcLength );
	WitTim( 1 ) ;													// 1ms Wait for prepare trans data
	CntRd( 0x5002 , PucData , (UINT_16)UcLength+1);
	for (i = 0; i < UcLength; i++)
		TRACE("data[%d]: 0x%0x", i, *(PucData+i));
	TRACE("end burst");

	return( 0 );
}

//********************************************************************************
// Function Name 	: WrUareaToPm
// Retun Value		: NON
// Argment Value	: chiperase
// Explanation		: Write user area data to PM
// History			: First edition
//********************************************************************************
UINT_8	WrUareaToPm( UINT_32 UlAddress, UINT_8 *PucData , UINT_8 UcLength , UINT_8 mode )
{
    UINT_8 i;
	UINT_8	PucBuf[254];	// address2Byte + data252Byte

	if(!UcLength)	return(0xff);

	if (UcLength > 252) {
		TRACE("long size beyonded!!!");
		return(0xff);
	}
	PucBuf[0] = 0x50;
	PucBuf[1] = 0x01;

	for(i = 0 ; i < UcLength ; i++)
	{
		PucBuf[i+2] = PucData[i];
TRACE("[%02x:%02x]",i+2,PucBuf[i+2]);
	}
TRACE("\n");
	if( !mode ){
		RamWrite32A( 0x5000 , UlAddress );
	}
	CntWrt( PucBuf , (UINT_16)UcLength+3 );

	return( 0 );
}

//********************************************************************************
// Function Name 	: WrUareaToFlash
// Retun Value		: NON
// Argment Value	: chiperase
// Explanation		: Update user area data from PM to Flash memory
// History			: First edition
//********************************************************************************
UINT_8	WrUareaToFlash( void )
{
		UINT_32 UlReadVal;
		UINT_32 UlCntE=0;
		UINT_32 UlCntW=0;
		UINT_8	ans=0;

		TRACE("%s, entry 000 !!!\n", __func__);

		ans = UnlockCodeSet();
		if( ans != 0 )	return( ans ) ; 						// Unlock Code Set
		WritePermission();									// Write Permission
		AddtionalUnlockCodeSet();							// Additional Unlock Code Set

		IOWrite32A( 0xE0701C , 0x00000000); 	//
	//	RamWrite32A( 0xF007, 0x00000000 );						// FlashAccess Setup

		RamWrite32A( 0x5005 , 0x00000000 ); 	// erase User area data on flash memory

		WitTim( 10 );//neo: need to reduce this time
		do{
			WitTim( 1 );
			if( UlCntE++ > 20 ) {
				IOWrite32A( 0xE0701C , 0x00000002);
				if( UnlockCodeClear() != 0 )	return (0x11) ; 			// unlock code clear ng
				TRACE("%s, entry 222 !!!\n", __func__);
				return (0x10) ; 								// erase ng
			}
			RamRead32A( 0x5005, &UlReadVal );					// complite Flagの読み出し
		}while ( UlReadVal != 0 );

		RamWrite32A( 0x5006 , 0x00000000 ); 	// write user area data from PM to flash memory

		WitTim( 20 );
		do{
			WitTim( 1 );
			if( UlCntW++ > 300 ) {
				IOWrite32A( 0xE0701C , 0x00000002);
				if( UnlockCodeClear() != 0 )	return (0x21) ; 			// unlock code clear ng
				return (0x20) ; 								// write ng
			}
			RamRead32A( 0x5006, &UlReadVal );					// complite Flagの読み出し
		}while ( UlReadVal != 0 );
		IOWrite32A( 0xE0701C , 0x00000002);
		if( UnlockCodeClear() != 0 )	return (0x31) ; 			// unlock code clear ng
		TRACE("[E-CNT: %d] [W-CNT: %d]\n",UlCntE,UlCntW);
		return( 0 );

}



