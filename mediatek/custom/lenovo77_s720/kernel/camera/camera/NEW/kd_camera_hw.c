#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <mach/camera_isp.h>
#include "kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"


int kdCISModulePowerOn(int a1, int a2, int a3, int a4)
{
  int v4; // r4@1
  int v5; // r10@1
  int v6; // r5@1
  int v7; // r8@1
  signed int v8; // r7@1
  int v9; // r6@7
  char *v10; // r2@7
  char *v11; // r3@7
  int v12; // r5@7
  int v13; // r9@7
  signed int v14; // r3@13
  int v15; // ST04_4@14
  int v16; // r4@18
  signed int v17; // r10@22
  int v18; // r10@34
  int v19; // r6@34
  int v20; // r8@34
  int v21; // r10@37
  signed int v22; // r6@47
  signed int v23; // r5@55
  signed int v24; // r5@62
  signed int v25; // r4@67
  signed int result; // r0@69
  int v27; // r6@71
  int v28; // r5@71
  int v29; // r7@74
  int v30; // r4@95
  signed int v31; // r10@99
  int v32; // r8@112
  int v33; // r10@112
  signed int v34; // r8@125
  signed int v35; // r5@133
  signed int v36; // r5@140
  signed int v37; // r4@145
  signed int v38; // r4@154
  int v39; // r10@159
  int v40; // r4@159
  int v41; // r8@159
  int v42; // r10@162
  signed int v43; // r4@172
  signed int v44; // r4@180
  int v45; // r4@182
  signed int v46; // r5@187
  signed int v47; // r4@192
  int v48; // [sp+8h] [bp-B4h]@18
  int v49; // [sp+8h] [bp-B4h]@95
  int v50; // [sp+Ch] [bp-B0h]@11
  int v51; // [sp+Ch] [bp-B0h]@112
  int v52; // [sp+Ch] [bp-B0h]@115
  int v53; // [sp+10h] [bp-ACh]@1
  int v54; // [sp+18h] [bp-A4h]@1
  int v55; // [sp+20h] [bp-9Ch]@1
  int v56; // [sp+28h] [bp-94h]@1
  int v57; // [sp+30h] [bp-8Ch]@1
  int v58; // [sp+38h] [bp-84h]@1
  int v59; // [sp+40h] [bp-7Ch]@1
  int v60; // [sp+48h] [bp-74h]@1
  int v61; // [sp+50h] [bp-6Ch]@1
  int v62; // [sp+58h] [bp-64h]@1
  int v63; // [sp+60h] [bp-5Ch]@1
  int v64; // [sp+6Ch] [bp-50h]@1
  char v65[44]; // [sp+90h] [bp-2Ch]@7

  v4 = a1;
  v5 = a2;
  v6 = a3;
  v7 = a4;
  memset(&v53, 0, 128);
  v8 = 1;
  v54 = 1;
  v53 = 221;
  v61 = 230;
  v63 = 220;
  v55 = 228;
  v56 = 1;
  v57 = 230;
  v58 = 1;
  v59 = 220;
  v60 = 1;
  v62 = 1;
  v64 = 1;
  if ( v4 == 1 )
    goto LABEL_199;
  if ( v4 == 2 )
    goto LABEL_6;
  if ( v4 != 3 )
LABEL_199:
    v8 = 0;
  else
    v8 = 2;
LABEL_6:
  if ( v6 )
  {
    printk("Sensor power on  SensorIdx = %d\r\n", v4);
    v9 = 32 * v8;
    v10 = &v65[32 * v8];
    v11 = &v65[32 * v8];
    v12 = *((DWORD *)v10 - 28);
    v13 = *((DWORD *)v11 - 27);
    if ( mt_set_gpio_mode(*((DWORD *)v10 - 28), *((DWORD *)v11 - 27)) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
    if ( mt_set_gpio_dir(v12, 1) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
    v50 = *(DWORD *)&v65[v9 - 100];
    if ( mt_set_gpio_out(v12, *(DWORD *)&v65[v9 - 100]) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
    v14 = 9;
    do
    {
      v15 = v14 - 1;
      mdelay(107374);
      v14 = v15;
    }
    while ( v15 != -1 );
    if ( v4 == 1 )
    {
      printk("Sensor ov8825  \r\n");
      if ( !hwPowerOn(8, 1800, v7) )
        goto LABEL_89;
      mdelay(107374);
      if ( hwPowerOn(19, 2800, v7) )
      {
        v38 = 3;
        do
        {
          --v38;
          mdelay(107374);
        }
        while ( v38 != -1 );
        if ( !hwPowerOn(7, 1500, v7) )
          goto LABEL_89;
        if ( hwPowerOn(9, 2800, v7) )
        {
          v39 = 1 - v8;
          mdelay(214748);
          mt_isp_mclk_ctrl(0);
          v40 = 32 * (1 - v8);
          v41 = *(DWORD *)&v65[32 * (1 - v8) - 128];
          if ( v41 != 255 )
          {
            if ( mt_set_gpio_mode(v41, *(DWORD *)&v65[32 * v39 - 124]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
            v42 = *(DWORD *)&v65[32 * v39 - 112];
            if ( mt_set_gpio_mode(v42, *(DWORD *)&v65[v40 - 108]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_dir(v41, 1) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_dir(v42, 1) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_out(v41, *(DWORD *)&v65[v40 - 116]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_out(v42, *(DWORD *)&v65[v40 - 100]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
          }
          v43 = 9;
          do
          {
            --v43;
            mdelay(107374);
          }
          while ( v43 != -1 );
          if ( mt_set_gpio_mode(v12, v13) )
            printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
          if ( mt_set_gpio_dir(v12, 1) )
            printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
          if ( mt_set_gpio_out(v12, *(DWORD *)&v65[v9 - 104]) )
            printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
          v44 = 2;
          do
          {
            --v44;
            mdelay(107374);
          }
          while ( v44 != -1 );
          v45 = *(DWORD *)&v65[v9 - 128];
          if ( v45 != 255 )
          {
            if ( mt_set_gpio_mode(*(DWORD *)&v65[v9 - 128], *(DWORD *)&v65[32 * v8 - 124]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_dir(v45, 1) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
            v46 = 2;
            do
            {
              --v46;
              mdelay(107374);
            }
            while ( v46 != -1 );
            if ( mt_set_gpio_out(v45, *(DWORD *)&v65[32 * v8 - 120]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
            mdelay(107374);
          }
          v47 = 24;
          mt_isp_mclk_ctrl(1);
          do
          {
            --v47;
            mdelay(107374);
          }
          while ( v47 != -1 );
          return 0;
        }
      }
    }
    else
    {
      if ( !v5 || strcmp(v5, "s5k8aayyuv") )
      {
        printk("Sensor ov9740 power on \r\n");
        mt_isp_mclk_ctrl(0);
        v16 = *(DWORD *)&v65[v9 - 128];
        v48 = *(DWORD *)&v65[32 * v8 - 124];
        if ( mt_set_gpio_mode(*(DWORD *)&v65[v9 - 128], *(DWORD *)&v65[32 * v8 - 124]) )
          printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
        if ( mt_set_gpio_dir(v16, 1) )
          printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
        v17 = 2;
        do
        {
          --v17;
          mdelay(107374);
        }
        while ( v17 != -1 );
        if ( mt_set_gpio_out(v16, *(DWORD *)&v65[v9 - 116]) )
          printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
        mdelay(107374);
        if ( mt_set_gpio_mode(v12, v13) )
          printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
        if ( mt_set_gpio_dir(v12, 1) )
          printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
        if ( mt_set_gpio_out(v12, v50) )
          printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
        if ( hwPowerOn(8, 1800, v7) )
        {
          if ( hwPowerOn(9, 2800, v7) )
          {
            v18 = 1 - v8;
            mdelay(107374);
            mt_isp_mclk_ctrl(1);
            v19 = 32 * (1 - v8);
            v20 = *(DWORD *)&v65[32 * (1 - v8) - 128];
            if ( v20 != 255 )
            {
              if ( mt_set_gpio_mode(v20, *(DWORD *)&v65[32 * v18 - 124]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
              v21 = *(DWORD *)&v65[32 * v18 - 112];
              if ( mt_set_gpio_mode(v21, *(DWORD *)&v65[v19 - 108]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v20, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v21, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_out(v20, *(DWORD *)&v65[v19 - 116]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_out(v21, *(DWORD *)&v65[v19 - 100]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
            }
            v22 = 9;
            do
            {
              --v22;
              mdelay(107374);
            }
            while ( v22 != -1 );
            if ( mt_set_gpio_mode(v12, v13) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_dir(v12, 1) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_out(v12, v50) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
            v23 = 9;
            do
            {
              --v23;
              mdelay(107374);
            }
            while ( v23 != -1 );
            if ( v16 != 255 )
            {
              if ( mt_set_gpio_mode(v16, v48) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v16, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
              v24 = 2;
              do
              {
                --v24;
                mdelay(107374);
              }
              while ( v24 != -1 );
              if ( mt_set_gpio_out(v16, *(DWORD *)&v65[32 * v8 - 120]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
              mdelay(107374);
            }
            v25 = 9;
            do
            {
              --v25;
              mdelay(107374);
            }
            while ( v25 != -1 );
            return 0;
          }
          goto LABEL_151;
        }
LABEL_89:
        printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] Fail to enable digital power\n", "kdCISModulePowerOn");
        return -5;
      }
      printk("Sensor S5K8AAY power on \r\n");
      mt_isp_mclk_ctrl(0);
      v30 = *(DWORD *)&v65[v9 - 128];
      v49 = *(DWORD *)&v65[32 * v8 - 124];
      if ( mt_set_gpio_mode(*(DWORD *)&v65[v9 - 128], *(DWORD *)&v65[32 * v8 - 124]) )
        printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
      if ( mt_set_gpio_dir(v30, 1) )
        printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
      v31 = 2;
      do
      {
        --v31;
        mdelay(107374);
      }
      while ( v31 != -1 );
      if ( mt_set_gpio_out(v30, *(DWORD *)&v65[v9 - 116]) )
        printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
      mdelay(107374);
      if ( mt_set_gpio_mode(v12, v13) )
        printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
      if ( mt_set_gpio_dir(v12, 1) )
        printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
      if ( mt_set_gpio_out(v12, v50) )
        printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
      if ( hwPowerOn(9, 2800, v7) )
      {
        mdelay(107374);
        if ( hwPowerOn(1, 1200, v7) )
        {
          mdelay(107374);
          if ( hwPowerOn(8, 1800, v7) )
          {
            v51 = 1 - v8;
            mdelay(107374);
            v32 = 32 * (1 - v8);
            mt_isp_mclk_ctrl(1);
            v33 = *(DWORD *)&v65[v32 - 128];
            if ( v33 != 255 )
            {
              if ( mt_set_gpio_mode(*(DWORD *)&v65[v32 - 128], *(DWORD *)&v65[32 * v51 - 124]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
              v52 = *(DWORD *)&v65[32 * v51 - 112];
              if ( mt_set_gpio_mode(v52, *(DWORD *)&v65[v32 - 108]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v33, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v52, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_out(v33, *(DWORD *)&v65[v32 - 116]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_out(v52, *(DWORD *)&v65[v32 - 100]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
            }
            v34 = 9;
            do
            {
              --v34;
              mdelay(107374);
            }
            while ( v34 != -1 );
            if ( mt_set_gpio_mode(v12, v13) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_dir(v12, 1) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
            if ( mt_set_gpio_out(v12, *(DWORD *)&v65[v9 - 104]) )
              printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
            v35 = 9;
            do
            {
              --v35;
              mdelay(107374);
            }
            while ( v35 != -1 );
            if ( v30 != 255 )
            {
              if ( mt_set_gpio_mode(v30, v49) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
              if ( mt_set_gpio_dir(v30, 1) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
              v36 = 2;
              do
              {
                --v36;
                mdelay(107374);
              }
              while ( v36 != -1 );
              if ( mt_set_gpio_out(v30, *(DWORD *)&v65[32 * v8 - 120]) )
                printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
              mdelay(107374);
            }
            v37 = 9;
            do
            {
              --v37;
              mdelay(107374);
            }
            while ( v37 != -1 );
            return 0;
          }
        }
        goto LABEL_89;
      }
    }
LABEL_151:
    printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] Fail to enable analog power\n", "kdCISModulePowerOn");
    return -5;
  }
  printk("[OFF]sensorIdx:%d \n", v4);
  v27 = 32 * v8;
  v28 = *(DWORD *)&v65[32 * v8 - 128];
  if ( v28 != 255 )
  {
    if ( mt_set_gpio_mode(v28, *(DWORD *)&v65[32 * v8 - 124]) )
      printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio mode failed!! \n", "kdCISModulePowerOn");
    v29 = *(DWORD *)&v65[32 * v8 - 112];
    if ( mt_set_gpio_mode(v29, *(DWORD *)&v65[v27 - 108]) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio mode failed!! \n", "kdCISModulePowerOn");
    if ( mt_set_gpio_dir(v28, 1) )
      printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio dir failed!! \n", "kdCISModulePowerOn");
    if ( mt_set_gpio_dir(v29, 1) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio dir failed!! \n", "kdCISModulePowerOn");
    if ( mt_set_gpio_out(v28, *(DWORD *)&v65[v27 - 116]) )
      printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] set gpio failed!! \n", "kdCISModulePowerOn");
    if ( mt_set_gpio_out(v29, *(DWORD *)&v65[v27 - 100]) )
      printk("<6>[kd_camera_hw]%s: [CAMERA LENS] set gpio failed!! \n", "kdCISModulePowerOn");
  }
  mt_isp_mclk_ctrl(0);
  if ( v4 != 1 )
  {
    printk("Sensor s5k8aa power off \r\n");
    mdelay(107374);
    if ( v5 && !strcmp(v5, "s5k8aayyuv") )
    {
      if ( !hwPowerDown(8, v7) || !hwPowerDown(1, v7) )
        goto LABEL_89;
    }
    else if ( !hwPowerDown(8, v7) )
    {
      goto LABEL_89;
    }
    if ( !hwPowerDown(9, v7) )
      goto LABEL_89;
    return 0;
  }
  printk("Sensor ov8825 power off \r\n");
  if ( !hwPowerDown(9, v7) )
    goto LABEL_151;
  if ( hwPowerDown(7, v7) )
  {
    if ( hwPowerDown(19, v7) )
    {
      if ( !hwPowerDown(8, v7) )
        goto LABEL_89;
      return 0;
    }
    printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] Fail to OFF analog power\n", "kdCISModulePowerOn");
    result = -5;
  }
  else
  {
    printk("<6>[kd_camera_hw]%s: [CAMERA SENSOR] Fail to OFF digital power\n", "kdCISModulePowerOn");
    result = -5;
  }
  return result;
}

EXPORT_SYMBOL(kdCISModulePowerOn);


