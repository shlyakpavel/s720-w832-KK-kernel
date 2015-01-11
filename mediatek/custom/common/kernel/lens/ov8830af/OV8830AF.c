#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "OV8830AF.h"
#include "../camera/kd_camera_hw.h"
//#include "kd_cust_lens.h"

//#include <mach/mt6573_pll.h>
//#include <mach/mt6573_gpt.h>
//#include <mach/mt6573_gpio.h>


#define OV8830AF_DRVNAME "OV8830AF"
#define OV8830AF_VCM_WRITE_ID           0x18

//#define OV8830AF_DEBUG
#ifdef OV8830AF_DEBUG
//#define OV8830AFDB printk
#define OV8830AFDB(fmt, args...) printk(KERN_ERR"[OV8830AF]%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#else
#define OV8830AFDB(x,...)
#endif

static spinlock_t g_OV8830AF_SpinLock;
/* Kirby: remove old-style driver
static unsigned short g_pu2Normal_OV8830AF_i2c[] = {OV8830AF_VCM_WRITE_ID , I2C_CLIENT_END};
static unsigned short g_u2Ignore_OV8830AF = I2C_CLIENT_END;

static struct i2c_client_address_data g_stOV8830AF_Addr_data = {
    .normal_i2c = g_pu2Normal_OV8830AF_i2c,
    .probe = &g_u2Ignore_OV8830AF,
    .ignore = &g_u2Ignore_OV8830AF
};*/

static struct i2c_client * g_pstOV8830AF_I2Cclient = NULL;

static dev_t g_OV8830AF_devno;
static struct cdev * g_pOV8830AF_CharDrv = NULL;
static struct class *actuator_class = NULL;

static int  g_s4OV8830AF_Opened = 0;
static long g_i4MotorStatus = 0;
static long g_i4Dir = 0;
static long g_i4Position = 0;
static unsigned long g_u4OV8830AF_INF = 0;
static unsigned long g_u4OV8830AF_MACRO = 1023;
static unsigned long g_u4TargetPosition = 0;
static unsigned long g_u4CurrPosition   = 0;
//static struct work_struct g_stWork;     // --- Work queue ---
//static XGPT_CONFIG	g_GPTconfig;		// --- Interrupt Config ---


//extern s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode);
//extern s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
//extern s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);


static int s4OV8830AF_ReadReg(unsigned short * a_pu2Result)
{
    int  i4RetValue = 0;
    char pBuff[2];
	OV8830AFDB("test");
    i4RetValue = i2c_master_recv(g_pstOV8830AF_I2Cclient, pBuff , 2);

    if (i4RetValue < 0) 
    {
        OV8830AFDB("[OV8830AF] I2C read failed!! \n");
        return -1;
    }

    *a_pu2Result = (((u16)pBuff[0]) << 4) + (pBuff[1] >> 4);

    return 0;
}

static int s4OV8830AF_WriteReg(u16 a_u2Data)
{
    int  i4RetValue = 0;

    char puSendCmd[2] = {(char)(a_u2Data >> 4) , (char)(((a_u2Data & 0xF) << 4)+0xF)};
	OV8830AFDB("test");
	//mt_set_gpio_out(97,1);
    i4RetValue = i2c_master_send(g_pstOV8830AF_I2Cclient, puSendCmd, 2);
	//mt_set_gpio_out(97,0);
	
    if (i4RetValue < 0) 
    {
        OV8830AFDB("[OV8830AF] I2C send failed!! \n");
        return -1;
    }

    return 0;
}

inline static int getOV8830AFInfo(__user stOV8830AF_MotorInfo * pstMotorInfo)
{
    stOV8830AF_MotorInfo stMotorInfo;
	OV8830AFDB("test");
    stMotorInfo.u4MacroPosition   = g_u4OV8830AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4OV8830AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
	if (g_i4MotorStatus == 1)	{stMotorInfo.bIsMotorMoving = TRUE;}
	else						{stMotorInfo.bIsMotorMoving = FALSE;}

	if (g_s4OV8830AF_Opened >= 1)	{stMotorInfo.bIsMotorOpen = TRUE;}
	else						{stMotorInfo.bIsMotorOpen = FALSE;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stOV8830AF_MotorInfo)))
    {
        OV8830AFDB("[OV8830AF] copy to user failed when getting motor information \n");
    }

    return 0;
}

inline static int moveOV8830AF(unsigned long a_u4Position)
{
	OV8830AFDB("test");
    if((a_u4Position > g_u4OV8830AF_MACRO) || (a_u4Position < g_u4OV8830AF_INF))
    {
        OV8830AFDB("[OV8830AF] out of range \n");
        return -EINVAL;
    }

	if (g_s4OV8830AF_Opened == 1)
	{
		unsigned short InitPos;
	
		if(s4OV8830AF_ReadReg(&InitPos) == 0)
		{
			OV8830AFDB("[OV8830AF] Init Pos %6d \n", InitPos);
		
			g_u4CurrPosition = (unsigned long)InitPos;
		}
		else
		{
			g_u4CurrPosition = 0;
		}
		
		g_s4OV8830AF_Opened = 2;
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
				if(s4OV8830AF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
				{
					g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
				}
				else
				{
					OV8830AFDB("[OV8830AF] set I2C failed when moving the motor \n");
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
				if(s4OV8830AF_WriteReg((unsigned short)g_u4TargetPosition) == 0)
				{
					g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
				}
				else
				{
					OV8830AFDB("[OV8830AF] set I2C failed when moving the motor \n");
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

inline static int setOV8830AFInf(unsigned long a_u4Position)
{
	OV8830AFDB("test");
	g_u4OV8830AF_INF = a_u4Position;
	return 0;
}

inline static int setOV8830AFMacro(unsigned long a_u4Position)
{
	OV8830AFDB("test");
	g_u4OV8830AF_MACRO = a_u4Position;
	return 0;	
}

////////////////////////////////////////////////////////////////
static long OV8830AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;
	OV8830AFDB("test");
    switch(a_u4Command)
    {
        case OV8830AFIOC_G_MOTORINFO :
            i4RetValue = getOV8830AFInfo((__user stOV8830AF_MotorInfo *)(a_u4Param));
        break;

        case OV8830AFIOC_T_MOVETO :
            i4RetValue = moveOV8830AF(a_u4Param);
        break;
 
 		case OV8830AFIOC_T_SETINFPOS :
			 i4RetValue = setOV8830AFInf(a_u4Param);
		break;

 		case OV8830AFIOC_T_SETMACROPOS :
			 i4RetValue = setOV8830AFMacro(a_u4Param);
		break;
		
        default :
      	     OV8830AFDB("[OV8830AF] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    return i4RetValue;
}
/*
static void OV8830AF_WORK(struct work_struct *work)
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
	
    if(s4OV8830AF_WriteReg((unsigned short)g_i4Position) == 0)
    {
        g_u4CurrPosition = (unsigned long)g_i4Position;
    }
    else
    {
        OV8830AFDB("[OV8830AF] set I2C failed when moving the motor \n");
        g_i4MotorStatus = -1;
    }
}

static void OV8830AF_ISR(UINT16 a_input)
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
static int OV8830AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    spin_lock(&g_OV8830AF_SpinLock);
	OV8830AFDB("test");
    if(g_s4OV8830AF_Opened)
    {
        spin_unlock(&g_OV8830AF_SpinLock);
        OV8830AFDB("[OV8830AF] the device is opened \n");
        return -EBUSY;
    }

    g_s4OV8830AF_Opened = 1;
		
    spin_unlock(&g_OV8830AF_SpinLock);

	// --- Config Interrupt ---
	//g_GPTconfig.num = XGPT7;
	//g_GPTconfig.mode = XGPT_REPEAT;
	//g_GPTconfig.clkDiv = XGPT_CLK_DIV_1;//32K
	//g_GPTconfig.u4Compare = 32*2; // 2ms
	//g_GPTconfig.bIrqEnable = TRUE;
	
	//XGPT_Reset(g_GPTconfig.num);	
	//XGPT_Init(g_GPTconfig.num, OV8830AF_ISR);

	//if (XGPT_Config(g_GPTconfig) == FALSE)
	//{
        //OV8830AFDB("[OV8830AF] ISR Config Fail\n");	
	//	return -EPERM;
	//}

	//XGPT_Start(g_GPTconfig.num);		

	// --- WorkQueue ---	
	//INIT_WORK(&g_stWork,OV8830AF_WORK);

    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int OV8830AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
	unsigned int cnt = 0;

	if (g_s4OV8830AF_Opened)
	{
		moveOV8830AF(g_u4OV8830AF_INF);

		while(g_i4MotorStatus)
		{
			msleep(1);
			cnt++;
			if (cnt>1000)	{break;}
		}
		
    	spin_lock(&g_OV8830AF_SpinLock);

	    g_s4OV8830AF_Opened = 0;

    	spin_unlock(&g_OV8830AF_SpinLock);

    	//hwPowerDown(CAMERA_POWER_VCAM_A,"kd_camera_hw");

		//XGPT_Stop(g_GPTconfig.num);
	}

    return 0;
}

static const struct file_operations g_stOV8830AF_fops = 
{
    .owner = THIS_MODULE,
    .open = OV8830AF_Open,
    .release = OV8830AF_Release,
    .unlocked_ioctl = OV8830AF_Ioctl
};

inline static int Register_OV8830AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_OV8830AF_devno, 0, 1,OV8830AF_DRVNAME) )
    {
        OV8830AFDB("[OV8830AF] Allocate device no failed\n");

        return -EAGAIN;
    }

    //Allocate driver
    g_pOV8830AF_CharDrv = cdev_alloc();

    if(NULL == g_pOV8830AF_CharDrv)
    {
        unregister_chrdev_region(g_OV8830AF_devno, 1);

        OV8830AFDB("[OV8830AF] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pOV8830AF_CharDrv, &g_stOV8830AF_fops);

    g_pOV8830AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pOV8830AF_CharDrv, g_OV8830AF_devno, 1))
    {
        OV8830AFDB("[OV8830AF] Attatch file operation failed\n");

        unregister_chrdev_region(g_OV8830AF_devno, 1);

        return -EAGAIN;
    }

    actuator_class = class_create(THIS_MODULE, "actuatordrv3");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        OV8830AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }

    vcm_device = device_create(actuator_class, NULL, g_OV8830AF_devno, NULL, OV8830AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    return 0;
}

inline static void Unregister_OV8830AF_CharDrv(void)
{
    //Release char driver
    cdev_del(g_pOV8830AF_CharDrv);

    unregister_chrdev_region(g_OV8830AF_devno, 1);
    
    device_destroy(actuator_class, g_OV8830AF_devno);

    class_destroy(actuator_class);
}

//////////////////////////////////////////////////////////////////////
/* Kirby: remove old-style driver
static int OV8830AF_i2c_attach(struct i2c_adapter * a_pstAdapter);
static int OV8830AF_i2c_detach_client(struct i2c_client * a_pstClient);
static struct i2c_driver OV8830AF_i2c_driver = {
    .driver = {
    .name = OV8830AF_DRVNAME,
    },
    //.attach_adapter = OV8830AF_i2c_attach,
    //.detach_client = OV8830AF_i2c_detach_client
};*/

/* Kirby: add new-style driver { */
//static int OV8830AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int OV8830AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int OV8830AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id OV8830AF_i2c_id[] = {{OV8830AF_DRVNAME,0},{}};   
//static unsigned short force[] = {IMG_SENSOR_I2C_GROUP_ID, OV8830AF_VCM_WRITE_ID, I2C_CLIENT_END, I2C_CLIENT_END};   
//static const unsigned short * const forces[] = { force, NULL };              
//static struct i2c_client_address_data addr_data = { .forces = forces,}; 
struct i2c_driver OV8830AF_i2c_driver = {                       
    .probe = OV8830AF_i2c_probe,                                   
    .remove = OV8830AF_i2c_remove,                           
    .driver.name = OV8830AF_DRVNAME,                 
    .id_table = OV8830AF_i2c_id,                             
};  

#if 0 
static int OV8830AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
    strcpy(info->type, OV8830AF_DRVNAME);                                                         
    return 0;                                                                                       
}      
#endif 
static int OV8830AF_i2c_remove(struct i2c_client *client) {
    return 0;
}
/* Kirby: } */


/* Kirby: remove old-style driver
int OV8830AF_i2c_foundproc(struct i2c_adapter * a_pstAdapter, int a_i4Address, int a_i4Kind)
*/
/* Kirby: add new-style driver {*/
static int OV8830AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
/* Kirby: } */
{
    int i4RetValue = 0;

    OV8830AFDB("[OV8830AF] Attach I2C \n");

    /* Kirby: remove old-style driver
    //Check I2C driver capability
    if (!i2c_check_functionality(a_pstAdapter, I2C_FUNC_SMBUS_BYTE_DATA))
    {
        OV8830AFDB("[OV8830AF] I2C port cannot support the format \n");
        return -EPERM;
    }

    if (!(g_pstOV8830AF_I2Cclient = kzalloc(sizeof(struct i2c_client), GFP_KERNEL)))
    {
        return -ENOMEM;
    }

    g_pstOV8830AF_I2Cclient->addr = a_i4Address;
    g_pstOV8830AF_I2Cclient->adapter = a_pstAdapter;
    g_pstOV8830AF_I2Cclient->driver = &OV8830AF_i2c_driver;
    g_pstOV8830AF_I2Cclient->flags = 0;

    strncpy(g_pstOV8830AF_I2Cclient->name, OV8830AF_DRVNAME, I2C_NAME_SIZE);

    if(i2c_attach_client(g_pstOV8830AF_I2Cclient))
    {
        kfree(g_pstOV8830AF_I2Cclient);
    }
    */
    /* Kirby: add new-style driver { */
    g_pstOV8830AF_I2Cclient = client;
    g_pstOV8830AF_I2Cclient->addr = OV8830AF_VCM_WRITE_ID >> 1;
//    g_pstOV8830AF_I2Cclient->addr = g_pstOV8830AF_I2Cclient->addr >> 1;
    
    /* Kirby: } */

    //Register char driver
    i4RetValue = Register_OV8830AF_CharDrv();

    if(i4RetValue){

        OV8830AFDB("[OV8830AF] register char device failed!\n");

        /* Kirby: remove old-style driver
        kfree(g_pstOV8830AF_I2Cclient); */

        return i4RetValue;
    }

    spin_lock_init(&g_OV8830AF_SpinLock);

    OV8830AFDB("[OV8830AF] Attached!! \n");

    return 0;
}

/* Kirby: remove old-style driver
static int OV8830AF_i2c_attach(struct i2c_adapter * a_pstAdapter)
{

    if(a_pstAdapter->id == 0)
    {
    	 return i2c_probe(a_pstAdapter, &g_stOV8830AF_Addr_data ,  OV8830AF_i2c_foundproc);
    }

    return -1;

}

static int OV8830AF_i2c_detach_client(struct i2c_client * a_pstClient)
{
    int i4RetValue = 0;

    Unregister_OV8830AF_CharDrv();

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

static int OV8830AF_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&OV8830AF_i2c_driver);
}

static int OV8830AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&OV8830AF_i2c_driver);
    return 0;
}

static int OV8830AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
//    int retVal = 0;
//    retVal = hwPowerDown(MT6516_POWER_VCAM_A,OV8830AF_DRVNAME);

    return 0;
}

static int OV8830AF_resume(struct platform_device *pdev)
{
/*
    if(TRUE != hwPowerOn(MT6516_POWER_VCAM_A, VOL_2800,OV8830AF_DRVNAME))
    {
        OV8830AFDB("[OV8830AF] failed to resume OV8830AF\n");
        return -EIO;
    }
*/
    return 0;
}

// platform structure
static struct platform_driver g_stOV8830AF_Driver = {
    .probe		= OV8830AF_probe,
    .remove	= OV8830AF_remove,
    .suspend	= OV8830AF_suspend,
    .resume	= OV8830AF_resume,
    .driver		= {
        .name	= "lens_actuator3",
        .owner	= THIS_MODULE,
    }
};

static int __init OV8830AF_i2C_init(void)
{
	OV8830AFDB("OV8830AF_i2C_init\n");
    if(platform_driver_register(&g_stOV8830AF_Driver)){
        OV8830AFDB("failed to register OV8830AF driver\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit OV8830AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stOV8830AF_Driver);
}

module_init(OV8830AF_i2C_init);
module_exit(OV8830AF_i2C_exit);

MODULE_DESCRIPTION("OV8830AF lens module driver");
MODULE_AUTHOR("Gipi Lin <Gipi.Lin@Mediatek.com>");
MODULE_LICENSE("GPL");


