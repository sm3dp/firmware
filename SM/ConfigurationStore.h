#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include "Configuration.h"

	
//void _EEPROM_writeData(u32* address, uint8_t* value, uint8_t size);
//void _EEPROM_readData(u32* address, uint8_t* value, uint8_t size);

//void Config_ResetDefault(void);

//#ifdef EEPROM_CHITCHAT
void Config_PrintSettings(void);
//#else
//#endif
#ifdef EEPROM_SETTINGS
void Config_StoreSettings(void);
void Config_RetrieveSettings(void);
#endif

#endif//CONFIG_STORE_H
