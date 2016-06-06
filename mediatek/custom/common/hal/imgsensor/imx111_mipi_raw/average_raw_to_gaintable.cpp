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

//#define eeprom_debug 

#define EEPROM_SHOW_LOG 1

#ifdef EEPROM_SHOW_LOG
#define CAMEEPROM_LOG(fmt, arg...)    XLOGE(fmt, ##arg)
#else
#define CAMEEPROM_LOG(fmt, arg...)    void(0)
#endif


LSC_CAL_INI_PARAM_T g_rLSCCaliINIParam;
LSC_CALI_INFO_T g_lsc_cali_info;
LSC_PARAM_T g_lsc_param;
float *g_src_tbl_float;
int g_col_idx[GRID_MAX];
int g_row_idx[GRID_MAX];
LSC_RESULT_T g_lsc_result;


MINT32 ShadingATNTable[21][6]={
    {     0,	         0,	         0,	         0,       0,       1000000,},    // 0  100%	
    { 35556,	   -195556,	    131111,	    -21111,       0,       1000000,},	 // 1  95%
    {-640000,	   1173333,	   -760000,	    126667,       0,       1000000,},	 // 2  90%
    {-35556,	    -17778,	   -131111,	     34444,       0,       1000000,},    // 3  85%
    {-142222,	    355556,	   -524444,	    111111,       0,       1000000,},	 // 4  80%
    {746667,	  -1546667,	    833333,	   -283333,       0,       1000000,},	 // 5  75%
    {640000,	  -1173333,	    440000,	   -206667,       0,       1000000,},	 // 6  70%
    {533333,	   -800000,	     46667,	   -130000,       0,       1000000,},	 // 7  65%
    {284444,	   -711111,	    408889,	   -382222,       0,       1000000,},	 // 8  60%
    {888889,	  -1902222,	   1037778,	   -474444,       0,       1000000,},	 // 9  55%
    {-497778,	   1031111,	   -875556,	   -157778,       0,       1000000,},	 // 10 50%
    {675556,	  -2008889,	   1531111,	   -747778,       0,       1000000,},	 // 11 45%
    {2133333,	  -4906667,	   3386667,	  -1213333,       0,       1000000,},	 // 12 40%
    {177778,	   -977778,	    975556,	   -825556,       0,       1000000,},	 // 13 35%
    { 71111,	   -604444,	    582222,	   -748889,       0,       1000000,},	 // 14 30%
    {5511111,	 -11751111,	   7522222,	  -2032222,       0,       1000000,},	 // 15 25%
    {3555556,	  -7822222,	   5111111,	  -1644444,       0,       1000000,},	 // 16 20%
    {320000,	  -1333333,	   1180000,	  -1016667,       0,       1000000,},	 // 17 15%
    {-1066667,	   1600000,	   -733333,	   -700000,       0,       1000000,},	 // 18 10%
    {391111,	  -1297778,	   1122222,	  -1165556,       0,       1000000,},	 // 19 5%
    {-924444,	   1884444,	  -1168889,	   -771111,       0,       1000000 },	 // 20 0%
};


MUINT32 u4LSC_Integer_SQRT(unsigned long long x)
{
#define INIT_GUESS    30
    unsigned long long op, res, one;
    op = x;
    res = 0;
    one = (1uL << INIT_GUESS);

    while (one > op) one >>= 2;
    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return((MUINT32)res);
}

MINT32 mrLSC_POLY_Ratio_Get(int d, int dmax, RATIO_POLY_T coef_t)
{
    float di,dni,ri;
    float coefpoly[6];
    int k;
    coefpoly[0]=coef_t.coef_f;
    coefpoly[1]=coef_t.coef_e;
    coefpoly[2]=coef_t.coef_d;
    coefpoly[3]=coef_t.coef_c;
    coefpoly[4]=coef_t.coef_b;
    coefpoly[5]=coef_t.coef_a;

    di=(float)d/(float)dmax;
    dni=di;
    ri=coefpoly[0];
    for(k=1 ; k<6 ; k++)
    {
        ri+=(coefpoly[k]*dni);
        dni*=di;
    }
    return ((int)(ri*(float)(1<<RATIO_POLY_BIT)));
}



float mrLSC_POLY_RatioFloat_Get(int d, int dmax, RATIO_POLY_T coef_t)
{
    float di,dni,ri;
    float coefpoly[6];
    int k;
    coefpoly[0]=coef_t.coef_f;
    coefpoly[1]=coef_t.coef_e;
    coefpoly[2]=coef_t.coef_d;
    coefpoly[3]=coef_t.coef_c;
    coefpoly[4]=coef_t.coef_b;
    coefpoly[5]=coef_t.coef_a;

    di=(float)d/(float)dmax;
    dni=di;
    ri=coefpoly[0];
    for(k=1 ; k<6 ; k++)
    {
        ri+=(coefpoly[k]*dni);
        dni*=di;
    }
    return ri;//((int)(ri*(float)(1<<RATIO_POLY_BIT)));
}

void vLSC_PARAM_INIT(LSC_CAL_INI_PARAM_T a_rLSCCaliINIParam)
{
    int i,u;
        // init calibration parameters 
    g_lsc_param.raw_wd=a_rLSCCaliINIParam.u4ImgWidth;
    g_lsc_param.raw_ht=a_rLSCCaliINIParam.u4ImgHeight;
    g_lsc_param.avg_pixel_size=AVG_PIXEL_SIZE;
    g_lsc_param.bayer_order=a_rLSCCaliINIParam.u2BayerStart;

    g_lsc_param.x_grid_num=a_rLSCCaliINIParam.i4GridXNUM;
    g_lsc_param.y_grid_num=a_rLSCCaliINIParam.i4GridYNUM;
    g_lsc_param.pxl_gain_max=MAX_PIXEL_GAIN;
    
    g_lsc_param.poly_coef.coef_a=a_rLSCCaliINIParam.poly_coef.coef_a;
    g_lsc_param.poly_coef.coef_b=a_rLSCCaliINIParam.poly_coef.coef_b;
    g_lsc_param.poly_coef.coef_c=a_rLSCCaliINIParam.poly_coef.coef_c;
    g_lsc_param.poly_coef.coef_d=a_rLSCCaliINIParam.poly_coef.coef_d;
    g_lsc_param.poly_coef.coef_e=a_rLSCCaliINIParam.poly_coef.coef_e;
    g_lsc_param.poly_coef.coef_f=a_rLSCCaliINIParam.poly_coef.coef_f;
    
    //check if apply ratio compensation from poly coef 
    if( g_lsc_param.poly_coef.coef_a!=0.0 ||
            g_lsc_param.poly_coef.coef_b!=0.0 ||
            g_lsc_param.poly_coef.coef_c!=0.0 ||
            g_lsc_param.poly_coef.coef_d!=0.0 ||
            g_lsc_param.poly_coef.coef_e!=0.0)
    {
        g_lsc_param.poly_coef.ratio_poly_flag=1;
    }
    else 
    {
        g_lsc_param.poly_coef.ratio_poly_flag=0;
    }

    g_lsc_param.plane_wd=(a_rLSCCaliINIParam.u4ImgWidth>>1);
    g_lsc_param.plane_ht=(a_rLSCCaliINIParam.u4ImgHeight>>1);
    g_lsc_param.block_wd=(g_lsc_param.plane_wd)/(g_lsc_param.x_grid_num-1);
    g_lsc_param.block_ht=(g_lsc_param.plane_ht)/(g_lsc_param.y_grid_num-1);

    // condition check

    if(g_lsc_param.raw_wd-(g_lsc_param.raw_wd&0xfffffffe)) // raw width must multiple of 4 pixel
        CAMEEPROM_LOG(" ERROR :: LSC calibraton raw width illegal, raw width must multiple of 2 pixel \n");
    if(g_lsc_param.raw_ht-(g_lsc_param.raw_ht&0xfffffffe)) // raw height must multiple of 2 pixel
        CAMEEPROM_LOG(" ERROR :: LSC calibraton raw height illegal, raw height must multiple of 2 pixel \n");

}

void vLSC_Calibration_INIT(void)
{
    int i,j;
    int m=g_lsc_param.x_grid_num;
    int n=g_lsc_param.y_grid_num;
	g_src_tbl_float= (float *)malloc(m*n*4*sizeof(float));

    
    /* init grid x coordinates */
    g_col_idx[0] = 0;
    g_col_idx[g_lsc_param.x_grid_num-1] = g_lsc_param.plane_wd-1;
    for(i = 1; i < g_lsc_param.x_grid_num-1; i++) 
        g_col_idx[i]=g_lsc_param.block_wd*i;

    /* init grid y coordinates */
    g_row_idx[0] = 0;
    g_row_idx[g_lsc_param.y_grid_num-1] = g_lsc_param.plane_ht-1;
    for(j = 1; j < g_lsc_param.y_grid_num-1; j++) 
        g_row_idx[j]=g_lsc_param.block_ht*j;

	g_lsc_cali_info.tbl_info.src_tbl_addr_float=g_src_tbl_float;

}


void vLSC_AvgRaw_Process(float *src_addr_float, MUINT8 OB_B, MUINT8 OB_Gb, MUINT8 OB_Gr, MUINT8 OB_R, LSC_PARAM_T* plsc_param)
{
   
	int i,j;
	float temp1[4];
	int ch[4];
    int m       = plsc_param->x_grid_num;               
    int n       = plsc_param->y_grid_num;              
    int bayer   = plsc_param->bayer_order;
    
	if ( bayer == 0 )
	{
		ch[0]=0;	ch[1]=1;	ch[2]=2;	ch[3]=3;
	}
	else if ( bayer == 1 )
	{
		ch[0]=1;	ch[1]=0;	ch[2]=3;	ch[3]=2;
	}
	else if ( bayer == 2 )
	{
		ch[0]=2;	ch[1]=3;	ch[2]=0;	ch[3]=1;
	}
	else if ( bayer == 3 )
	{
		ch[0]=3;	ch[1]=2;	ch[2]=1;	ch[3]=0;
	}
	else
	{
        CAMEEPROM_LOG("Byer Parameter Error!!\n");
       
	} 

    //Rempa to bayer order 0 and OB
	for(i=0; i<m*n*4; i+=4)
	{
		temp1[0] = src_addr_float[i+ch[0]];
		temp1[1] = src_addr_float[i+ch[1]];
		temp1[2] = src_addr_float[i+ch[2]];
		temp1[3] = src_addr_float[i+ch[3]];
		src_addr_float[i+0] = (temp1[0]<OB_B )?0:(temp1[0]-OB_B );
		src_addr_float[i+1] = (temp1[1]<OB_Gb)?0:(temp1[1]-OB_Gb);
		src_addr_float[i+2] = (temp1[2]<OB_Gr)?0:(temp1[2]-OB_Gr);
		src_addr_float[i+3] = (temp1[3]<OB_R )?0:(temp1[3]-OB_R );
	}
#ifdef eeprom_debug

  for(i=0;i<n;i++)
   	for(j=0;j<m;j++)
		CAMEEPROM_LOG("src_float: %f,%f,%f,%f\n", src_addr_float[(i*m+j)*4+0], src_addr_float[(i*m+j)*4+1],
		 src_addr_float[(i*m+j)*4+2], src_addr_float[(i*m+j)*4+3]);
 #endif 
}


void vLSC_AvgRaw_Load(CAM_CAL_EEPROM_DATA data, float *src_addr_float, LSC_PARAM_T* plsc_param)
{
    MUINT32 i, j,k;
    char inStr[200];
    char *dummy_str, *channelname;
    MUINT8 chIdx;
    int m       = plsc_param->x_grid_num;               
    int n       = plsc_param->y_grid_num;  

    
   
   	  
   	  	for(i=0; i<n; i++)
    	{
         for(j=0; j<m; j++)
         {                    
             src_addr_float[(i*m+j)*4+0] =(float) data.GbChannel_LSC_DATA[i*m+j];
             
         }
        }
   	  
   	  
   	   for(i=0; i<n; i++)
    	{
         for(j=0; j<m; j++)
         {                    
             src_addr_float[(i*m+j)*4+1] = (float)data.BChannel_LSC_DATA[i*m+j];
             
         }
        }
   	  
	  
   	  	for(i=0; i<n; i++)
    	{
         for(j=0; j<m; j++)
         {                    
             src_addr_float[(i*m+j)*4+2] = (float)data.RChannel_LSC_DATA[i*m+j];
             
         }
        }
   	  
   	 
   	   for(i=0; i<n; i++)
    	{
         for(j=0; j<m; j++)
         {                    
             src_addr_float[(i*m+j)*4+3] = (float)data.GrChannel_LSC_DATA[i*m+j];
             
         }
        }
   	 
       
  #ifdef eeprom_debug
   
	for(i=0;i<n;i++)
   	for(j=0;j<m;j++)
		CAMEEPROM_LOG("src_float load: %f,%f,%0x,%0x\n", src_addr_float[(i*m+j)*4+0], src_addr_float[(i*m+j)*4+1],
		 data.RChannel_LSC_DATA[i*m+j], data.GrChannel_LSC_DATA[i*m+j]);
  #endif

}


void mrLSC_AvgToGainTbl(float *src_addr_float, LSC_PARAM_T* plsc_param,UINT32* gain_table)
{
    int i,j,k;
    int kx,ky,x0,y0;
    int idx0, fidx;
    unsigned int vlong0,vlong1;

	float val[4];
	float ratio_poly_float  = 1.0;
	float pixnum_avg        = plsc_param->avg_pixel_size*plsc_param->avg_pixel_size;
    int bayer_order         = plsc_param->bayer_order;              // bayer order
    int m                   = plsc_param->x_grid_num;               // x-dir grid number
    int n                   = plsc_param->y_grid_num;               // y-dir grid number
    int avg_pxl             = plsc_param->avg_pixel_size;           // avg_pxl*avg_pxl pixels sqare region for pixel value average, avoid noise
    unsigned long long wd   = plsc_param->plane_wd;                 // raw domain width
    unsigned long long ht   = plsc_param->plane_ht;                 // raw domain height
    int ratio_poly_enable   = plsc_param->poly_coef.ratio_poly_flag;// en/disable of compensation ratio

    int pxl_gain_max;                           // max pixel gain in fix point
    int dis_max;                                // for compensation ratio usage, the max distance to center point of raw
    int dis_cur;                                // for compensation ratio usage, distance to center from current grid point
    int index=0;                       //for gain table index
    //Get max source val for each channel
     for(j = 0; j < n; j++)
     {
        for(i = 0; i < m; i++)
        {
            idx0=(j*m+i)*4;
            val[0] = *(src_addr_float+(idx0    ));
            val[1] = *(src_addr_float+(idx0+1  ));
            val[2] = *(src_addr_float+(idx0+2  ));
            val[3] = *(src_addr_float+(idx0+3  ));

            for(k=0;k<4;k++)
            {
                if(val[k]>g_lsc_result.max_val[k])
                {
                    g_lsc_result.max_val[k]=val[k];
                }
            }
        }
    }  

    /* pixel gain upper bound is min of user defined or 16 bit */
    pxl_gain_max=MIN2(((1<<16)-1),(g_lsc_param.pxl_gain_max)<<GN_BIT); 

    //max distance to raw center: from (0,0) to (wd/2,ht/2), for sqrt calculation, right shift SQRT_NORMALIZE_BIT bit for accuracy
    dis_max=u4LSC_Integer_SQRT((unsigned long long)((((long long)wd*(long long)wd)>>2)+(((long long)ht*(long long)ht)>>2))<<SQRT_NORMALIZE_BIT); 


    for(j = 0; j < n; j++)
    {
        for(i = 0; i < m; i++)
        {
            /* do compensation ratio */
            kx=g_col_idx[i];
            ky=g_row_idx[j];
            
            if(ratio_poly_enable)
            {
                x0=(wd>>1);
                y0=(ht>>1);
                dis_cur=u4LSC_Integer_SQRT((unsigned long long)((long long)(kx-x0)*(long long)(kx-x0)+(long long)(ky-y0)*(long long)(ky-y0))<<SQRT_NORMALIZE_BIT);
				ratio_poly_float=mrLSC_POLY_RatioFloat_Get(dis_cur, dis_max, g_lsc_param.poly_coef);				
            }
            /* parse original grid average pixel values from buffer */
			idx0=(j*m+i)*4;
            val[0]=*(src_addr_float+idx0  )  ;
            val[1]=*(src_addr_float+idx0+1 );
            val[2]=*(src_addr_float+idx0+2);
            val[3]=*(src_addr_float+idx0+3);


            /* calculate pixel gain of each grid point, right shift GN_BIT bit to fix point */
            for(k=0;k<4;k++)
            {
                if(val[k]==0) 
					val[k]=pxl_gain_max;
                else 
				{
					val[k]=((g_lsc_result.max_val[k])*ratio_poly_float )/val[k];//((g_lsc_result.max_val[k])*ratio_poly)/val[k];
				}
                if(val[k]>pxl_gain_max) 
					val[k]=pxl_gain_max;          
            }
            
			fidx=(j*m+i)*4;
			*(src_addr_float+(fidx  ))=val[0];
			*(src_addr_float+(fidx+1))=val[1];
			*(src_addr_float+(fidx+2))=val[2];
			*(src_addr_float+(fidx+3))=val[3];
     #ifdef eeprom_debug
            CAMEEPROM_LOG("val: %f,%f,%f,%f\n",val[0],val[1],val[2],val[3]);
	 #endif	
            val[0] = MIN(65535,(val[0]*8192+0.5));
            val[1] = MIN(65535,(val[1]*8192+0.5));
            val[2] = MIN(65535,(val[2]*8192+0.5));
            val[3] = MIN(65535,(val[3]*8192+0.5));
			gain_table[j*m+i+index]=(((int)val[1]<<16)|(int)val[0]);
			gain_table[j*m+i+index+1]=(((int)val[3]<<16)|(int)val[2]);
			index+=1;
			

        }
    }
  
}

void LSC_AVGRAW_to_Gaintable(CAM_CAL_EEPROM_DATA eeprom_data,UINT32* gain_table)
{
    char	*pdest;			
	unsigned char u1_attn_index;
	float f_temp0,f_temp1, f_temp2,f_temp3,f_temp4,f_temp5;
	char gbitdepth;
	char tmpstr[256];
    int OB_R, OB_Gr, OB_B, OB_Gb, bAvgRawIn;
   g_rLSCCaliINIParam.u4ImgWidth=eeprom_data.imgwidth;
   g_rLSCCaliINIParam.u4ImgHeight=eeprom_data.imgheight;
   g_rLSCCaliINIParam.u2BayerStart=eeprom_data.bitorder;
   g_rLSCCaliINIParam.i4GridXNUM=eeprom_data.xblock_num;
   g_rLSCCaliINIParam.i4GridYNUM=eeprom_data.yblock_num;
   u1_attn_index = eeprom_data.Attenutation_Index;	  
   if ((u1_attn_index >= 0) && (u1_attn_index<21)) 
        {
            f_temp0 = (int)ShadingATNTable[u1_attn_index][0];
            f_temp1 = (int)ShadingATNTable[u1_attn_index][1];
            f_temp2 = (int)ShadingATNTable[u1_attn_index][2];
            f_temp3 = (int)ShadingATNTable[u1_attn_index][3];
            f_temp4 = (int)ShadingATNTable[u1_attn_index][4];
               f_temp5 = (int)ShadingATNTable[u1_attn_index][5];

            g_rLSCCaliINIParam.poly_coef.coef_a = f_temp0 / f_temp5;
            g_rLSCCaliINIParam.poly_coef.coef_b = f_temp1 / f_temp5;
            g_rLSCCaliINIParam.poly_coef.coef_c = f_temp2 / f_temp5;
            g_rLSCCaliINIParam.poly_coef.coef_d = f_temp3 / f_temp5;
            g_rLSCCaliINIParam.poly_coef.coef_e = f_temp4 / f_temp5;
            g_rLSCCaliINIParam.poly_coef.coef_f = f_temp5 / f_temp5;
        }
        
   OB_B      = eeprom_data.OB_B;
   OB_Gb     = eeprom_data.OB_Gb;
   OB_Gr     = eeprom_data.OB_Gr;
   OB_R      = eeprom_data.OB_R;
   gbitdepth = eeprom_data.bitdepth;
   vLSC_PARAM_INIT(g_rLSCCaliINIParam);
   vLSC_Calibration_INIT();
   vLSC_AvgRaw_Load(eeprom_data,g_lsc_cali_info.tbl_info.src_tbl_addr_float, &g_lsc_param);
   vLSC_AvgRaw_Process(g_lsc_cali_info.tbl_info.src_tbl_addr_float,
                                                OB_B, 
                                                OB_Gb, 
                                                OB_Gr,
                                                OB_R,
                                                &g_lsc_param);

  mrLSC_AvgToGainTbl(g_lsc_cali_info.tbl_info.src_tbl_addr_float, &g_lsc_param,gain_table);
  free(g_src_tbl_float);
         
}

