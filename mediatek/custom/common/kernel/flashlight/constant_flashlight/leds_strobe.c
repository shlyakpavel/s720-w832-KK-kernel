#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include "kd_flashlight.h"
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_hw.h"
/******************************************************************************
 * Definition
******************************************************************************/
#ifndef TRUE
#define TRUE KAL_TRUE
#endif
#ifndef FALSE
#define FALSE KAL_FALSE
#endif
 
/* device name and major number */
#define STROBE_DEVNAME            "leds_strobe"

#define DELAY_MS(ms) {mdelay(ms);}//unit: ms(10^-3)
#define DELAY_US(us) {mdelay(us);}//unit: us(10^-6)
#define DELAY_NS(ns) {mdelay(ns);}//unit: ns(10^-9)
/*
    non-busy dealy(/kernel/timer.c)(CANNOT be used in interrupt context):
        ssleep(sec)
        msleep(msec)
        msleep_interruptible(msec)

    kernel timer


*/

/******************************************************************************
 * Debug configuration
******************************************************************************/
#define PFX "[LEDS_STROBE]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define PK_WARN(fmt, arg...)        printk(KERN_WARNING PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      printk(KERN_NOTICE PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              printk(PFX "<%s>\n", __FUNCTION__);
#define PK_TRC_VERBOSE(fmt, arg...) printk(PFX fmt, ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_TRC PK_DBG_NONE /* PK_TRC_FUNC */
#define PK_VER PK_DBG_NONE /* PK_TRC_VERBOSE */
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_TRC(a,...)
#define PK_VER(a,...)
#define PK_ERR(a,...)
#endif

#define LENOVO_PROJECT_SEATTLE
/*******************************************************************************
* structure & enumeration
*******************************************************************************/

/* cotta-- added for high current solution */

#define FLASH_LIGHT_WDT_DISABLE 0
/*#define FLASH_LIGHT_WDT_TIMEOUT_MS          300	 max operation time of high current strobe */
/*#define FLASH_LIGHT_CAPTURE_DELAY_FRAME     (1)  should be same as pSensorInfo->CaptureDelayFrame = 1; */

/* --cotta */

struct strobe_data
{
    spinlock_t lock;
    wait_queue_head_t read_wait;
    struct semaphore sem;
};

/******************************************************************************
 * local variables
******************************************************************************/
static struct work_struct g_tExpEndWorkQ;   /* --- Work queue ---*/
static struct work_struct g_tTimeOutWorkQ;  /* --- Work queue ---*/
static GPT_CONFIG g_strobeGPTConfig;          /*cotta : Interrupt Config, modified for high current solution */
 

#if 0
static struct class *strobe_class = NULL;
static struct device *strobe_device = NULL;
static dev_t strobe_devno;
static struct cdev strobe_cdev;
#endif

/* static struct strobe_data strobe_private;*/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */

static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = FALSE;
static u32 strobe_width = 0; /* 0 is disable */
static eFlashlightState strobe_eState = FLASHLIGHTDRV_STATE_PREVIEW;
static MUINT32 sensorCaptureDelay = 0;                      /* cotta-- added for auto-set sensor capture delay mechanism*/
static MUINT32 g_WDTTimeout_ms = FLASH_LIGHT_WDT_DISABLE;   /* cotta-- disable WDT, added for high current solution */
static MUINT32 g_CaptureDelayFrame = 0;                     /* cotta-- added  for count down capture delay */

/* cotta-- added for level mapping 

     user can select 1-32 level value
     but driver will map 1-32 level to 1-12 level

     the reason why driver only uses 12 levels is protection reason
     according to LED spec
     if driving current larger than 200mA, LED might have chance to be damaged
     therefore, level 13-32 must use carafully and with strobe protection mechanism
     on the other hand, level 1-12 can use freely 
*/
static const MUINT32 strobeLevelLUT[32] = {1,2,3,4,5,6,7,8,9,10,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12}; 

/*****************************************************************************
Functions
*****************************************************************************/
#if 0
#define GPIO_CAMERA_FLASH_MODE GPIO95
#define GPIO_CAMERA_FLASH_MODE_M_GPIO  GPIO_MODE_00
    /*CAMERA-FLASH-T/F
           H:flash mode
           L:torch mode*/
#define GPIO_CAMERA_FLASH_EN GPIO46
#define GPIO_CAMERA_FLASH_EN_M_GPIO  GPIO_MODE_00
    /*CAMERA-FLASH-EN */


ssize_t gpio_FL_Init(void)
{
    /*set torch mode*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_MODE,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_MODE,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_MODE,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    /*Init. to disable*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    return 0;
}

ssize_t gpio_FL_Uninit(void)
{
    /*Uninit. to disable*/
    if(mt_set_gpio_mode(GPIO_CAMERA_FLASH_EN,GPIO_CAMERA_FLASH_MODE_M_GPIO)){PK_DBG("[constant_flashlight] set gpio mode failed!! \n");}
    if(mt_set_gpio_dir(GPIO_CAMERA_FLASH_EN,GPIO_DIR_OUT)){PK_DBG("[constant_flashlight] set gpio dir failed!! \n");}
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    return 0;
}

ssize_t gpio_FL_Enable(void)
{
    /*Enable*/
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ONE)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    return 0;
}

ssize_t gpio_FL_Disable(void)
{
    /*Enable*/
    if(mt_set_gpio_out(GPIO_CAMERA_FLASH_EN,GPIO_OUT_ZERO)){PK_DBG("[constant_flashlight] set gpio failed!! \n");}
    return 0;
}

ssize_t gpio_FL_dim_duty(kal_uint8 duty)
{
    /*N/A*/
    
    return 0;
}

/* abstract interface */
#define FL_Init gpio_FL_Init
#define FL_Uninit gpio_FL_Uninit
#define FL_Enable gpio_FL_Enable
#define FL_Disable gpio_FL_Disable
#define FL_dim_duty gpio_FL_dim_duty

#elif 1
#if defined(LENOVO_PROJECT_SEATTLE)||defined(LENOVO_PROJECT_SPAIN)||defined(LENOVO_PROJECT_PORTUGAL)
/*  lenovo seattle board flash led config  -- liaoxl.lenovo 6.12.2012 start */
#if defined(LENOVO_PROJECT_SEATTLE)||defined(LENOVO_PROJECT_PORTUGAL)
#define LM3554_TORCH_PIN  47
#define LM3554_FLASH_PIN  104
#define LM3554_EN_PIN     106
#elif defined(LENOVO_PROJECT_SPAIN)
/* lenovo-sw zhouwl, 2012-07-18, for spain config */
#define LM3554_EN_PIN     47  //HWEN
#define LM3554_FLASH_PIN  104 //STROBE
#define LM3554_TORCH_PIN  106
#define LM3554_ENVM_PIN   107 //ENVM
#endif


static struct i2c_client *lm3554_i2c_client = NULL;
static int lm3554_torch_config(u8 brightness);
static int lm3554_flash_config(u8 brightness);
static int lm3554_flash_disable(void);

/* abstract interface */
    
/*  cotta-- modified for high current solution */
int FL_Init(void)   /* Low current setting fuction*/
{
    lm3554_torch_config(4);//71mA

    spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
    
    g_WDTTimeout_ms = FLASH_LIGHT_WDT_DISABLE;  /* disable WDT */
    
    spin_unlock(&g_strobeSMPLock);
    
    PK_DBG(" Low current mode\n");
    return 0;
}

int FL_Uninit(void)
{    
    return 0;
}

int FL_Enable(void) /* turn on strobe */
{
    if(lm3554_i2c_client == NULL)
    {
    	return 0;
    }
    if(FLASHLIGHTDRV_STATE_STILL == strobe_eState)
    {
    	mt_set_gpio_out(LM3554_FLASH_PIN, GPIO_OUT_ONE);
    	PK_DBG(" FL_Enable : flash\n");
    }
    else
    {
	    mt_set_gpio_out(LM3554_TORCH_PIN, GPIO_OUT_ONE);
	    PK_DBG(" FL_Enable : torch\n");
	}

    return 0;
}

int FL_Disable(void)    /* turn off strobe */
{
    if(lm3554_i2c_client == NULL)
    {
    	return 0;
    }

    if(FLASHLIGHTDRV_STATE_STILL == strobe_eState)
    {
    	mt_set_gpio_out(LM3554_TORCH_PIN, GPIO_OUT_ZERO);
	    mt_set_gpio_out(LM3554_FLASH_PIN, GPIO_OUT_ZERO);
    	PK_DBG(" FL_Disable : flash\n");
    }
    else
    {
	    mt_set_gpio_out(LM3554_TORCH_PIN, GPIO_OUT_ZERO);
	    mt_set_gpio_out(LM3554_FLASH_PIN, GPIO_OUT_ZERO);
	    PK_DBG(" FL_Disable : torch\n");
	}

    return 0;
}

int FL_dim_duty(kal_uint32 duty)    /* adjust duty */
{
    PK_DBG(" strobe duty : %u when eState=%d\n",duty, strobe_eState);

    if(duty == 0)
    {
    	lm3554_flash_disable();
		PK_DBG(" end disable.");
    }

    return 0;
}

/*  cotta-- added for high current solution */
int FL_HighCurrent_Setting(void)  /*  high current setting fuction*/
{    
    if(lm3554_i2c_client == NULL)
    {
    	return 0;
    }
	lm3554_flash_config(10);//729mA

    spin_lock(&g_strobeSMPLock);        /* cotta-- SMP proection */
    
    g_WDTTimeout_ms = FLASH_LIGHT_WDT_TIMEOUT_MS;   /* enable WDT */
    
    spin_unlock(&g_strobeSMPLock);

    PK_DBG(" High current mode\n");

    return 0;
}
/* lenovo seattle board flash led config  -- liaoxl.lenovo 6.12.2012 end */
#else
extern void hwBacklightFlashlightTuning(kal_uint32 duty, kal_uint32 mode);
extern void hwBacklightFlashlightTurnOn(void);
extern void hwBacklightFlashlightTurnOff(void);
extern void upmu_flash_step_sel(kal_uint32 val);
extern void upmu_flash_mode_select(kal_uint32 val);
extern void upmu_flash_dim_duty(kal_uint32 val);

/* abstract interface */
    
/*  cotta-- modified for high current solution */
int FL_Init(void)   /* Low current setting fuction*/
{
    upmu_flash_dim_duty(12); 
    
    spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
    
    g_WDTTimeout_ms = FLASH_LIGHT_WDT_DISABLE;  /* disable WDT */
    
    spin_unlock(&g_strobeSMPLock);
    
    PK_DBG(" Low current mode\n");
    return 0;
}

int FL_Uninit(void)
{    
    return 0;
}

int FL_Enable(void) /* turn on strobe */
{
    hwBacklightFlashlightTurnOn();    
    return 0;
}

int FL_Disable(void)    /* turn off strobe */
{
    hwBacklightFlashlightTurnOff();   
    return 0;
}

int FL_dim_duty(kal_uint32 duty)    /* adjust duty */
{
    PK_DBG(" strobe duty : %u\n",duty);
   
    upmu_flash_step_sel(7);  
    upmu_flash_dim_duty(duty); 

    /*upmu_flash_mode_select(1);   register mode */
    /*hwBacklightFlashlightTuning(duty,0x0); */
    return 0;
}

/*  cotta-- added for high current solution */
int FL_HighCurrent_Setting(void)  /*  high current setting fuction*/
{    
    upmu_flash_step_sel(7);      /* max current */
    upmu_flash_dim_duty(0x1F);   /* max duty */   
    spin_lock(&g_strobeSMPLock);        /* cotta-- SMP proection */
    
    g_WDTTimeout_ms = FLASH_LIGHT_WDT_TIMEOUT_MS;   /* enable WDT */
    
    spin_unlock(&g_strobeSMPLock);

    PK_DBG(" High current mode\n");

    /* upmu_flash_mode_select(1);    register mode */
    /* hwBacklightFlashlightTuning(0x1F,0x0); */
    
    return 0;
}

#endif

#else
    #error "unknown arch"
#endif


/*****************************************************************************
debug
*****************************************************************************/
#define PREFLASH_PROFILE 0

#if PREFLASH_PROFILE
static void ledDrvProfile(unsigned int logID)
{
struct timeval tv1={0,0};
unsigned int u4MS;
    do_gettimeofday(&tv1);
    u4MS =  (unsigned int)(tv1.tv_sec * 1000 + tv1.tv_usec /1000);
    printk("[ledDrvProfile]state:[%d],time:[%d]ms \n",logID,u4MS);
}
#endif


/*****************************************************************************
User interface
*****************************************************************************/
static void strobe_xgptIsr(UINT16 a_input)
{
	schedule_work(&g_tTimeOutWorkQ);
}

/* cotta : modified to GPT usage */
static int strobe_WDTConfig(void)
{
    spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
    
    /* --- Config Interrupt --- */
    g_strobeGPTConfig.num         = GPT3;             /* GPT3 is assigned to strobe */
    g_strobeGPTConfig.mode        = GPT_ONE_SHOT;     /* trigger ones responed ones */
    g_strobeGPTConfig.clkSrc      = GPT_CLK_SRC_RTC;  /* clock source 32K */
    g_strobeGPTConfig.clkDiv      = GPT_CLK_DIV_11;   /* divided to 3.2k */
    g_strobeGPTConfig.bIrqEnable  = TRUE;             /* enable interrupt */
    g_strobeGPTConfig.u4CompareL  = ((g_WDTTimeout_ms * 5400) + 500) / 1000; /* 960 -> 300ms , 3200 -> 1000ms */
    g_strobeGPTConfig.u4CompareH  = 0;

    spin_unlock(&g_strobeSMPLock);
    
     /*
    if (GPT_IsStart(g_strobeGPTConfig.num))
    {
        PK_ERR("GPT_ISR is busy now\n");
        return -EBUSY;
    }
    */

    GPT_Reset(GPT3);
    
    if(GPT_Config(g_strobeGPTConfig) == FALSE)
    {
        PK_ERR("GPT_ISR Config Fail\n");
        return -EPERM;
    }   
    
    GPT_Init(g_strobeGPTConfig.num, strobe_xgptIsr);      

    PK_DBG("[GPT_ISR Config OK] g_strobeGPTConfig.u4CompareL = %u, WDT_timeout = %d\n",g_strobeGPTConfig.u4CompareL,FLASH_LIGHT_WDT_TIMEOUT_MS);
    
#if 0
    XGPT_IsStart(g_tStrobeXGPTConfig.num);
	XGPT_Start(g_tStrobeXGPTConfig.num);
	XGPT_Stop(g_tStrobeXGPTConfig.num);
    XGPT_Restart(g_tStrobeXGPTConfig.num);
#endif

    return 0;
}

static void strobe_WDTStart(void)
{
    GPT_Start(g_strobeGPTConfig.num);
    PK_DBG(" WDT start\n");
}

static void strobe_WDTStop(void)
{
    GPT_Stop(g_strobeGPTConfig.num);
    PK_DBG(" WDT stop\n");
}


static int constant_flashlight_ioctl(MUINT32 cmd, MUINT32 arg)
{
    int i4RetValue = 0;
    int iFlashType = (int)FLASHLIGHT_NONE;
    struct timeval now; /* cotta--log */
    
    //PK_DBG(" start: %u, %u, %d, %d\n",cmd,arg,strobe_width,g_strobe_On);

    /* when ON state , only disable command is permitted */
    if((TRUE == g_strobe_On) && !((FLASHLIGHTIOC_T_ENABLE == cmd) && (0 == arg)))
    {
        PK_DBG(" is already ON OR check parameters!\n");
	    switch(cmd)
	    {
	    	case FLASHLIGHTIOC_T_ENABLE :
	    		PK_DBG(" skip T_ENABLE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	    		break;
	        case FLASHLIGHTIOC_T_LEVEL:
	        	PK_DBG(" skip T_LEVEL: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	            break;
	        case FLASHLIGHTIOC_T_FLASHTIME:
	        	PK_DBG(" skip T_FLASHTIME: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	            break;
	        case FLASHLIGHTIOC_T_STATE:
	        	PK_DBG(" skip T_STATE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	            break;
	        case FLASHLIGHTIOC_G_FLASHTYPE:
	        	PK_DBG(" skip T_FLASHTYPE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	            break;
	        case FLASHLIGHTIOC_T_DELAY : /* cotta-- added for auto-set sensor capture delay mechanism*/
	        	PK_DBG(" skip T_DELAY: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
	            break;                  
	    	default :
	    		PK_DBG(" skip No such command \n");
	    		break;
	    }        

        /* goto strobe_ioctl_error; */
        return i4RetValue;
    }

    switch(cmd)
    {
    	case FLASHLIGHTIOC_T_ENABLE :
    		PK_DBG(" start T_ENABLE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            if (arg && strobe_width)
            {                
                if( (FLASHLIGHTDRV_STATE_PREVIEW == strobe_eState) || \
                    (FLASHLIGHTDRV_STATE_STILL == strobe_eState && KD_STROBE_HIGH_CURRENT_WIDTH != strobe_width) )
                {                
                    /* turn on strobe */
                    FL_Enable();
                    
                    spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */

                    g_strobe_On = TRUE;

                    spin_unlock(&g_strobeSMPLock);
                    
                    do_gettimeofday(&now);  /* cotta--log */
                    PK_DBG(" flashlight_IOCTL_Enable: %lu\n",now.tv_sec * 1000000 + now.tv_usec);
                }
                else
                {
                    PK_DBG(" flashlight_IOCTL_Enable : skip due to flash mode\n");  /* use VDIrq to control strobe under high current mode */                  
                } 
            }
            else
            {                      
                if(FL_Disable())
                {
                    PK_ERR(" FL_Disable fail!\n");
                    goto strobe_ioctl_error;
                }

                spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
                
                g_strobe_On = FALSE;                

                spin_unlock(&g_strobeSMPLock);

                if (g_WDTTimeout_ms != FLASH_LIGHT_WDT_DISABLE)
                {                    
                    strobe_WDTStop();   /* disable strobe watchDog timer */ 
                    FL_Init();  /* confirm everytime start from low current mode */
                }                

                do_gettimeofday(&now);  /* cotta--log */
                PK_DBG(" flashlight_IOCTL_Disable: %lu\n",now.tv_sec * 1000000 + now.tv_usec);                
            }
    		break;
        case FLASHLIGHTIOC_T_LEVEL:
        	PK_DBG(" start T_LEVEL: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            //PK_DBG(" user level:%u \n",arg);
            if (KD_STROBE_HIGH_CURRENT_WIDTH == arg)   /* high current mode */
            {
                spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
                
                strobe_width = KD_STROBE_HIGH_CURRENT_WIDTH;

                spin_unlock(&g_strobeSMPLock);
                FL_HighCurrent_Setting();                
            }
            else    /* low current mode */
            {            
                /* cotta-- added for level mapping */

                if(arg > 32)
                {
                    arg = 32;
                }
                else if(arg < 1)
                {
                    arg = 1;
                }
                
                arg = strobeLevelLUT[arg-1];  
                PK_DBG(" mapping level:%u \n",(int)arg);

                /* --cotta */
            
                spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
                
                strobe_width = arg;                

                spin_unlock(&g_strobeSMPLock);

                if (arg > 0)
                {
                    /* set to low current mode */
                    FL_Init();
                    
                    if(FL_dim_duty((kal_int8)arg - 1))
                    {
                        /* 0(weak)~31(strong) */
                        PK_ERR(" FL_dim_duty fail!\n");
                        
                        /* i4RetValue = -EINVAL; */
                        goto strobe_ioctl_error;
                    }
                }
            }
            break;
        case FLASHLIGHTIOC_T_FLASHTIME:
        	PK_DBG(" start T_FLASHTIME: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
            
            strobe_Timeus = (u32)arg;
            
            spin_unlock(&g_strobeSMPLock);
            PK_DBG(" strobe_Timeus:%u \n",strobe_Timeus);
            break;
        case FLASHLIGHTIOC_T_STATE:
        	PK_DBG(" start T_STATE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
             
            strobe_eState = (eFlashlightState)arg;

            spin_unlock(&g_strobeSMPLock);
            PK_DBG(" strobe_state:%u\n",arg);
            break;
        case FLASHLIGHTIOC_G_FLASHTYPE:
        	PK_DBG(" start T_FLASHTYPE: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */

            iFlashType = FLASHLIGHT_LED_CONSTANT;

            spin_unlock(&g_strobeSMPLock);
            if(copy_to_user((void __user *) arg , (void*)&iFlashType , _IOC_SIZE(cmd)))
            {
                PK_DBG(" ioctl copy to user failed\n");
                return -EFAULT;
            }
            break;
        case FLASHLIGHTIOC_T_DELAY : /* cotta-- added for auto-set sensor capture delay mechanism*/
        	PK_DBG(" start T_DELAY: %u, %d, %d\n",arg,strobe_width,g_strobe_On);
            if(arg >= 0)
            {                
                spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
                
                sensorCaptureDelay = arg;                
                g_CaptureDelayFrame = sensorCaptureDelay;

                spin_unlock(&g_strobeSMPLock);
                PK_DBG("capture delay = %u, count down value = %u\n",sensorCaptureDelay,g_CaptureDelayFrame);
            }
            break;                  
    	default :
    		PK_DBG(" No such command \n");
    		i4RetValue = -EPERM;
    		break;
    }

    /* PK_DBG(" done\n"); */
    return i4RetValue;

strobe_ioctl_error:
    PK_DBG(" Error or ON state!\n");
    return -EPERM;
}


static void strobe_expEndWork(struct work_struct *work)
{
    PK_DBG(" Exposure End Disable\n");
    constant_flashlight_ioctl(FLASHLIGHTIOC_T_ENABLE, 0);
}


static void strobe_timeOutWork(struct work_struct *work)
{
    PK_DBG(" Force OFF LED\n");
    constant_flashlight_ioctl(FLASHLIGHTIOC_T_ENABLE, 0);
}


static int constant_flashlight_open(void *pArg)
{
    int i4RetValue = 0;
    PK_DBG(" start\n");

    if (0 == strobe_Res)
    {
        FL_Init();

		/* enable HW here if necessary */
        if(FL_dim_duty(0))
        {
            /* 0(weak)~31(strong) */
            PK_ERR(" FL_dim_duty fail!\n");
            i4RetValue = -EINVAL;
        }

        /* --- WorkQueue --- */
        INIT_WORK(&g_tExpEndWorkQ,strobe_expEndWork);
        INIT_WORK(&g_tTimeOutWorkQ,strobe_timeOutWork);

        spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
        
        g_strobe_On = FALSE;
        
        spin_unlock(&g_strobeSMPLock);
    }
    
    spin_lock_irq(&g_strobeSMPLock);
    
    if(strobe_Res)
    {
        PK_ERR(" busy!\n");
        i4RetValue = -EBUSY;
    }
    else
    {
        strobe_Res += 1;
    }

    spin_unlock_irq(&g_strobeSMPLock);

    PK_DBG(" Done\n");

    return i4RetValue;

}


static int constant_flashlight_release(void *pArg)
{    
    PK_DBG(" start\n");

    if (strobe_Res)
    {
        spin_lock_irq(&g_strobeSMPLock);

        strobe_Res = 0;
        strobe_Timeus = 0;

        /* LED On Status */
        g_strobe_On = FALSE;

        spin_unlock_irq(&g_strobeSMPLock);

    	/* uninit HW here if necessary */
        if(FL_dim_duty(0))
        {
            PK_ERR(" FL_dim_duty fail!\n");
        }        
        
    	FL_Uninit();

        GPT_Stop(g_strobeGPTConfig.num);       
    }

    PK_DBG(" Done\n");

    return 0;

}


FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};


MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{ 
    if (pfFunc != NULL)
    {
        *pfFunc = &constantFlashlightFunc;
    }
    return 0;
}



/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{    
    struct timeval now; /* cotta--log */
    
    if ((strobe_width == KD_STROBE_HIGH_CURRENT_WIDTH) && (strobe_eState == FLASHLIGHTDRV_STATE_STILL))
    {
        PK_DBG(" start \n");
       
        if (g_CaptureDelayFrame == 0)
        {
            /* confirm WDT config value is correct */
            if(g_WDTTimeout_ms == FLASH_LIGHT_WDT_DISABLE)
            {
                spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
                
                g_WDTTimeout_ms = FLASH_LIGHT_WDT_TIMEOUT_MS;   /* enable WDT */             

                spin_unlock(&g_strobeSMPLock); 
            }
            
            /* enable strobe watchDog timer */
            strobe_WDTConfig();
                       
            if (g_WDTTimeout_ms != FLASH_LIGHT_WDT_DISABLE)
            {                
                strobe_WDTStart(); /* enable strobe watchDog timer */
            } 

            /* turn on strobe */
            FL_Enable(); 
            
            do_gettimeofday(&now);  /* cotta--log */
            PK_DBG(" strobe_VDIrq_Enable: %lu\n",now.tv_sec * 1000000 + now.tv_usec);

            spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */

            g_strobe_On = TRUE;

            /* cotta-- modified for auto-set sensor capture delay mechanism*/
            g_CaptureDelayFrame = sensorCaptureDelay;                 
            
            strobe_eState = FLASHLIGHTDRV_STATE_PREVIEW;            
            
            spin_unlock(&g_strobeSMPLock);    /* cotta-- SMP proection */

            PK_DBG(" g_CaptureDelayFrame : %u\n",g_CaptureDelayFrame);
        }
        else
        {
            spin_lock(&g_strobeSMPLock);    /* cotta-- SMP proection */
            
            --g_CaptureDelayFrame;           

            spin_unlock(&g_strobeSMPLock);    /* cotta-- SMP proection */
        }
    }
    
    return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);


#if 0
/*****************************************************************************/
/* Kernel interface */
static struct file_operations strobe_fops = {
    .owner      = THIS_MODULE,
    .ioctl      = strobe_ioctl,
    .open       = strobe_open,
    .release    = strobe_release,
};

/*****************************************************************************
Driver interface
*****************************************************************************/
#define ALLOC_DEVNO
static int strobe_probe(struct platform_device *dev)
{
    int ret = 0, err = 0;

	PK_DBG("[constant_flashlight] start\n");

#ifdef ALLOC_DEVNO
    ret = alloc_chrdev_region(&strobe_devno, 0, 1, STROBE_DEVNAME);
    if (ret) {
        PK_ERR("[constant_flashlight] alloc_chrdev_region fail: %d\n", ret);
        goto strobe_probe_error;
    } else {
        PK_DBG("[constant_flashlight] major: %d, minor: %d\n", MAJOR(strobe_devno), MINOR(strobe_devno));
    }
    cdev_init(&strobe_cdev, &strobe_fops);
    strobe_cdev.owner = THIS_MODULE;
    err = cdev_add(&strobe_cdev, strobe_devno, 1);
    if (err) {
        PK_ERR("[constant_flashlight] cdev_add fail: %d\n", err);
        goto strobe_probe_error;
    }
#else
    #define STROBE_MAJOR 242
    ret = register_chrdev(STROBE_MAJOR, STROBE_DEVNAME, &strobe_fops);
    if (ret != 0) {
        PK_ERR("[constant_flashlight] Unable to register chardev on major=%d (%d)\n", STROBE_MAJOR, ret);
        return ret;
    }
    strobe_devno = MKDEV(STROBE_MAJOR, 0);
#endif


    strobe_class = class_create(THIS_MODULE, "strobedrv");
    if (IS_ERR(strobe_class)) {
        PK_ERR("[constant_flashlight] Unable to create class, err = %d\n", (int)PTR_ERR(strobe_class));
        goto strobe_probe_error;
    }

    strobe_device = device_create(strobe_class, NULL, strobe_devno, NULL, STROBE_DEVNAME);
    if(NULL == strobe_device){
        PK_ERR("[constant_flashlight] device_create fail\n");
        goto strobe_probe_error;
    }

    /*initialize members*/
    spin_lock_init(&strobe_private.lock);
    init_waitqueue_head(&strobe_private.read_wait);
    init_MUTEX(&strobe_private.sem);

    //LED On Status
    g_strobe_On = FALSE;

    PK_DBG("[constant_flashlight] Done\n");
    return 0;

strobe_probe_error:
#ifdef ALLOC_DEVNO
    if (err == 0)
        cdev_del(&strobe_cdev);
    if (ret == 0)
        unregister_chrdev_region(strobe_devno, 1);
#else
    if (ret == 0)
        unregister_chrdev(MAJOR(strobe_devno), STROBE_DEVNAME);
#endif
    return -1;
}

static int strobe_remove(struct platform_device *dev)
{

    PK_DBG("[constant_flashlight] start\n");

#ifdef ALLOC_DEVNO
    cdev_del(&strobe_cdev);
    unregister_chrdev_region(strobe_devno, 1);
#else
    unregister_chrdev(MAJOR(strobe_devno), STROBE_DEVNAME);
#endif
    device_destroy(strobe_class, strobe_devno);
    class_destroy(strobe_class);

    //LED On Status
    g_strobe_On = FALSE;

    PK_DBG("[constant_flashlight] Done\n");
    return 0;
}


static struct platform_driver strobe_platform_driver =
{
    .probe      = strobe_probe,
    .remove     = strobe_remove,
    .driver     = {
        .name = STROBE_DEVNAME,
		.owner	= THIS_MODULE,
    },
};

static struct platform_device strobe_platform_device = {
    .name = STROBE_DEVNAME,
    .id = 0,
    .dev = {
    }
};

static int __init strobe_init(void)
{
    int ret = 0;
    PK_DBG("[constant_flashlight] start\n");

	ret = platform_device_register (&strobe_platform_device);
	if (ret) {
        PK_ERR("[constant_flashlight] platform_device_register fail\n");
        return ret;
	}

    ret = platform_driver_register(&strobe_platform_driver);
	if(ret){
		PK_ERR("[constant_flashlight] platform_driver_register fail\n");
		return ret;
	}

	PK_DBG("[constant_flashlight] done!\n");
    return ret;
}

static void __exit strobe_exit(void)
{
    PK_DBG("[constant_flashlight] start\n");
    platform_driver_unregister(&strobe_platform_driver);
    //to flush work queue
    //flush_scheduled_work();
    PK_DBG("[constant_flashlight] done!\n");
}

/*****************************************************************************/
module_init(strobe_init);
module_exit(strobe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jackie Su <jackie.su@mediatek.com>");
MODULE_DESCRIPTION("LED strobe control Driver");


/* LED flash control for capture mode*/
ssize_t strobe_StillExpCfgStart(void)
{
    if (strobe_width&&(FLASHLIGHTDRV_STATE_STILL == strobe_eState))
    {
        strobe_ioctl(0,0,FLASHLIGHTIOC_T_ENABLE, 1);
        PK_DBG("[constant_flashlight] FLASHLIGHTIOC_T_ENABLE:1\n");
    }
#if PREFLASH_PROFILE
    ledDrvProfile(999);
#endif
    return 0;
}

ssize_t strobe_StillExpEndIrqCbf(void)
{
    if (strobe_width&&(FLASHLIGHTDRV_STATE_STILL == strobe_eState))
    {
        schedule_work(&g_tExpEndWorkQ);
        strobe_eState = FLASHLIGHTDRV_STATE_PREVIEW;
    }
    return 0;
}

EXPORT_SYMBOL(strobe_StillExpCfgStart);
EXPORT_SYMBOL(strobe_StillExpEndIrqCbf);
#endif


/* lenovo seattle board flash led config  -- liaoxl.lenovo 6.12.2012  */
/*
* Copyright (C) 2012 Texas Instruments
*
* License Terms: GNU General Public License v2
*
* Simple driver for Texas Instruments LM3554 LED driver chip 
*
* Author: G.Shark Jeong <a0414807@ti.com>
*/
#include <linux/i2c.h>
#include <linux/leds.h>
#include "leds-lm3554.h"


//#define USE_LM3554_ANDROID_APP //For LM3554 Android Application.

#define LM3554_REG_TORCH		(0xA0)
#define LM3554_REG_FLASH		(0xB0)
#define LM3554_REG_FLASH_TIME		(0xC0)
#define LM3554_REG_FLAG			(0xD0)
#define LM3554_REG_CONF1		(0xE0)
#define LM3554_REG_CONF2		(0xF0)
#define LM3554_REG_GPIO			(0x20)
#define LM3554_REG_VIN_MON		(0x80)

#define LM3554_INDIC_I_SHIFT	 		(6)
#define LM3554_TORCH_I_SHIFT	 		(3)
#define LM3554_ENABLE_SHIFT 			(0)
#define LM3554_VM_SHIFT 			(2)
#define LM3554_STROBE_SHIFT 			(7) 
#define LM3554_FLASH_I_SHIFT 			(3)
#define LM3554_FLASH_I_LIMIT_SHIFT		(5)
#define LM3554_FLASH_TOUT_SHIFT 		(0)
#define LM3554_HW_TORCH_MODE_SHIFT		(7)
#define LM3554_TX2_POLARITY_SHIFT 		(6)
#define LM3554_ENVM_TX2_SHIFT 			(5)
#define LM3554_LEDI_NTC_SHIFT 			(3)
#define LM3554_EX_FLASH_INHIBIT_SHIFT 		(2)
#define LM3554_OV_SELECT_SHIFT 			(1)
#define LM3554_DISABLE_LIGHT_LOAD_SHIFT		(0)
#define LM3554_VIN_MONITOR_SHDN_SHIFT		(3)
#define LM3554_AET_MODE_SHIFT 			(2)
#define LM3554_NTC_SHDN_SHIFT 			(1)
#define LM3554_TX2_SHDN_SHIFT 			(0)
#define LM3554_NTC_EXTERNAL_FLAG_SHIFT		(6)
#define LM3554_GPIO2_DATA_SHIFT 		(5)
#define LM3554_GPIO2_DIR_SHIFT 			(4)
#define LM3554_GPIO2_CTRL_SHIFT 		(3)
#define LM3554_GPIO1_DATA_SHIFT 		(2)
#define LM3554_GPIO1_DIR_SHIFT 			(1)
#define LM3554_GPIO1_CTRL_SHIFT 		(0)
#define LM3554_VIN_THRESHOLD_SHIFT 		(1)
#define LM3554_VIN_MONITOR_EN_SHIFT 		(0)


#define LM3554_INDIC_I_MASK 			(0x3)
#define LM3554_TORCH_I_MASK 			(0x7)
#define LM3554_ENABLE_MASK 			(0x7)
#define LM3554_VM_MASK 				(0x1)
#define LM3554_STROBE_MASK 			(0x1)
#define LM3554_FLASH_I_MASK 			(0xF)
#define LM3554_FLASH_I_LIMIT_MASK 		(0x3)
#define LM3554_FLASH_TOUT_MASK 			(0x1F)
#define LM3554_HW_TORCH_MODE_MASK 		(0x1)
#define LM3554_TX2_POLARITY_MASK 		(0x1)
#define LM3554_ENVM_TX2_MASK 			(0x1)
#define LM3554_LEDI_NTC_MASK 			(0x1)
#define LM3554_EX_FLASH_INHIBIT_MASK	 	(0x1) 
#define LM3554_OV_SELECT_MASK 			(0x1) 
#define LM3554_DISABLE_LIGHT_LOAD_MASK		(0x1) 
#define LM3554_VIN_MONITOR_SHDN_MASK 		(0x1) 
#define LM3554_AET_MODE_MASK 			(0x1) 
#define LM3554_NTC_SHDN_MASK 			(0x1) 
#define LM3554_TX2_SHDN_MASK 			(0x1) 
#define LM3554_NTC_EXTERNAL_FLAG_MASK		(0x1) 
#define LM3554_GPIO2_DATA_MASK 			(0x1) 
#define LM3554_GPIO2_DIR_MASK 			(0x1) 
#define LM3554_GPIO2_CTRL_MASK 			(0x1)
#define LM3554_GPIO1_DATA_MASK 			(0x1) 
#define LM3554_GPIO1_DIR_MASK 			(0x1) 
#define LM3554_GPIO1_CTRL_MASK 			(0x1) 
#define LM3554_VIN_THRESHOLD_MASK 		(0x3) 
#define LM3554_VIN_MONITOR_EN_MASK 		(0x1)

struct lm3554_chip_data {
	struct i2c_client *client;

	struct led_classdev cdev_flash;
	struct led_classdev cdev_torch;
	struct led_classdev cdev_indicator;

	struct lm3554_platform_data *pdata;
	struct mutex lock;    

	u8 last_flag; 
	#if defined(USE_LM3554_ANDROID_APP)
		u8 rw_regno;
	#endif
	u8 no_pdata; 
};


/* i2c access*/
int lm3554_read_reg(struct i2c_client *client, u8 reg,u8 *val)
{
	int ret;
	struct lm3554_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret = i2c_smbus_read_byte_data(client, reg);
	mutex_unlock(&chip->lock);

	if (ret < 0) {
		printk(KERN_ERR "failed reading at 0x%02x error %d\n",reg, ret);
		return ret;
	}
	*val = ret&0xff; 

	return 0;
}
EXPORT_SYMBOL_GPL(lm3554_read_reg);

int lm3554_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int ret=0;
	struct lm3554_chip_data *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->lock);
	ret =  i2c_smbus_write_byte_data(client, reg, val);
	mutex_unlock(&chip->lock);
	
	if (ret < 0)
		printk(KERN_ERR "failed writting at 0x%02x\n", reg);
	return ret;
}
EXPORT_SYMBOL_GPL(lm3554_write_reg);

int lm3554_write_bits(struct i2c_client *client,
			u8 reg, u8 val,u8 mask,u8 shift)
{
	int ret;
	u8 reg_val;

	ret = lm3554_read_reg(client,reg,&reg_val);
	if(ret<0)
		return ret;
	reg_val &= (~(mask<<shift));  
	reg_val |= ((val&mask)<<shift);

	ret= lm3554_write_reg(client,reg,reg_val);
	return ret;
}
EXPORT_SYMBOL_GPL(lm3554_write_bits);


/* chip initialize*/
static int lm3554_chip_init(struct lm3554_chip_data *chip)
{
	int ret =0; 
	struct i2c_client *client = chip->client;
	struct lm3554_platform_data *pdata = chip->pdata; 

    mt_set_gpio_mode(LM3554_EN_PIN, 0);
    mt_set_gpio_dir(LM3554_EN_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(LM3554_EN_PIN, GPIO_OUT_ONE);

    mt_set_gpio_mode(LM3554_FLASH_PIN, 0);
    mt_set_gpio_dir(LM3554_FLASH_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(LM3554_FLASH_PIN, GPIO_OUT_ZERO);

    mt_set_gpio_mode(LM3554_TORCH_PIN, 0);
    mt_set_gpio_dir(LM3554_TORCH_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out(LM3554_TORCH_PIN, GPIO_OUT_ZERO);

	ret |= lm3554_write_bits(client, LM3554_REG_FLASH_TIME,
			LM3554_FLASH_T_256_MS, LM3554_FLASH_TOUT_MASK, LM3554_FLASH_TOUT_SHIFT);

	ret |= lm3554_write_bits(client, LM3554_REG_CONF1,
		pdata->torch_pin_enable,
		LM3554_HW_TORCH_MODE_MASK,LM3554_HW_TORCH_MODE_SHIFT); 

	ret |= lm3554_write_bits(client, LM3554_REG_CONF1,
		pdata->pam_sync_pin_enable,
		LM3554_ENVM_TX2_MASK,LM3554_ENVM_TX2_SHIFT); 

	ret |= lm3554_write_bits(client, LM3554_REG_CONF1,
		pdata->thermal_comp_mode_enable,
		LM3554_LEDI_NTC_MASK,LM3554_LEDI_NTC_SHIFT); 

	ret |= lm3554_write_bits(client, LM3554_REG_CONF1,
		pdata->strobe_pin_disable,
		LM3554_EX_FLASH_INHIBIT_MASK,LM3554_EX_FLASH_INHIBIT_SHIFT);
	return ret;
}


/* chip control*/
static void lm3554_control(struct lm3554_chip_data *chip,
		u8 brightness,enum lm3554_mode opmode)
{
	struct i2c_client *client = chip->client;
	struct lm3554_platform_data *pdata = chip->pdata;	 

	lm3554_read_reg(client,LM3554_REG_FLAG,&chip->last_flag);
	if(chip->last_flag)
		printk(KERN_ERR "LM3554 Last FLAG is 0x%x \n",chip->last_flag);
	
	if(!brightness)
		opmode = LM3554_MODE_SHDN;

	switch(opmode){
		case LM3554_MODE_TORCH:
			if(pdata->torch_pin_enable){
				lm3554_write_bits(client,LM3554_REG_CONF1,0x01,
					LM3554_HW_TORCH_MODE_MASK,
					LM3554_HW_TORCH_MODE_SHIFT);
				opmode = LM3554_MODE_SHDN; 
			}
			lm3554_write_bits(client,LM3554_REG_TORCH,brightness-1,
				LM3554_TORCH_I_MASK,LM3554_TORCH_I_SHIFT);
		break; 

		case LM3554_MODE_FLASH:	
			if(pdata->strobe_pin_disable){
				lm3554_write_bits(client,LM3554_REG_CONF1,0x01,
					LM3554_EX_FLASH_INHIBIT_MASK,
					LM3554_EX_FLASH_INHIBIT_SHIFT);
			}
			else
				opmode = LM3554_MODE_SHDN; 
			lm3554_write_bits(client,LM3554_REG_FLASH,brightness-1,
				LM3554_FLASH_I_MASK,LM3554_FLASH_I_SHIFT);
		break; 

		case LM3554_MODE_INDIC:
			if(pdata->thermal_comp_mode_enable){
				lm3554_write_bits(client,LM3554_REG_CONF1,0x01,
					LM3554_EX_FLASH_INHIBIT_MASK,
					LM3554_EX_FLASH_INHIBIT_SHIFT);
				opmode = LM3554_MODE_SHDN; 
				break; 
			}	
			lm3554_write_bits(client,LM3554_REG_TORCH,brightness-1,
				LM3554_INDIC_I_MASK,LM3554_INDIC_I_SHIFT);
		break; 
		case LM3554_MODE_SHDN:
			break; 
		default:
			return; 
	}	 
	
	if(pdata->vout_mode_enable) //set Voltage Out Mode.
		opmode |= (LM3554_VM_MASK<<LM3554_VM_SHIFT); 

	lm3554_write_bits(client,LM3554_REG_TORCH,opmode,
		LM3554_ENABLE_MASK,LM3554_ENABLE_SHIFT);

	return; 
}

/*torch */
static void lm3554_torch_brightness_set(struct led_classdev *cdev,
				  enum led_brightness brightness)
{
	struct lm3554_chip_data *chip =
		container_of(cdev, struct lm3554_chip_data, cdev_torch);

	lm3554_control(chip,(u8)brightness,LM3554_MODE_TORCH);		 
	return;
}

/* flash */
static void lm3554_strobe_brightness_set(struct led_classdev *cdev,
				enum led_brightness brightness)
{
	struct lm3554_chip_data *chip =
		container_of(cdev, struct lm3554_chip_data, cdev_flash);

	lm3554_control(chip,(u8)brightness,LM3554_MODE_FLASH);		 
	return;
}

/* indicator */
static void lm3554_indicator_brightness_set(
		struct led_classdev *cdev,enum led_brightness brightness)
{
	struct lm3554_chip_data *chip =
		container_of(cdev, struct lm3554_chip_data, cdev_indicator);

	lm3554_control(chip,(u8)brightness,LM3554_MODE_INDIC); 
	return;
}

#if defined(USE_LM3554_ANDROID_APP)
/* App Access */
static ssize_t lm3554_register_store(struct device *dev,
		 struct device_attribute *devAttr,const char *buf, size_t size)
{

	struct i2c_client *client = container_of(dev->parent,
									struct i2c_client,dev);  
    struct lm3554_chip_data *chip = i2c_get_clientdata(client);
	 
	if(buf[0]=='w')
		lm3554_write_bits(client,(u8)buf[1],(u8)buf[2],(u8)buf[3],(u8)buf[4]);
	else
		chip->rw_regno= (u8)buf[1];
	return 0;
}

static ssize_t lm3554_register_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
    u8 reg_val; 
	struct i2c_client *client = container_of(dev->parent,
									struct i2c_client,dev);
	struct lm3554_chip_data *chip = i2c_get_clientdata(client);
	
	lm3554_read_reg(client,chip->rw_regno,&reg_val);
	return sprintf(buf, "%u\n", reg_val);
}
static DEVICE_ATTR(register,0777,lm3554_register_show,lm3554_register_store);
#endif

/*   for MTK platform porting               --- liaoxl.lenovo  6.12.2012 start  */
static int lm3554_torch_config(u8 brightness)
{
	struct lm3554_chip_data *chip;

	if(lm3554_i2c_client == NULL)
	{
		return 0;
	}

	chip = i2c_get_clientdata(lm3554_i2c_client);

	lm3554_control(chip, brightness, LM3554_MODE_TORCH);

	return 0;
}


static int lm3554_flash_config(u8 brightness)
{
	struct lm3554_chip_data *chip;

	if(lm3554_i2c_client == NULL)
	{
		return 0;
	}

	chip = i2c_get_clientdata(lm3554_i2c_client);

	lm3554_control(chip, brightness, LM3554_MODE_FLASH);

	return 0;
}


static int lm3554_flash_disable(void)
{
	struct lm3554_chip_data *chip;

	if(lm3554_i2c_client == NULL)
	{
		return 0;
	}

	chip = i2c_get_clientdata(lm3554_i2c_client);

	lm3554_control(chip, 0, LM3554_MODE_SHDN);

	return 0;
}


/*   for MTK platform porting               --- liaoxl.lenovo  6.12.2012 end  */


/* Module Initialize */
static int lm3554_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct lm3554_chip_data *chip;
	struct lm3554_platform_data *pdata = client->dev.platform_data;

	int err = -1;

	printk(KERN_ERR  "lm3554_probe start--->.\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		printk(KERN_ERR  "LM3554 i2c functionality check fail.\n");
		return err; 
	}

	chip = kzalloc(sizeof(struct lm3554_chip_data), GFP_KERNEL);
	chip->client = client;	

	mutex_init(&chip->lock);	
	i2c_set_clientdata(client, chip);

	if(pdata == NULL){ //values are set to Zero. 
		printk(KERN_ERR  "LM3554 Platform data does not exist\n");
		pdata = kzalloc(sizeof(struct lm3554_platform_data),GFP_KERNEL);
		chip->pdata  = pdata;
		chip->no_pdata = 1; 
	}
	
	chip->pdata  = pdata;
	if(lm3554_chip_init(chip)<0)
		goto err_chip_init;

	lm3554_flash_config(10);//729mA
    lm3554_torch_config(4);//71mA


	//flash
	chip->cdev_flash.name="flash";
	chip->cdev_flash.max_brightness = 16;
	chip->cdev_flash.brightness_set = lm3554_strobe_brightness_set;
	if(led_classdev_register((struct device *)
		&client->dev,&chip->cdev_flash)<0)
		goto err_create_flash_file;	
	//torch
	chip->cdev_torch.name="torch";
	chip->cdev_torch.max_brightness = 8;
	chip->cdev_torch.brightness_set = lm3554_torch_brightness_set;
	if(led_classdev_register((struct device *)
		&client->dev,&chip->cdev_torch)<0)
		goto err_create_torch_file;
	//indicator
	chip->cdev_indicator.name="indicator";
	chip->cdev_indicator.max_brightness = 4;
	chip->cdev_indicator.brightness_set = lm3554_indicator_brightness_set;
	if(led_classdev_register((struct device *)
		&client->dev,&chip->cdev_indicator)<0)
		goto err_create_indicator_file; 

#if defined(USE_LM3554_ANDROID_APP)
	if(device_create_file(chip->cdev_flash.dev, &dev_attr_register)<0)
		goto err_create_reg_file; 
#endif 
	printk(KERN_INFO "LM3554 Initializing is done \n");

	lm3554_i2c_client = client;

	return 0;

#if defined(USE_LM3554_ANDROID_APP)
err_create_reg_file:
	led_classdev_unregister(&chip->cdev_indicator);
#endif 

err_create_indicator_file:
	led_classdev_unregister(&chip->cdev_torch);
err_create_torch_file:
	led_classdev_unregister(&chip->cdev_flash);
err_create_flash_file:
err_chip_init:	
	i2c_set_clientdata(client, NULL);
	kfree(chip);
	printk(KERN_ERR "LM3554 probe is failed \n");
	return -ENODEV;
}

static int lm3554_remove(struct i2c_client *client)
{
	struct lm3554_chip_data *chip = i2c_get_clientdata(client);

	led_classdev_unregister(&chip->cdev_indicator);
	led_classdev_unregister(&chip->cdev_torch);
	led_classdev_unregister(&chip->cdev_flash);

    if(chip->no_pdata)
		kfree(chip->pdata);
	kfree(chip);
	printk(KERN_INFO "LM3554 Removing is done");
	return 0;
}

static const struct i2c_device_id lm3554_id[] = {
	{LM3554_NAME, 0},
	{}
};
//MODULE_DEVICE_TABLE(i2c, lm3554_id);

static struct i2c_driver lm3554_i2c_driver = {
	.driver = {
		.name  = LM3554_NAME,
//		.owner = THIS_MODULE,
//		.pm	= NULL,
	},
	.probe	= lm3554_probe,
	.remove   = __devexit_p(lm3554_remove),
	.id_table = lm3554_id,
};

struct lm3554_platform_data lm3554_pdata = {1, 0, 0, 0, 0};
static struct i2c_board_info __initdata i2c_LM3554={ I2C_BOARD_INFO(LM3554_NAME, 0x53), \
													.platform_data = &lm3554_pdata,};

static int __init lm3554_init(void)
{
	printk(KERN_INFO "%s\n","lm3554_init");
	i2c_register_board_info(0, &i2c_LM3554, 1);

	return i2c_add_driver(&lm3554_i2c_driver);
}

static void __exit lm3554_exit(void)
{
	i2c_del_driver(&lm3554_i2c_driver);
}

module_init(lm3554_init);
module_exit(lm3554_exit);

MODULE_DESCRIPTION("Flash Lighting driver for LM3554");
MODULE_AUTHOR("G.Shark Jeong <a0414807@ti.com>");
MODULE_LICENSE("GPL v2");


