#include <cust_leds.h>
#include <cust_leds_def.h>
#include <mach/mt_pwm.h>

#include <linux/kernel.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>

#include <mach/mt_gpio.h>
#include "cust_gpio_usage.h"
#include <asm/delay.h>

#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/delay.h>

extern int mtkfb_set_backlight_level(unsigned int level);
extern int mtkfb_set_backlight_pwm(int div);

unsigned int brightness_mapping(unsigned int level)
{
    unsigned int mapped_level;
    
    mapped_level = level;
       
	return mapped_level;
}

unsigned int Cust_SetBacklight(int level, int div)
{
    mtkfb_set_backlight_pwm(div);
    mtkfb_set_backlight_level(brightness_mapping(level));
    return 0;
}

#define KTD253_MAX_LEVEL 32
static int g_ktd_level = 0;
      
    DEFINE_SEMAPHORE(ktd253_mutex);

#if 0
  #define LED_DBG(fmt, arg...) \
	printk("[LED-LCM] %s (line:%d) :" fmt "\r\n", __func__, __LINE__, ## arg)
#else
  #define LED_DBG(fmt, arg...) do {} while (0)
#endif

unsigned int Cust_SetBacklight_KTD253(int level, int div)
{
	int loop, ktd_level, pulse = 0;
	if ( level < 0 || level > 255 ){
		LED_DBG("The level is out of range.");
		return -EFAULT;
	}
	if (down_interruptible(&ktd253_mutex)){
		LED_DBG("The KTD253 is bussy.");
		return -EAGAIN;
	}
	mt_set_gpio_mode(GPIO68, GPIO_MODE_GPIO);
	mt_set_gpio_dir(GPIO68, GPIO_DIR_OUT);	
	if (level){
		/*
		 * Mapping soft level (0~255) to hard level (32~1)
		 * "1" is the top level for KTD253
		 */
		ktd_level = (255 - level) / 8 + 1;
		if (g_ktd_level != ktd_level){
			if (ktd_level > g_ktd_level)
				pulse = ktd_level - g_ktd_level;
			else
				pulse = KTD253_MAX_LEVEL - g_ktd_level + ktd_level ;

			//local_irq_disable();
			preempt_disable();
			for (loop = 0; loop < pulse; loop++){
				mt_set_gpio_out(GPIO68,0);
				udelay(1);	    
				mt_set_gpio_out(GPIO68,1);
				udelay(1);
			}
			preempt_enable();
			//local_irq_enable();
			g_ktd_level = ktd_level;
		}
	}
	else{
		LED_DBG("The KTD253 will shut down.");
		mt_set_gpio_out(GPIO68,0);
		msleep(5);
		g_ktd_level = 0;
	}
	up(&ktd253_mutex);
    return 0;
}
/*
static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	{"red",               MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_NLED_ISINK4,{0}},
	{"green",             MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_NLED_ISINK5,{0}},
	{"blue",              MT65XX_LED_MODE_NONE, -1,{0}},
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1,{0}},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1,{0}},
	{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON,{0}}, 
	{"lcd-backlight",     MT65XX_LED_MODE_CUST_LCM, Cust_SetBacklight_KTD253,{0}},
}; */
static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	{"red",               MT65XX_LED_MODE_NONE, -1,{0}},
	{"green",             MT65XX_LED_MODE_NONE, -1,{0}},
	{"blue",              MT65XX_LED_MODE_NONE, -1,{0}},
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1,{0}},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1,{0}},
	{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON,{0}},
	{"lcd-backlight",     MT65XX_LED_MODE_CUST_LCM, (int)Cust_SetBacklight,{0}},
};

struct cust_mt65xx_led *get_cust_led_list(void)
{
	return cust_led_list;
}

