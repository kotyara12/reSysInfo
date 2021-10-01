#include <stdio.h>
#include <string.h>
#include "reSysInfo.h"
#include "rLog.h"
#include "rStrings.h"
#include "reStates.h"
#include "reEvents.h"
#include "reWiFi.h"
#include "project_config.h"
#if CONFIG_MQTT_TIME_ENABLE
#include "reMqtt.h"
#endif // CONFIG_MQTT_TIME_ENABLE

static const char* logTAG = "SINF";

// -----------------------------------------------------------------------------------------------------------------------
// ----------------------------------------------------- Date and time ---------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

// Operating time of the device in days, hours and minutes
static worktime_t _worktime = {0, 0, 0};

// String representations of the current date and time
static char _strTime[10];
static char _strDate[12];
static char _strWeekDay[12];
static char _strTimeDay[32];
static char _strDatetime1[32];
static char _strDatetime2[48];

void sysinfoWorkTimeInc()
{
  _worktime.minutes++;
  if (_worktime.minutes > 59) {
    _worktime.minutes = 0;
    _worktime.hours++;
    if (_worktime.hours > 23) {
      _worktime.hours = 0;
      _worktime.days++;
    };
  };
}

worktime_t getWorkTime()
{
  return _worktime;
}

void sysinfoFixDateTime(const struct tm currTime)
{
  strftime(_strTime, sizeof(_strTime), CONFIG_FORMAT_TIME, &currTime);
  strftime(_strDate, sizeof(_strDate), CONFIG_FORMAT_DATE, &currTime);
  switch (currTime.tm_wday) {
  case 1:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY1);
    break;
  case 2:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY2);
    break;
  case 3:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY3);
    break;
  case 4:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY4);
    break;
  case 5:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY5);
    break;
  case 6:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY6);
    break;
  case 0:
    strcpy(_strWeekDay, CONFIG_FORMAT_WDAY7);
    break;
  };
  #if CONFIG_FORMAT_WT
    snprintf(_strTimeDay, sizeof(_strTimeDay), CONFIG_FORMAT_TIMEDAY, _strWeekDay, _strTime);
  #else
    snprintf(_strTimeDay, sizeof(_strTimeDay), CONFIG_FORMAT_TIMEDAY, _strTime, _strWeekDay);
  #endif // CONFIG_FORMAT_WTD
  #if CONFIG_FORMAT_TD
    snprintf(_strDatetime1, sizeof(_strDatetime1), CONFIG_FORMAT_DATETIME1, _strTime, _strDate);
    snprintf(_strDatetime2, sizeof(_strDatetime2), CONFIG_FORMAT_DATETIME2, _strTimeDay, _strDate);
  #else 
    snprintf(_strDatetime1, sizeof(_strDatetime1), CONFIG_FORMAT_DATETIME1, _strDate, _strTime);
    snprintf(_strDatetime2, sizeof(_strDatetime2), CONFIG_FORMAT_DATETIME2, _strDate, _strTimeDay);
  #endif // CONFIG_FORMAT_TD
}

str_datetime_t getDateTimeStrings()
{
  static str_datetime_t ret;
  ret.date = (char*)&_strDate;
  ret.time = (char*)&_strTime;
  ret.weekday = (char*)&_strWeekDay;
  ret.timeday = (char*)&_strTimeDay;
  ret.datetime_s = (char*)&_strDatetime1;
  ret.datetime_l = (char*)&_strDatetime2;
  return ret;
}

// -----------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Publish Date & Time -------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

#if CONFIG_MQTT_TIME_ENABLE

static char* _mqttTopicTime = NULL;

char* mqttTopicDateTimeCreate(const bool primary)
{
  if (_mqttTopicTime) free(_mqttTopicTime);
  _mqttTopicTime = mqttGetTopic1(primary, false, CONFIG_MQTT_TIME_TOPIC);
  rlog_i(logTAG, "Generated topic for publishing date and time: [ %s ]", _mqttTopicTime);
  return _mqttTopicTime;
}

char* mqttTopicDateTimeGet()
{
  return _mqttTopicTime;
}

void mqttTopicDateTimeFree()
{
  if (_mqttTopicTime) free(_mqttTopicTime);
  _mqttTopicTime = nullptr;
  rlog_d(logTAG, "Topic for publishing date and time has been scrapped");
}

void mqttPublishDateTime(const struct tm timeinfo)
{
  if ((_mqttTopicTime) && mqttIsConnected()) {
    // rlog_d(logTAG, "Date and time publishing...");
    
    #if CONFIG_MQTT_TIME_AS_PLAIN
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "time"), _strTime, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "date"), _strDate, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "weekday"), _strWeekDay, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "timeday"), _strTimeDay, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "datetime1"), _strDatetime1, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "datetime2"), _strDatetime2, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "year"), malloc_stringf("%d", timeinfo.tm_year+1900), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "month"), malloc_stringf("%d", timeinfo.tm_mon), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "day"), malloc_stringf("%d", timeinfo.tm_mday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "hour"), malloc_stringf("%d", timeinfo.tm_hour), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "min"), malloc_stringf("%d", timeinfo.tm_min), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "wday"), malloc_stringf("%d", timeinfo.tm_wday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "yday"), malloc_stringf("%d", timeinfo.tm_yday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/days"), malloc_stringf("%d", _worktime.days), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/hours"), malloc_stringf("%d", _worktime.hours), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/minutes"), malloc_stringf("%d", _worktime.minutes), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true, true);
    #endif // CONFIG_MQTT_TIME_AS_PLAIN

    #if CONFIG_MQTT_TIME_AS_JSON
      char * json = malloc_stringf("{\"time\":\"%s\",\"date\":\"%s\",\"weekday\":\"%s\",\"timeday\":\"%s\",\"datetime1\":\"%s\",\"datetime2\":\"%s\",\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"min\":%d,\"wday\":%d,\"yday\":%d,\"worktime\":{\"days\":%d,\"hours\":%d,\"minutes\":%d}}", 
        _strTime, _strDate,  _strWeekDay, _strTimeDay, _strDatetime1, _strDatetime2, 
        timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_wday, timeinfo.tm_yday,
        _worktime.days, _worktime.hours, _worktime.minutes);
      if (json) mqttPublish(_mqttTopicTime, json, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false, true);
    #endif // CONFIG_MQTT_TIME_AS_JSON
  };
};
#endif // CONFIG_MQTT_TIME_ENABLE

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------- Publish system information ---------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

#if CONFIG_MQTT_SYSINFO_ENABLE

static char* _mqttTopicSysInfo = NULL;

char* mqttTopicSysInfoCreate(const bool primary)
{
  if (_mqttTopicSysInfo) free(_mqttTopicSysInfo);
  _mqttTopicSysInfo = mqttGetTopic1(primary, CONFIG_MQTT_SYSINFO_LOCAL, CONFIG_MQTT_SYSINFO_TOPIC);
  rlog_i(logTAG, "Generated topic for publishing system info: [ %s ]", _mqttTopicSysInfo);
  return _mqttTopicSysInfo;
}

char* mqttTopicSysInfoGet()
{
  return _mqttTopicSysInfo;
}

void mqttTopicSysInfoFree()
{
  if (_mqttTopicSysInfo) free(_mqttTopicSysInfo);
  _mqttTopicSysInfo = nullptr;
  rlog_d(logTAG, "Topic for publishing system info has been scrapped");
}

#endif // CONFIG_MQTT_SYSINFO_ENABLE

#if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE

void sysinfoPublishSysInfo()
{
  double heap_total = (double)heap_caps_get_total_size(MALLOC_CAP_DEFAULT) / 1024.0;
  double heap_free  = (double)heap_caps_get_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
  double heap_min   = (double)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT) / 1024.0;
  wifi_ap_record_t wifi_info = wifiInfo();
  esp_netif_ip_info_t wifi_ip = wifiLocalIP();
  uint8_t * ip = (uint8_t*)&(wifi_ip.ip);
  uint8_t * mask = (uint8_t*)&(wifi_ip.netmask);
  uint8_t * gw = (uint8_t*)&(wifi_ip.gw);

  rlog_d(logTAG, "Heap total: %.2f kB, free: %.2f kB (%.0f%%), minimum: %.2f kB (%.0f%%)",
    heap_total, heap_free, 100.0*heap_free/heap_total, heap_min, 100.0*heap_min/heap_total);

  if (statesWiFiIsConnected()) {
    rlog_d(logTAG, "System information publishing...");
    
    if (statesMqttIsConnected()) {
      char * s_status = malloc_stringf("%.2d : %.2d : %.2d\nRSSI: %d dBi\n%.1fKb %.0f%%",
        _worktime.days, _worktime.hours, _worktime.minutes, wifi_info.rssi, heap_free, 100.0*heap_free/heap_total);
        
      if (s_status) {
        #if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO
          if (mqttTopicStatusGet()) {
              #if CONFIG_MQTT_STATUS_ONLINE_SYSINFO
                mqttPublish(mqttTopicStatusGet(), malloc_stringf("%s", s_status), 
                  CONFIG_MQTT_STATUS_QOS, CONFIG_MQTT_STATUS_RETAINED, true, false, true);
              #else
                #if CONFIG_MQTT_STATUS_ONLINE
                  mqttPublish(mqttTopicStatusGet(), (char*)CONFIG_MQTT_STATUS_ONLINE_PAYLOAD, 
                    CONFIG_MQTT_STATUS_QOS, CONFIG_MQTT_STATUS_RETAINED, true, false, false);
                #endif // CONFIG_MQTT_STATUS_ONLINE
              #endif // CONFIG_MQTT_STATUS_ONLINE_SYSINFO
          };
        #endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO
        
        #if CONFIG_MQTT_SYSINFO_ENABLE
          if (_mqttTopicSysInfo) {
            char * s_wifi = malloc_stringf("{\"ssid\":\"%s\",\"rssi\":%d,\"ip\":\"%d.%d.%d.%d\",\"mask\":\"%d.%d.%d.%d\",\"gw\":\"%d.%d.%d.%d\"}",
              wifi_info.ssid, wifi_info.rssi,
              ip[0], ip[1], ip[2], ip[3], mask[0], mask[1], mask[2], mask[3], gw[0], gw[1], gw[2], gw[3]);
            char * s_work = malloc_stringf("{\"days\":%d,\"hours\":%d,\"minutes\":%d}",
              _worktime.days, _worktime.hours, _worktime.minutes);
            char * s_heap = malloc_stringf("{\"total\":\"%.1f\",\"free\":\"%.1f\",\"free_percents\":\"%.0f\",\"free_min\":\"%.1f\",\"free_min_percents\":\"%.0f\"}",
              heap_total, heap_free, 100.0*heap_free/heap_total, heap_min, 100.0*heap_min/heap_total);
            
            if ((s_wifi) && (s_work) && (s_heap) && (s_status)) {
              char * json = malloc_stringf("{\"firmware\":\"%s\",\"wifi\":%s,\"worktime\":%s,\"heap\":%s,\"summary\":\"%s\"}", 
                APP_VERSION, s_wifi, s_work, s_heap, s_status);
              if (json) mqttPublish(_mqttTopicSysInfo, json, 
                CONFIG_MQTT_SYSINFO_QOS, CONFIG_MQTT_SYSINFO_RETAINED, false, false, true);
            };

            if (s_heap) free(s_heap);
            if (s_work) free(s_work);
            if (s_wifi) free(s_wifi);
          };
        #endif // CONFIG_MQTT_SYSINFO_ENABLE

        free(s_status);
      };
    };
  }
  else {
    rlog_w(logTAG, "Failed to publish system information - WiFi not connected");
  }
};
#endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE

// -----------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------- Event handlers ---------------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------

static void sysinfoMqttEventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  // MQTT connected
  if (event_id == RE_MQTT_CONNECTED) {
    re_mqtt_event_data_t* data = (re_mqtt_event_data_t*)event_data;

    #if CONFIG_MQTT_TIME_ENABLE
    mqttTopicDateTimeCreate(data->primary);
    #endif // CONFIG_MQTT_TIME_ENABLE

    #if !CONFIG_MQTT_STATUS_LWT && (CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO)
    mqttTopicStatusCreate(data->primary);
    #endif // !CONFIG_MQTT_STATUS_LWT && (CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO)

    #if CONFIG_MQTT_SYSINFO_ENABLE
    mqttTopicSysInfoCreate(data->primary);
    #endif // CONFIG_MQTT_SYSINFO_ENABLE

    #if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE
    sysinfoPublishSysInfo();
    #endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE
  } 

  // MQTT disconnected
  else if ((event_id == RE_MQTT_CONN_LOST) || (event_id == RE_MQTT_CONN_FAILED)) {

    #if CONFIG_MQTT_TIME_ENABLE
    mqttTopicDateTimeFree();
    #endif // CONFIG_MQTT_TIME_ENABLE

    #if CONFIG_MQTT_STATUS_LWT || CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO
    mqttTopicStatusFree();
    #endif // CONFIG_MQTT_STATUS_LWT || CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO

    #if CONFIG_MQTT_SYSINFO_ENABLE
    mqttTopicSysInfoFree();
    #endif // CONFIG_MQTT_SYSINFO_ENABLE
  };
}

bool sysinfoEventHandlerRegister()
{
  return eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONNECTED, &sysinfoMqttEventHandler, nullptr)
      && eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONN_LOST, &sysinfoMqttEventHandler, nullptr);
};

bool sysinfoEventHandlerUnregister()
{
  return eventHandlerUnregister(RE_MQTT_EVENTS, RE_MQTT_CONNECTED, &sysinfoMqttEventHandler)
      && eventHandlerUnregister(RE_MQTT_EVENTS, RE_MQTT_CONN_LOST, &sysinfoMqttEventHandler);
};