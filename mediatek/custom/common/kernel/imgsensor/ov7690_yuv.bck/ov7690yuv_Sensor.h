/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   CMOS sensor header file
 *
 ****************************************************************************/
#ifndef __SENSOR_H
#define __SENSOR_H

#include "image_sensor.h"//get IMAGE_SENSOR_DRVNAME
/* SENSOR PREVIEW/CAPTURE ISP CLOCK */
#define OV7690_PREVIEW_CLK                   13000000
#define OV7690_VIDEO_LOW_CLK                 6500000
#define OV7690_CAPTURE_CLK                   13000000

/* SENSOR SIZE */
#define OV7690_IMAGE_SENSOR_WIDTH_DRV        640
#define OV7690_IMAGE_SENSOR_HEIGHT_DRV       480

/* SENSOR HORIZONTAL/VERTICAL ACTIVE REGION */
#define OV7690_IMAGE_SENSOR_HSTART           105
#define OV7690_IMAGE_SENSOR_VSTART           12
#define OV7690_IMAGE_SENSOR_HACTIVE          OV7690_IMAGE_SENSOR_WIDTH_DRV
#define OV7690_IMAGE_SENSOR_VACTIVE          OV7690_IMAGE_SENSOR_HEIGHT_DRV

/* SENSOR HORIZONTAL/VERTICAL BLANKING IN ONE PERIOD */
#define OV7690_IMAGE_SENSOR_HBLANKING        206
#define OV7690_IMAGE_SENSOR_VBLANKING        32

/* SENSOR PIXEL/LINE NUMBERS IN ONE PERIOD */
#define OV7690_PERIOD_PIXEL_NUMS             (OV7690_IMAGE_SENSOR_HACTIVE + OV7690_IMAGE_SENSOR_HBLANKING) /* 846 */
#define OV7690_PERIOD_LINE_NUMS              (OV7690_IMAGE_SENSOR_VACTIVE + OV7690_IMAGE_SENSOR_VBLANKING) /* 512 */

/* SENSOR START/END POSITION */
#define OV7690_X_START                       0
#define OV7690_Y_START                       1
#define OV7690_IMAGE_SENSOR_WIDTH            OV7690_IMAGE_SENSOR_WIDTH_DRV
#define OV7690_IMAGE_SENSOR_HEIGHT           (OV7690_IMAGE_SENSOR_HEIGHT_DRV - 2) /* -2 for frame ready done */

/* 50HZ/60HZ */
#define OV7690_NUM_50HZ                      50
#define OV7690_NUM_60HZ                      60

/* SENSOR LIMITATION */
#define OV7690_SHUTTER_MAX_MARGIN            10
#define OV7690_MAX_LINELENGTH                0x7FF
#define OV7690_MAX_FRAMEHEIGHT               0xFFF

/* SENSOR READ/WRITE ID */
#define OV7690_WRITE_ID                      0x42
#define OV7690_READ_ID                       0x43

/* FRAME RATE UNIT */
#define OV7690_FPS(x)                        ((kal_uint32)(10 * (x)))

//export functions
UINT32 OV7690Open(void);
UINT32 OV7690GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution);
UINT32 OV7690GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_INFO_STRUCT *pSensorInfo, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV7690Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow, MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData);
UINT32 OV7690FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId, UINT8 *pFeaturePara,UINT32 *pFeatureParaLen);
UINT32 OV7690Close(void);




#endif /* __SENSOR_H */ 
