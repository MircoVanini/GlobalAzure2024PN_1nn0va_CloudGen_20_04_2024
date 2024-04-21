
#ifndef __COMMON_H__
#define __COMMON_H__

#include <Arduino.h>

class Common 
{    
public:

    static String GetDeiceId(void)
    {
        uint8_t baseMac[6];

	    esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
	    char baseMacChr[18] = {0};
	    sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
	    
        return String(baseMacChr);        
    }
        
private:
};

#endif // __COMMON_H__
