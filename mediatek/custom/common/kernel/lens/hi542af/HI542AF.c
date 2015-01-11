#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "HI542AF.h"
#include "../camera/kd_camera_hw.h"
//#include "kd_cust_lens.h"

//#include <mach/mt6573_pll.h>
//#include <mach/mt6573_gpt.h>
//#include <mach/mt6573_gpio.h>


#define HI542AF_DRVNAME "HI542AF"
#define HI542AF_VCM_WRITE_ID           0x18

//#define HI542AF_DEBUG
#ifdef HI542AF_DEBUG
//#define HI542AFDB printk
#define HI542AFDB(fmt, args...) printk(KERN_ERR"[HI542AF]%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#else
#define HI542AFDB(x,...)
#endif

static spinlock_t g_HI542AF_SpinLock;
/* Kirby: remove old-style driver
static unsigned short g_pu2Normal_HI542AF_i2c[] = {HI542AF_VCM_WRITE_ID , I2C_CLIENT_END};
static unsigned short g_u2Ignore_HI542AF = I2C_CLIENT_END;

static struct i2c_client_address_data g_stHI542AF_Addr_data = {
    .normal_i2c = g_pu2Normal_HI542AF_i2c,
    .probe = &g_u2Ignore_HI542AF,
    .ignore = &g_u2Ignore_HI542AF
};*/

static struct i2c_client * g_pstHI542AF_I2Cclient = NULL;

static dev_t g_HI542AF_devno;
static struct cdev * g_pHI542AF_CharDrv = NULL;
static struct class *actuator_class = NULL;

static int  g_s4HI542AF_Opened = 0;
static long g_i4MotorStatus = 0;
static long g_i4Dir = 0;
static long g_i4Position = 0;
static unsigned long g_u4HI542AF_INF = 0;
static unsigned long g_u4HI542AF_MACRO = 1023;
static unsigned long g_u4TargetPosition = 0;
static unsigned long g_u4CurrPosition   = 0;
//static struct work_struct g_stWork;     // --- Work queue ---
//static XGPT_CONFIG	g_GPTconfig;		// --- Interrupt Config ---


//extern s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode);
//extern s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
//extern s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);


static int s4HI542AF_ReadReg(unsigned short * a_pu2Result)
{
    int  i4RetValue = 0;
    char pBuff[2];
	HI542AFDB("test");
    i4RetValue = i2c_master_recv(g_pstHI542AF_I2Cclient, pBuff , 2);

    if (i4RetValue < 0) 
    {
        HI542AFDB("[HI542AF] I2C read failed!! \n");
        return -1;
    }

    *a_pu2Result = (((u16)pBuff[0]) << 4) + (pBuff[1] >> 4);

    return 0;
}

static int s4HI542AF_WriteReg(u16 a_u2Data)
{
    int  i4RetValue = 0;

    char puSendCmd[2] = {(char)(a_u2Data >> 4) , (char)(((a_u2Data & 0xF) << 4)+0xF)};
	HI542AFDB("test");
	//mt_set_gpio_out(97,1);
    i4RetValue = i2c_master_send(g_pstHI542AF_I2Cclient, puSendCmd, 2);
	//mt_set_gpio_out(97,0);
	
    if (i4RetValue < 0) 
    {
        HI542AFDB("[HI542AF] I2C send failed!! \n");
        return -1;
    }

    return 0;
}

inline static int getHI542AFInfo(__user stHI542AF_MotorInfo * pstMotorInfo)
{
    stHI542AF_MotorInfo stMotorInfo;
	HI542AFDB("test");
    stMotorInfo.u4MacroPosition   = g_u4HI542AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4HI542AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	if (g_i4MotorStatus == 1)	{stMotorInfo.bIsMotorMoving = TRUE;}
	else						{stMotorInfo.bIsMotorMoving = FALSE;}

	if (g_s4HI542AF_Opened >= 1)	{stMotorInfo.bIsMotorOpen = TRUE;}
	else						{stMotorInfo.bIsMotorOpen = FALSE;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stHI542AF_MotorInfo)))
    {
        HI542AFDB("[HI542AF] copy to user failed when getting motor information \n");
    }

    return 0;
}

inline static int moveHI542AF(unsigned long a_u4Position)
{
	HI542AFDB("test");
    if((a_u4Position > g_u4HI542AF_MACRO) || (a_u4Position < g_u4HI542AF_INF))
    {
        HI542AFDB("[HI542AF] out of range \n");
        return -EINVAL;
    }

	if (g_s4HI542AF_Opened == 1)
	{
		unsigned short InitPos;
	
		if(s4HI542AF_ReadReg(&InitPos) == 0)
		{
			HI542AFDB("[HI542AF] Init Pos %6d \n", InitPos);
		
			g_u4CurrPosition = (unsigned long)InitPos;
		}
		else
		{
			g_u4CurrPosition = 0;
		}
		
		g_s4HI542AF_Opened = 2;
	}

	if      (g_u4CurrPosition < a_u4Position)	{g_i4Dir = 1;}
	else if (g_u4CurrPosition > a_u4Position)	{g_i4Dir = -1;}
	else										{return 0;}

	if (1)
	{
		g_i4Position = (long)g_u4CurrPosition;
		g_u4TargetPosition = a_u4Position;

		if (g_i4Dir == 1)
		{
			//if ((g_u4TargetPosition - g_u4CurrPosition)<60)
			{		
				g_i4MotorStatus = 0;
				if(s4HI542AF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
				{
					g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
				}
				else
				{
					HI542AFDB("[HI542AF] set I2C failed when moving the motor \n");
					g_i4MotorStatus = -1;
				}
			}
			//else
			//{
			//	g_i4MotorStatus = 1;
			//}
		}
		else if (g_i4Dir == -1)
		{
			//if ((g_u4CurrPosition - g_u4TargetPosition)<60)
			{
				g_i4MotorStatus = 0;		
				if(s4HI542AF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
				{
					g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
				}
				else
				{
					HI542AFDB("[HI542AF] set I2C failed when moving the motor \n");
					g_i4MotorStatus = -1;
				}
			}
			//else
			//{
			//	g_i4MotorStatus = 1;		
			//}
		}
	}
	else
	{
	g_i4Position = (long)g_u4CurrPosition;
	g_u4TargetPosition = a_u4Position;
	g_i4MotorStatus = 1;
	}

    return 0;
}

inline static int setHI542AFInf(unsigned long a_u4Position)
{
	HI542AFDB("test");
	g_u4HI542AF_INF = a_u4Position;
	return 0;
}

inline static int setHI542AFMacro(unsigned long a_u4Position)
{
	HI542AFDB("test");
	g_u4HI542AF_MACRO = a_u4Position;
	return 0;	
}

////////////////////////////////////////////////////////////////
static long HI542AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;
	HI542AFDB("test");
    switch(a_u4Command)
    {
        case HI542AFIOC_G_MOTORINFO :
            i4RetValue = getHI542AFInfo((__user stHI542AF_MotorInfo *)(a_u4Param));
        break;

        case HI542AFIOC_T_MOVETO :
            i4RetValue = moveHI542AF(a_u4Param);
        break;
 
 		case HI542AFIOC_T_SETINFPOS :
			 i4RetValue = setHI542AFInf(a_u4Param);
		break;

 		case HI542AFIOC_T_SETMACROPOS :
			 i4RetValue = setHI542AFMacro(a_u4Param);
		break;
		
        default :
      	     HI542AFDB("[HI542AF] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    return i4RetValue;
}
/*
static void HI542AF_WORK(struct work_struct *work)
{
    g_i4Position += (25 * g_i4Dir);

    if ((g_i4Dir == 1) && (g_i4Position >= (long)g_u4TargetPosition))
	{
        g_i4Position = (long)g_u4TargetPosition;
        g_i4MotorStatus = 0;
    }

    if ((g_i4Dir == -1) && (g_i4Position <= (long)g_u4TargetPosition))
    {
        g_i4Position = (long)g_u4TargetPosition;
        g_i4MotorStatus = 0; 		
    }
	
    if(s4HI542AF_WriteReg((unsigned short)g_i4Position) == 0)
    {
        g_u4CurrPosition = (unsigned long)g_i4Position;
    }
    else
    {
        HI542AFDB("[HI542AF] set I2C failed when moving the motor \n");
        g_i4MotorStatus = -1;
    }
}

static void HI542AF_ISR(UINT16 a_input)
{
	if (g_i4MotorStatus == 1)
	{	
		schedule_work(&g_stWork);		
	}
}
*/
//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
//CAM_RESET
static int HI542AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    spin_lock(&g_HI542AF_SpinLock);
	HI542AFDB("test");
    if(g_s4HI542AF_Opened)
    {
        spin_unlock(&g_HI542AF_SpinLock);
        HI542AFDB("[HI542AF] the device is opened \n");
        return -EBUSY;
    }

    g_s4HI542AF_Opened = 1;
		
    spin_unlock(&g_HI542AF_SpinLock);

	// --- Config Interrupt ---
	//g_GPTconfig.num = XGPT7;
	//g_GPTconfig.mode = XGPT_REPEAT;
	//g_GPTconfig.clkDiv = XGPT_CLK_DIV_1;//32K
	//g_GPTconfig.u4Compare = 32*2; // 2ms
	//g_GPTconfig.bIrqEnable = TRUE;
	
	//XGPT_Reset(g_GPTconfig.num);	
	//XGPT_Init(g_GPTconfig.num, HI542AF_ISR);

	//if (XGPT_Config(g_GPTconfig) == FALSE)
	//{
        //HI542AFDB("[HI542AF] ISR Config Fail\n");	
	//	return -EPERM;
	//}

	//XGPT_Start(g_GPTconfig.num);		

	// --- WorkQueue ---	
	//INIT_WORK(&g_stWork,HI542AF_WORK);

    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int HI542AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
	unsigned int cnt = 0;

	if (g_s4HI542AF_Opened)
	{
		moveHI542AF(g_u4HI542AF_INF);

		while(g_i4MotorStatus)
		{
			msleep(1);
			cnt++;
			if (cnt>1000)	{break;}
		}
		
    	spin_lock(&g_HI542AF_SpinLock);

	    g_s4HI542AF_Opened = 0;

    	spin_unlock(&g_HI542AF_SpinLock);

    	//hwPowerDown(CAMERA_POWER_VCAM_A,"kd_camera_hw");

		//XGPT_Stop(g_GPTconfig.num);
	}

    return 0;
}

static const struct file_operations g_stHI542AF_fops = 
{
    .owner = THIS_MODULE,
    .open = HI542AF_Open,
    .release = HI542AF_Release,
    .unlocked_ioctl = HI542AF_Ioctl
};

inline static int Register_HI542AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_HI542AF_devno, 0, 1,HI542AF_DRVNAME) )
    {
        HI542AFDB("[HI542AF] Allocate device no failed\n");

        return -EAGAIN;
    }

    //Allocate driver
    g_pHI542AF_CharDrv = cdev_alloc();

    if(NULL == g_pHI542AF_CharDrv)
    {
        unregister_chrdev_region(g_HI542AF_devno, 1);

        HI542AFDB("[HI542AF] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pHI542AF_CharDrv, &g_stHI542AF_fops);

    g_pHI542AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pHI542AF_CharDrv, g_HI542AF_devno, 1))
    {
        HI542AFDB("[HI542AF] Attatch file operation failed\n");

        unregister_chrdev_region(g_HI542AF_devno, 1);

        return -EAGAIN;
    }

    actuator_class = class_create(THIS_MODULE, "actuatordrv1");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        HI542AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }

    vcm_device = device_create(actuator_class, NULL, g_HI542AF_devno, NULL, HI542AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    return 0;
}

inline static void Unregister_HI542AF_CharDrv(void)
{
    //Release char driver
    cdev_del(g_pHI542AF_CharDrv);

    unregister_chrdev_region(g_HI542AF_devno, 1);
    
    device_destroy(actuator_class, g_HI542AF_devno);

    class_destroy(actuator_class);
}

//////////////////////////////////////////////////////////////////////
/* Kirby: remove old-style driver
static int HI542AF_i2c_attach(struct i2c_adapter * a_pstAdapter);
static int HI542AF_i2c_detach_client(struct i2c_client * a_pstClient);
static struct i2c_driver HI542AF_i2c_driver = {
    .driver = {
    .name = HI542AF_DRVNAME,
    },
    //.attach_adapter = HI542AF_i2c_attach,
    //.detach_client = HI542AF_i2c_detach_client
};*/

/* Kirby: add new-style driver { */
//static int HI542AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int HI542AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int HI542AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id HI542AF_i2c_id[] = {{HI542AF_DRVNAME,0},{}};   
//static unsigned short force[] = {IMG_SENSOR_I2C_GROUP_ID, HI542AF_VCM_WRITE_ID, I2C_CLIENT_END, I2C_CLIENT_END};   
//static const unsigned short * const forces[] = { force, NULL };              
//static struct i2c_client_address_data addr_data = { .forces = forces,}; 
struct i2c_driver HI542AF_i2c_driver = {                       
    .probe = HI542AF_i2c_probe,                                   
    .remove = HI542AF_i2c_remove,                           
    .driver.name = HI542AF_DRVNAME,                 
    .id_table = HI542AF_i2c_id,                             
};  

#if 0 
static int HI542AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
    strcpy(info->type, HI542AF_DRVNAME);                                                         
    return 0;                                                                                       
}      
#endif 
static int HI542AF_i2c_remove(struct i2c_client *client) {
    return 0;
}
/* Kirby: } */


/* Kirby: remove old-style driver
int HI542AF_i2c_foundproc(struct i2c_adapter * a_pstAdapter, int a_i4Address, int a_i4Kind)
*/
/* Kirby: add new-style driver {*/
static int HI542AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
/* Kirby: } */
{
    int i4RetValue = 0;

    HI542AFDB("[HI542AF] Attach I2C \n");

    /* Kirby: remove old-style driver
    //Check I2C driver capability
    if (!i2c_check_functionality(a_pstAdapter, I2C_FUNC_SMBUS_BYTE_DATA))
    {
        HI542AFDB("[HI542AF] I2C port cannot support the format \n");
        return -EPERM;
    }

    if (!(g_pstHI542AF_I2Cclient = kzalloc(sizeof(struct i2c_client), GFP_KERNEL)))
    {
        return -ENOMEM;
    }

    g_pstHI542AF_I2Cclient->addr = a_i4Address;
    g_pstHI542AF_I2Cclient->adapter = a_pstAdapter;
    g_pstHI542AF_I2Cclient->driver = &HI542AF_i2c_driver;
    g_pstHI542AF_I2Cclient->flags = 0;

    strncpy(g_pstHI542AF_I2Cclient->name, HI542AF_DRVNAME, I2C_NAME_SIZE);

    if(i2c_attach_client(g_pstHI542AF_I2Cclient))
    {
        kfree(g_pstHI542AF_I2Cclient);
    }
    */
    /* Kirby: add new-style driver { */
    g_pstHI542AF_I2Cclient = client;
    g_pstHI542AF_I2Cclient->addr = HI542AF_VCM_WRITE_ID >> 1;
//    g_pstHI542AF_I2Cclient->addr = g_pstHI542AF_I2Cclient->addr >> 1;
    
    /* Kirby: } */

    //Register char driver
    i4RetValue = Register_HI542AF_CharDrv();

    if(i4RetValue){

        HI542AFDB("[HI542AF] register char device failed!\n");

        /* Kirby: remove old-style driver
        kfree(g_pstHI542AF_I2Cclient); */

        return i4RetValue;
    }

    spin_lock_init(&g_HI542AF_SpinLock);

    HI542AFDB("[HI542AF] Attached!! \n");

    return 0;
}

/* Kirby: remove old-style driver
static int HI542AF_i2c_attach(struct i2c_adapter * a_pstAdapter)
{

    if(a_pstAdapter->id == 0)
    {
    	 return i2c_probe(a_pstAdapter, &g_stHI542AF_Addr_data ,  HI542AF_i2c_foundproc);
    }

    return -1;

}

static int HI542AF_i2c_detach_client(struct i2c_client * a_pstClient)
{
    int i4RetValue = 0;

    Unregister_HI542AF_CharDrv();

    //detach client
    i4RetValue = i2c_detach_client(a_pstClient);
    if(i4RetValue)
    {
        dev_err(&a_pstClient->dev, "Client deregistration failed, client not detached.\n");
        return i4RetValue;
    }

    kfree(i2c_get_clientdata(a_pstClient));

    return 0;
}*/

static int HI542AF_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&HI542AF_i2c_driver);
}

static int HI542AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&HI542AF_i2c_driver);
    return 0;
}

static int HI542AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
//    int retVal = 0;
//    retVal = hwPowerDown(MT6516_POWER_VCAM_A,HI542AF_DRVNAME);

    return 0;
}

static int HI542AF_resume(struct platform_device *pdev)
{
/*
    if(TRUE != hwPowerOn(MT6516_POWER_VCAM_A, VOL_2800,HI542AF_DRVNAME))
    {
        HI542AFDB("[HI542AF] failed to resume HI542AF\n");
        return -EIO;
    }
*/
    return 0;
}

// platform structure
static struct platform_driver g_stHI542AF_Driver = {
    .probe		= HI542AF_probe,
    .remove	= HI542AF_remove,
    .suspend	= HI542AF_suspend,
    .resume	= HI542AF_resume,
    .driver		= {
        .name	= "lens_actuator1",
        .owner	= THIS_MODULE,
    }
};

static int __init HI542AF_i2C_init(void)
{
	HI542AFDB("HI542AF_i2C_init\n");
    if(platform_driver_register(&g_stHI542AF_Driver)){
        HI542AFDB("failed to register HI542AF driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit HI542AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stHI542AF_Driver);
}

module_init(HI542AF_i2C_init);
module_exit(HI542AF_i2C_exit);

MODULE_DESCRIPTION("HI542AF lens module driver");
MODULE_AUTHOR("Gipi Lin <Gipi.Lin@Mediatek.com>");
MODULE_LICENSE("GPL");


