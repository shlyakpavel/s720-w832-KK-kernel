/*****************************************************************************
 *
 * Filename:
 * ---------
 *   calibration.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Header file of calibration driver
 *
 *
 * Author:
 * -------
 *   YH Shi (MTK70694)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision: 1.1 $
 * $Modtime:$
 * $Log: camera_calibration_eeprom.h,v $
 *
 * 05 13 2013 hanli.huang
 * [ALPS00642283] IMX111 Check In
 * .
 * Revision 1.1  2012/02/27 01:16:36  birdlibo
 * no message
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef __CAMERA_CALIBRATION_EEPROM_H
#define __CAMERA_CALIBRATION_EEPROM_H

UINT32 LensGetCalData(UINT32* pGetSensorCalData);

#define CAM_CAL_MAX_LSC_SIZE 0x840 //(2112) //6589 Ver

#define MAX_SENSOR_CAL_SIZE             (2048) //Byte
/****************************************************************/
//for CAM_CAL_SINGLE_LSC_STRUCT SingleLsc;
/****************************************************************/
#define CAM_CAL_SINGLE_LSC_SIZE 0xdd //(221)
#define MAX_MTK_SHADING_SLIM_TABLE_SIZE (2048)//(16*16*2*4)
#define MAX_SENSOR_SHADING_TALE_SIZE MAX_SENSOR_CAL_SIZE//(2048)//(16*16*2*4)
#define MAX_GAIN_TABLE_SIZE 442
#define EEPROM_LSC_DATA_SIZE 221

typedef struct
{
	MUINT8 xblock_num;
	MUINT8 yblock_num;
	MUINT8 bitorder;
	MUINT16 imgwidth;
	MUINT16 imgheight;
	MUINT8 Attenutation_Index;
	MUINT8 bitdepth;
	MUINT8 OB_B;
	MUINT8 OB_Gb;
	MUINT8 OB_Gr;
	MUINT8 OB_R;
	unsigned char RChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
	unsigned char GrChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
	unsigned char GbChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
	unsigned char BChannel_LSC_DATA[EEPROM_LSC_DATA_SIZE];
   
}CAM_CAL_EEPROM_DATA;



////

#define FLOAT_VERSION
//#define READ_GAIN_TABLE




#define CAL_ASSERT_OPT    0
/*******************************************************************************
 *
 ********************************************************************************/
#if (CAL_ASSERT_OPT == 1)
#define CAL_ASSERT(x, str); \
    if (x) {printf("[Assert %s, %d]: %s", __FILE__, __LINE__, str); LOGE("[Assert %s, %d]: %s", __FILE__, __LINE__, str); while(1);} \
    else {}
#else
#define CAL_ASSERT(x, str);
#endif


/****************************************
 *	Trivial define, not used in embedded sys.
 ****************************************/
#define LSC_Debug_Table_Output 1
#define Shading_Table_Size 4096

#ifdef FLOAT_VERSION
	#define AVG_PIXEL_SIZE (33)  //  (32) //(16)
#else
	#define AVG_PIXEL_SIZE (16)
#endif
#define MAX_PIXEL_GAIN (4)

/****************************************/
/*	LSC_CalHwTbl.cpp                                    */
/****************************************/
/****************************************/
/*	MACROS                                                  */
/****************************************/
#define MAX2(a,b) (a>b?a:b)
#define MIN2(a,b) (a>b?b:a)

#define GRID_MAX 33 // if > 17 must modify vLSC_Calibration_INIT() lsc_hw_tbl_cal()...
#define GN_BIT 13
#define COEF_BASE_BIT 15 /* it dangerous when (COEF_BASE_BIT+3)+(COEF_DIV_BIT) close to 32 bit */
#define COEF_DIV_BIT 10
#define COEF_DIV6_VAL ((1<<COEF_DIV_BIT)/6.0+0.5)
#define COEF_DIV3_VAL ((1<<COEF_DIV_BIT)/3.0+0.5)
#define COEF_FXX_BIT (COEF_BASE_BIT-GN_BIT)
#define COEF_BIT0 9
#define COEF_BIT1 8
#define COEF_BIT2 7
#define MIN(x, y)     ((x) <= (y)) ? (x) : (y)
#define MAX(x, y)     ((x) >= (y)) ? (x) : (y)
#define CLIP(v, x, y)  MAX(MIN((v), (y)), (x))

/****************************************
 *	isp_lsc_core.h
 ****************************************/
#define RATIO_POLY_BIT 8
#define SQRT_NORMALIZE_BIT 12
/* new added */
typedef struct {
	int grid_num; // grid number
	int di;	// d[i]=x[i]-x[i-1]
	int dn;	// d[n]=x[n]-x[n-1], n=GRID_N or GRID_M
	int di2;	// d[i]*d[i]
	int dn2;	// d[n]*d[n]
	float dir;	// 1/d[i]
	float dnr;	// 1/d[n]
	float di2r; // 1/(d[i]*d[i])
	float ddir;	// 1/(x[i+1]-x[i-1])
	float ddnr;	// 1/(x[n+1]-x[n-1]), n=GRID_N-1 or GRID_M-1
} DIM_INFO_T;
typedef struct {
	int dx2;
	int dy2;
	int f00;
	int f01;
	int f10;
	int f11;
	float f00x2;
	float f01x2;
	float f10x2;
	float f11x2;
	float f00y2;
	float f01y2;
	float f10y2;
	float f11y2;
} BLK_INFO_T;

 typedef struct
{
        float SL2_CENTR_X;
        float SL2_CENTR_Y;
        float SL2_RR_0;
        float SL2_RR_1;
        float SL2_RR_2;
		unsigned int regsetting[4];
} SL2_PARAM_T, *P_SL2_PARAM_T;

typedef struct {
	unsigned int reg_mn;
	unsigned int reg_info0;
	unsigned int reg_info1;
	unsigned int *src_tbl_addr;
	unsigned int *dst_tbl_addr;
	float *src_tbl_addr_float;
	SL2_PARAM_T sl2_setting;
} TBL_INFO_T;
typedef struct {
	TBL_INFO_T tbl_info;
	unsigned short *raw_img_addr;
} LSC_CALI_INFO_T;
typedef struct
{
	float coef_a;
	float coef_b;
	float coef_c;
	float coef_d;
	float coef_e;
	float coef_f;
	int ratio_poly_flag;
}RATIO_POLY_T;
typedef struct
{
	int raw_wd;
	int raw_ht;
	int plane_wd;
	int plane_ht;
	int bayer_order;
	int crop_ini_x;
	int crop_end_x;
	int crop_ini_y;
	int crop_end_y;
	int block_wd;
	int block_ht;
	int block_wd_last;
	int block_ht_last;
	int avg_pixel_size;
	int avg_pixel_size_bit;
	int x_grid_num;
	int y_grid_num;
	int pxl_gain_max;
	RATIO_POLY_T poly_coef;
}LSC_PARAM_T;
typedef struct
{
#ifdef FLOAT_VERSION
	float max_val[4];
#else
	int max_val[4];
#endif
	int x_max_pos[4];
	int y_max_pos[4];
}LSC_RESULT_T;
enum
{
	BB_PLANE = 0,
	GB_PLANE,
	GR_PLANE,
	RR_PLANE,
};
typedef struct
{
       int i4GridXNUM;
       int i4GridYNUM;
       int i4XINIBorder;
       int i4XENDBorder;
       int i4YINIBorder;
       int i4YENDBorder;
       unsigned int u4ImgWidth;
       unsigned int u4ImgHeight;
       unsigned short u2BayerStart;
	RATIO_POLY_T poly_coef;
}LSC_CAL_INI_PARAM_T;


/****************************************
 *	 SVDtool.cpp
 ****************************************/
#define PC_SIM	(0)
#define MEDIA_PATH ".\\data"
#if PC_SIM
#define Capture_BPC_Table_NAME ".\\data\\Capture_BPC_Table_NAME.txt"
#define Preview_BPC_Table_NAME ".\\data\\Preview_BPC_Table_NAME.txt"

#define Preview_SVD_MATRIX_NAME ".\\data\\Preview_SVD_Matrix.h"
#define Capture_SVD_MATRIX_NAME ".\\data\\Capture_SVD_Matrix.h"
#else
#define Capture_BPC_Table_NAME "//data//Capture_BPC_Table_NAME.txt"
#define Preview_BPC_Table_NAME "//data//Preview_BPC_Table_NAME.txt"

#define Preview_SVD_MATRIX_NAME "//data//Preview_SVD_Matrix.h"
#define Capture_SVD_MATRIX_NAME "//data//Capture_SVD_Matrix.h"
#endif
#define EIGEN_SEQUENCE_MODIFICATION 1

#define NR_END 1
//#define FREE_ARG char*
static float maxarg1,maxarg2;
#define FMAX(a,b) (maxarg1=(a),maxarg2=(b),(maxarg1) > (maxarg2) ?\
        (maxarg1) : (maxarg2))

static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
        (iminarg1) : (iminarg2))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

static float sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)

typedef struct{
	int min_val;
	unsigned char len_frac;
}SVD_UV_INFO_T;


/****************************************
 *	badpixel calibration
 ****************************************/
#define  BPC_Debug_Table_Output 0
#define  CAL_Table_Size 24576 //5M sensor BPC ratio 0.1% = 5000 pixel == 20K bytes

typedef struct
{
	unsigned int u4CalibrateMode;
       unsigned int u4ImgWidth;
       unsigned int u4ImgHeight;
	float fBrightPixelLevel;
	float fDarkPixelLevel;
	unsigned char *raw_img_addr;
	unsigned char *mask_buffer;
}BPC_CAL_INI_PARAM_T;

typedef struct
{
	int **valid_cnt;
	int **MAX_cnt;
	long **ACC;
}Block_Info;



#endif /* __CAMERA_CALIBRATION_H */

