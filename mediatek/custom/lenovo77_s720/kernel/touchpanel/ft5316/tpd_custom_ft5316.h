

#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_USE_VIRTUAL_KEY
#define TPD_TYPE_CAPACITIVE
//#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE         MT6575_POWER_VGP2
#define TPD_I2C_NUMBER           1
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define TPD_DELAY                (2*HZ/100)
//#define TPD_CUST_RES_X          	480
//#define TPD_CUST_RES_Y           854
#define TPD_CALIBRATION_MATRIX  {962,0,0,0,1600,0,0,0};

#define TPD_HAVE_CALIBRATION
#define TPD_HAVE_BUTTON
//#define TPD_HAVE_TREMBLE_ELIMINATION

//#define TPD_HAVE_POWER_ON_OFF

#define PRESSURE_FACTOR	10

#define TPD_BUTTON_HEIGHT		100//854
//#define TPD_KEY_COUNT           3
//#define TPD_KEYS                {KEY_HOME, KEY_MENU, KEY_BACK}
//#define TPD_KEYS_DIM            {{106,501,106,43},{212,501,106,43},{318,501,106,43}}
//Ivan (centerx,centery,width,height)
#define VIRTUAL_KEY_DEB_TIME	3
#define TPD_Y_OFFSET		0

#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_MENU,KEY_HOMEPAGE,KEY_BACK}

//TP virtual key customization
#define TPD_KEYS_DIM {{70,1000,139,100},{270,1000,172,100},{400,1000,158,100}} // Pavel Shlyak by Oleg.svs 

#endif /* TOUCHPANEL_H__ */
