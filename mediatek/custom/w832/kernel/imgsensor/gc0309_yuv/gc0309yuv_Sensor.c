/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of MediaTek Inc. (C) 2005
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 *  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
 *  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
 *
 *****************************************************************************/

/*****************************************************************************
 *
 * Filename:
 * ---------
 *   gc0309yuv_Sensor.c
 *
 * Project:
 * --------
 *   MAUI
 *
 * Description:
 * ------------
 *   Image sensor driver function
 *   V1.2.3
 *
 * Author:
 * -------
 *   Leo
 *
 *=============================================================
 *             HISTORY
 * Below this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Log$
 * 2012.02.29  kill bugs
 *   
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by GCoreinc. DO NOT MODIFY!!
 *=============================================================
 ******************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "gc0309yuv_Sensor.h"
#include "gc0309yuv_Camera_Sensor_para.h"
#include "gc0309yuv_CameraCustomized.h"

//#define GC0309YUV_DEBUG
#ifdef GC0309YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

kal_uint16 GC0309_write_cmos_sensor(kal_uint8 addr, kal_uint8 para)
{
    char puSendCmd[2] = {(char)(addr & 0xFF) , (char)(para & 0xFF)};
	
	iWriteRegI2C(puSendCmd , 2, GC0309_WRITE_ID);

}
kal_uint16 GC0309_read_cmos_sensor(kal_uint8 addr) //aproved
{
	kal_uint16 get_byte=0;
    char puSendCmd = { (char)(addr & 0xFF) };
	iReadRegI2C(&puSendCmd , 1, (u8*)&get_byte, 1, GC0309_READ_ID);
	
    return get_byte;
}


/*******************************************************************************
 * // Adapter for Winmo typedef
 ********************************************************************************/
#define WINMO_USE 0

#define Sleep(ms) mdelay(ms)
#define RETAILMSG(x,...)
#define TEXT

kal_bool   GC0309_MPEG4_encode_mode = KAL_FALSE;
kal_uint16 GC0309_dummy_pixels = 0, GC0309_dummy_lines = 0;
kal_bool   GC0309_MODE_CAPTURE = KAL_FALSE;
kal_bool   GC0309_NIGHT_MODE = KAL_FALSE;

kal_uint32 GC0309_isp_master_clock;
static kal_uint32 GC0309_g_fPV_PCLK = 24;

kal_uint8 GC0309_sensor_write_I2C_address = GC0309_WRITE_ID;
kal_uint8 GC0309_sensor_read_I2C_address = GC0309_READ_ID;

UINT8 GC0309PixelClockDivider=0;

MSDK_SENSOR_CONFIG_STRUCT GC0309SensorConfigData;

#define GC0309_SET_PAGE0 	GC0309_write_cmos_sensor(0xfe, 0x00)	//approved
#define GC0309_SET_PAGE1 	GC0309_write_cmos_sensor(0xfe, 0x01)	//approved



/*************************************************************************
 * FUNCTION
 *	GC0309_SetShutter
 *
 * DESCRIPTION
 *	This function set e-shutter of GC0309 to change exposure time.
 *
 * PARAMETERS
 *   iShutter : exposured lines
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0309_Set_Shutter(kal_uint16 iShutter)
{
} /* Set_GC0309_Shutter */


/*************************************************************************
 * FUNCTION
 *	GC0309_read_Shutter
 *
 * DESCRIPTION
 *	This function read e-shutter of GC0309 .
 *
 * PARAMETERS
 *  None
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0309_Read_Shutter(void)
{
    	kal_uint8 temp_reg1, temp_reg2;
	kal_uint16 shutter;

	temp_reg1 = GC0309_read_cmos_sensor(0x04);
	temp_reg2 = GC0309_read_cmos_sensor(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* GC0309_read_shutter */


/*************************************************************************
 * FUNCTION
 *	GC0309_write_reg
 *
 * DESCRIPTION
 *	This function set the register of GC0309.
 *
 * PARAMETERS
 *	addr : the register index of GC0309
 *  para : setting parameter of the specified register of GC0309
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0309_write_reg(kal_uint32 addr, kal_uint32 para)
{
	GC0309_write_cmos_sensor(addr, para);
} /* GC0309_write_reg() */


/*************************************************************************
 * FUNCTION
 *	GC0309_read_cmos_sensor
 *
 * DESCRIPTION
 *	This function read parameter of specified register from GC0309.
 *
 * PARAMETERS
 *	addr : the register index of GC0309
 *
 * RETURNS
 *	the data that read from GC0309
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint32 GC0309_read_reg(kal_uint32 addr)
{
	return GC0309_read_cmos_sensor(addr);
} /* OV7670_read_reg() */


/*************************************************************************
* FUNCTION
*	GC0309_AWB_enable
*
* DESCRIPTION
*	This function enable or disable the awb (Auto White Balance).
*
* PARAMETERS
*	1. kal_bool : KAL_TRUE - enable awb, KAL_FALSE - disable awb.
*
* RETURNS
*	kal_bool : It means set awb right or not.
*
*************************************************************************/


static void GC0309_AWB_enable(kal_bool AWB_enable) //approved
{	 
	kal_uint16 temp_AWB_reg = 0;

	temp_AWB_reg = GC0309_read_cmos_sensor(0x22);
	
	if (AWB_enable == KAL_TRUE)
	{
		GC0309_write_cmos_sensor(0x22, (temp_AWB_reg |0x02));
	}
	else
	{
		GC0309_write_cmos_sensor(0x22, (temp_AWB_reg & (~0x02)));
	}

}




static void  GC0309_set_AE_mode(kal_bool AE_enable) //aproved
{
       kal_uint8 temp_AE_reg = 0;
	   
	temp_AE_reg = GC0309_read_cmos_sensor(0xd2);
	
   if (AE_enable == KAL_TRUE)
 	{
  	
		 GC0309_write_cmos_sensor(0xd2, (temp_AE_reg | 0x80));          
 	}
  else
  	{
   		 GC0309_write_cmos_sensor(0xd2, (temp_AE_reg & (~0x80)));
  	}

}

/*************************************************************************
 * FUNCTION
 *	GC0309_config_window
 *
 * DESCRIPTION
 *	This function config the hardware window of GC0309 for getting specified
 *  data of that window.
 *
 * PARAMETERS
 *	start_x : start column of the interested window
 *  start_y : start row of the interested window
 *  width  : column widht of the itnerested window
 *  height : row depth of the itnerested window
 *
 * RETURNS
 *	the data that read from GC0309
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0309_config_window(kal_uint16 startx, kal_uint16 starty, kal_uint16 width, kal_uint16 height)
{
} /* GC0309_config_window */


/*************************************************************************
 * FUNCTION
 *	GC0309_SetGain
 *
 * DESCRIPTION
 *	This function is to set global gain to sensor.
 *
 * PARAMETERS
 *   iGain : sensor global gain(base: 0x40)
 *
 * RETURNS
 *	the actually gain set to sensor.
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
kal_uint16 GC0309_SetGain(kal_uint16 iGain)
{
	return iGain;
}


/*************************************************************************
 * FUNCTION
 *	GC0309_NightMode
 *
 * DESCRIPTION
 *	This function night mode of GC0309.
 *
 * PARAMETERS
 *	bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
 *
 * RETURNS
 *	None
 *
 * GLOBALS AFFECTED
 *
 *************************************************************************/
void GC0309NightMode(kal_bool bEnable)	//approved
{
	if (bEnable)
	{	

		if(GC0309_MPEG4_encode_mode == KAL_TRUE)
			GC0309_write_cmos_sensor(0xec, 0x00);
		else
			GC0309_write_cmos_sensor(0xec, 0x30);
		GC0309_NIGHT_MODE = KAL_TRUE;
	}
	else 
	{
		if(GC0309_MPEG4_encode_mode == KAL_TRUE)
			GC0309_write_cmos_sensor(0xec, 0x00);
		else
			GC0309_write_cmos_sensor(0xec, 0x20);
		GC0309_NIGHT_MODE = KAL_FALSE;
	}
} /* GC0309_NightMode */

/*************************************************************************
* FUNCTION
*	GC0309_Sensor_Init
*
* DESCRIPTION
*	This function apply all of the initial setting to sensor.
*
* PARAMETERS
*	None
*
* RETURNS
*	None
*
*************************************************************************/
void GC0309_Sensor_Init(void)	//Reversed twice by KosBeg and me
{
  GC0309_write_cmos_sensor(0xFE, 0x80);
  GC0309_write_cmos_sensor(0xFE, 0x00);
  GC0309_write_cmos_sensor(0x1A, 0x16);
  GC0309_write_cmos_sensor(0xD2, 0x10);
  GC0309_write_cmos_sensor(0x22, 0x55);
  GC0309_write_cmos_sensor(0x5A, 0x56);
  GC0309_write_cmos_sensor(0x5B, 0x40);
  GC0309_write_cmos_sensor(0x5C, 0x4A);
  GC0309_write_cmos_sensor(0x22, 0x57);
  GC0309_write_cmos_sensor(0x01, 0xFA);
  GC0309_write_cmos_sensor(0x02, 0x70);
  GC0309_write_cmos_sensor(0x0F, 0x01);
  GC0309_write_cmos_sensor(0xE2, 0x00);
  GC0309_write_cmos_sensor(0xE3, 0x64);
  GC0309_write_cmos_sensor(0x03, 0x01);
  GC0309_write_cmos_sensor(0x04, 0x2C);
  GC0309_write_cmos_sensor(0x05, 0x00);
  GC0309_write_cmos_sensor(0x06, 0x00);
  GC0309_write_cmos_sensor(0x07, 0x00);
  GC0309_write_cmos_sensor(0x08, 0x00);
  GC0309_write_cmos_sensor(0x09, 0x01);
  GC0309_write_cmos_sensor(0x0A, 0xE8);
  GC0309_write_cmos_sensor(0x0B, 0x02);
  GC0309_write_cmos_sensor(0x0C, 0x88);
  GC0309_write_cmos_sensor(0x0D, 0x02);
  GC0309_write_cmos_sensor(0x0E, 0x02);
  GC0309_write_cmos_sensor(0x10, 0x22);
  GC0309_write_cmos_sensor(0x11, 0x0D);
  GC0309_write_cmos_sensor(0x12, 0x2A);
  GC0309_write_cmos_sensor(0x13, 0x00);
  GC0309_write_cmos_sensor(0x14, 0x10);
  GC0309_write_cmos_sensor(0x15, 0x0A);
  GC0309_write_cmos_sensor(0x16, 0x05);
  GC0309_write_cmos_sensor(0x17, 0x01);
  GC0309_write_cmos_sensor(0x1B, 0x03);
  GC0309_write_cmos_sensor(0x1C, 0xC1);
  GC0309_write_cmos_sensor(0x1D, 0x08);
  GC0309_write_cmos_sensor(0x1E, 0x20);
  GC0309_write_cmos_sensor(0x1F, 0x16);
  GC0309_write_cmos_sensor(0x20, 0xFF);
  GC0309_write_cmos_sensor(0x21, 0xF8);
  GC0309_write_cmos_sensor(0x24, 0xA0);
  GC0309_write_cmos_sensor(0x25, 0x0F);
  GC0309_write_cmos_sensor(0x26, 0x02);
  GC0309_write_cmos_sensor(0x2F, 0x01);
  GC0309_write_cmos_sensor(0x30, 0xF7);
  GC0309_write_cmos_sensor(0x31, 0x40);
  GC0309_write_cmos_sensor(0x32, 0x00);
  GC0309_write_cmos_sensor(0x39, 0x04);
  GC0309_write_cmos_sensor(0x3A, 0x20);
  GC0309_write_cmos_sensor(0x3B, 0x20);
  GC0309_write_cmos_sensor(0x3C, 0x02);
  GC0309_write_cmos_sensor(0x3D, 0x02);
  GC0309_write_cmos_sensor(0x3E, 0x02);
  GC0309_write_cmos_sensor(0x3F, 0x02);
  GC0309_write_cmos_sensor(0x50, 0x24);
  GC0309_write_cmos_sensor(0x53, 0x82);
  GC0309_write_cmos_sensor(0x54, 0x80);
  GC0309_write_cmos_sensor(0x55, 0x80);
  GC0309_write_cmos_sensor(0x56, 0x82);
  GC0309_write_cmos_sensor(0x8B, 0x20);
  GC0309_write_cmos_sensor(0x8C, 0x20);
  GC0309_write_cmos_sensor(0x8D, 0x20);
  GC0309_write_cmos_sensor(0x8E, 0x10);
  GC0309_write_cmos_sensor(0x8F, 0x10);
  GC0309_write_cmos_sensor(0x90, 0x10);
  GC0309_write_cmos_sensor(0x91, 0x3C);
  GC0309_write_cmos_sensor(0x92, 0x50);
  GC0309_write_cmos_sensor(0x5D, 0x12);
  GC0309_write_cmos_sensor(0x5E, 0x1A);
  GC0309_write_cmos_sensor(0x5F, 0x24);
  GC0309_write_cmos_sensor(0x60, 0x07);
  GC0309_write_cmos_sensor(0x61, 0x0E);
  GC0309_write_cmos_sensor(0x62, 0x0C);
  GC0309_write_cmos_sensor(0x64, 0x03);
  GC0309_write_cmos_sensor(0x66, 0xE8);
  GC0309_write_cmos_sensor(0x67, 0x86);
  GC0309_write_cmos_sensor(0x68, 0xA2);
  GC0309_write_cmos_sensor(0x69, 0x20);
  GC0309_write_cmos_sensor(0x6A, 0x0F);
  GC0309_write_cmos_sensor(0x6B, 0x00);
  GC0309_write_cmos_sensor(0x6C, 0x53);
  GC0309_write_cmos_sensor(0x6D, 0x83);
  GC0309_write_cmos_sensor(0x6E, 0xAC);
  GC0309_write_cmos_sensor(0x6F, 0xAC);
  GC0309_write_cmos_sensor(0x70, 0x15);
  GC0309_write_cmos_sensor(0x71, 0x33);
  GC0309_write_cmos_sensor(0x72, 0xDC);
  GC0309_write_cmos_sensor(0x73, 0x80);
  GC0309_write_cmos_sensor(0x74, 0x02);
  GC0309_write_cmos_sensor(0x75, 0x3F);
  GC0309_write_cmos_sensor(0x76, 0x02);
  GC0309_write_cmos_sensor(0x77, 0x54);
  GC0309_write_cmos_sensor(0x78, 0x88);
  GC0309_write_cmos_sensor(0x79, 0x81);
  GC0309_write_cmos_sensor(0x7A, 0x81);
  GC0309_write_cmos_sensor(0x7B, 0x22);
  GC0309_write_cmos_sensor(0x7C, 0xFF);
  GC0309_write_cmos_sensor(0x93, 0x45);
  GC0309_write_cmos_sensor(0x94, 0x00);
  GC0309_write_cmos_sensor(0x95, 0x00);
  GC0309_write_cmos_sensor(0x96, 0x00);
  GC0309_write_cmos_sensor(0x97, 0x45);
  GC0309_write_cmos_sensor(0x98, 0xF0);
  GC0309_write_cmos_sensor(0x9C, 0x00);
  GC0309_write_cmos_sensor(0x9D, 0x03);
  GC0309_write_cmos_sensor(0x9E, 0x00);
  GC0309_write_cmos_sensor(0xB1, 0x40);
  GC0309_write_cmos_sensor(0xB2, 0x40);
  GC0309_write_cmos_sensor(0xB8, 0x20);
  GC0309_write_cmos_sensor(0xBE, 0x36);
  GC0309_write_cmos_sensor(0xBF, 0x00);
  GC0309_write_cmos_sensor(0xD0, 0xC9);
  GC0309_write_cmos_sensor(0xD1, 0x10);
  GC0309_write_cmos_sensor(0xD3, 0x80);
  GC0309_write_cmos_sensor(0xD5, 0xF2);
  GC0309_write_cmos_sensor(0xD6, 0x16);
  GC0309_write_cmos_sensor(0xDB, 0x92);
  GC0309_write_cmos_sensor(0xDC, 0xA5);
  GC0309_write_cmos_sensor(0xDF, 0x23);
  GC0309_write_cmos_sensor(0xD9, 0x00);
  GC0309_write_cmos_sensor(0xDA, 0x00);
  GC0309_write_cmos_sensor(0xE0, 0x09);
  GC0309_write_cmos_sensor(0xEC, 0x20);
  GC0309_write_cmos_sensor(0xED, 0x04);
  GC0309_write_cmos_sensor(0xEE, 0xA0);
  GC0309_write_cmos_sensor(0xEF, 0x40);
  GC0309_write_cmos_sensor(0xC0, 0x00);
  GC0309_write_cmos_sensor(0xC1, 0x0B);
  GC0309_write_cmos_sensor(0xC2, 0x15);
  GC0309_write_cmos_sensor(0xC3, 0x27);
  GC0309_write_cmos_sensor(0xC4, 0x39);
  GC0309_write_cmos_sensor(0xC5, 0x49);
  GC0309_write_cmos_sensor(0xC6, 0x5A);
  GC0309_write_cmos_sensor(0xC7, 0x6A);
  GC0309_write_cmos_sensor(0xC8, 0x89);
  GC0309_write_cmos_sensor(0xC9, 0xA8);
  GC0309_write_cmos_sensor(0xCA, 0xC6);
  GC0309_write_cmos_sensor(0xCB, 0xE3);
  GC0309_write_cmos_sensor(0xCC, 0xFF);
  GC0309_write_cmos_sensor(0xF0, 0x02);
  GC0309_write_cmos_sensor(0xF1, 0x01);
  GC0309_write_cmos_sensor(0xF2, 0x00);
  GC0309_write_cmos_sensor(0xF3, 0x30);
  GC0309_write_cmos_sensor(0xF7, 0x04);
  GC0309_write_cmos_sensor(0xF8, 0x02);
  GC0309_write_cmos_sensor(0xF9, 0x9F);
  GC0309_write_cmos_sensor(0xFA, 0x78);
  GC0309_write_cmos_sensor(0xFE, 0x01);
  GC0309_write_cmos_sensor(0x00, 0xF5);
  GC0309_write_cmos_sensor(0x02, 0x1A);
  GC0309_write_cmos_sensor(0x0A, 0xA0);
  GC0309_write_cmos_sensor(0x0B, 0x60);
  GC0309_write_cmos_sensor(0x0C, 0x08);
  GC0309_write_cmos_sensor(0x0E, 0x4C);
  GC0309_write_cmos_sensor(0x0F, 0x39);
  GC0309_write_cmos_sensor(0x11, 0x3F);
  GC0309_write_cmos_sensor(0x12, 0x72);
  GC0309_write_cmos_sensor(0x13, 0x13);
  GC0309_write_cmos_sensor(0x14, 0x42);
  GC0309_write_cmos_sensor(0x15, 0x43);
  GC0309_write_cmos_sensor(0x16, 0xC2);
  GC0309_write_cmos_sensor(0x17, 0xA8);
  GC0309_write_cmos_sensor(0x18, 0x18);
  GC0309_write_cmos_sensor(0x19, 0x40);
  GC0309_write_cmos_sensor(0x1A, 0xD0);
  GC0309_write_cmos_sensor(0x1B, 0xF5);
  GC0309_write_cmos_sensor(0x70, 0x40);
  GC0309_write_cmos_sensor(0x71, 0x58);
  GC0309_write_cmos_sensor(0x72, 0x30);
  GC0309_write_cmos_sensor(0x73, 0x48);
  GC0309_write_cmos_sensor(0x74, 0x20);
  GC0309_write_cmos_sensor(0x75, 0x60);
  GC0309_write_cmos_sensor(0xFE, 0x00);
  GC0309_write_cmos_sensor(0xD2, 0x90);
}



UINT32 GC0309GetSensorID(UINT32 *sensorID)
{
    kal_uint16 sensor_id=0;
    int i;

    Sleep(20);

    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0309_read_cmos_sensor(0x00);
	            	printk("GC0309 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0309_SENSOR_ID)
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0309_SENSOR_ID)
    {
        SENSORDB("GC0309 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    *sensorID = sensor_id;

    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
	
    return ERROR_NONE;
}


/*************************************************************************
* FUNCTION
*	GC0309_GAMMA_Select
*
* DESCRIPTION
*	This function is served for FAE to select the appropriate GAMMA curve.
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
void GC0309GammaSelect(kal_uint32 GammaLvl)	//Reversed twice by KosBeg and me
{
  switch ( GammaLvl )
  {
    case 1:
      GC0309_write_cmos_sensor(0x9F, 0x0B);
      GC0309_write_cmos_sensor(0xA0, 0x16);
      GC0309_write_cmos_sensor(0xA1, 0x29);
      GC0309_write_cmos_sensor(0xA2, 0x3C);
      GC0309_write_cmos_sensor(0xA3, 0x4F);
      GC0309_write_cmos_sensor(0xA4, 0x5F);
      GC0309_write_cmos_sensor(0xA5, 0x6F);
      GC0309_write_cmos_sensor(0xA6, 0x8A);
      GC0309_write_cmos_sensor(0xA7, 0x9F);
      GC0309_write_cmos_sensor(0xA8, 0xB4);
      GC0309_write_cmos_sensor(0xA9, 0xC6);
      GC0309_write_cmos_sensor(0xAA, 0xD3);
      GC0309_write_cmos_sensor(0xAB, 0xDD);
      GC0309_write_cmos_sensor(0xAC, 0xE5);
      GC0309_write_cmos_sensor(0xAD, 0xF1);
      GC0309_write_cmos_sensor(0xAE, 0xFA);
      GC0309_write_cmos_sensor(0xAF, 0xFF);
      break;
    case 2:
      GC0309_write_cmos_sensor(0x9F, 0x0E);
      GC0309_write_cmos_sensor(0xA0, 0x1C);
      GC0309_write_cmos_sensor(0xA1, 0x34);
      GC0309_write_cmos_sensor(0xA2, 0x48);
      GC0309_write_cmos_sensor(0xA3, 0x5A);
      GC0309_write_cmos_sensor(0xA4, 0x6B);
      GC0309_write_cmos_sensor(0xA5, 0x7B);
      GC0309_write_cmos_sensor(0xA6, 0x95);
      GC0309_write_cmos_sensor(0xA7, 0xAB);
      GC0309_write_cmos_sensor(0xA8, 0xBF);
      GC0309_write_cmos_sensor(0xA9, 0xCE);
      GC0309_write_cmos_sensor(0xAA, 0xD9);
      GC0309_write_cmos_sensor(0xAB, 0xE4);
      GC0309_write_cmos_sensor(0xAC, 0xEC);
      GC0309_write_cmos_sensor(0xAD, 0xF7);
      GC0309_write_cmos_sensor(0xAE, 0xFD);
      GC0309_write_cmos_sensor(0xAF, 0xFF);
      break;
    case 3:
      GC0309_write_cmos_sensor(0x9F, 0x10);
      GC0309_write_cmos_sensor(0xA0, 0x20);
      GC0309_write_cmos_sensor(0xA1, 0x38);
      GC0309_write_cmos_sensor(0xA2, 0x4E);
      GC0309_write_cmos_sensor(0xA3, 0x63);
      GC0309_write_cmos_sensor(0xA4, 0x76);
      GC0309_write_cmos_sensor(0xA5, 0x87);
      GC0309_write_cmos_sensor(0xA6, 0xA2);
      GC0309_write_cmos_sensor(0xA7, 0xB8);
      GC0309_write_cmos_sensor(0xA8, 0xCA);
      GC0309_write_cmos_sensor(0xA9, 0xD8);
      GC0309_write_cmos_sensor(0xAA, 0xE3);
      GC0309_write_cmos_sensor(0xAB, 0xEB);
      GC0309_write_cmos_sensor(0xAC, 0xF0);
      GC0309_write_cmos_sensor(0xAD, 0xF8);
      GC0309_write_cmos_sensor(0xAE, 0xFD);
      GC0309_write_cmos_sensor(0xAF, 0xFF);
      break;
    case 4:
      GC0309_write_cmos_sensor(0x9F, 0x14);
      GC0309_write_cmos_sensor(0xA0, 0x28);
      GC0309_write_cmos_sensor(0xA1, 0x44);
      GC0309_write_cmos_sensor(0xA2, 0x5D);
      GC0309_write_cmos_sensor(0xA3, 0x72);
      GC0309_write_cmos_sensor(0xA4, 0x86);
      GC0309_write_cmos_sensor(0xA5, 0x95);
      GC0309_write_cmos_sensor(0xA6, 0xB1);
      GC0309_write_cmos_sensor(0xA7, 0xC6);
      GC0309_write_cmos_sensor(0xA8, 0xD5);
      GC0309_write_cmos_sensor(0xA9, 0xE1);
      GC0309_write_cmos_sensor(0xAA, 0xEA);
      GC0309_write_cmos_sensor(0xAB, 0xF1);
      GC0309_write_cmos_sensor(0xAC, 0xF5);
      GC0309_write_cmos_sensor(0xAD, 0xFB);
      GC0309_write_cmos_sensor(0xAE, 0xFE);
      GC0309_write_cmos_sensor(0xAF, 0xFF);
      break;
    case 5:
      GC0309_write_cmos_sensor(0x9F, 0x15);
      GC0309_write_cmos_sensor(0xA0, 0x2A);
      GC0309_write_cmos_sensor(0xA1, 0x4A);
      GC0309_write_cmos_sensor(0xA2, 0x67);
      GC0309_write_cmos_sensor(0xA3, 0x79);
      GC0309_write_cmos_sensor(0xA4, 0x8C);
      GC0309_write_cmos_sensor(0xA5, 0x9A);
      GC0309_write_cmos_sensor(0xA6, 0xB3);
      GC0309_write_cmos_sensor(0xA7, 0xC5);
      GC0309_write_cmos_sensor(0xA8, 0xD5);
      GC0309_write_cmos_sensor(0xA9, 0xDF);
      GC0309_write_cmos_sensor(0xAA, 0xE8);
      GC0309_write_cmos_sensor(0xAB, 0xEE);
      GC0309_write_cmos_sensor(0xAC, 0xF3);
      GC0309_write_cmos_sensor(0xAD, 0xFA);
      GC0309_write_cmos_sensor(0xAE, 0xFD);
      GC0309_write_cmos_sensor(0xAF, 0xFF);
      break;
    default:
      return;
  }
}

/*************************************************************************
* FUNCTION
*	GC0309_Write_More_Registers
*
* DESCRIPTION
*	This function is served for FAE to modify the necessary Init Regs. Do not modify the regs
*     in init_GC0309() directly.
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
void GC0309_Write_More_Registers(void)	//Reversed twice by KosBeg and me
{
  GC0309_write_cmos_sensor(0x8B, 0x22);
  GC0309_write_cmos_sensor(0x71, 0x43);
  GC0309_write_cmos_sensor(0x93, 0x45);
  GC0309_write_cmos_sensor(0x94, 0x00);
  GC0309_write_cmos_sensor(0x95, 0x05);
  GC0309_write_cmos_sensor(0x96, 0xE8);
  GC0309_write_cmos_sensor(0x97, 0x40);
  GC0309_write_cmos_sensor(0x98, 0xF8);
  GC0309_write_cmos_sensor(0x9C, 0x00);
  GC0309_write_cmos_sensor(0x9D, 0x00);
  GC0309_write_cmos_sensor(0x9E, 0x00);
  GC0309_write_cmos_sensor(0xD0, 0xCB);
  GC0309_write_cmos_sensor(0xD3, 0x50);
  GC0309_write_cmos_sensor(0x31, 0x60);
  GC0309_write_cmos_sensor(0x1C, 0x49);
  GC0309_write_cmos_sensor(0x1D, 0x98);
  GC0309_write_cmos_sensor(0x10, 0x26);
  GC0309_write_cmos_sensor(0x1A, 0x26);
  GC0309_write_cmos_sensor(0x57, 0x80);
  GC0309_write_cmos_sensor(0x58, 0x80);
  GC0309_write_cmos_sensor(0x59, 0x80);
  GC0309GammaSelect(2);
}


/*************************************************************************
 * FUNCTION
 *	GC0309Open
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
UINT32 GC0309Open(void)
{
    kal_uint16 sensor_id=0;
    int i;

    Sleep(20);

    do
    {
        	// check if sensor ID correct
        	for(i = 0; i < 3; i++)
		{
	            	sensor_id = GC0309_read_cmos_sensor(0x00);
	            	printk("GC0309 Sensor id = %x\n", sensor_id);
	            	if (sensor_id == GC0309_SENSOR_ID)
			{
	               	break;
	            	}
        	}
        	mdelay(50);
    }while(0);

    if(sensor_id != GC0309_SENSOR_ID)
    {
        SENSORDB("GC0309 Sensor id read failed, ID = %x\n", sensor_id);
        return ERROR_SENSOR_CONNECT_FAIL;
    }

    GC0309_MPEG4_encode_mode = KAL_FALSE;  // lanking
    RETAILMSG(1, (TEXT("Sensor Read ID OK \r\n")));
    // initail sequence write in
    GC0309_Sensor_Init();
    GC0309_Write_More_Registers();
	
    return ERROR_NONE;
} /* GC0309Open */


/*************************************************************************
 * FUNCTION
 *	GC0309Close
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
UINT32 GC0309Close(void) //aproved
{
    return ERROR_NONE;
} /* GC0309Close */


/*************************************************************************
 * FUNCTION
 * GC0309Preview
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
UINT32 GC0309Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    kal_uint32 iTemp;
    kal_uint16 iStartX = 0, iStartY = 1;

    if(sensor_config_data->SensorOperationMode == MSDK_SENSOR_OPERATION_MODE_VIDEO)		// MPEG4 Encode Mode
    {
        RETAILMSG(1, (TEXT("Camera Video preview\r\n")));
        GC0309_MPEG4_encode_mode = KAL_TRUE;
       
    }
    else
    {
        RETAILMSG(1, (TEXT("Camera preview\r\n")));
        GC0309_MPEG4_encode_mode = KAL_FALSE;
    }

    image_window->GrabStartX= IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY= IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth = IMAGE_SENSOR_PV_WIDTH;
    image_window->ExposureWindowHeight =IMAGE_SENSOR_PV_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0309SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0309Preview */


/*************************************************************************
 * FUNCTION
 *	GC0309Capture
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
UINT32 GC0309Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
        MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)

{
    GC0309_MODE_CAPTURE=KAL_TRUE;

    image_window->GrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
    image_window->GrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
    image_window->ExposureWindowWidth= IMAGE_SENSOR_FULL_WIDTH;
    image_window->ExposureWindowHeight = IMAGE_SENSOR_FULL_HEIGHT;

    // copy sensor_config_data
    memcpy(&GC0309SensorConfigData, sensor_config_data, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0309_Capture() */



UINT32 GC0309GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)  //not sure, but seems to be alright
{
    pSensorResolution->SensorFullWidth=IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight=IMAGE_SENSOR_FULL_HEIGHT;
    pSensorResolution->SensorPreviewWidth=IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight=IMAGE_SENSOR_PV_HEIGHT;
    return ERROR_NONE;
} /* GC0309GetResolution() */


UINT32 GC0309GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
        MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    pSensorInfo->SensorPreviewResolutionX=IMAGE_SENSOR_PV_WIDTH;
    pSensorInfo->SensorPreviewResolutionY=IMAGE_SENSOR_PV_HEIGHT;
    pSensorInfo->SensorFullResolutionX=IMAGE_SENSOR_FULL_WIDTH;
    pSensorInfo->SensorFullResolutionY=IMAGE_SENSOR_FULL_HEIGHT;

    pSensorInfo->SensorCameraPreviewFrameRate=30;
    pSensorInfo->SensorVideoFrameRate=30;
    pSensorInfo->SensorStillCaptureFrameRate=10;
    pSensorInfo->SensorWebCamCaptureFrameRate=15;
    pSensorInfo->SensorResetActiveHigh=FALSE;
    pSensorInfo->SensorResetDelayCount=1;
    pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_YUYV;
    pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 1;
    pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;

#if 0
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_5M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_5M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_1M_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_1M_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=FALSE;
 #endif
    pSensorInfo->CaptureDelayFrame = 1;
    pSensorInfo->PreviewDelayFrame = 0;
    pSensorInfo->VideoDelayFrame = 4;
    pSensorInfo->SensorMasterClockSwitch = 0;
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_2MA;

    switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
   // case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    //case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    default:
        pSensorInfo->SensorClockFreq=24;
        pSensorInfo->SensorClockDividCount= 3;
        pSensorInfo->SensorClockRisingCount=0;
        pSensorInfo->SensorClockFallingCount=2;
        pSensorInfo->SensorPixelClockCount=3;
        pSensorInfo->SensorDataLatchCount=2;
        pSensorInfo->SensorGrabStartX = IMAGE_SENSOR_VGA_GRAB_PIXELS;
        pSensorInfo->SensorGrabStartY = IMAGE_SENSOR_VGA_GRAB_LINES;
        break;
    }
    GC0309PixelClockDivider=pSensorInfo->SensorPixelClockCount;
    memcpy(pSensorConfigData, &GC0309SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
    return ERROR_NONE;
} /* GC0309GetInfo() */



UINT32 GC0309Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
        MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)	//now seems alright
{
	switch ( ScenarioId )
  {
    case 0:
    case 1:
    case 2:
      GC0309Preview(pImageWindow, pSensorConfigData);
      break;
    case 3:
    case 4:
      GC0309Capture(pImageWindow, pSensorConfigData);
    default:
      break;
  }
  
    /*switch (ScenarioId)
    {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
	  GC0309Preview(pImageWindow, pSensorConfigData);
      result = 1;
      break;
    //case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
   // case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
    default:
       GC0309Preview(pImageWindow, pSensorConfigData);   //  lanking    preview 
        break;
    }
*/

    return TRUE;
}	/* GC0309Control() */



BOOL GC0309_set_param_wb(UINT16 para) //approved
{
	switch (para)
	{
		case AWB_MODE_OFF:

		break;
		
		case AWB_MODE_AUTO:
			GC0309_write_cmos_sensor(0x5a,0x56); //for AWB can adjust back approved
			GC0309_write_cmos_sensor(0x5b,0x40);
			GC0309_write_cmos_sensor(0x5c,0x4a);	
			GC0309_AWB_enable(KAL_TRUE);
		break;
		
		case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy approved
			GC0309_AWB_enable(KAL_FALSE);
			GC0309_write_cmos_sensor(0x5a,0x8c); //WB_manual_gain 
			GC0309_write_cmos_sensor(0x5b,0x50);
			GC0309_write_cmos_sensor(0x5c,0x40);
		break;
		
		case AWB_MODE_DAYLIGHT: //sunny was 0x74, 0x52, 0x50. Fixed
			GC0309_AWB_enable(KAL_FALSE);
			GC0309_write_cmos_sensor(0x5a,0x58); 
			GC0309_write_cmos_sensor(0x5b,0x3c);
			GC0309_write_cmos_sensor(0x5c,0x4f);			
		break;
		
		case AWB_MODE_INCANDESCENT: //office approved
			GC0309_AWB_enable(KAL_FALSE);
			GC0309_write_cmos_sensor(0x5a,0x48);
			GC0309_write_cmos_sensor(0x5b,0x40);
			GC0309_write_cmos_sensor(0x5c,0x5c);
		break;
		
		case AWB_MODE_TUNGSTEN: //home approved
			GC0309_AWB_enable(KAL_FALSE);
			GC0309_write_cmos_sensor(0x5a,0x40);
			GC0309_write_cmos_sensor(0x5b,0x54);
			GC0309_write_cmos_sensor(0x5c,0x70);
		break;
		
		case AWB_MODE_FLUORESCENT: //approved
			GC0309_AWB_enable(KAL_FALSE);
			GC0309_write_cmos_sensor(0x5a,0x40);
			GC0309_write_cmos_sensor(0x5b,0x42);
			GC0309_write_cmos_sensor(0x5c,0x50);
		break;
		
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0309_set_param_wb */


BOOL GC0309_set_param_effect(UINT16 para)  //probably right. At least it should work propetly without effects
{
	kal_uint32  ret = KAL_TRUE;

	switch (para)
	{
		case MEFFECT_OFF:
			GC0309_write_cmos_sensor(0x23,0x00);
			GC0309_write_cmos_sensor(0x2d,0x0a); // 0x08
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x73,0x00);
			GC0309_write_cmos_sensor(0x77,0x66);
			GC0309_write_cmos_sensor(0xB1, 0);	//by Pavel
			GC0309_write_cmos_sensor(0xB2, 0); //by Pavel
			
			GC0309_write_cmos_sensor(0xb3,0x40);  // was 0x48, fixed
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0x00);
			GC0309_write_cmos_sensor(0xbb,0x00);
		break;
		
		case MEFFECT_SEPIA:
			GC0309_write_cmos_sensor(0x23,0x02);		
			GC0309_write_cmos_sensor(0x2d,0x0a);
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x73,0x00);

			GC0309_write_cmos_sensor(0xb3,0x40);
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0xd0);
			GC0309_write_cmos_sensor(0xbb,0x28);	
		break;
		
		case MEFFECT_NEGATIVE:
			GC0309_write_cmos_sensor(0x23,0x01);		
			GC0309_write_cmos_sensor(0x2d,0x0a);
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x73,0x00);

			GC0309_write_cmos_sensor(0xb3,0x40);
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0x00);
			GC0309_write_cmos_sensor(0xbb,0x00);	
		break;
		
		case MEFFECT_SEPIAGREEN:
			GC0309_write_cmos_sensor(0x23,0x02);	
			GC0309_write_cmos_sensor(0x2d,0x0a);
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x77,0x88);

			GC0309_write_cmos_sensor(0xb3,0x40);
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0xc0);
			GC0309_write_cmos_sensor(0xbb,0xc0);	
		break;
		
		case MEFFECT_SEPIABLUE:
			GC0309_write_cmos_sensor(0x23,0x02);	
			GC0309_write_cmos_sensor(0x2d,0x0a);
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x73,0x00);

			GC0309_write_cmos_sensor(0xb3,0x40);
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0x50);
			GC0309_write_cmos_sensor(0xbb,0xe0);
		break;

		case MEFFECT_MONO:
			GC0309_write_cmos_sensor(0x23,0x02);	
			GC0309_write_cmos_sensor(0x2d,0x0a);
			GC0309_write_cmos_sensor(0x20,0xff);
			GC0309_write_cmos_sensor(0xd2,0x90);
			GC0309_write_cmos_sensor(0x73,0x00);

			GC0309_write_cmos_sensor(0xb3,0x40);
			GC0309_write_cmos_sensor(0xb4,0x80);
			GC0309_write_cmos_sensor(0xba,0x00);
			GC0309_write_cmos_sensor(0xbb,0x00);	
		break;
		default:
			ret = FALSE;
	}

	return ret;

} /* GC0309_set_param_effect */


BOOL GC0309_set_param_banding(UINT16 para)	//approved
{
	switch (para)
	{
		case AE_FLICKER_MODE_50HZ:
			GC0309_write_cmos_sensor(0x01  ,0x26); 	
			GC0309_write_cmos_sensor(0x02  ,0x98); 
			GC0309_write_cmos_sensor(0x0f  ,0x03);

			GC0309_write_cmos_sensor(0xe2  ,0x00); 	//anti-flicker step [11:8]
			GC0309_write_cmos_sensor(0xe3  ,0x50);   //anti-flicker step [7:0]

			GC0309_write_cmos_sensor(0xe4  ,0x02);   //exp level 0  12.5fps
			GC0309_write_cmos_sensor(0xe5  ,0x80); 
			GC0309_write_cmos_sensor(0xe6  ,0x03);   //exp level 1  10fps  8
			GC0309_write_cmos_sensor(0xe7  ,0x20);   //was 0xc0, fixed
			GC0309_write_cmos_sensor(0xe8  ,0x04);   //exp level 2  7.69fps  7.14
			GC0309_write_cmos_sensor(0xe9  ,0x10);	 //was 0x60, fixed
			GC0309_write_cmos_sensor(0xea  ,0x06);   //exp level 3  5.00fps
			GC0309_write_cmos_sensor(0xeb  ,0x40); 
			break;

		case AE_FLICKER_MODE_60HZ:
			GC0309_write_cmos_sensor(0x01  ,0x97); 	
			GC0309_write_cmos_sensor(0x02  ,0x84); 
			GC0309_write_cmos_sensor(0x0f  ,0x03);

			GC0309_write_cmos_sensor(0xe2  ,0x00); 	//anti-flicker step [11:8]
			GC0309_write_cmos_sensor(0xe3  ,0x3e);   //anti-flicker step [7:0]
				
			GC0309_write_cmos_sensor(0xe4  ,0x02);   //exp level 0  12.00fps
			GC0309_write_cmos_sensor(0xe5  ,0x6c); 
			GC0309_write_cmos_sensor(0xe6  ,0x02);   //exp level 1  10.00fps
			GC0309_write_cmos_sensor(0xe7  ,0xe8); 
			GC0309_write_cmos_sensor(0xe8  ,0x03);   //exp level 2  7.50fps
			GC0309_write_cmos_sensor(0xe9  ,0xe0); 
			GC0309_write_cmos_sensor(0xea  ,0x05);   //exp level 3  5.00fps
			GC0309_write_cmos_sensor(0xeb  ,0xd0); 
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0309_set_param_banding */


BOOL GC0309_set_param_exposure(UINT16 para)	//approved
{

	switch (para)
	{
		case AE_EV_COMP_n13: //approved
			GC0309_write_cmos_sensor(0xb5, 0xc0);
			GC0309_write_cmos_sensor(0xd3, 0x30);
		break;
		
		case AE_EV_COMP_n10: //approved
			GC0309_write_cmos_sensor(0xb5, 0xd0);
			GC0309_write_cmos_sensor(0xd3, 0x38);
		break;
		
		case AE_EV_COMP_n07: //approved
			GC0309_write_cmos_sensor(0xb5, 0xe0);
			GC0309_write_cmos_sensor(0xd3, 0x40);
		break;
		
		case AE_EV_COMP_n03: //approved
			GC0309_write_cmos_sensor(0xb5, 0xf0);
			GC0309_write_cmos_sensor(0xd3, 0x48);
		break;				
		
		case AE_EV_COMP_00:
			GC0309_write_cmos_sensor(0xb5, 0xfd);
			GC0309_write_cmos_sensor(0xd3, 0x54);
		break;

		case AE_EV_COMP_03: //approved
			GC0309_write_cmos_sensor(0xb5, 0x10);
			GC0309_write_cmos_sensor(0xd3, 0x60);
		break;
		
		case AE_EV_COMP_07: //approved
			GC0309_write_cmos_sensor(0xb5, 0x20);
			GC0309_write_cmos_sensor(0xd3, 0x70);
		break;
		
		case AE_EV_COMP_10: //approved
			GC0309_write_cmos_sensor(0xb5, 0x30);
			GC0309_write_cmos_sensor(0xd3, 0x80);
		break;
		
		case AE_EV_COMP_13: //approved
			GC0309_write_cmos_sensor(0xb5, 0x40);
			GC0309_write_cmos_sensor(0xd3, 0x90);
		break;
		default:
		return FALSE;
	}

	return TRUE;
} /* GC0309_set_param_exposure */


UINT32 GC0309YUVSetVideoMode(UINT16 u2FrameRate)    // lanking add
{
  
        GC0309_MPEG4_encode_mode = KAL_TRUE;
     if (u2FrameRate == 30)
   	{
   	
   	    /*********video frame ************/
		
   	}
    else if (u2FrameRate == 15)       
    	{
    	
   	    /*********video frame ************/
		
    	}
    else
   	{
   	
            SENSORDB("Wrong Frame Rate"); 
			
   	}

      return TRUE;

}


UINT32 GC0309YUVSensorSetting(FEATURE_ID iCmd, UINT16 iPara)	//approved
{
    switch (iCmd) {
    case FID_AWB_MODE:
        GC0309_set_param_wb(iPara);
        break;
    case FID_COLOR_EFFECT:
        GC0309_set_param_effect(iPara);
        break;
    case FID_AE_EV:
        GC0309_set_param_exposure(iPara);
        break;
    case FID_AE_FLICKER:
        GC0309_set_param_banding(iPara);
		break;
    case FID_SCENE_MODE:
		GC0309NightMode(iPara);
        break;
    case FID_AE_SCENE_MODE: 
	  if (iPara == AE_MODE_OFF) 
	  	{	
			GC0309_set_AE_mode(KAL_FALSE);
		}
	else {
			GC0309_set_AE_mode(KAL_TRUE);
		}
	break;  
    default:
        break;
    }
    return TRUE;
} /* GC0309YUVSensorSetting */


UINT32 GC0309FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 GC0309SensorRegNumber;
    UINT32 i;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

    RETAILMSG(1, (_T("gaiyang GC0309FeatureControl FeatureId=%d\r\n"), FeatureId));

    switch (FeatureId)
    {
    case SENSOR_FEATURE_GET_RESOLUTION:
        *pFeatureReturnPara16++=IMAGE_SENSOR_FULL_WIDTH;
        *pFeatureReturnPara16=IMAGE_SENSOR_FULL_HEIGHT;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PERIOD:
        *pFeatureReturnPara16++=(VGA_PERIOD_PIXEL_NUMS)+GC0309_dummy_pixels;
        *pFeatureReturnPara16=(VGA_PERIOD_LINE_NUMS)+GC0309_dummy_lines;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        *pFeatureReturnPara32 = GC0309_g_fPV_PCLK;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_ESHUTTER:
        break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
        //GC0309NightMode((BOOL) *pFeatureData16);
        break;
    case SENSOR_FEATURE_SET_GAIN:
    case SENSOR_FEATURE_SET_FLASHLIGHT:
        break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        GC0309_isp_master_clock=*pFeatureData32;
        break;
    case SENSOR_FEATURE_SET_REGISTER:
        GC0309_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
        break;
    case SENSOR_FEATURE_GET_REGISTER:
        pSensorRegData->RegData = GC0309_read_cmos_sensor(pSensorRegData->RegAddr);
        break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
        memcpy(pSensorConfigData, &GC0309SensorConfigData, sizeof(MSDK_SENSOR_CONFIG_STRUCT));
        *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
        break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_COUNT:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
        break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
        // if EEPROM does not exist in camera module.
        *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
        *pFeatureParaLen=4;
        break;
    case SENSOR_FEATURE_SET_YUV_CMD:
        GC0309YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
        break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:    //  lanking
	 GC0309YUVSetVideoMode(*pFeatureData16);
	 break;
    case SENSOR_FEATURE_CHECK_SENSOR_ID:
	GC0309GetSensorID(pFeatureData32);
	break;
    default:
        break;
	}
return ERROR_NONE;
}	/* GC0309FeatureControl() */


SENSOR_FUNCTION_STRUCT	SensorFuncGC0309YUV=
{
	GC0309Open,
	GC0309GetInfo,
	GC0309GetResolution,
	GC0309FeatureControl,
	GC0309Control,
	GC0309Close
};


UINT32 GC0309_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)	//approved
{
	/* To Do : Check Sensor status here */
	if (pfFunc!=NULL)
		*pfFunc=&SensorFuncGC0309YUV;
	return ERROR_NONE;
} /* SensorInit() */
