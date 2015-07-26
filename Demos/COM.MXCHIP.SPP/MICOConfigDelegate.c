/**
  ******************************************************************************
  * @file    MICOConfigDelegate.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provide delegate functons from Easylink function and FTC
  *          server. 
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */ 
#include "MICO.h"
#include "Platform_config.h"

#include "JSON-C/json.h"
#include "MICOAppDefine.h"
#include "SppProtocol.h"  
#include "StringUtils.h"

#define SYS_LED_TRIGGER_INTERVAL 100 
#define SYS_LED_TRIGGER_INTERVAL_AFTER_EASYLINK 500 
  
#define config_delegate_log(M, ...) custom_log("Config Delegate", M, ##__VA_ARGS__)
#define config_delegate_log_trace() custom_log_trace("Config Delegate")
  
extern volatile ring_buffer_t  rx_buffer;
extern volatile uint8_t        rx_data[UART_BUFFER_LENGTH];

static mico_timer_t _Led_EL_timer;

static void _led_EL_Timeout_handler( void* arg )
{
  (void)(arg);
  MicoGpioOutputTrigger((mico_gpio_t)MICO_SYS_LED);
}

USED void mico_system_delegate_config_will_start( void )
{
  config_delegate_log_trace();
    /*Led trigger*/
  mico_stop_timer(&_Led_EL_timer);
  mico_deinit_timer( &_Led_EL_timer );
  mico_init_timer(&_Led_EL_timer, SYS_LED_TRIGGER_INTERVAL, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);
  return;
}

USED void ConfigSoftApWillStart( void )
{
}

USED void mico_system_delegate_config_will_stop( void )
{
  config_delegate_log_trace();

  mico_stop_timer(&_Led_EL_timer);
  mico_deinit_timer( &_Led_EL_timer );
  MicoGpioOutputHigh((mico_gpio_t)MICO_SYS_LED);
  return;
}

USED void mico_system_delegate_config_recv_ssid ( void )
{
  config_delegate_log_trace();

  mico_stop_timer(&_Led_EL_timer);
  mico_deinit_timer( &_Led_EL_timer );
  mico_init_timer(&_Led_EL_timer, SYS_LED_TRIGGER_INTERVAL_AFTER_EASYLINK, _led_EL_Timeout_handler, NULL);
  mico_start_timer(&_Led_EL_timer);
  return;
}

USED void mico_system_delegate_config_success( mico_config_source_t source )
{
  config_delegate_log( "Configed by %d", source );
}


USED OSStatus mico_system_delegate_config_recv_auth_data(char * anthData  )
{
  config_delegate_log_trace();
  (void)(anthData);
  return kNoErr;
}

json_object* ConfigCreateReportJsonMessage( mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  config_delegate_log_trace();
  char name[50], *tempString;
  OTA_Versions_t versions;
  char rfVersion[50] = {0};
  json_object *sectors, *sector, *subMenuSectors, *subMenuSector, *mainObject = NULL;

  application_config_t *appConfig = mico_system_context_get_user_data( inContext );

  MicoGetRfVer( rfVersion, 50 );

  if(inContext->flashContentInRam.micoSystemConfig.configured == wLanUnConfigured){
    /*You can upload a specific menu*/
  }

  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  snprintf(name, 50, "%s(%c%c%c%c%c%c)",MODEL, 
                                        inContext->micoStatus.mac[9],  inContext->micoStatus.mac[10], 
                                        inContext->micoStatus.mac[12], inContext->micoStatus.mac[13],
                                        inContext->micoStatus.mac[15], inContext->micoStatus.mac[16]);

  versions.fwVersion = FIRMWARE_REVISION;
  versions.hdVersion = HARDWARE_REVISION;
  versions.protocol =  PROTOCOL;
  versions.rfVersion = NULL;

  sectors = json_object_new_array();
  require( sectors, exit );

  err = MICOAddTopMenu(&mainObject, name, sectors, versions);
  require_noerr(err, exit);

  /*Sector 1*/
  sector = json_object_new_array();
  require( sector, exit );
  err = MICOAddSector(sectors, "MICO SYSTEM",    sector);
  require_noerr(err, exit);

    /*name cell*/
    err = MICOAddStringCellToSector(sector, "Device Name",    inContext->flashContentInRam.micoSystemConfig.name,               "RW", NULL);
    require_noerr(err, exit);

    //RF power save switcher cell
    err = MICOAddSwitchCellToSector(sector, "RF power save",  inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable,  "RW");
    require_noerr(err, exit);

    //MCU power save switcher cell
    err = MICOAddSwitchCellToSector(sector, "MCU power save", inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable, "RW");
    require_noerr(err, exit);

    /*sub menu*/
    subMenuSectors = json_object_new_array();
    require( subMenuSectors, exit );
    err = MICOAddMenuCellToSector(sector, "Detail", subMenuSectors);
    require_noerr(err, exit);
      
      subMenuSector = json_object_new_array();
      require( subMenuSector, exit );
      err = MICOAddSector(subMenuSectors,  "",    subMenuSector);
      require_noerr(err, exit);

        err = MICOAddStringCellToSector(subMenuSector, "Firmware Rev.",  FIRMWARE_REVISION, "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "Hardware Rev.",  HARDWARE_REVISION, "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "MICO OS Rev.",   MicoGetVer(),      "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "RF Driver Rev.", rfVersion,         "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "Model",          MODEL,             "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "Manufacturer",   MANUFACTURER,      "RO", NULL);
        require_noerr(err, exit);
        err = MICOAddStringCellToSector(subMenuSector, "Protocol",       PROTOCOL,          "RO", NULL);
        require_noerr(err, exit);

      subMenuSector = json_object_new_array();
      err = MICOAddSector(subMenuSectors,  "WLAN",    subMenuSector);
      require_noerr(err, exit);
      
        tempString = DataToHexStringWithColons( (uint8_t *)inContext->flashContentInRam.micoSystemConfig.bssid, 6 );
        err = MICOAddStringCellToSector(subMenuSector, "BSSID",        tempString, "RO", NULL);
        require_noerr(err, exit);
        free(tempString);

        err = MICOAddNumberCellToSector(subMenuSector, "Channel",      inContext->flashContentInRam.micoSystemConfig.channel, "RO", NULL);
        require_noerr(err, exit);

        switch(inContext->flashContentInRam.micoSystemConfig.security){
          case SECURITY_TYPE_NONE:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "Open system", "RO", NULL); 
            break;
          case SECURITY_TYPE_WEP:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WEP",         "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA_TKIP:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WPA TKIP",    "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA_AES:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WPA AES",     "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_TKIP:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WPA2 TKIP",   "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_AES:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WPA2 AES",    "RO", NULL); 
            break;
          case SECURITY_TYPE_WPA2_MIXED:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "WPA2 MIXED",  "RO", NULL); 
            break;
          default:
            err = MICOAddStringCellToSector(subMenuSector, "Security",   "Auto",      "RO", NULL); 
            break;
        }
        require_noerr(err, exit); 

        if(inContext->flashContentInRam.micoSystemConfig.keyLength == maxKeyLen){ /*This is a PMK key, generated by user key in WPA security type*/
          tempString = calloc(maxKeyLen+1, 1);
          require_action(tempString, exit, err=kNoMemoryErr);
          memcpy(tempString, inContext->flashContentInRam.micoSystemConfig.key, maxKeyLen);
          err = MICOAddStringCellToSector(subMenuSector, "PMK",          tempString, "RO", NULL);
          require_noerr(err, exit);
          free(tempString);
        }
        else{
          err = MICOAddStringCellToSector(subMenuSector, "KEY",          inContext->flashContentInRam.micoSystemConfig.user_key,  "RO", NULL);
          require_noerr(err, exit);
        }

  /*Sector 3*/
  sector = json_object_new_array();
  require( sector, exit );
  err = MICOAddSector(sectors, "WLAN",           sector);
  require_noerr(err, exit);
    /*SSID cell*/
    err = MICOAddStringCellToSector(sector, "Wi-Fi",        inContext->flashContentInRam.micoSystemConfig.ssid,     "RW", NULL);
    require_noerr(err, exit);
    /*PASSWORD cell*/
    err = MICOAddStringCellToSector(sector, "Password",     inContext->flashContentInRam.micoSystemConfig.user_key, "RW", NULL);
    require_noerr(err, exit);
    /*DHCP cell*/
    err = MICOAddSwitchCellToSector(sector, "DHCP",        inContext->flashContentInRam.micoSystemConfig.dhcpEnable,   "RW");
    require_noerr(err, exit);
    /*Local cell*/
    err = MICOAddStringCellToSector(sector, "IP address",  inContext->micoStatus.localIp,   "RW", NULL);
    require_noerr(err, exit);
    /*Netmask cell*/
    err = MICOAddStringCellToSector(sector, "Net Mask",    inContext->micoStatus.netMask,   "RW", NULL);
    require_noerr(err, exit);
    /*Gateway cell*/
    err = MICOAddStringCellToSector(sector, "Gateway",     inContext->micoStatus.gateWay,   "RW", NULL);
    require_noerr(err, exit);
    /*DNS server cell*/
    err = MICOAddStringCellToSector(sector, "DNS Server",  inContext->micoStatus.dnsServer, "RW", NULL);
    require_noerr(err, exit);

  /*Sector 4*/
  sector = json_object_new_array();
  require( sector, exit );
  err = MICOAddSector(sectors, "SPP Remote Server",           sector);
  require_noerr(err, exit);

    // SPP protocol remote server connection enable
    err = MICOAddSwitchCellToSector(sector, "Connect SPP Server",   appConfig->remoteServerEnable,   "RW");
    require_noerr(err, exit);

    //Seerver address cell
    err = MICOAddStringCellToSector(sector, "SPP Server",           appConfig->remoteServerDomain,   "RW", NULL);
    require_noerr(err, exit);

    //Seerver port cell
    err = MICOAddNumberCellToSector(sector, "SPP Server Port",      appConfig->remoteServerPort,   "RW", NULL);
    require_noerr(err, exit);

  /*Sector 5*/
  sector = json_object_new_array();
  require( sector, exit );
  err = MICOAddSector(sectors, "MCU IOs",            sector);
  require_noerr(err, exit);

    /*UART Baurdrate cell*/
    json_object *selectArray;
    selectArray = json_object_new_array();
    require( selectArray, exit );
    json_object_array_add(selectArray, json_object_new_int(9600));
    json_object_array_add(selectArray, json_object_new_int(19200));
    json_object_array_add(selectArray, json_object_new_int(38400));
    json_object_array_add(selectArray, json_object_new_int(57600));
    json_object_array_add(selectArray, json_object_new_int(115200));
    err = MICOAddNumberCellToSector(sector, "Baurdrate", 115200, "RW", selectArray);
    require_noerr(err, exit);

  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  
exit:
  if(err != kNoErr && mainObject){
    json_object_put(mainObject);
    mainObject = NULL;
  }
  return mainObject;
}

OSStatus ConfigIncommingJsonMessage( const char *input, bool *need_reboot, mico_Context_t * const inContext )
{
  config_delegate_log_trace();
  OSStatus err = kNoErr;
  json_object *new_obj;
  application_config_t *appConfig = mico_system_context_get_user_data( inContext );

  *need_reboot = false;
  new_obj = json_tokener_parse(input);
  require_action(new_obj, exit, err = kUnknownErr);
  config_delegate_log("Recv config object=%s", json_object_to_json_string(new_obj));
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  json_object_object_foreach(new_obj, key, val) {
    if(!strcmp(key, "Device Name")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.name, json_object_get_string(val), maxNameLen);
      *need_reboot = true;
    }else if(!strcmp(key, "RF power save")){
      inContext->flashContentInRam.micoSystemConfig.rfPowerSaveEnable = json_object_get_boolean(val);
      *need_reboot = true;
    }else if(!strcmp(key, "MCU power save")){
      inContext->flashContentInRam.micoSystemConfig.mcuPowerSaveEnable = json_object_get_boolean(val);
      *need_reboot = true;
    }else if(!strcmp(key, "Wi-Fi")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.ssid, json_object_get_string(val), maxSsidLen);
      inContext->flashContentInRam.micoSystemConfig.channel = 0;
      memset(inContext->flashContentInRam.micoSystemConfig.bssid, 0x0, 6);
      inContext->flashContentInRam.micoSystemConfig.security = SECURITY_TYPE_AUTO;
      memcpy(inContext->flashContentInRam.micoSystemConfig.key, inContext->flashContentInRam.micoSystemConfig.user_key, maxKeyLen);
      inContext->flashContentInRam.micoSystemConfig.keyLength = inContext->flashContentInRam.micoSystemConfig.user_keyLength;
      *need_reboot = true;
    }else if(!strcmp(key, "Password")){
      inContext->flashContentInRam.micoSystemConfig.security = SECURITY_TYPE_AUTO;
      strncpy(inContext->flashContentInRam.micoSystemConfig.key, json_object_get_string(val), maxKeyLen);
      strncpy(inContext->flashContentInRam.micoSystemConfig.user_key, json_object_get_string(val), maxKeyLen);
      inContext->flashContentInRam.micoSystemConfig.keyLength = strlen(inContext->flashContentInRam.micoSystemConfig.key);
      inContext->flashContentInRam.micoSystemConfig.user_keyLength = strlen(inContext->flashContentInRam.micoSystemConfig.key);
      *need_reboot = true;
    }else if(!strcmp(key, "DHCP")){
      inContext->flashContentInRam.micoSystemConfig.dhcpEnable   = json_object_get_boolean(val);
      *need_reboot = true;
    }else if(!strcmp(key, "IP address")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.localIp, json_object_get_string(val), maxIpLen);
      *need_reboot = true;
    }else if(!strcmp(key, "Net Mask")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.netMask, json_object_get_string(val), maxIpLen);
      *need_reboot = true;
    }else if(!strcmp(key, "Gateway")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.gateWay, json_object_get_string(val), maxIpLen);
      *need_reboot = true;
    }else if(!strcmp(key, "DNS Server")){
      strncpy(inContext->flashContentInRam.micoSystemConfig.dnsServer, json_object_get_string(val), maxIpLen);
      *need_reboot = true;
    }else if(!strcmp(key, "Connect SPP Server")){
      appConfig->remoteServerEnable = json_object_get_boolean(val);
      *need_reboot = true;
    }else if(!strcmp(key, "SPP Server")){
      strncpy(appConfig->remoteServerDomain, json_object_get_string(val), 64);
      *need_reboot = true;
    }else if(!strcmp(key, "SPP Server Port")){
      appConfig->remoteServerPort = json_object_get_int(val);
      *need_reboot = true;
    }else if(!strcmp(key, "Baurdrate")){
      appConfig->USART_BaudRate = json_object_get_int(val);
      *need_reboot = true;
    }
  }
  json_object_put(new_obj);
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);

exit:
  return err; 
}