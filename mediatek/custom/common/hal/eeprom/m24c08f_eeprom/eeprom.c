//#include "eeprom_layout.h"
#include "camera_custom_eeprom.h"
 
EEPROM_TYPE_ENUM EEPROMInit(void)
{
	return EEPROM_USED;
}

unsigned int EEPROMDeviceName(char* DevName)
{
       char* str= "EEPROM_M24C08F";
       strcat (DevName, str );
	return 0;
}


