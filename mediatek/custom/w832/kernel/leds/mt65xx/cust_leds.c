#include <cust_leds.h>
#include <mach/mt_pwm.h>

#include <linux/kernel.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>

#include <mach/mt6577_gpio.h>
#include <linux/delay.h>
#include "cust_gpio_usage.h"

extern int mtkfb_set_backlight_level(unsigned int level);
extern int mtkfb_set_backlight_pwm(int div);
#define ERROR_BL_LEVEL 0xFFFFFFFF

unsigned int brightness_mapping(unsigned int level)
{
    unsigned int mapped_level;
    
	mapped_level = level/4;
	//mapped_level = level/6;
	//mapped_level = (level*100)/455;
       
	return mapped_level;
}


unsigned int Cust_SetBacklight(int level, int div)
{
    mtkfb_set_backlight_pwm(div);
    mtkfb_set_backlight_level(brightness_mapping(level));
    return 0;
}

static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
	{"red",               MT65XX_LED_MODE_PWM, PWM1,{0}}, //kaka_12_0112
	{"green",             MT65XX_LED_MODE_PWM, PWM2,{0}},//kaka_12_0112
	{"blue",              MT65XX_LED_MODE_PWM, PWM3,{0}},//kaka_12_0112
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1,{0}},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1,{0}},
	{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON,{0}}, //kaka_11_1230 Keypad LED
	{"lcd-backlight",     MT65XX_LED_MODE_PWM, PWM0,{0}}, //Clarc
	//{"lcd-backlight",     MT65XX_LED_MODE_PWM, PWM0,{1,CLK_DIV2,32,32}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PWM, PWM0,{1,CLK_DIV8,5,5}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PWM, PMW 1;2;3;4;5;6,{0}},
	//{"lcd-backlight",     MT65XX_LED_MODE_CUST, (int)Cust_SetBacklight,{0}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD_ISINK,{0}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD_BOOST,{0}},
	//{"lcd-backlight",     MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_LCD,{0}},
        {"torch",             MT65XX_LED_MODE_NONE, -1,{0}},
	//{"flash-light",       MT65XX_LED_MODE_GPIO, GPIO_CAMERA_FLASH_EN_PIN,{0}}, //kaka_12_0112 FlashLight control  for FTM use only
};


struct cust_mt65xx_led *get_cust_led_list(void)
{
   return cust_led_list;
}
