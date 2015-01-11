/*****************************************************************************
 *
 * Filename:
 * ---------
 *   Sensor.c
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/io.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "ov7690yuv_Sensor.h"
#include "ov7690yuv_Camera_Sensor_para.h"
#include "ov7690yuv_CameraCustomized.h"

#define Sleep(ms) mdelay(ms)

MSDK_SENSOR_CONFIG_STRUCT OV7690SensorConfigData;

#define OV7690YUV_DEBUG
#ifdef OV7690YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

#if 0
extern int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId);
extern int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId);
static int sensor_id_fail = 0; 
#define OV7690_write_cmos_sensor(addr,para) iWriteReg((u16) addr , (u32) para ,1,OV7690_WRITE_ID)
//#define OV7690_write_cmos_sensor_2(addr, para, bytes) iWriteReg((u16) addr , (u32) para ,bytes,OV7690_WRITE_ID)
kal_uint16 OV7690_read_cmos_sensor(kal_uint32 addr)
{
kal_uint16 get_byte=0;
    iReadReg((u16) addr ,(u8*)&get_byte,OV7690_WRITE_ID);
    return get_byte;
}
#endif

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);
static void OV7690_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
kal_uint8 out_buff[2];

    out_buff[0] = addr;
    out_buff[1] = para;

    if (0 != iWriteRegI2C((u8*)out_buff , (u16)sizeof(out_buff), OV7690_WRITE_ID)) {
        SENSORDB("ERROR: OV7690_write_cmos_sensor \n");
    }
}

static kal_uint8 OV7690_read_cmos_sensor(kal_uint8 addr)
{
  kal_uint8 in_buff[1] = {0xFF};
  kal_uint8 out_buff[1];
  
  out_buff[0] = addr;

    if (0 != iReadRegI2C((u8*)out_buff , (u16) sizeof(out_buff), (u8*)in_buff, (u16) sizeof(in_buff), OV7690_WRITE_ID)) {
        SENSORDB("ERROR: OV7690_read_cmos_sensor \n");
    }

  return in_buff[0];
}


static struct
{
  kal_uint16 Effect;
  kal_uint16 Exposure;
  kal_uint16 Wb;
  kal_uint8 Mirror;
  kal_bool BypassAe;
  kal_bool BypassAwb;
  kal_bool VideoMode;
  kal_uint8 Ctrl3A;
  kal_uint8 SdeCtrl;
  kal_uint8 Reg15;
  kal_uint16 Fps;
  kal_uint32 IntClk; /* internal clock = half of pclk */
  kal_uint16 FrameHeight;
  kal_uint16 LineLength;
  //sensor_data_struct *NvramData;
} OV7690Sensor;
/*************************************************************************
* FUNCTION
*    OV7690HalfAdjust
*
* DESCRIPTION
*    This function dividend / divisor and use round-up.
*
* PARAMETERS
*    DividEnd
*    Divisor
*
* RETURNS
*    [DividEnd / Divisor]
*
* LOCAL AFFECTED
*
*************************************************************************/
__inline static kal_uint32 OV7690HalfAdjust(kal_uint32 DividEnd, kal_uint32 Divisor)
{
  return (DividEnd + (Divisor >> 1)) / Divisor; /* that is [DividEnd / Divisor + 0.5]*/
}



/*************************************************************************
* FUNCTION
*    OV7690SetMirror
*
* DESCRIPTION
*    This function set the Mirror to the CMOS sensor
*
* PARAMETERS
*    Mirror
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690SetMirror(kal_uint8 Mirror)
{
  kal_uint8 Reg = 0x16;

  if (OV7690Sensor.Mirror == Mirror)
  {
    return;
  }
  OV7690Sensor.Mirror = Mirror;
  switch (OV7690Sensor.Mirror)
  {
  case IMAGE_NORMAL:
    break;
  case IMAGE_H_MIRROR:
    Reg |= 0x40;
    break;
  case IMAGE_V_MIRROR:
    Reg |= 0x80;
    break;
  case IMAGE_HV_MIRROR:
    Reg |= 0xC0;
    break;
  }
  OV7690_write_cmos_sensor(0x0C, Reg);
}

/*************************************************************************
* FUNCTION
*    OV7690SetClock
*
* DESCRIPTION
*    This function set sensor vt clock and op clock
*
* PARAMETERS
*    Clk: vt clock
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690SetClock(kal_uint32 Clk)
{
  static const kal_uint8 ClkSetting[][1] =
  {
    {0x00}, /* MCLK: 26M, INTERNAL CLK: 13M, PCLK: 26M */
    {0x01}, /* MCLK: 26M, INTERNAL CLK: 6.5M, PCLK: 13M */
  };
  kal_uint8 i = 0;
  
  if (OV7690Sensor.IntClk == Clk)
  {
    return;
  }
  OV7690Sensor.IntClk = Clk;
  switch (OV7690Sensor.IntClk)
  {
  case 13000000: i = 0; break;
  case 6500000:  i = 1; break;
  default: ASSERT(0);
  }
/*
  PLL Control:
  PLL clock(f1) = MCLK x N / M, where
    reg29[7:6]: PLL divider
      00: /1, 01: /2, 10: /3, 11: /4
    reg29[5:4]: PLL output control
      00: Bypass PLL, 01: 4x, 10: 6x, 11: 8x
  Int. clock(f2) = f1 / (reg0x11[5:0] + 1)
  PCLK = f2 / 2 / L, where L = 1 if 0x3E[4] = 1, otherwise L = 2
*/
  OV7690_write_cmos_sensor(0x11, ClkSetting[i][0]);
}

#if 0 /* not referenced currently */
/*************************************************************************
* FUNCTION
*    OV7690WriteShutter
*
* DESCRIPTION
*    This function apply Shutter to sensor
*
* PARAMETERS
*    Shutter: integration time
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690WriteShutter(kal_uint32 Shutter)
{
  OV7690_write_cmos_sensor(0x10, Shutter);
  OV7690_write_cmos_sensor(0x0F, Shutter >> 8);
}

/*************************************************************************
* FUNCTION
*    OV7690ReadShutter
*
* DESCRIPTION
*    This function get shutter from sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    Shutter: integration time
*
* LOCAL AFFECTED
*
*************************************************************************/
static kal_uint16 OV7690ReadShutter(void)
{
  return (OV7690_read_cmos_sensor(0x0F) << 8)|OV7690_read_cmos_sensor(0x10);
}
#endif
/*************************************************************************
* FUNCTION
*    OV7690LSCSetting
*
* DESCRIPTION
*    This function set Lens Shading Correction.
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690LSCSetting(void)
{
  static const kal_uint8 Data[] =
  {
#if 0 /* Sunny F28 */
    0x90,0x18,0xb0,0xA0,0x24,0x1f,0x21,
#elif 0 /* Sunny F24 */
    0x90,0x18,0x10,0x00,0x32,0x2c,0x30,
#elif 0 /* Phoenix F28 */
    0x90,0x18,0xb0,0xA0,0x32,0x2c,0x30,
#elif 1 /* Phoenix F24 */
    0x90,0x00,0xa0,0x80,0x18,0x14,0x15,
#elif 0 /* Dongya F24 */
    0x90,0x18,0x10,0x00,0x32,0x2c,0x30,
#endif
  };
  
  OV7690_write_cmos_sensor(0x85, Data[0]);
  OV7690_write_cmos_sensor(0x86, Data[1]);
  OV7690_write_cmos_sensor(0x87, Data[2]);
  OV7690_write_cmos_sensor(0x88, Data[3]);
  OV7690_write_cmos_sensor(0x89, Data[4]);
  OV7690_write_cmos_sensor(0x8a, Data[5]);
  OV7690_write_cmos_sensor(0x8b, Data[6]);
}

/*************************************************************************
* FUNCTION
*    OV7690AeEnable
*
* DESCRIPTION
*    disable/enable AE
*
* PARAMETERS
*    Enable
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690AeEnable(kal_bool Enable)
{
  const kal_bool AeEnable = (OV7690Sensor.Ctrl3A&0x05) ? KAL_TRUE : KAL_FALSE;
  
  if (OV7690Sensor.BypassAe)
  {
    Enable = KAL_FALSE;
  }
  if (AeEnable == Enable)
  {
    return;
  }
  if (Enable)
  {
    OV7690Sensor.Ctrl3A |= 0x05;
    OV7690Sensor.Reg15 |= 0x80;
  }
  else
  {
    OV7690Sensor.Ctrl3A &= 0xF8;
    /* extra line can not be set if not fix frame rate!!! */
    OV7690Sensor.Reg15 &= 0x7F;
  }
  OV7690_write_cmos_sensor(0x13, OV7690Sensor.Ctrl3A);
  OV7690_write_cmos_sensor(0x15, OV7690Sensor.Reg15);
}



/*************************************************************************
* FUNCTION
*    OV7690AwbEnable
*
* DESCRIPTION
*    disable/enable awb
*
* PARAMETERS
*    Enable
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690AwbEnable(kal_bool Enable)
{
  const kal_bool AwbEnable = (OV7690Sensor.Ctrl3A&0x02) ? KAL_TRUE : KAL_FALSE;
  
  if (OV7690Sensor.BypassAwb)
  {
    Enable = KAL_FALSE;
  }
  if (AwbEnable == Enable)
  {
    return;
  }
  if (Enable && AWB_MODE_AUTO == OV7690Sensor.Wb)
  {
    OV7690Sensor.Ctrl3A |= 0x02;
  }
  else
  {
    OV7690Sensor.Ctrl3A &= 0xFD;
  }
  OV7690_write_cmos_sensor(0x13, OV7690Sensor.Ctrl3A);
}

/*************************************************************************
* FUNCTION
*    OV7690SetDummy
*
* DESCRIPTION
*    This function add DummyPixel and DummyLine.
*
* PARAMETERS
*    DummyPixel
*    DummyLine
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690SetDummy(kal_uint16 DummyPixel, kal_uint16 DummyLine)
{
  kal_bool update = KAL_FALSE; /* need config banding */
  const kal_uint16 LineLength = DummyPixel + OV7690_PERIOD_PIXEL_NUMS;
  const kal_uint16 FrameHeight = DummyLine + OV7690_PERIOD_LINE_NUMS;
  
  /* config line length */
  if (LineLength != OV7690Sensor.LineLength)
  {
    if (LineLength > OV7690_MAX_LINELENGTH)
    {
      ASSERT(0);
    }
    update = KAL_TRUE;
    OV7690Sensor.LineLength = LineLength;
    OV7690_write_cmos_sensor(0x2B, LineLength); /* Actually, the 0x2a,0x2b is not DummyPixel, it's the total line length */
  }
  /* config frame height */
  if (FrameHeight != OV7690Sensor.FrameHeight)
  {
    if (DummyLine > OV7690_MAX_FRAMEHEIGHT)
    {
      ASSERT(0);
    }
    update = KAL_TRUE;
    OV7690Sensor.FrameHeight = FrameHeight;
    OV7690_write_cmos_sensor(0x2C, DummyLine);
  }
  /* config banding */
  if (update)
  {
    kal_uint8 BandStep50, BandStep60, MaxBand50, MaxBand60;
    
    OV7690_write_cmos_sensor(0x2A, ((LineLength >> 4)&0x70)|((DummyLine >> 8)&0x0F)|(OV7690_read_cmos_sensor(0x2A)&0x80));
    BandStep50 = OV7690HalfAdjust(OV7690Sensor.IntClk, LineLength * OV7690_NUM_50HZ * 2);
    BandStep60 = OV7690HalfAdjust(OV7690Sensor.IntClk, LineLength * OV7690_NUM_60HZ * 2);
    
    OV7690_write_cmos_sensor(0x50, 0x99);//BandStep50); /* 50Hz banding step */
    OV7690_write_cmos_sensor(0x51, 0x7F);//BandStep60); /* 60Hz banding step */
    
    /* max banding in a frame */
    MaxBand50 = FrameHeight / BandStep50;
    MaxBand60 = FrameHeight / BandStep60;
    OV7690_write_cmos_sensor(0x20, ((MaxBand50&0x10) << 3)|((MaxBand60&0x10) << 2));
    OV7690_write_cmos_sensor(0x21, 0x23);//((MaxBand50&0x0F) << 4)|(MaxBand60&0x0F));
  }
}

/*************************************************************************
* FUNCTION
*    OV7690CalFps
*
* DESCRIPTION
*    This function calculate & set frame rate and fix frame rate when video mode
*    MUST BE INVOKED AFTER OV7690_preview() !!!
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690CalFps(void)
{
  /* camera preview also fix frame rate for auto frame rate will cause AE peak */
  if (OV7690Sensor.VideoMode) /* fix frame rate when video mode */
  {
    kal_uint32 LineLength, FrameHeight;
    
    /* get max line length */
    LineLength = OV7690Sensor.IntClk * OV7690_FPS(1) / (OV7690Sensor.Fps * OV7690Sensor.FrameHeight);
    if (LineLength > OV7690_PERIOD_PIXEL_NUMS * 2) /* overflow check */
    {
      OV7690SetClock(OV7690_VIDEO_LOW_CLK); /* slow down clock */
      LineLength = OV7690Sensor.IntClk * OV7690_FPS(1) / (OV7690Sensor.Fps * OV7690Sensor.FrameHeight);
      if (LineLength > OV7690_MAX_LINELENGTH) /* overflow check */
      {
        LineLength = OV7690_MAX_LINELENGTH;
      }
    }
    if (LineLength < OV7690Sensor.LineLength)
    {
      LineLength = OV7690Sensor.LineLength;
    }
    /* get frame height */
    FrameHeight = OV7690Sensor.IntClk * OV7690_FPS(1) / (OV7690Sensor.Fps * LineLength);
    if (FrameHeight < OV7690Sensor.FrameHeight)
    {
      FrameHeight = OV7690Sensor.FrameHeight;
    }
    OV7690SetDummy(LineLength - OV7690_PERIOD_PIXEL_NUMS, FrameHeight - OV7690_PERIOD_LINE_NUMS);
    
    /* fix frame rate */
    OV7690Sensor.Reg15 &= 0x7F;
    OV7690_write_cmos_sensor(0x15, OV7690Sensor.Reg15);
  }
  else
  {
    kal_uint16 CurFps;
    
    CurFps = OV7690HalfAdjust(OV7690Sensor.IntClk * OV7690_FPS(1),
                              OV7690Sensor.Fps * OV7690Sensor.LineLength * OV7690Sensor.FrameHeight);
    /* to force frame rate change when change night mode to normal mode */
    OV7690Sensor.Reg15 &= 0x0F;
    switch (CurFps)
    {
    case 0:
    case 1: break;
    case 2: OV7690Sensor.Reg15 |= 0x10; break;
    case 3: OV7690Sensor.Reg15 |= 0x20; break;
    case 4: OV7690Sensor.Reg15 |= 0x30; break;
    default: OV7690Sensor.Reg15 |= 0x40; break;
    }
    OV7690_write_cmos_sensor(0x15, OV7690Sensor.Reg15);
    OV7690Sensor.Reg15 |= 0x80;
    OV7690_write_cmos_sensor(0x15, OV7690Sensor.Reg15);
  }
}

/*************************************************************************
* FUNCTION
*    OV7690InitialSetting
*
* DESCRIPTION
*    This function initialize the registers of CMOS sensor
*
* PARAMETERS
*    None
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690InitialSetting(void)
{
   //jashe porting 7692 (s)
   OV7690_write_cmos_sensor(0x12,0x80);
   msleep(5);
   OV7690_write_cmos_sensor(0x0C,0x16);
   OV7690_write_cmos_sensor(0x0E,0x08);
   OV7690_write_cmos_sensor(0x69,0x52);
   OV7690_write_cmos_sensor(0x1E,0xB3);
   OV7690_write_cmos_sensor(0x48,0x42);
   OV7690_write_cmos_sensor(0xFF,0x01);
   OV7690_write_cmos_sensor(0xB5,0x30);
   OV7690_write_cmos_sensor(0xFF,0x00);
   OV7690_write_cmos_sensor(0x16,0x03);
   OV7690_write_cmos_sensor(0x62,0x10);
   OV7690_write_cmos_sensor(0x12,0x00);
   OV7690_write_cmos_sensor(0x17,0x65);
   OV7690_write_cmos_sensor(0x18,0xA4);
   OV7690_write_cmos_sensor(0x19,0x0A);
   OV7690_write_cmos_sensor(0x1A,0xF6);
   OV7690_write_cmos_sensor(0x3E,0x20);
   OV7690_write_cmos_sensor(0x64,0x11);
   OV7690_write_cmos_sensor(0x67,0x20);
   OV7690_write_cmos_sensor(0x81,0x3F);
   OV7690_write_cmos_sensor(0xCC,0x02);
   OV7690_write_cmos_sensor(0xCD,0x80);
   OV7690_write_cmos_sensor(0xCE,0x01);
   OV7690_write_cmos_sensor(0xCF,0xE0);
   OV7690_write_cmos_sensor(0xC8,0x02);
   OV7690_write_cmos_sensor(0xC9,0x80);
   OV7690_write_cmos_sensor(0xCA,0x01);
   OV7690_write_cmos_sensor(0xCB,0xE0);
   OV7690_write_cmos_sensor(0xD0,0x48);
   OV7690_write_cmos_sensor(0x82,0x03);
   OV7690_write_cmos_sensor(0x0E,0x00);
   OV7690_write_cmos_sensor(0x70,0x00);
   OV7690_write_cmos_sensor(0x71,0x34);
   OV7690_write_cmos_sensor(0x74,0x28);
   OV7690_write_cmos_sensor(0x75,0x98);
   OV7690_write_cmos_sensor(0x76,0x00);
   OV7690_write_cmos_sensor(0x77,0x64);
   OV7690_write_cmos_sensor(0x78,0x01);
   OV7690_write_cmos_sensor(0x79,0xC2);
   OV7690_write_cmos_sensor(0x7A,0x4E);
   OV7690_write_cmos_sensor(0x7B,0x1F);
   OV7690_write_cmos_sensor(0x7C,0x00);
   OV7690_write_cmos_sensor(0x11,0x00); //5/29 for noise
   OV7690_write_cmos_sensor(0x20,0x00);
   OV7690_write_cmos_sensor(0x21,0x23);
   OV7690_write_cmos_sensor(0x50,0x99);
   OV7690_write_cmos_sensor(0x51,0x7F);
   OV7690_write_cmos_sensor(0x4c,0x7D);
   OV7690_write_cmos_sensor(0x0E,0x00);
   OV7690_write_cmos_sensor(0x80,0x7F);
   OV7690_write_cmos_sensor(0x85,0x00);
   OV7690_write_cmos_sensor(0x86,0x00);
   OV7690_write_cmos_sensor(0x87,0x00);
   OV7690_write_cmos_sensor(0x88,0x00);
   OV7690_write_cmos_sensor(0x89,0x2A);
   OV7690_write_cmos_sensor(0x8A,0x1d);
   OV7690_write_cmos_sensor(0x8B,0x1d);
   OV7690_write_cmos_sensor(0xBB,0xAB);
   OV7690_write_cmos_sensor(0xBC,0x84);
   OV7690_write_cmos_sensor(0xBD,0x27);
   OV7690_write_cmos_sensor(0xBE,0x0E);
   OV7690_write_cmos_sensor(0xBF,0xB8);
   OV7690_write_cmos_sensor(0xC0,0xC5);
   OV7690_write_cmos_sensor(0xC1,0x1E);
   OV7690_write_cmos_sensor(0xB7,0x04);
   OV7690_write_cmos_sensor(0xB8,0x09);
   OV7690_write_cmos_sensor(0xB9,0x00);
   OV7690_write_cmos_sensor(0xBA,0x18);
   OV7690_write_cmos_sensor(0x5A,0x1F);
   OV7690_write_cmos_sensor(0x5B,0x9F);
   OV7690_write_cmos_sensor(0x5C,0x69);
   OV7690_write_cmos_sensor(0x5d,0x42);
   OV7690_write_cmos_sensor(0x24,0x78);
   OV7690_write_cmos_sensor(0x25,0x68);
   OV7690_write_cmos_sensor(0x26,0xB3);
   OV7690_write_cmos_sensor(0xA3,0x0B);
   OV7690_write_cmos_sensor(0xA4,0x15);
   OV7690_write_cmos_sensor(0xA5,0x29);
   OV7690_write_cmos_sensor(0xA6,0x4A);
   OV7690_write_cmos_sensor(0xA7,0x58);
   OV7690_write_cmos_sensor(0xA8,0x65);
   OV7690_write_cmos_sensor(0xA9,0x70);
   OV7690_write_cmos_sensor(0xAA,0x7B);
   OV7690_write_cmos_sensor(0xAB,0x85);
   OV7690_write_cmos_sensor(0xAC,0x8E);
   OV7690_write_cmos_sensor(0xAD,0xA0);
   OV7690_write_cmos_sensor(0xAE,0xB0);
   OV7690_write_cmos_sensor(0xAF,0xCB);
   OV7690_write_cmos_sensor(0xB0,0xE1);
   OV7690_write_cmos_sensor(0xB1,0xF1);
   OV7690_write_cmos_sensor(0xB2,0x14);
   OV7690_write_cmos_sensor(0xB4,0x00);
   OV7690_write_cmos_sensor(0xB6,0x04); //
   OV7690_write_cmos_sensor(0x8E,0x92);
   OV7690_write_cmos_sensor(0x96,0xFF);
   OV7690_write_cmos_sensor(0x97,0x00);
   OV7690_write_cmos_sensor(0x14,0x2B);

   OV7690_write_cmos_sensor(0x89,0x2A);
   OV7690_write_cmos_sensor(0x8A,0x1D);
   OV7690_write_cmos_sensor(0x8B,0x1D);
   OV7690_write_cmos_sensor(0x81,0x3F);
   OV7690_write_cmos_sensor(0xD2,0x02);
   OV7690_write_cmos_sensor(0xD8,0x31);
   OV7690_write_cmos_sensor(0xD9,0x31);
   OV7690_write_cmos_sensor(0xB2,0x20);
   OV7690_write_cmos_sensor(0xA3,0x02);
   OV7690_write_cmos_sensor(0xA4,0x16);
   OV7690_write_cmos_sensor(0xA5,0x2A);
   OV7690_write_cmos_sensor(0xA6,0x4E);
   OV7690_write_cmos_sensor(0xA7,0x61);
   OV7690_write_cmos_sensor(0xA8,0x6F);
   OV7690_write_cmos_sensor(0xA9,0x7B);
   OV7690_write_cmos_sensor(0xAA,0x86);
   OV7690_write_cmos_sensor(0xAB,0x8E);
   OV7690_write_cmos_sensor(0xAC,0x97);
   OV7690_write_cmos_sensor(0xAD,0xA4);
   OV7690_write_cmos_sensor(0xAE,0xAF);
   OV7690_write_cmos_sensor(0xAF,0xC5);
   OV7690_write_cmos_sensor(0xB0,0xD7);
   OV7690_write_cmos_sensor(0xB1,0xE8);
   OV7690_write_cmos_sensor(0x0E,0x00);

   OV7690_write_cmos_sensor(0xB7,0x08);
   OV7690_write_cmos_sensor(0x87,0x20);
   OV7690_write_cmos_sensor(0x89,0x19);
   OV7690_write_cmos_sensor(0x8A,0x16);
   OV7690_write_cmos_sensor(0x8B,0x14);

   OV7690_write_cmos_sensor(0x81,0x3F);
   OV7690_write_cmos_sensor(0xD3,0x10);
   OV7690_write_cmos_sensor(0xD2,0x06);
   OV7690_write_cmos_sensor(0xDC,0x09);
#if 0
//advance awb
   OV7690_write_cmos_sensor(0x8C,0x5c);
   OV7690_write_cmos_sensor(0x8D,0x11);
   OV7690_write_cmos_sensor(0x8E,0x12);
   OV7690_write_cmos_sensor(0x8F,0x19);
   OV7690_write_cmos_sensor(0x90,0x50);
   OV7690_write_cmos_sensor(0x91,0x20);
   OV7690_write_cmos_sensor(0x92,0x84);
   OV7690_write_cmos_sensor(0x93,0x84);
   OV7690_write_cmos_sensor(0x94,0x0C);
   OV7690_write_cmos_sensor(0x95,0x0C);
   OV7690_write_cmos_sensor(0x96,0xFF);
   OV7690_write_cmos_sensor(0x97,0x00);
   OV7690_write_cmos_sensor(0x98,0x31);
   OV7690_write_cmos_sensor(0x99,0x2D);
   OV7690_write_cmos_sensor(0x9A,0x4B);
   OV7690_write_cmos_sensor(0x9B,0x3B);
   OV7690_write_cmos_sensor(0x9C,0xF0);
   OV7690_write_cmos_sensor(0x9D,0xF0);
   OV7690_write_cmos_sensor(0x9E,0xF0);
   OV7690_write_cmos_sensor(0x9F,0xFF); 
   OV7690_write_cmos_sensor(0xA0,0x5B);
   OV7690_write_cmos_sensor(0xA1,0x4B);
   OV7690_write_cmos_sensor(0xA2,0x10); 
#endif
   //jashe porting 7692 (e)
}

/*************************************************************************
* FUNCTION
*	OV7690Open
*
* DESCRIPTION
*	This function initialize the registers of CMOS sensor
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint32 OV7690Open(void)

{
	kal_uint16 sensor_id=0; 

    SENSORDB("OV7690Open_start new\n");


	sensor_id=((OV7690_read_cmos_sensor(0x0A)<< 8)|OV7690_read_cmos_sensor(0x0B));

	if (sensor_id != OV7690_SENSOR_ID) {
        SENSORDB("sensor_id error \n");
	    return ERROR_SENSOR_CONNECT_FAIL;
	}
	OV7690InitialSetting();
	
    SENSORDB("OV7690Open_start1111 \n");
	memset(&OV7690Sensor, 0xFF, sizeof(OV7690Sensor));
	OV7690Sensor.BypassAe = KAL_FALSE;
	OV7690Sensor.BypassAwb = KAL_FALSE;
	OV7690Sensor.Ctrl3A = OV7690_read_cmos_sensor(0x13);
	OV7690Sensor.SdeCtrl = OV7690_read_cmos_sensor(0xD2);
	OV7690Sensor.Reg15 = OV7690_read_cmos_sensor(0x15);



    SENSORDB("OV7690Open_end \n");
    
	return ERROR_NONE;
}   /* OV7690Open  */

UINT32 OV7690GetSensorID(UINT32 *sensorID) 
{
kal_uint16 sensor_id = 0;
sensor_id=((OV7690_read_cmos_sensor(0x0A)<< 8)|OV7690_read_cmos_sensor(0x0B));
	if (sensor_id != OV7690_SENSOR_ID) {
        SENSORDB("sensor_id error \n");
*sensorID = 0xFFFFFFFF;
	    return ERROR_SENSOR_CONNECT_FAIL;
	}
*sensorID = sensor_id;
		SENSORDB("OV7690Open sensor_id is %x\n", (unsigned int)*sensorID );
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*	OV7690Close
*
* DESCRIPTION
*	This function is to turn off sensor module power.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint32 OV7690Close(void)
{


	return ERROR_NONE;
}   /* OV7690Close */
/*************************************************************************
* FUNCTION
* OV7690_Preview
*
* DESCRIPTION
*	This function start the sensor preview.
*
* PARAMETERS
*	*image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 OV7690_Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
					  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
	kal_uint16 DummyPixel = 0;
    SENSORDB("[IN] OV7690_Preview \n");

	OV7690_FPS(30);
	sensor_config_data->SensorImageMirror = IMAGE_NORMAL;
	OV7690SetMirror(sensor_config_data->SensorImageMirror);
	OV7690SetClock(OV7690_PREVIEW_CLK);
	OV7690SetDummy(DummyPixel, 0);
	OV7690CalFps(); /* need cal new fps */
	OV7690_write_cmos_sensor(0x14,0x2b );//for banding 50HZ
	OV7690AeEnable(KAL_TRUE);
	OV7690AwbEnable(KAL_TRUE);


	image_window->GrabStartX= OV7690_X_START;
	image_window->GrabStartY= OV7690_Y_START;
	image_window->ExposureWindowWidth = OV7690_IMAGE_SENSOR_WIDTH;
	image_window->ExposureWindowHeight =OV7690_IMAGE_SENSOR_HEIGHT;
    SENSORDB("[OUT] OV7690_Preview \n");

	return ERROR_NONE;

}   /*  OV7690_Preview   */

/*************************************************************************
* FUNCTION
*	OV7690_Capture
*
* DESCRIPTION
*	This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*	None
*
* GLOBALS AFFECTED
*
*************************************************************************/
static kal_uint32 OV7690_Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
						  MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    SENSORDB("[IN] OV7690_Capture \n");

	image_window->GrabStartX= OV7690_X_START;
	image_window->GrabStartY= OV7690_Y_START;
	image_window->ExposureWindowWidth = OV7690_IMAGE_SENSOR_WIDTH;
	image_window->ExposureWindowHeight =OV7690_IMAGE_SENSOR_HEIGHT; 
    SENSORDB("[OUT] OV7690_Capture \n");

	return ERROR_NONE;
}   /* OV7576_Capture() */

void OV7690_NightMode(kal_bool bEnable)
{
	kal_uint16 dummy = 0;
	
    SENSORDB("[IN]OV7690_NightMode \n");
	if(bEnable == KAL_FALSE)
	    OV7690_FPS(30);
	else if(bEnable == KAL_TRUE)
		OV7690_FPS(15);
	else
		printk("Wrong mode setting \n");

	OV7690SetDummy(dummy, 0);
  	OV7690CalFps(); /* need cal new fps */
}

kal_uint32 OV7690GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{

	pSensorResolution->SensorFullWidth=OV7690_IMAGE_SENSOR_WIDTH_DRV;
	pSensorResolution->SensorFullHeight=OV7690_IMAGE_SENSOR_HEIGHT_DRV;
    pSensorResolution->SensorPreviewWidth=OV7690_IMAGE_SENSOR_WIDTH_DRV - 2;
	pSensorResolution->SensorPreviewHeight=OV7690_IMAGE_SENSOR_HEIGHT_DRV - 2;
	return ERROR_NONE;
}	/* OV7690GetResolution() */

kal_uint32 OV7690GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
					  MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{

	pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;
	pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
	pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
#if 1
    /* Workaround for the short green screen when enabling this camera. */
    SENSORDB("[OV7692] Applying the workaround for the short green screen\n");
    pSensorInfo->PreviewDelayFrame = 30;
    pSensorInfo->VideoDelayFrame = 30;
    SENSORDB("[OV7692]      PreviewDelayFram = %d\n", pSensorInfo->PreviewDelayFrame);
    SENSORDB("[OV7692]      VideoDelayFram = %d\n", pSensorInfo->VideoDelayFrame);
#endif

	
	pSensorInfo->SensorMasterClockSwitch = 0; 
      pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;   		


	//CAMERA_CONTROL_FLOW(ScenarioId,ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:		
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount=	3;
			pSensorInfo->SensorClockRisingCount= 0;
			pSensorInfo->SensorClockFallingCount= 2;
			pSensorInfo->SensorPixelClockCount= 3;
			pSensorInfo->SensorDataLatchCount= 2;
			
			pSensorInfo->SensorGrabStartX = OV7690_X_START; 
			pSensorInfo->SensorGrabStartY = OV7690_Y_START;			   
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:		
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;	
			
			pSensorInfo->SensorGrabStartX = OV7690_X_START; 
			pSensorInfo->SensorGrabStartY = OV7690_Y_START;			   
		break;
		default:
			pSensorInfo->SensorClockFreq=26;
			pSensorInfo->SensorClockDividCount= 3;
			pSensorInfo->SensorClockRisingCount=0;
			pSensorInfo->SensorClockFallingCount=2;
			pSensorInfo->SensorPixelClockCount=3;
			pSensorInfo->SensorDataLatchCount=2;
			
			pSensorInfo->SensorGrabStartX = OV7690_X_START; 
			pSensorInfo->SensorGrabStartY = OV7690_Y_START;			   
		break;
	}

	memcpy(pSensorConfigData, &OV7690SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
	
	return ERROR_NONE;
}	/* OV7690GetInfo() */


kal_uint32 OV7690Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
					  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("[IN] OV7690Control ScenarioId=%d \n",ScenarioId);

	switch (ScenarioId)
	{
		case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
		case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
			OV7690_Preview(pImageWindow, pSensorConfigData);
		break;
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
		case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
			OV7690_Capture(pImageWindow, pSensorConfigData);
		break;
		default:
			return ERROR_INVALID_SCENARIO_ID;
	}
	return TRUE;
}	/* MT9P012Control() */



static BOOL OV7690_set_param_wb(UINT16 para)
{
    kal_uint8  i = 5;
    kal_uint8  temp_reg;
    const static kal_uint8 AwbGain[5][2]=
    { /* R gain, B gain. base: 0x40 */
     {0x56,0x5c}, /* cloud */
     {0x56,0x58}, /* daylight */
     {0x40,0x67}, /* INCANDESCENCE */
     {0x60,0x58}, /* FLUORESCENT */
     {0x40,0xA0}, /* TUNGSTEN */
    };
   if (OV7690Sensor.Wb == para)
    {
      return TRUE;
    }
    OV7690Sensor.Wb = para;
    temp_reg=OV7690_read_cmos_sensor(0x13);
	switch (para)
	{
		case AWB_MODE_AUTO:
            OV7690_write_cmos_sensor(0x13,temp_reg|0x2);
			//OV7690AwbEnable(KAL_TRUE);	
			break;
		case AWB_MODE_DAYLIGHT: //sunny
			OV7690_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7690_write_cmos_sensor(0x01, 0x8A); 
			OV7690_write_cmos_sensor(0x02, 0xA7); 
			OV7690_write_cmos_sensor(0x03, 0x80);
			//i = 1;
			break;
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
			OV7690_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7690_write_cmos_sensor(0x01, 0x7A); 
			OV7690_write_cmos_sensor(0x02, 0xA7); 
			OV7690_write_cmos_sensor(0x03, 0x80);
			//i = 0;
			break;
		case AWB_MODE_INCANDESCENT: //office 
			OV7690_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7690_write_cmos_sensor(0x01, 0xB7); 
			OV7690_write_cmos_sensor(0x02, 0x6A); 
			OV7690_write_cmos_sensor(0x03, 0x80);
			//i = 2;			
			break;
		case AWB_MODE_TUNGSTEN: //home 
			OV7690_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7690_write_cmos_sensor(0x01, 0xC6); 
			OV7690_write_cmos_sensor(0x02, 0x70); 
			OV7690_write_cmos_sensor(0x03, 0x80);
			//i = 4;
			break;
		case AWB_MODE_FLUORESCENT: 
			OV7690_write_cmos_sensor(0x13,temp_reg&~0x2);  // Disable AWB 
			OV7690_write_cmos_sensor(0x01, 0x87); 
			OV7690_write_cmos_sensor(0x02, 0x8E); 
			OV7690_write_cmos_sensor(0x03, 0x80);
			//i = 3;
			break; 
		default:
			return FALSE;
	}
	//if (i < 5) {
		//OV7690AwbEnable(KAL_FALSE);
   		//OV7690_write_cmos_sensor(0x02, AwbGain[i][0]); /* AWb R gain */
   		//OV7690_write_cmos_sensor(0x01, AwbGain[i][1]); /* AWb B gain */
	//}
	return TRUE;
} /* OV7690_set_param_wb */


static BOOL OV7690_set_param_effect(UINT16 para)
{
	
	static const kal_uint8 Data[6][3]=
    {
      {0x00,0x80,0x80}, /* NORMAL */
      {0x18,0x80,0x80}, /* GRAYSCALE */
      {0x18,0x40,0xA0}, /* SEPIA */
      {0x18,0x60,0x60}, /* SEPIAGREEN */
      {0x18,0xA0,0x40}, /* SEPIABLUE */
      {0x40,0x80,0x80}, /* COLORINV */
    };
    kal_uint8 i;
	BOOL ret = TRUE;
    
    if (OV7690Sensor.Effect == para)
    {
      return TRUE;
    }
    OV7690Sensor.Effect = para;
	switch (para)
	{
		case MEFFECT_OFF:
			i = 0;		
			break;
		case MEFFECT_SEPIA:
			i = 2;	
			break;
		case MEFFECT_NEGATIVE:
			i = 5;
			break;
		case MEFFECT_SEPIAGREEN:
			i = 3;
			break;
		case MEFFECT_SEPIABLUE:
			i = 4;
			break;
		case MEFFECT_MONO://CAM_EFFECT_ENC_GRAYSCALE: 
			i = 1;
			break;
		default:
			ret = FALSE;
	}
	OV7690Sensor.SdeCtrl &= 0xA7; /* disable bit3/4/6 */
	OV7690Sensor.SdeCtrl |= Data[i][0];
	OV7690_write_cmos_sensor(0xD2, OV7690Sensor.SdeCtrl);
	OV7690_write_cmos_sensor(0xDA, Data[i][1]);
	OV7690_write_cmos_sensor(0xDB, Data[i][2]);

	return ret;

} /* OV7690_set_param_effect */

static BOOL OV7690_set_param_banding(UINT16 para)
{

	
	
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			OV7690_write_cmos_sensor(0x14, 0x2b);
			break;
		case AE_FLICKER_MODE_60HZ:
			OV7690_write_cmos_sensor(0x14, 0x2a);
			break;
		default:
			return FALSE;
	}

	return TRUE;
} /* OV7690_set_param_banding */

static BOOL OV7690_set_param_exposure(UINT16 para)
{
	static const kal_uint8 Data[9][2]=
    {
      {0x09,0x40}, /* EV -4 */
      {0x09,0x30}, /* EV -3 */
      {0x09,0x20}, /* EV -2 */
      {0x09,0x10}, /* EV -1 */
      {0x01,0x00}, /* EV 0 */
      {0x01,0x10}, /* EV +1 */
      {0x01,0x20}, /* EV +2 */
      {0x01,0x30}, /* EV +3 */
      {0x01,0x40}, /* EV +4 */
    };
    kal_uint8 i;
	
	if (OV7690Sensor.Exposure == para)
	{
	  return TRUE;
	}
	OV7690Sensor.Exposure = para;

	switch (para)
	{
		case AE_EV_COMP_00:	// Disable EV compenate
			//i = 4;
			OV7690_write_cmos_sensor(0x24, 0x78); 
			OV7690_write_cmos_sensor(0x25, 0x68); 
			OV7690_write_cmos_sensor(0x26, 0xB3);	
			break;
		case AE_EV_COMP_05:// EV compensate 0.5
			i = 3;
			break;
		case AE_EV_COMP_10:// EV compensate 1.0
			//i = 2;
			OV7690_write_cmos_sensor(0x24, 0x88); 
			OV7690_write_cmos_sensor(0x25, 0x78); 
			OV7690_write_cmos_sensor(0x26, 0xC4);
			break;
		case AE_EV_COMP_15:// EV compensate 1.5
			i = 1;
			break;
		case AE_EV_COMP_20:// EV compensate 2.0
			OV7690_write_cmos_sensor(0x24, 0x98); 
			OV7690_write_cmos_sensor(0x25, 0x88); 
			OV7690_write_cmos_sensor(0x26, 0xD5);
			//i = 0;
			break;
		case AE_EV_COMP_n05:// EV compensate -0.5
			i = 5;
			break;
		case AE_EV_COMP_n10:// EV compensate -1.0
			OV7690_write_cmos_sensor(0x24, 0x68); 
			OV7690_write_cmos_sensor(0x25, 0x58); 
			OV7690_write_cmos_sensor(0x26, 0xA2);
			//i = 6;
			break;
		case AE_EV_COMP_n15:// EV compensate -1.5
			i = 7;
			break;
		case AE_EV_COMP_n20:// EV compensate -2.0
			OV7690_write_cmos_sensor(0x24, 0x58); 
			OV7690_write_cmos_sensor(0x25, 0x48); 
			OV7690_write_cmos_sensor(0x26, 0x91);
			//i = 8;
			break;
		default:
			return FALSE;
		}
	//OV7690_write_cmos_sensor(0xDC, Data[i][0]); /* SGNSET */
   //OV7690_write_cmos_sensor(0xD3, Data[i][1]); /* YBright */

	return TRUE;
} /* OV7690_set_param_exposure */

static kal_uint32 OV7690_YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)
{
	
    SENSORDB("[IN] OV7690_YUVSensorSetting iCmd =%d, iPara =%d\n",iCmd, iPara);
	switch (iCmd) 
	{
		case FID_SCENE_MODE:
            if (iPara == SCENE_MODE_OFF)
			{

			//SENSORDB("OV7690_NightMode = FALSE \n");
			    OV7690_NightMode(FALSE); 
			}
			else if (iPara == SCENE_MODE_NIGHTSCENE)
			{
				//SENSORDB("OV7690_NightMode = TRUE \n");
			    OV7690_NightMode(TRUE); 
			}	    
		    break; 
		case FID_AWB_MODE:
			OV7690_set_param_wb(iPara);
		break;
		case FID_COLOR_EFFECT:
			OV7690_set_param_effect(iPara);
		break;
		case FID_AE_EV:	
			OV7690_set_param_exposure(iPara);
		break;
		case FID_AE_FLICKER:
			OV7690_set_param_banding(iPara);
		break;
		
		case FID_ZOOM_FACTOR:
		default:
		break;
	}
	
	return TRUE;
}   /* OV7690_YUVSensorSetting */

static kal_uint32 OV7690_YUVSetVideoMode(UINT16 u2FrameRate)
{
    kal_uint16 dummy;
    if (u2FrameRate == 30)
    {
	    OV7690_FPS(30);
    }
    else if (u2FrameRate == 15)       
    {
		OV7690_FPS(15);
    }
    else 
    {
        printk("Wrong frame rate setting \n");
    }   

	OV7690SetDummy(dummy, 0);
  	OV7690CalFps(); /* need cal new fps */
    
	printk("\n OV7690_YUVSetVideoMode:u2FrameRate=%d\n\n",u2FrameRate);
    return TRUE;
}


/*************************************************************************
* FUNCTION
*    OV7690_get_size
*
* DESCRIPTION
*    This function return the image width and height of image sensor.
*
* PARAMETERS
*    *sensor_width: address pointer of horizontal effect pixels of image sensor
*    *sensor_height: address pointer of vertical effect pixels of image sensor
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690_get_size(kal_uint16 *sensor_width, kal_uint16 *sensor_height)
{
  *sensor_width = OV7690_IMAGE_SENSOR_WIDTH_DRV; /* must be 4:3 */
  *sensor_height = OV7690_IMAGE_SENSOR_HEIGHT_DRV;
}

/*************************************************************************
* FUNCTION
*    OV7690_get_period
*
* DESCRIPTION
*    This function return the image width and height of image sensor.
*
* PARAMETERS
*    *pixel_number: address pointer of pixel numbers in one period of HSYNC
*    *line_number: address pointer of line numbers in one period of VSYNC
*
* RETURNS
*    None
*
* LOCAL AFFECTED
*
*************************************************************************/
static void OV7690_get_period(kal_uint16 *pixel_number, kal_uint16 *line_number)
{
  *pixel_number = OV7690_PERIOD_PIXEL_NUMS;
  *line_number = OV7690_PERIOD_LINE_NUMS;
}

/*************************************************************************
* FUNCTION
*    OV7690_feature_control
*
* DESCRIPTION
*    This function control sensor mode
*
* PARAMETERS
*    id: scenario id
*    image_window: image grab window
*    cfg_data: config data
*
* RETURNS
*    error code
*
* LOCAL AFFECTED
*
*************************************************************************/
kal_uint32 OV7690FeatureControl(MSDK_SENSOR_FEATURE_ENUM id, kal_uint8 *para, kal_uint32 *len)
{
	UINT32 *pFeatureData32=(UINT32 *) para;
	UINT32 *pFeatureReturnPara32=(UINT32 *) para;
	if((id!=3000)&&(id!=3004)&&(id!=3006)){
	    //CAMERA_CONTROL_FLOW(id,id);
	}

	switch (id)
	{
		case SENSOR_FEATURE_GET_RESOLUTION: /* no use */
			OV7690_get_size((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_GET_PERIOD:
			OV7690_get_period((kal_uint16 *)para, (kal_uint16 *)(para + sizeof(kal_uint16)));
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
			*(kal_uint32 *)para = OV7690Sensor.IntClk ;
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_SET_ESHUTTER:
			break;
		case SENSOR_FEATURE_SET_NIGHTMODE: 
			OV7690_NightMode((kal_bool)*(kal_uint16 *)para);
			break;
		case SENSOR_FEATURE_SET_GAIN:
		case SENSOR_FEATURE_SET_FLASHLIGHT:
		case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
			break;
		case SENSOR_FEATURE_SET_REGISTER:
			OV7690_write_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr, ((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData);
			break;
		case SENSOR_FEATURE_GET_REGISTER: /* 10 */
			((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegData = OV7690_read_cmos_sensor(((MSDK_SENSOR_REG_INFO_STRUCT *)para)->RegAddr);
			break;
		case SENSOR_FEATURE_SET_CCT_REGISTER:
		case SENSOR_FEATURE_GET_CCT_REGISTER:
		case SENSOR_FEATURE_SET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_ENG_REGISTER:
		case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
		case SENSOR_FEATURE_GET_CONFIG_PARA: /* no use */
			break;
		case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
			break;
		case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
			break;
		case SENSOR_FEATURE_GET_GROUP_COUNT:
		case SENSOR_FEATURE_GET_GROUP_INFO: /* 20 */
		case SENSOR_FEATURE_GET_ITEM_INFO:
		case SENSOR_FEATURE_SET_ITEM_INFO:
		case SENSOR_FEATURE_GET_ENG_INFO:
			break;
		case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
		/*
		* get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
		* if EEPROM does not exist in camera module.
		*/
			*(kal_uint32 *)para = LENS_DRIVER_ID_DO_NOT_CARE;
			*len = sizeof(kal_uint32);
			break;
		case SENSOR_FEATURE_SET_YUV_CMD:
	//		OV7690_YUVSensorSetting((FEATURE_ID)(UINT32 *)para, (UINT32 *)(para+1));
			
			OV7690_YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
			break;
		case SENSOR_FEATURE_SET_VIDEO_MODE:
			
			SENSORDB("[IN]SENSOR_FEATURE_SET_VIDEO_MODE \n");
			OV7690_YUVSetVideoMode(*para);
			break; 		

		case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV7690GetSensorID(pFeatureReturnPara32); 
			break;	
		default:
			break;
	}
	return ERROR_NONE;
}


SENSOR_FUNCTION_STRUCT	SensorFuncOV7690=
{
	OV7690Open,
	OV7690GetInfo,
	OV7690GetResolution,
	OV7690FeatureControl,
	OV7690Control,
	OV7690Close
};

UINT32 OV7690_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncOV7690;

	return ERROR_NONE;
}	/* SensorInit() */
