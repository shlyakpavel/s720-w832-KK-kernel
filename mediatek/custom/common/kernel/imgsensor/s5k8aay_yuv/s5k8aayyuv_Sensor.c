#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/xlog.h>
#include <asm/atomic.h>
#include <asm/io.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"


#include "s5k8aayyuv_Sensor.h"
#include "s5k8aayyuv_Camera_Sensor_para.h"
#include "s5k8aayyuv_CameraCustomized.h"

#define S5K8AAYX_DEBUG
#ifdef S5K8AAYX_DEBUG
#define SENSORDB  printk
//#define SENSORDB(fmt, arg...)  printk(KERN_ERR fmt, ##arg)
//#define SENSORDB(fmt, arg...) xlog_printk(ANDROID_LOG_DEBUG, "[S5K8AAYX]", fmt, ##arg)
#else
#define SENSORDB(x,...)
#endif

#define S5K8AAYXYUV_TEST_PATTERN_CHECKSUM (0xe801a0a3)


typedef struct
{
    UINT16  iSensorVersion;
    UINT16  iNightMode;
    UINT16  iWB;
    UINT16  isoSpeed;
    UINT16  iEffect;
    UINT16  iEV;
    UINT16  iBanding;
    UINT16  iMirror;
    UINT16  iFrameRate;
} S5K8AAYXStatus;
S5K8AAYXStatus S5K8AAYCurrentStatus;

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
extern int iBurstWriteReg(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//static int sensor_id_fail = 0;
static kal_uint32 zoom_factor = 0;


kal_uint16 S5K8AAYX_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;
    char puSendCmd[2] = {(char)(addr >> 8) , (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd , 2, (u8*)&get_byte,2,S5K8AAY_WRITE_ID);
    return ((get_byte<<8)&0xff00)|((get_byte>>8)&0x00ff);
}

inline void S5K8AAY_write_cmos_sensor(u16 addr, u32 para)
{
    char puSendCmd[4] = {(char)(addr >> 8) , (char)(addr & 0xFF) ,(char)(para >> 8),(char)(para & 0xFF)};

    iWriteRegI2C(puSendCmd , 4,S5K8AAY_WRITE_ID);
}

#define I2C_BUFFER_LEN 4   // MT6572 i2c bus master fifo length is 8 bytes, changed to 4 by Pavel

// {addr, data} pair in para
// len is the total length of addr+data
// Using I2C multiple/burst write if the addr doesn't change
static kal_uint16 S5K8AAY_table_write_cmos_sensor(kal_uint16* para, kal_uint32 len)
{
   char puSendCmd[I2C_BUFFER_LEN]; //at most 2 bytes address and 6 bytes data for multiple write. MTK i2c master has only 8bytes fifo.
   kal_uint32 tosend, IDX;
   kal_uint16 addr, addr_last, data;

   tosend = 0;
   IDX = 0;
   while(IDX < len)
   {
       addr = para[IDX];

       if (tosend == 0) // new (addr, data) to send
       {
           puSendCmd[tosend++] = (char)(addr >> 8);
           puSendCmd[tosend++] = (char)(addr & 0xFF);
           data = para[IDX+1];
           puSendCmd[tosend++] = (char)(data >> 8);
           puSendCmd[tosend++] = (char)(data & 0xFF);
           IDX += 2;
           addr_last = addr;
       }
       else if (addr == addr_last) // to multiple write the data to the same address
       {
           data = para[IDX+1];
           puSendCmd[tosend++] = (char)(data >> 8);
           puSendCmd[tosend++] = (char)(data & 0xFF);
           IDX += 2;
       }
       // to send out the data if the sen buffer is full or last data or to program to the different address.
       if (tosend == I2C_BUFFER_LEN || IDX == len || addr != addr_last)
       {
           iWriteRegI2C(puSendCmd , tosend, S5K8AAY_WRITE_ID);
           tosend = 0;
       }
   }
   return 0;
}

/*******************************************************************************
* // Adapter for Winmo typedef
********************************************************************************/
#define WINMO_USE 0

//#define Sleep(ms) msleep(ms)
#define RETAILMSG(x,...)
#define TEXT


/*******************************************************************************
* // End Adapter for Winmo typedef
********************************************************************************/

#define  S5K8AAYX_LIMIT_EXPOSURE_LINES                (1253)
#define  S5K8AAYX_VIDEO_NORMALMODE_30FRAME_RATE       (30)
#define  S5K8AAYX_VIDEO_NORMALMODE_FRAME_RATE         (15)
#define  S5K8AAYX_VIDEO_NIGHTMODE_FRAME_RATE          (7.5)
#define  BANDING50_30HZ
/* Global Valuable */


kal_bool S5K8AAYX_MPEG4_encode_mode = KAL_FALSE, S5K8AAYX_MJPEG_encode_mode = KAL_FALSE;

kal_uint32 S5K8AAYX_pixel_clk_freq = 0, S5K8AAYX_sys_clk_freq = 0;          // 480 means 48MHz

kal_uint16 S5K8AAYX_CAP_dummy_pixels = 0;
kal_uint16 S5K8AAYX_CAP_dummy_lines = 0;

kal_uint16 S5K8AAYX_PV_cintr = 0;
kal_uint16 S5K8AAYX_PV_cintc = 0;
kal_uint16 S5K8AAYX_CAP_cintr = 0;
kal_uint16 S5K8AAYX_CAP_cintc = 0;

kal_bool S5K8AAY_night_mode_enable = KAL_FALSE;


//===============old============================================
//static kal_uint8 S5K8AAYX_exposure_line_h = 0, S5K8AAYX_exposure_line_l = 0,S5K8AAYX_extra_exposure_line_h = 0, S5K8AAYX_extra_exposure_line_l = 0;

//static kal_bool S5K8AAYX_gPVmode = KAL_TRUE; //PV size or Full size
//static kal_bool S5K8AAYX_VEDIO_encode_mode = KAL_FALSE; //Picture(Jpeg) or Video(Mpeg4)
static kal_bool S5K8AAYX_sensor_cap_state = KAL_FALSE; //Preview or Capture

//static kal_uint16 S5K8AAYX_dummy_pixels=0, S5K8AAYX_dummy_lines=0;

//static kal_uint16 S5K8AAYX_exposure_lines=0, S5K8AAYX_extra_exposure_lines = 0;



//static kal_uint8 S5K8AAYX_Banding_setting = AE_FLICKER_MODE_50HZ;  //Jinghe modified

/****** OVT 6-18******/
//static kal_uint16  S5K8AAYX_Capture_Max_Gain16= 6*16;
//static kal_uint16  S5K8AAYX_Capture_Gain16=0 ;
//static kal_uint16  S5K8AAYX_Capture_Shutter=0;
//static kal_uint16  S5K8AAYX_Capture_Extra_Lines=0;

//static kal_uint16  S5K8AAYX_PV_Dummy_Pixels =0, S5K8AAYX_Capture_Dummy_Pixels =0, S5K8AAYX_Capture_Dummy_Lines =0;
//static kal_uint16  S5K8AAYX_PV_Gain16 = 0;
//static kal_uint16  S5K8AAYX_PV_Shutter = 0;
//static kal_uint16  S5K8AAYX_PV_Extra_Lines = 0;
kal_uint16 S5K8AAYX_sensor_gain_base=0,S5K8AAYX_FAC_SENSOR_REG=0,S5K8AAYX_iS5K8AAYX_Mode=0,S5K8AAYX_max_exposure_lines=0;
kal_uint32 S5K8AAYX_capture_pclk_in_M=520,S5K8AAYX_preview_pclk_in_M=390,S5K8AAYX_PV_dummy_pixels=0,S5K8AAYX_PV_dummy_lines=0,S5K8AAYX_isp_master_clock=0;
static kal_uint32  S5K8AAYX_sensor_pclk=720;
//static kal_bool S5K8AAYX_AWB_ENABLE = KAL_TRUE;
static kal_bool S5K8AAYX_AE_ENABLE = KAL_TRUE;

//===============old============================================

kal_uint8 S5K8AAYX_sensor_write_I2C_address = S5K8AAY_WRITE_ID;
kal_uint8 S5K8AAYX_sensor_read_I2C_address = S5K8AAYX_READ_ID;
//kal_uint16 S5K8AAYX_Sensor_ID = 0;

//HANDLE S5K8AAYXhDrvI2C;
//I2C_TRANSACTION S5K8AAYXI2CConfig;

UINT8 S5K8AAYXPixelClockDivider=0;

static DEFINE_SPINLOCK(s5k8aayx_drv_lock);
MSDK_SENSOR_CONFIG_STRUCT S5K8AAYXSensorConfigData;


/*************************************************************************
* FUNCTION
*       S5K8AAYInitialPara
*
* DESCRIPTION
*       This function initialize the global status of  MT9V114
*
* PARAMETERS
*       None
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static void S5K8AAYInitialPara(void)
{
    S5K8AAYCurrentStatus.iNightMode = 0xFFFF;
    S5K8AAYCurrentStatus.iWB = AWB_MODE_AUTO;
    S5K8AAYCurrentStatus.isoSpeed = AE_ISO_100;
    S5K8AAYCurrentStatus.iEffect = MEFFECT_OFF;
    S5K8AAYCurrentStatus.iBanding = AE_FLICKER_MODE_50HZ;
    S5K8AAYCurrentStatus.iEV = AE_EV_COMP_n03;
    S5K8AAYCurrentStatus.iMirror = IMAGE_NORMAL;
    S5K8AAYCurrentStatus.iFrameRate = 25;
}


void S5K8AAY_set_mirror(kal_uint8 image_mirror)
{
SENSORDB("Enter S5K8AAY_set_mirror \n");

    if(S5K8AAYCurrentStatus.iMirror == image_mirror)
      return;

    S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002a, 0x01E8);

    switch (image_mirror)
    {
        case IMAGE_NORMAL:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);           // REG_0TC_PCFG_uPrevMirror
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);           // REG_0TC_PCFG_uCaptureMirror
             break;
        case IMAGE_H_MIRROR:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);           // REG_0TC_PCFG_uPrevMirror
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);           // REG_0TC_PCFG_uCaptureMirror
             break;
        case IMAGE_V_MIRROR:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0002);           // REG_0TC_PCFG_uPrevMirror
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0002);           // REG_0TC_PCFG_uCaptureMirror
             break;
        case IMAGE_HV_MIRROR:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0003);           // REG_0TC_PCFG_uPrevMirror
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0003);           // REG_0TC_PCFG_uCaptureMirror
             break;

        default:
             ASSERT(0);
             break;
    }
//this block should not be called for now as it seems to differ. Nethertheless, I'm to lazy to correct it
    S5K8AAY_write_cmos_sensor(0x002A,0x01A8);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0000); // #REG_TC_GP_ActivePrevConfig // Select preview configuration_0
    S5K8AAY_write_cmos_sensor(0x002A,0x01AC);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_PrevOpenAfterChange
    S5K8AAY_write_cmos_sensor(0x002A,0x01A6);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_NewConfigSync // Update preview configuration
    S5K8AAY_write_cmos_sensor(0x002A,0x01AA);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_PrevConfigChanged
    S5K8AAY_write_cmos_sensor(0x002A,0x019E);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_EnablePreview // Start preview
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001);  // #REG_TC_GP_EnablePreviewChanged

    S5K8AAYCurrentStatus.iMirror = image_mirror;
}

int SEN_SET_CURRENT = 1;
//unsigned int D02D4_Current = 0x0;//0x02aa; 555 commented by Pavel
unsigned int D52D9_Current = 0x0;//0x02aa; 555
unsigned int unknow_gpio_Current   = 0x0000; //555
unsigned int CLK_Current   = 0x000; //555
MSDK_SCENARIO_ID_ENUM S5K8AAYXCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_Initialize_Setting
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/

#define S5K8AAYX_PREVIEW_MODE             0
#define S5K8AAYX_VIDEO_MODE               1

void S5K8AAYX_Initialize_Setting(void)	//edited, approved
{
  SENSORDB("Enter S5K8AAY_Initialize_Setting\n");
  S5K8AAY_write_cmos_sensor(0xFCFC ,0xD000);
  S5K8AAY_write_cmos_sensor(0x0010 ,0x0001); // Reset
  S5K8AAY_write_cmos_sensor(0xFCFC ,0x0000);
  S5K8AAY_write_cmos_sensor(0x0000 ,0x0000); // Simmian bug workaround
  S5K8AAY_write_cmos_sensor(0xFCFC ,0xD000);
  S5K8AAY_write_cmos_sensor(0x1030 ,0x0000); // Clear host interrupt so main will wait
  S5K8AAY_write_cmos_sensor(0x0014 ,0x0001); // ARM go

  mdelay(5);

  {
    static kal_uint16 addr_data_pair[] =
    {
#include "init_table.h"
    };
    S5K8AAY_table_write_cmos_sensor(addr_data_pair, sizeof(addr_data_pair)/sizeof(kal_uint16));
  }
  SENSORDB("S5K8AAYX_Initialize_Setting Done\n");

}


/*void S5K8AAYX_SetFrameRate(MSDK_SCENARIO_ID_ENUM scen, UINT16 u2FrameRate)
{

    //spin_lock(&s5k8aayx_drv_lock);
    //MSDK_SCENARIO_ID_ENUM scen = S5K8AAYXCurrentScenarioId;
    //spin_unlock(&s5k8aayx_drv_lock);
    UINT32 u4frameTime = (1000 * 10) / u2FrameRate;

    if (15 >= u2FrameRate)
    {
       switch (scen)
       {
           default:
           case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            //7.5fps ~30fps
            S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
            S5K8AAY_write_cmos_sensor(0x002A, 0x01D6);
            S5K8AAY_write_cmos_sensor(0x0F12, 0x014D); //REG_xTC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            S5K8AAY_write_cmos_sensor(0x0F12, 0x0535); //REG_xTC_PCFG_usMaxFrTimeMsecMult10 //09C4h:4fps
            //To make Preview/Capture with the same exposure time, 
            //update Capture-Config here...also.
            S5K8AAY_write_cmos_sensor(0x002A, 0x02C8);
            S5K8AAY_write_cmos_sensor(0x0F12, 0x014D); //REG_0TC_CCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            S5K8AAY_write_cmos_sensor(0x0F12, 0x0535); //REG_0TC_CCFG_usMaxFrTimeMsecMult10 //09C4h:4fps

            break;
           case MSDK_SCENARIO_ID_VIDEO_PREVIEW: //15fps
            S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
            S5K8AAY_write_cmos_sensor(0x002A ,0x0206);
            S5K8AAY_write_cmos_sensor(0x0F12, u4frameTime); //REG_xTC_PCFG_usMaxFrTimeMsecMult10 //09C4h:4fps
            S5K8AAY_write_cmos_sensor(0x0F12, u4frameTime); //REG_xTC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            break;
        }
    }
    else
    {
      switch (scen)
      {
         default:
         case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
            //15~30fps
            S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
            S5K8AAY_write_cmos_sensor(0x002A, 0x01D6);
            S5K8AAY_write_cmos_sensor(0x0F12, 0x014D); //REG_xTC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            S5K8AAY_write_cmos_sensor(0x0F12, 0x029A); //REG_xTC_PCFG_usMaxFrTimeMsecMult10 //09C4h:4fps

            //To make Preview/Capture with the same exposure time, 
            //update Capture-Config here...also.
            S5K8AAY_write_cmos_sensor(0x002A, 0x02C8);
            S5K8AAY_write_cmos_sensor(0x0F12, 0x014D); //REG_0TC_CCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            S5K8AAY_write_cmos_sensor(0x0F12, 0x029A); //REG_0TC_CCFG_usMaxFrTimeMsecMult10 //09C4h:4fps
            break;
         case MSDK_SCENARIO_ID_VIDEO_PREVIEW: //30fps
            S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
            S5K8AAY_write_cmos_sensor(0x002A ,0x0206);
            S5K8AAY_write_cmos_sensor(0x0F12, u4frameTime); //REG_xTC_PCFG_usMaxFrTimeMsecMult10 //09C4h:4fps
            S5K8AAY_write_cmos_sensor(0x0F12, u4frameTime); //REG_xTC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps
            break;
      }
    }

    S5K8AAY_write_cmos_sensor(0x002A,0x01AC);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_PrevOpenAfterChange
    S5K8AAY_write_cmos_sensor(0x002A,0x01A6);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_NewConfigSync // Update preview configuration
    S5K8AAY_write_cmos_sensor(0x002A,0x01AA);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_PrevConfigChanged
    S5K8AAY_write_cmos_sensor(0x002A,0x019E);
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_EnablePreview // Start preview
    S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // #REG_TC_GP_EnablePreviewChanged
    return;
}
*/

/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_PV_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void S5K8AAYX_PV_Mode(unsigned int config_num)
{
    SENSORDB("Enter S5K8AAYX_PV_Mode\n");

    S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002A ,0x01A8);
    S5K8AAY_write_cmos_sensor(0x0F12 ,config_num);  // #REG_TC_GP_ActivePrevConfig
    S5K8AAY_write_cmos_sensor(0x002A ,0x01AC);
    S5K8AAY_write_cmos_sensor(0x0F12 ,0x0001);  // #REG_TC_GP_PrevOpenAfterChange
    S5K8AAY_write_cmos_sensor(0x002A ,0x01A6);
    S5K8AAY_write_cmos_sensor(0x0F12 ,0x0001);  // #REG_TC_GP_NewConfigSync
    S5K8AAY_write_cmos_sensor(0x002A ,0x01AA);
    S5K8AAY_write_cmos_sensor(0x0F12 ,0x0001);  // #REG_TC_GP_PrevConfigChanged
    S5K8AAY_write_cmos_sensor(0x002A ,0x019E);
    S5K8AAY_write_cmos_sensor(0x0F12 ,0x0001);  // #REG_TC_GP_EnablePreview
    S5K8AAY_write_cmos_sensor(0x0F12 ,0x0001);  // #REG_TC_GP_EnablePreviewChanged

    S5K8AAY_write_cmos_sensor(0x002A, 0x0164);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0001); // REG_TC_IPRM_InitParamsUpdated
    return;
}



/*****************************************************************************
 * FUNCTION
 *  S5K8AAYX_CAP_Mode
 * DESCRIPTION
 *
 * PARAMETERS
 *  void
 * RETURNS
 *  void
 *****************************************************************************/
void S5K8AAYX_CAP_Mode2(void)
{
//
}


void
S5K8AAYX_GetAutoISOValue(void)
{
    // Cal. Method : ((A-Gain*D-Gain)/100h)/2
    // A-Gain , D-Gain Read value is hex value.
    //   ISO 50  : 100(HEX)
    //   ISO 100 : 100 ~ 1FF(HEX)
    //   ISO 200 : 200 ~ 37F(HEX)
    //   ISO 400 : over 380(HEX)
    unsigned int A_Gain, D_Gain, ISOValue;
    S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
    S5K8AAY_write_cmos_sensor(0x002C, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002E, 0x20E0);
    A_Gain = S5K8AAYX_read_cmos_sensor(0x0F12);
    D_Gain = S5K8AAYX_read_cmos_sensor(0x0F12);

    ISOValue = (A_Gain >> 8);
    spin_lock(&s5k8aayx_drv_lock);
#if 0
    if (ISOValue == 256)
    {
        S5K8AAYCurrentStatus.isoSpeed = AE_ISO_50;
    }
    else if ((ISOValue >= 257) && (ISOValue <= 511 ))
    {
        S5K8AAYCurrentStatus.isoSpeed = AE_ISO_100;
    }
#endif
    if ((ISOValue >= 2) && (ISOValue < 4 ))
    {
        S5K8AAYCurrentStatus.isoSpeed = AE_ISO_200;
    }
    else if (ISOValue >= 4)
    {
        S5K8AAYCurrentStatus.isoSpeed = AE_ISO_400;
    }
    else
    {
        S5K8AAYCurrentStatus.isoSpeed = AE_ISO_100;
    }
    spin_unlock(&s5k8aayx_drv_lock);

    SENSORDB("[8AA] Auto ISO Value = %d \n", ISOValue);
}



void S5K8AAYX_CAP_Mode(void) //seems to be empty, by Pavel
{
    /*S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002A, 0x01B0);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);  //config select   0 :48, 1, 20,
    S5K8AAY_write_cmos_sensor(0x002a, 0x01A6);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
    S5K8AAY_write_cmos_sensor(0x002a, 0x01B2);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
    S5K8AAY_write_cmos_sensor(0x002a, 0x01A2);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
    S5K8AAY_write_cmos_sensor(0x002a, 0x01A4);
    S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
    mdelay(50);
    //S5K5CAGX_gPVmode = KAL_FALSE;

    S5K8AAYX_GetAutoISOValue();*/
}


/*void S5K8AAYX_AE_AWB_Enable(kal_bool enable)
{
    S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002A, 0x04D2);           // REG_TC_DBG_AutoAlgEnBits

    if (enable)
    {
        // Enable AE/AWB
        S5K8AAY_write_cmos_sensor(0x0F12, 0x077F); // Enable aa_all, ae, awb.
    }
    else
    {
        // Disable AE/AWB
        S5K8AAY_write_cmos_sensor(0x0F12, 0x0770); // Disable aa_all, ae, awb.
    }

}*/
static void S5K8AAYX_set_AE_mode(kal_bool AE_enable)
{
    if(AE_enable==KAL_TRUE)
    {
        S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000); // Set page
        S5K8AAY_write_cmos_sensor(0x0028, 0x7000); // Set address

        S5K8AAY_write_cmos_sensor(0x002A, 0x214A);
        S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
    }
    else
    {
        S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000); // Set page
        S5K8AAY_write_cmos_sensor(0x0028, 0x7000); // Set address

        S5K8AAY_write_cmos_sensor(0x002A, 0x214A);
        S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);
    }

}


/*************************************************************************
* FUNCTION
*       S5K8AAYX_night_mode
*
* DESCRIPTION
*       This function night mode of S5K8AAYX.
*
* PARAMETERS
*       none
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void S5K8AAY_night_mode(kal_bool enable)	//approved
{
    SENSORDB("S5K8AAY_night_mode [in] enable=%d \r\n",enable);
	S5K8AAY_write_cmos_sensor(0xFCFC,0xD000);
    S5K8AAY_write_cmos_sensor(0x0028,0x7000);
	S5K8AAY_write_cmos_sensor(0x002A,0x01D8);
    if (enable)
    {
		S5K8AAY_write_cmos_sensor(0x0F12,0x03E8);
		S5K8AAY_write_cmos_sensor(0x002A,0x01D6);
		S5K8AAY_write_cmos_sensor(0x0F12,0x03E8);
		S5K8AAY_write_cmos_sensor(0x002A,0x01A8);
        S5K8AAY_night_mode_enable = KAL_TRUE;
    }
    else
    {
		S5K8AAY_write_cmos_sensor(0x002A,0x01D8);
		S5K8AAY_write_cmos_sensor(0x0F12,0x014D);
		S5K8AAY_write_cmos_sensor(0x002A,0x01D6);
		S5K8AAY_write_cmos_sensor(0x0F12,0x014D);
        S5K8AAY_night_mode_enable = KAL_FALSE;
    }
	S5K8AAY_write_cmos_sensor(0x002A,0x01A8);
	S5K8AAY_write_cmos_sensor(0x0F12,0x0000);  
	S5K8AAY_write_cmos_sensor(0x002A,0x01AA);
	S5K8AAY_write_cmos_sensor(0x0F12,0x0001);
	S5K8AAY_write_cmos_sensor(0x002A,0x019E);
	S5K8AAY_write_cmos_sensor(0x0F12,0x0001);
	S5K8AAY_write_cmos_sensor(0x0F12,0x0001);
} /* S5K8AAYX_night_mode */

/*************************************************************************
* FUNCTION
*       S5K8AAYX_GetSensorID
*
* DESCRIPTION
*       This function get the sensor ID
*
* PARAMETERS
*       None
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
  kal_uint32 S5K8AAYX_GetSensorID(kal_uint32 *sensorID)
{
    kal_uint16 sensor_id = 0;
    //unsigned short version = 0;
    signed int retry = 3;

    //check if sensor ID correct
    while(--retry)
    {
        S5K8AAY_write_cmos_sensor(0xFCFC, 0x0000);
        sensor_id = S5K8AAYX_read_cmos_sensor(0x0040);
        SENSORDB("[8AA]Sensor Read S5K8AAYX ID: 0x%x \r\n", sensor_id);

        *sensorID = sensor_id;
        if (sensor_id == S5K8AAYX_SENSOR_ID)
        {
            SENSORDB("[8AA] Sensor ID: 0x%x, Read OK \r\n", sensor_id);
            //version=S5K8AAYX_read_cmos_sensor(0x0042);
            //SENSORDB("[8AA]~~~~~~~~~~~~~~~~ S5K8AAYX version: 0x%x \r\n",version);
            return ERROR_NONE;
        }
    }
    *sensorID = 0xFFFFFFFF;
    SENSORDB("[8AA] Sensor Read Fail \r\n");
    return ERROR_SENSOR_CONNECT_FAIL;
}


/*****************************************************************************/
/* Windows Mobile Sensor Interface */
/*****************************************************************************/
/*************************************************************************
* FUNCTION
*       S5K8AAYOpen
*
* DESCRIPTION
*       This function initialize the registers of CMOS sensor
*
* PARAMETERS
*       None
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K8AAYOpen(void)	//approved
{
    kal_uint16 sensor_id=0;


	 S5K8AAY_write_cmos_sensor(0xFCFC, 0x0000);
	 sensor_id=S5K8AAYX_read_cmos_sensor(0x0040);
	 SENSORDB("Sensor Read S5K8AAYX ID %x \r\n",sensor_id);

	if (sensor_id != S5K8AAYX_SENSOR_ID)
	{
	    SENSORDB("Sensor Read ByeBye \r\n");
			return ERROR_SENSOR_CONNECT_FAIL;
	}

    S5K8AAYInitialPara();
    S5K8AAYX_Initialize_Setting();

    S5K8AAY_set_mirror(IMAGE_HV_MIRROR);
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*       S5K8AAYClose
*
* DESCRIPTION
*       This function is to turn off sensor module power.
*
* PARAMETERS
*       None
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K8AAYClose(void)
{
    return ERROR_NONE;
} /* S5K8AAYClose() */

/*************************************************************************
* FUNCTION
*       S5K8AAYXPreview
*
* DESCRIPTION
*       This function start the sensor preview.
*
* PARAMETERS
*       *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*       None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 S5K8AAYXPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    SENSORDB("Enter S5K8AAYXPreview__\n");
    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYX_sensor_cap_state = KAL_FALSE;
    //S5K8AAYX_PV_dummy_pixels = 0x0400;
    S5K8AAYX_PV_dummy_lines = 0;

    if(sensor_config_data->SensorOperationMode==MSDK_SENSOR_OPERATION_MODE_VIDEO) // MPEG4 Encode Mode
    {
        SENSORDB("S5K8AAYXPreview MSDK_SENSOR_OPERATION_MODE_VIDEO \n");
        S5K8AAYX_MPEG4_encode_mode = KAL_TRUE;
        S5K8AAYX_MJPEG_encode_mode = KAL_FALSE;
    }
    else
    {
        SENSORDB("S5K8AAYXPreview Normal \n");
        S5K8AAYX_MPEG4_encode_mode = KAL_FALSE;
        S5K8AAYX_MJPEG_encode_mode = KAL_FALSE;
    }
    spin_unlock(&s5k8aayx_drv_lock);


    if (MSDK_SCENARIO_ID_CAMERA_PREVIEW == S5K8AAYXCurrentScenarioId)
    {
        S5K8AAYX_PV_Mode(S5K8AAYX_PREVIEW_MODE);
    }
    else
    {
        S5K8AAYX_PV_Mode(S5K8AAYX_VIDEO_MODE);
    }


    //S5K8AAYX_set_mirror(sensor_config_data->SensorImageMirror);

    image_window->GrabStartX = S5K8AAYX_IMAGE_SENSOR_PV_INSERTED_PIXELS;
    image_window->GrabStartY = S5K8AAYX_IMAGE_SENSOR_PV_INSERTED_LINES;
    image_window->ExposureWindowWidth = S5K8AAYX_IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight = S5K8AAYX_IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&S5K8AAYXSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	
	mdelay(60);
    return ERROR_NONE;
}   /* S5K8AAYXPreview() */

UINT32 S5K8AAYXCapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                       MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
   // kal_uint32 pv_integration_time = 0;  // Uinit - us
   // kal_uint32 cap_integration_time = 0;
   // kal_uint16 PV_line_len = 0;
   // kal_uint16 CAP_line_len = 0;
    SENSORDB("Enter S5K8AAYXCapture__\n");

    S5K8AAYX_sensor_cap_state = KAL_TRUE;
    S5K8AAYX_CAP_Mode();

    image_window->GrabStartX = S5K8AAYX_IMAGE_SENSOR_FULL_INSERTED_PIXELS;
    image_window->GrabStartY = S5K8AAYX_IMAGE_SENSOR_FULL_INSERTED_LINES;
    image_window->ExposureWindowWidth = S5K8AAYX_IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = S5K8AAYX_IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&S5K8AAYXSensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	
	mdelay(60);
    return ERROR_NONE;
}   /* S5K8AAYXCapture() */

UINT32 S5K8AAYGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)	//correct
{
    pSensorResolution->SensorFullWidth=S5K8AAYX_IMAGE_SENSOR_FULL_WIDTH;  //modify by yanxu
    pSensorResolution->SensorFullHeight=S5K8AAYX_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=S5K8AAYX_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=S5K8AAYX_IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
}   /* S5K8AAYGetResolution() */

UINT32 S5K8AAYXGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                       MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                       MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("Enter S5K8AAYXGetInfo\n");
    SENSORDB("S5K8AAYXGetInfo ScenarioId =%d \n",ScenarioId);
    switch(ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             pSensorInfo->SensorPreviewResolutionX=S5K8AAYX_IMAGE_SENSOR_FULL_WIDTH;
             pSensorInfo->SensorPreviewResolutionY=S5K8AAYX_IMAGE_SENSOR_FULL_HEIGHT;
             pSensorInfo->SensorCameraPreviewFrameRate=15;
             break;
        default:
             pSensorInfo->SensorPreviewResolutionX=S5K8AAYX_IMAGE_SENSOR_PV_WIDTH;
             pSensorInfo->SensorPreviewResolutionY=S5K8AAYX_IMAGE_SENSOR_PV_HEIGHT;
             pSensorInfo->SensorCameraPreviewFrameRate=30;
             break;
    }
    pSensorInfo->SensorFullResolutionX=S5K8AAYX_IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=S5K8AAYX_IMAGE_SENSOR_FULL_HEIGHT;
    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;

    pSensorInfo->SensorOutputDataFormat = SENSOR_OUTPUT_FORMAT_YUYV;

    pSensorInfo->SensorClockPolarity = SENSOR_CLOCK_POLARITY_HIGH;
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_HIGH;

    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_HIGH;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;

    #ifdef MIPI_INTERFACE
        pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;
    #else
        pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_PARALLEL;
    #endif

    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;//ISP_DRIVING_4MA;
    pSensorInfo->CaptureDelayFrame = 3;
    pSensorInfo->PreviewDelayFrame = 3;
    pSensorInfo->VideoDelayFrame = 4;

    //pSensorInfo->YUVAwbDelayFrame = 3;
    //pSensorInfo->YUVEffectDelayFrame = 2;

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             pSensorInfo->SensorClockFreq = S5K8AAYX_MCLK;
             pSensorInfo->SensorClockDividCount = 3;
             pSensorInfo->SensorClockRisingCount = 0;
             pSensorInfo->SensorClockFallingCount = 2;
             pSensorInfo->SensorPixelClockCount = 3;
             pSensorInfo->SensorDataLatchCount = 2;
             pSensorInfo->SensorGrabStartX = 2;
             pSensorInfo->SensorGrabStartY = 2;
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
             pSensorInfo->SensorClockFreq=S5K8AAYX_MCLK;
             pSensorInfo->SensorClockDividCount=   3;
             pSensorInfo->SensorClockRisingCount= 0;
             pSensorInfo->SensorClockFallingCount= 2;
             pSensorInfo->SensorPixelClockCount= 3;
             pSensorInfo->SensorDataLatchCount= 2;
             pSensorInfo->SensorGrabStartX = 2;
             pSensorInfo->SensorGrabStartY = 2;
             break;
        default:
             pSensorInfo->SensorClockFreq=S5K8AAYX_MCLK;
             pSensorInfo->SensorClockDividCount=3;
             pSensorInfo->SensorClockRisingCount=0;
             pSensorInfo->SensorClockFallingCount=2;
             pSensorInfo->SensorPixelClockCount=3;
             pSensorInfo->SensorDataLatchCount=2;
             pSensorInfo->SensorGrabStartX = 2;
             pSensorInfo->SensorGrabStartY = 2;
             break;
    }
    memcpy(pSensorConfigData, &S5K8AAYXSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
}/* S5K8AAYXGetInfo() */


UINT32 S5K8AAYControl(MSDK_SCENARIO_ID_ENUM ScenarioId,
                       MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                       MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData) //little bit fixed
{
    SENSORDB("Enter S5K8AAYControl\n");
    SENSORDB("S5K8AAYControl ScenarioId = %d ,\n",ScenarioId);

    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYXCurrentScenarioId = ScenarioId;
    spin_unlock(&s5k8aayx_drv_lock);

    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
             SENSORDB("YSZ_S5K8AAYX_S5K8AAYControl_preview");
             S5K8AAYXPreview(pImageWindow, pSensorConfigData);
             break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
             /*S5K8AAYXCapture(pImageWindow, pSensorConfigData);
             break;*/ //commented out by Pavel
        case MSDK_SCENARIO_ID_CAMERA_ZSD:	//right
			 SENSORDB("S5K8AAY MSDK_SCENARIO_ID_CAMERA_ZSD");
             S5K8AAYXCapture(pImageWindow, pSensorConfigData);
             break;
		case ACDK_SCENARIO_ID_CAMERA_CAPTURE_MEM: //added, seems to be right
			 S5K8AAYXCapture(pImageWindow, pSensorConfigData);
             break;
        default:
           return ERROR_INVALID_SCENARIO_ID;
    }
    return ERROR_NONE;
}   /* S5K8AAYControl() */



/* [TC] YUV sensor */



BOOL S5K8AAYX_set_param_wb(UINT16 para)
{
    kal_uint16 AWB_uGainsOut0,AWB_uGainsOut1,AWB_uGainsOut2;
    SENSORDB("Enter S5K8AAYX_set_param_wb\n");
    if(S5K8AAYCurrentStatus.iWB == para)
        return TRUE;
    SENSORDB("[Enter]S5K8AAYX set_param_wb func:para = %d\n",para);

    switch (para)
    {
        case AWB_MODE_AUTO:
             //Read Back
             S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
             S5K8AAY_write_cmos_sensor(0x002C, 0x7000);
             S5K8AAY_write_cmos_sensor(0x002E, 0x20EC);
             AWB_uGainsOut0=S5K8AAYX_read_cmos_sensor(0x0F12); //20EC
             AWB_uGainsOut1=S5K8AAYX_read_cmos_sensor(0x0F12); //20EE
             AWB_uGainsOut2=S5K8AAYX_read_cmos_sensor(0x0F12); //20F0

             S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
             S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
             S5K8AAY_write_cmos_sensor(0x002A, 0x0D2A);
             S5K8AAY_write_cmos_sensor(0x0F12, AWB_uGainsOut0); //0x0D2A
             S5K8AAY_write_cmos_sensor(0x0F12, AWB_uGainsOut1); //0x0D2C
             S5K8AAY_write_cmos_sensor(0x0F12, AWB_uGainsOut2); //0x0D2E

             S5K8AAY_write_cmos_sensor(0x002A, 0x2162);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);

             //S5K8AAY_write_cmos_sensor(0x002A,0x0408);
             //S5K8AAY_write_cmos_sensor(0x0F12,0x067F);//bit[3]:AWB Auto:1 menual:0
             break;
        case AWB_MODE_CLOUDY_DAYLIGHT:
             //======================================================================
             //      MWB : Cloudy_D65
             //======================================================================
             S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
             S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
             S5K8AAY_write_cmos_sensor(0x002A, 0x2162);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);
             S5K8AAY_write_cmos_sensor(0x002A, 0x03DA);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0850);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0400);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x04E0);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             break;
        case AWB_MODE_DAYLIGHT:
             //==============================================
             //      MWB : sun&daylight_D50
             //==============================================
             S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
             S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
             S5K8AAY_write_cmos_sensor(0x002A, 0x2162);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);
             S5K8AAY_write_cmos_sensor(0x002A, 0x03DA);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0620);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0400);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0530);
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
             break;
        case AWB_MODE_FLUORESCENT:
              //==================================================================
              //      MWB : Florescent_TL84
              //==================================================================
              S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
              S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
              S5K8AAY_write_cmos_sensor(0x002A, 0x2162);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);
              S5K8AAY_write_cmos_sensor(0x002A, 0x03DA);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0560);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0400);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0880);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              break;
        case AWB_MODE_INCANDESCENT:
        case AWB_MODE_TUNGSTEN:
              S5K8AAY_write_cmos_sensor(0xFCFC, 0xD000);
              S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
              S5K8AAY_write_cmos_sensor(0x002A, 0x2162);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0000);
              S5K8AAY_write_cmos_sensor(0x002A, 0x03DA);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x03C0);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0400);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0980);
              S5K8AAY_write_cmos_sensor(0x0F12, 0x0001);
              break;
        default:
              return KAL_FALSE;
    }
    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYCurrentStatus.iWB = para;
    spin_unlock(&s5k8aayx_drv_lock);
    return TRUE;
}




void S5K8AAYX_GetAEAWBLock(UINT32 *pAElockRet32,UINT32 *pAWBlockRet32)
{
    *pAElockRet32 = 0;
    *pAWBlockRet32 = 0;
    SENSORDB("S5K8AAYX_GetAEAWBLock,AE=%d ,AWB=%d\n,",*pAElockRet32,*pAWBlockRet32);
}


BOOL S5K8AAYX_set_param_effect(UINT16 para)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                                                                        */
    /*----------------------------------------------------------------*/
    kal_bool ret=KAL_TRUE;

    /*----------------------------------------------------------------*/
    /* Code Body                                                                                                                          */
    /*----------------------------------------------------------------*/

    SENSORDB("Enter S5K8AAYX_set_param_effect\n");

    if(S5K8AAYCurrentStatus.iEffect == para)
        return TRUE;
    SENSORDB("[Enter]s5k8aayx set_param_effect func:para = %d\n",para);

    S5K8AAY_write_cmos_sensor(0xFCFC,0xD000);
    S5K8AAY_write_cmos_sensor(0x0028,0x7000);
    S5K8AAY_write_cmos_sensor(0x002A,0x019C);
    switch (para)
    {
        case MEFFECT_OFF:
             S5K8AAY_write_cmos_sensor(0x0F12,0x0000); // Normal,
             break;
        case MEFFECT_MONO:
             S5K8AAY_write_cmos_sensor(0x0F12,0x0001); // Monochrome (Black & White)
             break;
        case MEFFECT_SEPIA:
             S5K8AAY_write_cmos_sensor(0x0F12,0x0004); // Sepia
             break;
        case MEFFECT_NEGATIVE:
             S5K8AAY_write_cmos_sensor(0x0F12,0x0003); // Negative Mono
             break;
        default:
             ret = KAL_FALSE;
    }
    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYCurrentStatus.iEffect = para;
    spin_unlock(&s5k8aayx_drv_lock);
    return ret;
}


BOOL S5K8AAYX_set_param_banding(UINT16 para)
{
    /*----------------------------------------------------------------*/
    /* Local Variables                                                */
    /*----------------------------------------------------------------*/
    /*----------------------------------------------------------------*/
    /* Code Body                                                      */
    /*----------------------------------------------------------------*/
    SENSORDB("Enter S5K8AAYX_set_param_banding\n");
//#if (defined(S5K8AAYX_MANUAL_ANTI_FLICKER))

    if(S5K8AAYCurrentStatus.iBanding == para)
        return TRUE;

    switch (para)
    {
        case AE_FLICKER_MODE_50HZ:
             S5K8AAY_write_cmos_sensor(0x0028,0x7000);
             S5K8AAY_write_cmos_sensor(0x002a,0x0408);
             S5K8AAY_write_cmos_sensor(0x0f12,0x065F);
             S5K8AAY_write_cmos_sensor(0x002a,0x03F4);
             S5K8AAY_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuant 1:50hz  2:60hz
             S5K8AAY_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuantChanged active flicker
             break;
        case AE_FLICKER_MODE_60HZ:
             S5K8AAY_write_cmos_sensor(0x0028,0x7000);
             S5K8AAY_write_cmos_sensor(0x002a,0x0408);
             S5K8AAY_write_cmos_sensor(0x0f12,0x065F);
             S5K8AAY_write_cmos_sensor(0x002a,0x03F4);
             S5K8AAY_write_cmos_sensor(0x0f12,0x0002); //REG_SF_USER_FlickerQuant 1:50hz  2:60hz
             S5K8AAY_write_cmos_sensor(0x0f12,0x0001); //REG_SF_USER_FlickerQuantChanged active flicker
             break;
        default:
             return KAL_FALSE;
    }
    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYCurrentStatus.iBanding = para;
    spin_unlock(&s5k8aayx_drv_lock);
    return TRUE;

//#else
    /* Auto anti-flicker method is enabled, then nothing need to do in this function.  */
//#endif /* #if (defined(S5K8AAYX_MANUAL_ANTI_FLICKER)) */
    return KAL_TRUE;
} /* S5K8AAYX_set_param_banding */



BOOL S5K8AAYX_set_param_exposure(UINT16 para)
{
    SENSORDB("Enter S5K8AAYX_set_param_exposure\n");
    if(S5K8AAYCurrentStatus.iEV == para)
        return TRUE;

    SENSORDB("[Enter]s5k8aayx set_param_exposure func:para = %d\n",para);
    S5K8AAY_write_cmos_sensor(0x0028, 0x7000);
    S5K8AAY_write_cmos_sensor(0x002a, 0x019A);

    switch (para)
    {
        case AE_EV_COMP_n10:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0080); //EV -1
             break;
        case AE_EV_COMP_00:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0100); //EV 0
             break;
        case AE_EV_COMP_10:
             S5K8AAY_write_cmos_sensor(0x0F12, 0x0200);  //EV +1
             break;
        case AE_EV_COMP_n13:
        case AE_EV_COMP_n07:
        case AE_EV_COMP_n03:
        case AE_EV_COMP_03:
        case AE_EV_COMP_07:
        case AE_EV_COMP_13:
             break;
        default:
             return FALSE;
    }
    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYCurrentStatus.iEV = para;
    spin_unlock(&s5k8aayx_drv_lock);
    return TRUE;

}/* S5K8AAYX_set_param_exposure */


UINT32 S5K8AAYXYUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{

	MSDK_SCENARIO_ID_ENUM scen = MSDK_SCENARIO_ID_CAMERA_PREVIEW;

    spin_lock(&s5k8aayx_drv_lock);
    scen = S5K8AAYXCurrentScenarioId;
    spin_unlock(&s5k8aayx_drv_lock);

    SENSORDB("Enter S5K8AAYXYUVSensorSetting\n");
    switch (iCmd)
    {
        case FID_SCENE_MODE:
            /*if (iPara == SCENE_MODE_OFF)
            {
                S5K8AAYX_SetFrameRate(scen, 30);
                S5K8AAY_night_mode(0);
            }
            else if (iPara == SCENE_MODE_NIGHTSCENE)
            {
                S5K8AAYX_SetFrameRate(scen, 15);
                S5K8AAY_night_mode(1);
            }*/
            break;
        case FID_AWB_MODE:
            S5K8AAYX_set_param_wb(iPara);
            break;
        case FID_COLOR_EFFECT:
            S5K8AAYX_set_param_effect(iPara);
            break;
        case FID_AE_EV:
            S5K8AAYX_set_param_exposure(iPara);
            break;
        case FID_AE_FLICKER:
            S5K8AAYX_set_param_banding(iPara);
            break;
        case FID_AE_SCENE_MODE:
            spin_lock(&s5k8aayx_drv_lock);
            if (iPara == AE_MODE_OFF) {
                S5K8AAYX_AE_ENABLE = KAL_FALSE;
            }
            else {
                S5K8AAYX_AE_ENABLE = KAL_TRUE;
            }
            spin_unlock(&s5k8aayx_drv_lock);
            S5K8AAYX_set_AE_mode(S5K8AAYX_AE_ENABLE);
            break;
        case FID_ZOOM_FACTOR:
            zoom_factor = iPara;
            break;
        default:
            break;
    }
    return ERROR_NONE;
}   /* S5K8AAYXYUVSensorSetting */


UINT32 S5K8AAYXYUVSetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("Enter S5K8AAYXYUVSetVideoMode,u2FrameRate=%d\n",u2FrameRate);

    spin_lock(&s5k8aayx_drv_lock);
    S5K8AAYCurrentStatus.iFrameRate = u2FrameRate;
    spin_unlock(&s5k8aayx_drv_lock);

    //S5K8AAYX_SetFrameRate(MSDK_SCENARIO_ID_VIDEO_PREVIEW, u2FrameRate);
    mdelay(100);

    return TRUE;
}


kal_uint16 S5K8AAYXReadShutter(void)
{
   kal_uint16 temp;
   S5K8AAY_write_cmos_sensor(0x002c,0x7000);
   S5K8AAY_write_cmos_sensor(0x002e,0x16E2);

   temp=S5K8AAYX_read_cmos_sensor(0x0f12);

   return temp;
}


kal_uint16 S5K8AAYXReadGain(void)
{
   kal_uint16 temp;
   S5K8AAY_write_cmos_sensor(0x002c,0x7000);
   S5K8AAY_write_cmos_sensor(0x002e,0x20D0);
   temp=S5K8AAYX_read_cmos_sensor(0x0f12);
   return temp;
}


kal_uint16 S5K8AAYXReadAwbRGain(void)
{
   kal_uint16 temp;
   S5K8AAY_write_cmos_sensor(0x002c,0x7000);
   S5K8AAY_write_cmos_sensor(0x002e,0x20D2);
   temp=S5K8AAYX_read_cmos_sensor(0x0f12);
   return temp;
}


kal_uint16 S5K8AAYXReadAwbBGain(void) //not quite sure about the values, but seems alright
{
   kal_uint16 temp;
   S5K8AAY_write_cmos_sensor(0x002c,0x7000);
   S5K8AAY_write_cmos_sensor(0x002e,0x20D2);
   temp=S5K8AAYX_read_cmos_sensor(0x0f12);
   return temp;
}


//#if defined(MT6575)
/*************************************************************************
* FUNCTION
*    S5K8AAYXGetEvAwbRef
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K8AAYXGetEvAwbRef(UINT32 pSensorAEAWBRefStruct/*PSENSOR_AE_AWB_REF_STRUCT Ref*/)
{
    PSENSOR_AE_AWB_REF_STRUCT Ref = (PSENSOR_AE_AWB_REF_STRUCT)pSensorAEAWBRefStruct;
    //SENSORDB("S5K8AAYXGetEvAwbRef ref = 0x%x \n", Ref);

    Ref->SensorAERef.AeRefLV05Shutter = 1503;
    Ref->SensorAERef.AeRefLV05Gain = 496 * 2; /* 7.75x, 128 base */
    Ref->SensorAERef.AeRefLV13Shutter = 49;
    Ref->SensorAERef.AeRefLV13Gain = 64 * 2; /* 1x, 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Rgain = 188; /* 1.46875x, 128 base */
    Ref->SensorAwbGainRef.AwbRefD65Bgain = 128; /* 1x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFRgain = 160; /* 1.25x, 128 base */
    Ref->SensorAwbGainRef.AwbRefCWFBgain = 164; /* 1.28125x, 128 base */
}
/*************************************************************************
* FUNCTION
*    S5K8AAYXGetCurAeAwbInfo
*
* DESCRIPTION
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void S5K8AAYXGetCurAeAwbInfo(UINT32 pSensorAEAWBCurStruct/*PSENSOR_AE_AWB_CUR_STRUCT Info*/)
{
    PSENSOR_AE_AWB_CUR_STRUCT Info = (PSENSOR_AE_AWB_CUR_STRUCT)pSensorAEAWBCurStruct;
    //SENSORDB("S5K8AAYXGetCurAeAwbInfo Info = 0x%x \n", Info);

    Info->SensorAECur.AeCurShutter = S5K8AAYXReadShutter();
    Info->SensorAECur.AeCurGain = S5K8AAYXReadGain() * 2; /* 128 base */

    Info->SensorAwbGainCur.AwbCurRgain = S5K8AAYXReadAwbRGain()<< 1; /* 128 base */

    Info->SensorAwbGainCur.AwbCurBgain = S5K8AAYXReadAwbBGain()<< 1; /* 128 base */
}
//#endif //6575

void S5K8AAYXGetAFMaxNumFocusAreas(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 0;
    SENSORDB("S5K8AAYXGetAFMaxNumFocusAreas, *pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);
}


void S5K8AAYXGetAFMaxNumMeteringAreas(UINT32 *pFeatureReturnPara32)
{
    *pFeatureReturnPara32 = 0;
    SENSORDB("S5K8AAYXGetAFMaxNumMeteringAreas,*pFeatureReturnPara32 = %d\n",*pFeatureReturnPara32);
}


void S5K8AAYXGetExifInfo(UINT32 exifAddr)
{
    SENSOR_EXIF_INFO_STRUCT* pExifInfo = (SENSOR_EXIF_INFO_STRUCT*)exifAddr;
    pExifInfo->FNumber = 28;
    pExifInfo->AEISOSpeed = S5K8AAYCurrentStatus.isoSpeed;
    pExifInfo->AWBMode = S5K8AAYCurrentStatus.iWB;
    pExifInfo->CapExposureTime = 0;
    pExifInfo->FlashLightTimeus = 0;
    pExifInfo->RealISOValue = S5K8AAYCurrentStatus.isoSpeed;
}


UINT32 S5K8AAYXYUVSetTestPatternMode(kal_bool bEnable)
{
    SENSORDB("[S5K8AAYXYUVSetTestPatternMode] Test pattern enable:%d\n", bEnable);

  if(bEnable)
  {
      //Address: D0003600
      //0x0000 -- bypass
      //0x0002 - solid color
      //0x0004 - gradient
      //0x0006 - address dependent noise
      //0x0008 - random
      //0x000A - gradient + address dependent noise
      //0x000C - gradient + random
      //0x000E - out pixel attributes
      S5K8AAY_write_cmos_sensor(0x3600,0x0004);
      //0x0002 - solid color
      S5K8AAY_write_cmos_sensor(0x3602,0x1F40);
      S5K8AAY_write_cmos_sensor(0x3604,0x1A40);
      S5K8AAY_write_cmos_sensor(0x3606,0x1A40);
      S5K8AAY_write_cmos_sensor(0x3608,0x1040);
      //0x0004 - gradient
      S5K8AAY_write_cmos_sensor(0x360a,0x0383);

  }
  else
  {
    S5K8AAY_write_cmos_sensor(0x3600,0x0000);
  }
    return ERROR_NONE;
}

UINT32 S5K8AAYXFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                              UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    //PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    //MSDK_SENSOR_ENG_INFO_STRUCT      *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;
#if WINMO_USE
    PMSDK_FEATURE_INFO_STRUCT pSensorFeatureInfo=(PMSDK_FEATURE_INFO_STRUCT) pFeaturePara;
#endif

    SENSORDB("Enter S5K8AAYXFeatureControl. ID=%d\n", FeatureId);

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
             *pFeatureReturnPara16++ = S5K8AAYX_IMAGE_SENSOR_FULL_WIDTH;
             *pFeatureReturnPara16 = S5K8AAYX_IMAGE_SENSOR_FULL_HEIGHT;
             *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_GET_PERIOD:
             //*pFeatureReturnPara16++ = S5K8AAYX_PV_PERIOD_PIXEL_NUMS + S5K8AAYX_PV_dummy_pixels;
             //*pFeatureReturnPara16 = S5K8AAYX_PV_PERIOD_LINE_NUMS + S5K8AAYX_PV_dummy_lines;
             //*pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
             *pFeatureReturnPara32 = S5K8AAYX_sensor_pclk/10;
             *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_SET_ESHUTTER:
             break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
             S5K8AAY_night_mode((BOOL) *pFeatureData16);
             break;
        case SENSOR_FEATURE_SET_GAIN:
        case SENSOR_FEATURE_SET_FLASHLIGHT:
             break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
             S5K8AAYX_isp_master_clock=*pFeatureData32;
             break;
        case SENSOR_FEATURE_SET_REGISTER:
             S5K8AAY_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
             break;
        case SENSOR_FEATURE_GET_REGISTER:
             pSensorRegData->RegData = S5K8AAYX_read_cmos_sensor(pSensorRegData->RegAddr);
             break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
             memcpy(pSensorConfigData, &S5K8AAYXSensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
             *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
             break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
        case SENSOR_FEATURE_GET_CCT_REGISTER:
        case SENSOR_FEATURE_SET_ENG_REGISTER:
        case SENSOR_FEATURE_GET_ENG_REGISTER:
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
        case SENSOR_FEATURE_GET_GROUP_INFO:
        case SENSOR_FEATURE_GET_ITEM_INFO:
        case SENSOR_FEATURE_SET_ITEM_INFO:
        case SENSOR_FEATURE_GET_ENG_INFO:
             break;
        case SENSOR_FEATURE_GET_GROUP_COUNT:
             *pFeatureReturnPara32++ = 0;
             *pFeatureParaLen = 4;
             break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
             // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
             // if EEPROM does not exist in camera module.
             *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
             *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_SET_YUV_CMD:
             //S5K8AAYXYUVSensorSetting((MSDK_ISP_FEATURE_ENUM)*pFeatureData16, *(pFeatureData16+1));
             S5K8AAYXYUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
             break;
             //break;
        #if WINMO_USE
             case SENSOR_FEATURE_QUERY:
                  S5K8AAYXQuery(pSensorFeatureInfo);
                  *pFeatureParaLen = sizeof(MSDK_FEATURE_INFO_STRUCT);
                  break;
             case SENSOR_FEATURE_SET_YUV_CAPTURE_RAW_SUPPORT:
                  /* update yuv capture raw support flag by *pFeatureData16 */
                  break;
        #endif

        case SENSOR_FEATURE_SET_VIDEO_MODE: //approved
             SENSORDB("S5K8AAYX SENSOR_FEATURE_SET_VIDEO_MODE\r\n");
             S5K8AAYXYUVSetVideoMode(*pFeatureData16);
             break;
        case SENSOR_FEATURE_GET_EV_AWB_REF:	//not quite sure
             S5K8AAYXGetEvAwbRef(*pFeatureData32);
             break;
        case SENSOR_FEATURE_GET_SHUTTER_GAIN_AWB_GAIN:
             S5K8AAYXGetCurAeAwbInfo(*pFeatureData32);
             break;
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
             S5K8AAYX_GetSensorID(pFeatureData32);
             break;

        case SENSOR_FEATURE_GET_AF_MAX_NUM_FOCUS_AREAS:
             S5K8AAYXGetAFMaxNumFocusAreas(pFeatureReturnPara32);
             *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_GET_AE_MAX_NUM_METERING_AREAS:
             S5K8AAYXGetAFMaxNumMeteringAreas(pFeatureReturnPara32);
             *pFeatureParaLen=4;
             break;
        case SENSOR_FEATURE_GET_EXIF_INFO:
             SENSORDB("SENSOR_FEATURE_GET_EXIF_INFO\n");
             SENSORDB("EXIF addr = 0x%x\n",*pFeatureData32);
             S5K8AAYXGetExifInfo(*pFeatureData32);
             break;
       case SENSOR_FEATURE_SET_TEST_PATTERN:
            S5K8AAYXYUVSetTestPatternMode((BOOL)*pFeatureData16);
            break;
       /* case SENSOR_FEATURE_GET_TEST_PATTERN_CHECKSUM_VALUE://for factory mode auto testing
            *pFeatureReturnPara32= S5K8AAYXYUV_TEST_PATTERN_CHECKSUM;
            *pFeatureParaLen=4;
            break;*/

        default:
             SENSORDB("Enter S5K8AAYXFeatureControl. default. return\n");
             break;
    }
    return ERROR_NONE;
}

SENSOR_FUNCTION_STRUCT SensorFuncS5K8AAY=
{
    S5K8AAYOpen,
    S5K8AAYXGetInfo,
    S5K8AAYGetResolution,
    S5K8AAYXFeatureControl,
    S5K8AAYControl,
    S5K8AAYClose
};

UINT32 S5K8AAY_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)	//approved
{
    SENSORDB("Enter S5K8AAY_YUV_SensorInit\n");
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncS5K8AAY;
    return ERROR_NONE;
}   /* SensorInit() */

