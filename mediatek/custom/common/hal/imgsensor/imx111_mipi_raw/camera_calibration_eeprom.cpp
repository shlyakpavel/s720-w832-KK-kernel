#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>
#include <cutils/xlog.h>

#include "MediaHal.h"
//#include "src/lib/inc/MediaLog.h" //#include "src/lib/inc/MediaLog.h"

#include "camera_custom_nvram.h"

//eeprom
#include "eeprom.h"
#include "eeprom_define.h"
extern "C"{
//#include "eeprom_layout.h"
#include "camera_custom_eeprom.h"
}
#include "camera_calibration_eeprom.h"

#include <stdio.h> //for rand?
#include <stdlib.h> //for rand?
//#include "src/core/scenario/camera/mhal_cam.h" //for timer
#ifdef LOG_TAG
#undef LOG_TAG  
#endif
#define LOG_TAG "CAM_CAL_EEPROM"
//Lens


#define DEBUG_CALIBRATION_LOAD

#define CUSTOM_EEPROM_ROTATION CUSTOM_EEPROM_ROTATION_180_DEGREE    
#define CUSTOM_EEPROM_COLOR_ORDER CUSTOM_EEPROM_COLOR_SHIFT_01     //SeanLin@20110629: 

#define CUSTOM_EEPROM_PART_NUMBERS_START_ADD 5
//#define CUSTOM_EEPROM_NEW_MODULE_NUMBER_CHECK 1 //

#define EEPROM_SHOW_LOG 1

#ifdef EEPROM_SHOW_LOG
#define CAMEEPROM_LOG(fmt, arg...)    XLOGE(fmt, ##arg)
#else
#define CAMEEPROM_LOG(fmt, arg...)    void(0)
#endif

////<
UINT32 DoDefectLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
UINT32 DoPregainLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
UINT32 DoISPSlimShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
UINT32 DoISPDynamicShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
UINT32 DoISPFixShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
UINT32 DoISPSensorShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
extern void LSC_AVGRAW_to_Gaintable(CAM_CAL_EEPROM_DATA eeprom_data, UINT32 * gain_table);
enum
{
	CALIBRATION_LAYOUT_Lens_LITEON_OTP=0,  //YW 120715
	MAX_CALIBRATION_LAYOUT_NUM
};
 
#define CALIBRATION_DATA_SIZE_SLIM_LSC1 	656
#define CALIBRATION_DATA_SIZE_SLIM_LSC2		3716
#define CALIBRATION_DATA_SIZE_DYANMIC_LSC1	2048
#define CALIBRATION_DATA_SIZE_DYANMIC_LSC2	5108
#define CALIBRATION_DATA_SIZE_FIX_LSC1		4944
#define CALIBRATION_DATA_SIZE_FIX_LSC2		8004
#define CALIBRATION_DATA_SIZE_SENSOR_LSC1	20
#define CALIBRATION_DATA_SIZE_SENSOR_LSC2	3088
#define CALIBRATION_DATA_SIZE_SUNNY_Q8N03D_LSC1 656 //SL 110317

#define CALIBRATION_DATA_SIZE_Lens_LITEON_OTP 656

#define MAX_CALIBRATION_DATA_SIZE			CALIBRATION_DATA_SIZE_FIX_LSC2

#define LSC_DATA_BYTES 4

/*****************************************************************************/
/* define for M24C08F EEPROM on L10 project */
/*****************************************************************************/
#define RChannel_Address 0xA
#define GrChannel_Address 0xE7
#define GbChannel_Address 0xC4
#define BChannel_Address 0xA1

/*****************************************************************************/


enum
{
	CALIBRATION_ITEM_DEFECT = 0,
	CALIBRATION_ITEM_PREGAIN,
	CALIBRATION_ITEM_SHADING,
	MAX_CALIBRATION_ITEM_NUM	
};

UINT32 GetCalErr[MAX_CALIBRATION_ITEM_NUM] =
{
	EEPROM_ERR_NO_DEFECT,
	EEPROM_ERR_NO_PREGAIN,
	EEPROM_ERR_NO_SHADING,
};

typedef struct
{
	UINT16 Include; //calibration layout include this item?
	UINT32 StartAddr; // item Start Address
	UINT32 (*GetCalDataProcess)(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData);
} CALIBRATION_ITEM_STRUCT;

typedef struct
{
	UINT32 HeaderAddr; //Header Address
	UINT32 HeaderId;   //Header ID
	UINT32 CheckShading; // Do check shading ID?
	UINT32 ShadingID;    // Shading ID
	CALIBRATION_ITEM_STRUCT CalItemTbl[MAX_CALIBRATION_ITEM_NUM];
} CALIBRATION_LAYOUT_STRUCT;

UINT8 gIsInited = 0;
const CALIBRATION_LAYOUT_STRUCT CalLayoutTbl[MAX_CALIBRATION_LAYOUT_NUM]=
{
	{//CALIBRATION_LAYOUT_Lens_LITEON_OTP
		0x0000FFFF, 0x00000F50, 0x00000000,0xFFFFFFFF ,      //A5141 ID=0x4B00, here is 0x0000004B
		{
			{0x00000000, 0x00000000, DoDefectLoad}, //CALIBRATION_ITEM_DEFECT
			{0x00000001, 0x00000000, DoPregainLoad}, //CALIBRATION_ITEM_PREGAIN
			{0x00000001, 0x00000000, DoISPSensorShadingLoad}  //CALIBRATION_ITEM_SHADING of WCP1
}
	}	
};


#if 0
//#if 0 // IMX111
UINT32 OTP_golden_rval =0x2b0;
UINT32 OTP_golden_gval =0x404;
UINT32 OTP_golden_bval =0x24f;
UINT32 OTP_current_rval =0x253;
UINT32 OTP_current_gval =0x3ff;
UINT32 OTP_current_bval =0x22A;
//#else // IMX219
UINT32 OTP_golden_rval =0x251;
UINT32 OTP_golden_gval =0x3fc;
UINT32 OTP_golden_bval =0x2b7;
UINT32 OTP_current_rval =0x253;
UINT32 OTP_current_gval =0x3ff;
UINT32 OTP_current_bval =0x22A;
//#endif
#endif
UINT32 OTP_golden_rval[2]={0x2b0,0x251};
UINT32 OTP_golden_gval[2]={0x404,0x3fc};
UINT32 OTP_golden_bval[2]={0x24f,0x2b7};
UINT32 OTP_current_rval =0x253;
UINT32 OTP_current_gval =0x3ff;
UINT32 OTP_current_bval =0x22A;
UINT32 Current_Sensor=0;
#define Main_Sensor_ID 0x111
#define Main_Backup_Sensor_ID 0x219
/*****************************************************************************/
/* Save LSC data for M24C08F EEPROM on L10 project */
/*****************************************************************************/
//unsigned char RChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
//unsigned char GrChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
//unsigned char GbChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
//unsigned char BChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
/*****************************************************************************/
CAM_CAL_EEPROM_DATA channel_lsc_data;
UINT32 GainTable[MAX_GAIN_TABLE_SIZE];


//#define LensMIPI_OTP
#if 1 //defined(LensMIPI_OTP)

#define FALSE 0
#define TRUE 1


typedef struct
        {
    MUINT32  Addr;
    MUINT32  Para;
} SENSOROTP_REG_STRUCT;

SENSOROTP_REG_STRUCT Lens_OTP_Parameters[]={
{0xffffffff, 0xffffffff}
};


static UINT16 LensOTP_read_cmos_sensor(INT32 epprom_fd,UINT16 adress)
{
	stEEPROM_INFO_STRUCT  eepromCfg;
	UINT16 Temp=0;
	UINT16 ReValue;
	UINT8 VAL=0;
	eepromCfg.u4Offset = (UINT16)adress;
    eepromCfg.u4Length = 2;
    eepromCfg.pu1Params = (u8 *)&Temp; 
    ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
    
  //  ReValue=(((Temp<<8)&0xff00)|((Temp>>8)&0x00ff));
     ReValue=Temp;
	 CAMEEPROM_LOG("[LensOTP_read_cmos_sensor]Read Temp = %x",Temp);
	 CAMEEPROM_LOG("[LensOTP_read_cmos_sensor]Read revalue = %x",ReValue);

	
	
	
	return ReValue;
}

static void LensOTP_write_cmos_sensor(INT32 epprom_fd,UINT16 adress,UINT16 para)
{   
	stEEPROM_INFO_STRUCT  eepromCfg;
	UINT16 Temp=0;
	UINT16 ReValue;
	eepromCfg.u4Offset = (UINT16)adress;
    eepromCfg.u4Length = 2;
	Temp = para;
    eepromCfg.pu1Params = (u8 *)&Temp;
    ioctl(epprom_fd, EEPROMIOC_S_WRITE , &eepromCfg);
}

#if 0 // not used
static void LensMIPI_OTP_Para_read(INT32 epprom_fd,  PEEPROM_DATA_STRUCT pEerom_data)
{
    CAMEEPROM_LOG("[%s]:enter LensMIPI_OTP_Para_read\n",__func__);
	
	UINT16 i=0;
	MUINT32 TempOTPpara=0;
	PEEPROM_DATA_STRUCT OTPEerom_data = (PEEPROM_DATA_STRUCT)pEerom_data; 

#if 0	
	OTP_current_rval = LensOTP_read_cmos_sensor(epprom_fd, 0x3808);
	OTP_current_gval = LensOTP_read_cmos_sensor(epprom_fd, 0x380A);
    OTP_current_bval = LensOTP_read_cmos_sensor(epprom_fd, 0x380C);
#else

  OTP_current_rval = LensOTP_read_cmos_sensor(epprom_fd, 0x0);
	OTP_current_gval = LensOTP_read_cmos_sensor(epprom_fd, 0x1);
  OTP_current_bval = LensOTP_read_cmos_sensor(epprom_fd, 0x2);
  OTP_current_bval = LensOTP_read_cmos_sensor(epprom_fd, 0x3);
#endif	
/*
	 SENSOROTP_REG_STRUCT *pLens_OTP_Parameters=Lens_OTP_Parameters;
	    
	OTPEerom_data->Shading.SensorCalTable[0]=32;  //data formart  32 bits
	OTPEerom_data->Shading.SensorCalTable[1]=0;
	OTPEerom_data->Shading.SensorCalTable[2]=0;
	OTPEerom_data->Shading.SensorCalTable[3]=0;
	OTPEerom_data->Shading.SensorCalTable[4]=53;  //(8+(106x2)+10x2)(8bits)=240 (8bits)   == 240(8bits)/4=60 (32bits)    valid data = 106x2(8bits)/4=53(32bits)
	OTPEerom_data->Shading.SensorCalTable[5]=0;
	OTPEerom_data->Shading.SensorCalTable[6]=0;
	OTPEerom_data->Shading.SensorCalTable[7]=0;

    while((pLens_OTP_Parameters->Addr !=0xffffffff) && (pLens_OTP_Parameters->Para!=0xffffffff))
	{
		TempOTPpara = LensOTP_read_cmos_sensor(epprom_fd, pLens_OTP_Parameters->Addr);
		OTPEerom_data->Shading.SensorCalTable[i*2+8]=(UINT8)(TempOTPpara&0xFF);   //LOW 8 bits  in 16 bits
		OTPEerom_data->Shading.SensorCalTable[i*2+9]=(UINT8)((TempOTPpara>>8)&0xFF);   //High 8 bits  in 16 bits
		pLens_OTP_Parameters++;
		i++;
	}
	
	 CAMEEPROM_LOG("LensMIPI_OTP_Para_read(ok) i=%d \n",i);
	 i=0;
*/
	 
}
static UINT8 LensMIPI_Auto_reading(INT32 epprom_fd,PEEPROM_DATA_STRUCT pEerom_data)
{
	 CAMEEPROM_LOG("[%s] Enter: \n",__func__);
	UINT16 checkvalue1 = LensOTP_read_cmos_sensor(epprom_fd, 0x38E4);
	UINT16 checkvalue2 = LensOTP_read_cmos_sensor(epprom_fd, 0x38E6);
	UINT16 bsuccess = FALSE;
	if((0xFFFF == checkvalue1) || (0xFFFF == checkvalue2))
    {   
			bsuccess = TRUE;
			CAMEEPROM_LOG("****LensMIPI_Auto_reading reading**** \n");
		LensMIPI_OTP_Para_read(epprom_fd, pEerom_data);
    }
	return bsuccess;
}
#endif // not used

static UINT8 LensMIPI_OTP_reading (INT32 epprom_fd,PEEPROM_DATA_STRUCT pEerom_data)
{
	UINT8 rc = FALSE;
	//get the type that we have the params
	//Do the OTP reading, From Type 35 to 30. read the data from the right type
	CAMEEPROM_LOG("LensMIPI_OTP_reading reading start! \n");
	
	OTP_current_rval = LensOTP_read_cmos_sensor(epprom_fd, 0x0000);
	OTP_current_bval = LensOTP_read_cmos_sensor(epprom_fd, 0x0200);
    OTP_current_gval = LensOTP_read_cmos_sensor(epprom_fd, 0x0400);

  if( (OTP_current_rval != 0) || (OTP_current_bval != 0) ||(OTP_current_gval != 0))
  	{
  		rc = TRUE;
  	}
	
	CAMEEPROM_LOG("The OTP reading success\n");
	return rc;
}


static UINT8 LensMIPI_OTP_reading_AF (INT32 epprom_fd,PEEPROM_DATA_STRUCT pEerom_data)
{
	UINT8 rc = FALSE;
	UINT16 LensStartCurrent, Macro10cm;

	CAMEEPROM_LOG("LensMIPI_OTP_reading_AF reading start! \n");
	
	LensStartCurrent = LensOTP_read_cmos_sensor(epprom_fd, 0x0600);
	Macro10cm = LensOTP_read_cmos_sensor(epprom_fd, 0x0800);

  if( (LensStartCurrent != 0) || (Macro10cm != 0) )
  	{
  		rc = TRUE;
  	}
	
	
#ifdef DEBUG_CALIBRATION_LOAD
	CAMEEPROM_LOG("======================AF READ OTP VALUE  ==================\n");	
	CAMEEPROM_LOG("[FM50AF_LENS_V5][LensStartCurrent] = 0x%x\n", LensStartCurrent);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][Macro10cm] = 0x%x\n", Macro10cm);
	CAMEEPROM_LOG("======================AF READ OTP VALUE  ==================\n");
#endif

	return rc;
}


static UINT8 LensMIPI_OTP_Calibration(INT32 epprom_fd,PEEPROM_DATA_STRUCT pEerom_data)
{
     UINT8 bsuccess ;        
     bsuccess=LensMIPI_OTP_reading(epprom_fd, pEerom_data);
     if (bsuccess==FALSE)
        {
        CAMEEPROM_LOG("LensMIPI_OTP_reading Failed! \n");
            return FALSE;
        }
	 
	return TRUE;
		
}

#endif


UINT32 DoDefectLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{
    PEEPROM_DATA_STRUCT pEerom_data = (PEEPROM_DATA_STRUCT)pGetSensorCalData;
    UINT32 err= GetCalErr[CALIBRATION_ITEM_DEFECT];
    CAMEEPROM_LOG("DoDefectLoad, please remake it\n");
    if(pEerom_data->Defect.ISPComm.CommReg[EEPROM_INFO_IN_COMM_LOAD] ==CAL_DATA_LOAD)
    {		
      CAMEEPROM_LOG("ALREADY HAS UPDATED DEFECT TABLE");
    }
    else
    {
      CAMEEPROM_LOG("PLEASE PUT Defect Table here!");	
      err = 0;
    }
    return err;

//#endif
//	return CAL_GET_DEFECT_FLAG|CAL_GET_PARA_FLAG|CAL_GET_PARA_FLAG;
}


UINT32 DoPregainLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{
    stEEPROM_INFO_STRUCT  eepromCfg;
   // double ratio=0;
    PEEPROM_DATA_STRUCT pEerom_data = (PEEPROM_DATA_STRUCT)pGetSensorCalData;
    UINT32 err= GetCalErr[CALIBRATION_ITEM_PREGAIN];

	CAMEEPROM_LOG("======================DoPregainLoad==================\n");
  

	LensMIPI_OTP_Calibration(epprom_fd, pEerom_data);

	LensMIPI_OTP_reading_AF (epprom_fd,pEerom_data);
   
    if(Main_Backup_Sensor_ID == pEerom_data->Shading.ISPComm.CommReg[0])
    {
        Current_Sensor=1;
    }

#ifdef DEBUG_CALIBRATION_LOAD
	CAMEEPROM_LOG("======================AWB READ OTP VALUE  ==================\n");	
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_golden_r] = 0x%x\n", OTP_golden_rval[Current_Sensor]);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_golden_g] = 0x%x\n", OTP_golden_gval[Current_Sensor]);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_golden_b] = 0x%x\n", OTP_golden_bval[Current_Sensor]);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_current_r] = 0x%x\n", OTP_current_rval);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_current_g] = 0x%x\n", OTP_current_gval);
	CAMEEPROM_LOG("[FM50AF_LENS_V5][OTP_current_b] = 0x%x\n", OTP_current_bval);
        CAMEEPROM_LOG("[FM50AF_LENS_V5][Sensor ID] = 0x%x (%d)\n", pEerom_data->Shading.ISPComm.CommReg[0],Current_Sensor);
	CAMEEPROM_LOG("======================AWB READ OTP VALUE  ==================\n");
#endif
    



    if((OTP_golden_rval[Current_Sensor]==0)||(OTP_golden_gval[Current_Sensor]==0)||(OTP_golden_bval[Current_Sensor]==0)||(OTP_current_rval==0)||(OTP_current_gval==0)||(OTP_current_bval==0))
    {
        //pre gain	
        pEerom_data->Pregain.rCalGainu4R = 512;
        pEerom_data->Pregain.rCalGainu4G = 512;
        pEerom_data->Pregain.rCalGainu4B  = 512;
        CAMEEPROM_LOG("Pegain has no Calinration Data!!!\n");
        err= GetCalErr[CALIBRATION_ITEM_PREGAIN];
    }
    else
    {
    //pre gain	
    /*
        ratio=1024.0/(double)(OTP_current_rval*1.0);
        pEerom_data->Pregain.rCalGainu4R = (MUINT32)(ratio*512);
     	pEerom_data->Pregain.rCalGainu4G = 512;
		ratio=1024.0/(double)(OTP_current_bval*1.0);
		//CAMEEPROM_LOG("ratio=%f",ratio);
        pEerom_data->Pregain.rCalGainu4B = (MUINT32)(ratio*512);
        */
        pEerom_data->Pregain.rCalGainu4R = OTP_current_rval*512/OTP_golden_rval[Current_Sensor];
        pEerom_data->Pregain.rCalGainu4G = 512;
        pEerom_data->Pregain.rCalGainu4B =OTP_current_bval*512/OTP_golden_bval[Current_Sensor];
    	err=0;
    }
    if((pEerom_data->Pregain.rCalGainu4R==0)||(pEerom_data->Pregain.rCalGainu4B==0))
    {
        //pre gain	
        pEerom_data->Pregain.rCalGainu4R = 512;
        pEerom_data->Pregain.rCalGainu4G = 512;
        pEerom_data->Pregain.rCalGainu4B  = 512;
        CAMEEPROM_LOG("RGB Gain is not reasonable!!!\n");
        err= GetCalErr[CALIBRATION_ITEM_PREGAIN];
    }    
#ifdef DEBUG_CALIBRATION_LOAD
    CAMEEPROM_LOG("======================AWB OTP ==================\n");
    CAMEEPROM_LOG("[rCalGain.u4R] = %d\n", pEerom_data->Pregain.rCalGainu4R);
    CAMEEPROM_LOG("[rCalGain.u4G] = %d\n",  pEerom_data->Pregain.rCalGainu4G);
    CAMEEPROM_LOG("[rCalGain.u4B] = %d\n", pEerom_data->Pregain.rCalGainu4B);	
    CAMEEPROM_LOG("======================AWB OTP==================\n");
#endif
//	err=0;
	return err;//CAL_GET_3ANVRAM_FLAG;
}


extern UINT32 DoISPSlimShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{
    PEEPROM_DATA_STRUCT pEerom_data = (PEEPROM_DATA_STRUCT)pGetSensorCalData;
    stEEPROM_INFO_STRUCT eepromCfg;		
    MUINT32 u4capW,u4capH,u4PvW,u4PvH;
    UINT32 uiSlimShadingBuffer[EEPROM_LSC_DATA_SIZE_SLIM_LSC1]={0};
    UINT32 PreviewREG[5], CaptureREG[5];
    INT32 i4XPreGrid, i4YPreGrid, i4XCapGrid, i4YCapGrid;
    UINT16 i;
    INT32 err= GetCalErr[CALIBRATION_ITEM_SHADING];
    
    CAMEEPROM_LOG("DoISPSlimShadingLoad(0x%8x) \n",start_addr);	
    if(pEerom_data->Shading.ISPComm.CommReg[EEPROM_INFO_IN_COMM_LOAD] ==CAL_DATA_LOAD)
    {		
        CAMEEPROM_LOG("NVRAM has EEPROM data!");     
    }
    else
    {
        eepromCfg.u4Offset = start_addr+0x44;
        eepromCfg.u4Length= EEPROM_LSC_DATA_SIZE_SLIM_LSC1*4;
        eepromCfg.pu1Params = (u8 *)uiSlimShadingBuffer;
        ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);	
        //shading PV
        eepromCfg.u4Offset = start_addr+0x1C;
        eepromCfg.u4Length = 20;
        eepromCfg.pu1Params = (u8 *)&PreviewREG[0];
        ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
        //shading Cap
        eepromCfg.u4Offset = start_addr+0x30;
        eepromCfg.u4Length = 20;
        eepromCfg.pu1Params = (u8 *)&CaptureREG[0];
        ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
        i4XPreGrid = ((PreviewREG[1]&0xF0000000) >> 28) + 2;
        i4YPreGrid = ((PreviewREG[1]&0x0000F000) >> 12) + 2;
        i4XCapGrid = ((CaptureREG[1]&0xF0000000) >> 28) + 2;
        i4YCapGrid = ((CaptureREG[1]&0x0000F000) >> 12) + 2;	  
        
        pEerom_data->Shading.CapRegister.shading_ctrl1 = CaptureREG[0];
        pEerom_data->Shading.CapRegister.shading_ctrl2 = CaptureREG[1];
        pEerom_data->Shading.CapRegister.shading_read_addr = CaptureREG[2];
        pEerom_data->Shading.CapRegister.shading_last_blk = CaptureREG[3];
        pEerom_data->Shading.CapRegister.shading_ratio_cfg = CaptureREG[4];
        
        pEerom_data->Shading.PreRegister.shading_ctrl1 = PreviewREG[0];
        pEerom_data->Shading.PreRegister.shading_ctrl2 = PreviewREG[1];
        pEerom_data->Shading.PreRegister.shading_read_addr = PreviewREG[2];
        pEerom_data->Shading.PreRegister.shading_last_blk = PreviewREG[3];
        pEerom_data->Shading.PreRegister.shading_ratio_cfg = PreviewREG[4];        
        
        // seanlin 110407 because SUNNY Module has wrong eeprom data about Preview register value
        //just using SUNNY module's block x number and y number and re-calculate the block size according to preview X/Y raw sizes

        u4PvW = pEerom_data->Shading.ISPComm.CommReg[EEPROM_INFO_IN_PV_WIDTH]/2;
        u4PvH = pEerom_data->Shading.ISPComm.CommReg[EEPROM_INFO_IN_PV_HEIGHT]/2;

        PreviewREG[1] = (PreviewREG[1] &0xf000f000)|(((u4PvW/(i4XPreGrid-1))&0xfff)<<16)|((u4PvH/(i4YPreGrid-1))&0xfff);
        PreviewREG[3] = (((u4PvW-((u4PvW/(i4XPreGrid-1))&0xfff)*(i4XPreGrid-2))&0xfff)<<16)|((u4PvH-((u4PvH/(i4YPreGrid-1))&0xfff)*(i4YPreGrid-2))&0xfff);
        pEerom_data->Shading.PreRegister.shading_ctrl2 = PreviewREG[1];
        pEerom_data->Shading.PreRegister.shading_last_blk = PreviewREG[3]; 
        
        pEerom_data->Shading.CaptureSize = (i4XCapGrid-1)*(i4YCapGrid-1)*16;
        pEerom_data->Shading.PreviewSize = (i4XPreGrid-1)*(i4YPreGrid-1)*16;	
        CAMEEPROM_LOG("pEerom_data->Shading.CaptureSize 0x%x, pEerom_data->Shading.PreviewSize 0x%x \n",pEerom_data->Shading.CaptureSize,pEerom_data->Shading.PreviewSize);		
        memcpy((char*)&pEerom_data->Shading.CaptureTable[0][0], (char*)uiSlimShadingBuffer,EEPROM_LSC_DATA_SIZE_SLIM_LSC1*4);
        err =0;
    }
    return err;
}
UINT32 DoISPDynamicShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{
    INT32 err= 0;//GetCalErr[CALIBRATION_ITEM_SHADING];
    //TBD
    CAMEEPROM_LOG("DoISPDynamicShadingLoad (NOT YET)\n");    
    return err;
}
UINT32 DoISPFixShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{		
    INT32 err= 0;//GetCalErr[CALIBRATION_ITEM_SHADING];
    CAMEEPROM_LOG("DoISPFixShadingLoad (NOT YET) \n");
    return err;
}





UINT32 DoISPSensorShadingLoad(INT32 epprom_fd, UINT32 start_addr, UINT32* pGetSensorCalData)
{
    INT32 err= 0;//GetCalErr[CALIBRATION_ITEM_SHADING]|EEPROM_ERR_SENSOR_SHADING;
    int i=0;
	
	stEEPROM_INFO_STRUCT eepromCfg;	 	
	unsigned int loop, checksum,ReadCheckSum;
	
	
#ifdef DEBUG_CALIBRATION_LOAD
	unsigned int SumR,SumGr,SumGb,SumB;
#endif
   PEEPROM_DATA_STRUCT pEerom_data = (PEEPROM_DATA_STRUCT)pGetSensorCalData;

	channel_lsc_data.imgwidth=3264;
	channel_lsc_data.imgheight=2448;
	channel_lsc_data.xblock_num=17;
    channel_lsc_data.yblock_num=13;
    channel_lsc_data.bitorder=0;//3;
	channel_lsc_data.Attenutation_Index=2;
	channel_lsc_data.bitdepth=8;
#if 0
#if 0 //IMX111
	channel_lsc_data.OB_B=29;
	channel_lsc_data.OB_Gb=29;
	channel_lsc_data.OB_Gr=29;
	channel_lsc_data.OB_R=29;
#else //IMX219
	//channel_lsc_data.OB_B=64;
	//channel_lsc_data.OB_Gb=64;
	//channel_lsc_data.OB_Gr=64;
	//channel_lsc_data.OB_R=64;
        channel_lsc_data.OB_B=16;
	channel_lsc_data.OB_Gb=16;
	channel_lsc_data.OB_Gr=16;
	channel_lsc_data.OB_R=16;
#endif
#endif

    if(Current_Sensor)
    {
        channel_lsc_data.OB_B=16;
	channel_lsc_data.OB_Gb=16;
	channel_lsc_data.OB_Gr=16;
	channel_lsc_data.OB_R=16;
        CAMEEPROM_LOG("[DoISPSensorShadingLoad][Back up sensor]\n");
    }
    else
    {
	channel_lsc_data.OB_B=29;
	channel_lsc_data.OB_Gb=29;
	channel_lsc_data.OB_Gr=29;
	channel_lsc_data.OB_R=29;
        CAMEEPROM_LOG("[DoISPSensorShadingLoad][Main sensor]\n");
    }




#ifdef DEBUG_CALIBRATION_LOAD
	CAMEEPROM_LOG("======================DoISPSensorShadingLoad  ==================\n");	
	CAMEEPROM_LOG("[DoISPSensorShadingLoad_imx219][OB_B] = 0x%x\n", channel_lsc_data.OB_B);
	CAMEEPROM_LOG("[DoISPSensorShadingLoad_imx219][OB_Gb] = 0x%x\n", channel_lsc_data.OB_Gb);
	CAMEEPROM_LOG("[DoISPSensorShadingLoad_imx219][OB_Gr] = 0x%x\n", channel_lsc_data.OB_Gr);
	CAMEEPROM_LOG("[DoISPSensorShadingLoad_imx219][OB_R] = 0x%x\n", channel_lsc_data.OB_R);
        CAMEEPROM_LOG("[DoISPSensorShadingLoad][Sensor ID] = 0x%x (%d)\n", pEerom_data->Shading.ISPComm.CommReg[0],Current_Sensor);
	CAMEEPROM_LOG("======================DoISPSensorShadingLoad  ==================\n");
#endif


	eepromCfg.u4Offset = ( (RChannel_Address << 8) | 0);
	eepromCfg.u4Length= EEPROM_LSC_DATA_SIZE;
	eepromCfg.pu1Params = channel_lsc_data.RChannel_LSC_DATA;
	ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
	
	eepromCfg.u4Offset = ( (GrChannel_Address << 8) | 1);
	eepromCfg.u4Length = EEPROM_LSC_DATA_SIZE;
	eepromCfg.pu1Params = channel_lsc_data.GrChannel_LSC_DATA;
	ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
	
	eepromCfg.u4Offset = ( (GbChannel_Address << 8) | 2);
	eepromCfg.u4Length = EEPROM_LSC_DATA_SIZE;
	eepromCfg.pu1Params = channel_lsc_data.GbChannel_LSC_DATA;
	ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
	
	eepromCfg.u4Offset = ( (BChannel_Address << 8) | 3);
	eepromCfg.u4Length = EEPROM_LSC_DATA_SIZE;
	eepromCfg.pu1Params = channel_lsc_data.BChannel_LSC_DATA;
	ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
       CAMEEPROM_LOG("RChannel_LSC_DATA=\n");
  
   
	ReadCheckSum=LensOTP_read_cmos_sensor(epprom_fd,0x7E7F);
	ReadCheckSum=(((ReadCheckSum<<8)&0xff00)|((ReadCheckSum>>8)&0x00ff));

#ifdef DEBUG_CALIBRATION_LOAD

	SumR=0;
	SumGr=0;
	SumGb=0;
	SumB=0;
#endif

	for(loop=0,checksum=0; loop < EEPROM_LSC_DATA_SIZE; loop++)
	{
		checksum += (channel_lsc_data.RChannel_LSC_DATA[loop] + channel_lsc_data.GrChannel_LSC_DATA[loop] + 
			channel_lsc_data.GbChannel_LSC_DATA[loop] + channel_lsc_data.BChannel_LSC_DATA[loop]);
#ifdef DEBUG_CALIBRATION_LOAD		
		SumR +=channel_lsc_data.RChannel_LSC_DATA[loop] ;
		SumGr +=channel_lsc_data.GrChannel_LSC_DATA[loop] ;
		SumGb +=channel_lsc_data.GbChannel_LSC_DATA[loop];
		SumB +=channel_lsc_data.BChannel_LSC_DATA[loop];
#endif
        CAMEEPROM_LOG("R:%x Gr:%x Gb:%x B:%x\n",channel_lsc_data.RChannel_LSC_DATA[loop],channel_lsc_data.GrChannel_LSC_DATA[loop],
        channel_lsc_data.GbChannel_LSC_DATA[loop],channel_lsc_data.BChannel_LSC_DATA[loop]);
	}
#ifdef DEBUG_CALIBRATION_LOAD
	CAMEEPROM_LOG("======================LSC OTP ==================\n");
    CAMEEPROM_LOG("[EEPROM R] = %d\n", SumR);
    CAMEEPROM_LOG("[EEPROM Gr] = %d\n", SumGr);
    CAMEEPROM_LOG("[EEPROM Gb] = %d\n", SumGb);	
	CAMEEPROM_LOG("[EEPROM B] = %d\n", SumB );	
	CAMEEPROM_LOG("[EEPROM Calculated Check Sum] = %d %x\n", checksum,checksum);
	CAMEEPROM_LOG("[EEPROM Read Check Sum] = %d %x\n", ReadCheckSum,ReadCheckSum);
    CAMEEPROM_LOG("======================LSC OTP==================\n");
#endif	

	if ( (checksum & 0xFFFF) != ReadCheckSum)
	{
	
		CAMEEPROM_LOG("LSC check sum fail~~!!!\n");
		err = 1;
	}
	
	pEerom_data->Shading.ColorOrder=0;//3;
//	pEerom_data->Shading.ISPComm.CommReg[EEPROM_INFO_IN_COMM_SHADING_123] = CAL_DATA_LSC_123;

	pEerom_data->Shading.PreRegister.shading_ctrl1=0x30000000;
	pEerom_data->Shading.PreRegister.shading_ctrl2=0xF066B066;
	pEerom_data->Shading.PreRegister.shading_read_addr=0x40000000;
	pEerom_data->Shading.PreRegister.shading_last_blk=0x00660066;
	pEerom_data->Shading.PreRegister.shading_ratio_cfg=0x20202020;
	pEerom_data->Shading.CapRegister.shading_ctrl1=0x30000000;
	pEerom_data->Shading.CapRegister.shading_ctrl2=0xF066B066;
	pEerom_data->Shading.CapRegister.shading_read_addr=0x40000000;
	pEerom_data->Shading.CapRegister.shading_last_blk=0x00660066;
	pEerom_data->Shading.CapRegister.shading_ratio_cfg=0x20202020;

    UINT32* pu4Tbl = reinterpret_cast<UINT32*>(pEerom_data->Shading.SensorCalTable);
	
	LSC_AVGRAW_to_Gaintable(channel_lsc_data, pu4Tbl);
	for(i=0;i<MAX_GAIN_TABLE_SIZE-4;i+=4)
	{
        CAMEEPROM_LOG("0x%08x   0x%08x    0x%08x     0x%08x\n",
            pu4Tbl[i], pu4Tbl[i+1], pu4Tbl[i+2], pu4Tbl[i+3]);
		
	}
    CAMEEPROM_LOG("0x%08x    0x%08x\n",
	    pu4Tbl[MAX_GAIN_TABLE_SIZE-2], pu4Tbl[MAX_GAIN_TABLE_SIZE-1]);

    UINT8* pu8Tbl = reinterpret_cast<UINT8*>(pEerom_data->Shading.SensorCalTable);
    for (MUINT32 idx = 0; idx < MAX_GAIN_TABLE_SIZE*4; idx += 16)
    {
        CAMEEPROM_LOG("0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
            *(pu8Tbl+idx), *(pu8Tbl+idx+1), *(pu8Tbl+idx+2), *(pu8Tbl+idx+3),
            *(pu8Tbl+idx+4), *(pu8Tbl+idx+5), *(pu8Tbl+idx+6), *(pu8Tbl+idx+7),
            *(pu8Tbl+idx+8), *(pu8Tbl+idx+9), *(pu8Tbl+idx+10), *(pu8Tbl+idx+11),
            *(pu8Tbl+idx+12), *(pu8Tbl+idx+13), *(pu8Tbl+idx+14), *(pu8Tbl+idx+15));
    }

    return (err|EEPROM_ERR_MTKOTP_SHADING);
}
/******************************************************************************
*
*******************************************************************************/
UINT32 LensGetCalData(UINT32* pGetSensorCalData)
{
    UCHAR cBuf[128] = "/dev/";
    UINT32 result = EEPROM_ERR_NO_DATA;
    //eeprom
    stEEPROM_INFO_STRUCT  eepromCfg;
    UINT32 CheckID,i ;
    INT32 err;
    UINT16 LayoutType;
    INT32 epprom_fd = 0;
    UINT16 u2IDMatch = 0;
	
    PEEPROM_DATA_STRUCT pEerom_data = (PEEPROM_DATA_STRUCT)pGetSensorCalData;
    
    CAMEEPROM_LOG("LensGetCalData !!!\n");
	
    if ((gIsInited==0) && (EEPROMInit() != EEPROM_NONE) && (EEPROMDeviceName(&cBuf[0]) == 0))
    {
        epprom_fd = open(cBuf, O_RDWR);	 
        if(epprom_fd == -1)
        {
            CAMEEPROM_LOG("----error: can't open EEPROM %s----\n",cBuf);
            result = GetCalErr[CALIBRATION_ITEM_SHADING]|GetCalErr[CALIBRATION_ITEM_PREGAIN]|GetCalErr[CALIBRATION_ITEM_DEFECT];
            return result;
        }	
        //result = 0;
        //read ID
        LayoutType = 0xFFFF; 
        eepromCfg.u4Offset = 0xFFFFFFFF;
        for (i = 0; i< MAX_CALIBRATION_LAYOUT_NUM; i++)
        {
            if (eepromCfg.u4Offset != CalLayoutTbl[i].HeaderAddr)
            {
                CheckID = 0x00000000;
                eepromCfg.u4Offset = CalLayoutTbl[i].HeaderAddr;
                eepromCfg.u4Length = 4;
                eepromCfg.pu1Params = (u8 *)&CheckID;
                err= ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
                if(err< 0)
                {
                    CAMEEPROM_LOG("[EEPROM] Read header ID fail err = 0x%x \n",err);
                    break;
                }
            }
            if (((CalLayoutTbl[i].HeaderId == 0xFFFFFFFF) && ((CheckID != 0xFFFFFFFF)&&(CheckID != 0x000000)))
            	|| ((CalLayoutTbl[i].HeaderId != 0xFFFFFFFF) && (CheckID == CalLayoutTbl[i].HeaderId)))
            {
                LayoutType = i;
                u2IDMatch = 1;
                pEerom_data->Shading.ShadingID = CalLayoutTbl[i].ShadingID;
        		    CAMEEPROM_LOG("ShadingID =  %d \n", pEerom_data->Shading.ShadingID);      
                #ifdef CUSTOM_EEPROM_ROTATION //seeanlin110421 because Sensor might rotated from PCB
                pEerom_data->Shading.TableRotation = 1;
                #endif                
                break;
            }			
        }
        CAMEEPROM_LOG("LayoutType= 0x%x",LayoutType);	

        if(u2IDMatch == 1)
        {     
            //result = 0;
            if (CalLayoutTbl[LayoutType].CheckShading != 0)
            {
                eepromCfg.u4Offset = CalLayoutTbl[LayoutType].CalItemTbl[CALIBRATION_ITEM_SHADING].StartAddr;
                eepromCfg.u4Length = 4;
                eepromCfg.pu1Params = (u8 *)&CheckID;
                ioctl(epprom_fd, EEPROMIOC_G_READ , &eepromCfg);
                
                CAMEEPROM_LOG("CheckID = 0x%x\n",CheckID);
                if (((CalLayoutTbl[LayoutType].ShadingID == 0xFFFFFFFF) && ((CheckID != 0xFFFFFFFF)&&(CheckID != 0x000000)))
                	|| ((CalLayoutTbl[LayoutType].ShadingID != 0xFFFFFFFF) && (CheckID == CalLayoutTbl[LayoutType].ShadingID)))
                {
                    result = 0;
                    for (i= 0; i < MAX_CALIBRATION_ITEM_NUM; i++)
                    {
                        if ((CalLayoutTbl[LayoutType].CalItemTbl[i].Include != 0) 
                        	&&(CalLayoutTbl[LayoutType].CalItemTbl[i].GetCalDataProcess != NULL))
                        {			
                            
                            result =  result | CalLayoutTbl[LayoutType].CalItemTbl[i].GetCalDataProcess(epprom_fd, CalLayoutTbl[LayoutType].CalItemTbl[i].StartAddr, pGetSensorCalData);	
                            
                        }
                        else
                        {
                            result = result| GetCalErr[i];
                            CAMEEPROM_LOG("result = 0x%x\n",result);							
                        }
                    }
                    //gIsInited = 1;					
                }
            }
            else
            {
            	result = 0;
                    for (i= 0; i < MAX_CALIBRATION_ITEM_NUM; i++)
                    {
                        if ((CalLayoutTbl[LayoutType].CalItemTbl[i].Include != 0) 
                        	&&(CalLayoutTbl[LayoutType].CalItemTbl[i].GetCalDataProcess != NULL))
                        {			
                            
                            result =  result | CalLayoutTbl[LayoutType].CalItemTbl[i].GetCalDataProcess(epprom_fd, CalLayoutTbl[LayoutType].CalItemTbl[i].StartAddr, pGetSensorCalData);	
                            
                        }
                        else
                        {
                            result = result| GetCalErr[i];
                            CAMEEPROM_LOG("result = 0x%x\n",result);							
                        }
                    }
                    //gIsInited = 1;		
                CAMEEPROM_LOG("CalLayoutTbl[LayoutType].CheckShading==0");
            }
        }
		close(epprom_fd);		 
	}	
    else
    {
        CAMEEPROM_LOG("----NO EEPROM_%s!----\n",cBuf);
        result = GetCalErr[CALIBRATION_ITEM_SHADING]|GetCalErr[CALIBRATION_ITEM_PREGAIN]|GetCalErr[CALIBRATION_ITEM_DEFECT];
    }
	result = result;
	return result;
}


