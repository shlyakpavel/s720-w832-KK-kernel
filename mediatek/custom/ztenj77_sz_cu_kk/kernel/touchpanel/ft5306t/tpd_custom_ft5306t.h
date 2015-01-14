#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE         
#define TPD_I2C_NUMBER           0
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define TPD_VELOCITY_CUSTOM_X 12
#define TPD_VELOCITY_CUSTOM_Y 16


#define TPD_DELAY                (2*HZ/100)
//#define TPD_RES_X                480
//#define TPD_RES_Y                800
#define TPD_CALIBRATION_MATRIX  {962,0,0,0,1600,0,0,0};

//#define TPD_HAVE_CALIBRATION
//zhaoshaopeng define this 20120612
#define TPD_HAVE_BUTTON
//#define TPD_HAVE_TREMBLE_ELIMINATION

#define TPD_BUTTON_HEIGH        (40)//(100)
#define TPD_KEY_COUNT           4//zhaoshaopeng from 3 20120512
#define TPD_KEYS               { KEY_MENU, KEY_HOMEPAGE ,KEY_BACK,KEY_SEARCH} //{ KEY_MENU, KEY_HOMEPAGE ,KEY_BACK}
#define TPD_KEYS_DIM            {{65,840,90,50},{185,840,90,50},{300,840,90,50},{420,840,90,50}}

#endif /* TOUCHPANEL_H__ */
