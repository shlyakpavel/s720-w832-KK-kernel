/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the 'License');
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an 'AS IS' BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _CAMERA_AE_PLINETABLE_IMX111RAW_H
#define _CAMERA_AE_PLINETABLE_IMX111RAW_H

#include "aaa_param.h"
static strEvSetting sPreviewPLineTable_60Hz[0] =
{
};

static strEvSetting sPreviewPLineTable_50Hz[0] =
{
};

static strAETable g_AE_PreviewTable =
{
    AETABLE_RPEVIEW_AUTO,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    95,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sPreviewPLineTable_60Hz,
    sPreviewPLineTable_50Hz,
    NULL,
};

static strEvSetting sVideoPLineTable_60Hz[0] =
{
};

static strEvSetting sVideoPLineTable_50Hz[0] =
{
};

static strAETable g_AE_VideoTable =
{
    AETABLE_VIDEO,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sVideoPLineTable_60Hz,
    sVideoPLineTable_50Hz,
    NULL,
};

static strEvSetting sVideoNightPLineTable_60Hz[0] =
{
};

static strEvSetting sVideoNightPLineTable_50Hz[0] =
{
};

static strAETable g_AE_VideoNightTable =
{
    AETABLE_VIDEO_NIGHT,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sVideoNightPLineTable_60Hz,
    sVideoNightPLineTable_50Hz,
    NULL,
};

static strEvSetting sCaptureZSDPLineTable_60Hz[0] =
{
};

static strEvSetting sCaptureZSDPLineTable_50Hz[0] =
{
};

static strAETable g_AE_CaptureZSDTable =
{
    AETABLE_CAPTURE_ZSD,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sCaptureZSDPLineTable_60Hz,
    sCaptureZSDPLineTable_50Hz,
    NULL,
};

static strEvSetting sCapturePLineTable_60Hz[0] =
{
};

static strEvSetting sCapturePLineTable_50Hz[0] =
{
};

static strAETable g_AE_CaptureTable =
{
    AETABLE_CAPTURE_AUTO,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sCapturePLineTable_60Hz,
    sCapturePLineTable_50Hz,
    NULL,
};

static strEvSetting sCaptureISO100PLineTable_60Hz[0] =
{
};

static strEvSetting sCaptureISO100PLineTable_50Hz[0] =
{
};

static strAETable g_AE_CaptureISO100Table =
{
    AETABLE_CAPTURE_ISO100,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_100, //ISO SPEED
    sCaptureISO100PLineTable_60Hz,
    sCaptureISO100PLineTable_50Hz,
    NULL,
};

static strEvSetting sCaptureISO200PLineTable_60Hz[0] =
{
};

static strEvSetting sCaptureISO200PLineTable_50Hz[0] =
{
};

static strAETable g_AE_CaptureISO200Table =
{
    AETABLE_CAPTURE_ISO200,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_200, //ISO SPEED
    sCaptureISO200PLineTable_60Hz,
    sCaptureISO200PLineTable_50Hz,
    NULL,
};

static strEvSetting sCaptureISO400PLineTable_60Hz[0] =
{
};

static strEvSetting sCaptureISO400PLineTable_50Hz[0] =
{
};

static strAETable g_AE_CaptureISO400Table =
{
    AETABLE_CAPTURE_ISO400,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_400, //ISO SPEED
    sCaptureISO400PLineTable_60Hz,
    sCaptureISO400PLineTable_50Hz,
    NULL,
};

static strEvSetting sAEMode1PLineTable_60Hz[0] =
{
};

static strEvSetting sAEMode1PLineTable_50Hz[0] =
{
};

static strAETable g_AE_ModeTable1 =
{
    AETABLE_MODE_INDEX1,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sAEMode1PLineTable_60Hz,
    sAEMode1PLineTable_50Hz,
    NULL,
};

static strEvSetting sAEMode2PLineTable_60Hz[0] =
{
};

static strEvSetting sAEMode2PLineTable_50Hz[0] =
{
};

static strAETable g_AE_ModeTable2 =
{
    AETABLE_MODE_INDEX2,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sAEMode2PLineTable_60Hz,
    sAEMode2PLineTable_50Hz,
    NULL,
};

static strEvSetting sAEMode3PLineTable_60Hz[0] =
{
};

static strEvSetting sAEMode3PLineTable_50Hz[0] =
{
};

static strAETable g_AE_ModeTable3 =
{
    AETABLE_MODE_INDEX3,    //eAETableID
    0,    //u4TotalIndex
    20,    //u4StrobeTrigerBV
    0,    //i4MaxBV
    0,    //i4MinBV
    LIB3A_AE_ISO_SPEED_AUTO, //ISO SPEED
    sAEMode3PLineTable_60Hz,
    sAEMode3PLineTable_50Hz,
    NULL,
};

static strAETable g_AE_VideoDynamicTable =
{
    AETABLE_VIDEO_DYNAMIC,    //eAETableID
