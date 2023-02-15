#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "reSysInfo.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "soc/rtc.h" 
#include "rLog.h"
#include "rStrings.h"
#include "reStates.h"
#include "reEvents.h"
#include "reWiFi.h"
#include "project_config.h"
#include "sdkconfig.h"
#include "def_consts.h"
#include "reEsp32.h"
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

void sysinfoFixDateTime(const struct tm* currTime)
{
  strftime(_strTime, sizeof(_strTime), CONFIG_FORMAT_TIME, currTime);
  strftime(_strDate, sizeof(_strDate), CONFIG_FORMAT_DATE, currTime);
  switch (currTime->tm_wday) {
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
  _mqttTopicTime = mqttGetTopicDevice1(primary, false, CONFIG_MQTT_TIME_TOPIC);
  if (_mqttTopicTime) {
    rlog_i(logTAG, "Generated topic for publishing date and time: [ %s ]", _mqttTopicTime);
  } else {
    rlog_e(logTAG, "Failed to generate topic for publishing date and time");
  };
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

void mqttPublishDateTime(const struct tm* timeinfo)
{
  if ((_mqttTopicTime) && statesMqttIsEnabled()) {
    // rlog_d(logTAG, "Date and time publishing...");
    
    #if CONFIG_MQTT_TIME_AS_PLAIN
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "time"), _strTime, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "date"), _strDate, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "weekday"), _strWeekDay, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "timeday"), _strTimeDay, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "datetime1"), _strDatetime1, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "datetime2"), _strDatetime2, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, false);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "year"), malloc_stringf("%d", timeinfo->tm_year+1900), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "month"), malloc_stringf("%d", timeinfo->tm_mon), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "day"), malloc_stringf("%d", timeinfo->tm_mday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "hour"), malloc_stringf("%d", timeinfo->tm_hour), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "min"), malloc_stringf("%d", timeinfo->tm_min), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "wday"), malloc_stringf("%d", timeinfo->tm_wday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "yday"), malloc_stringf("%d", timeinfo->tm_yday), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/days"), malloc_stringf("%d", _worktime.days), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/hours"), malloc_stringf("%d", _worktime.hours), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
      mqttPublish(mqttGetSubTopic(_mqttTopicTime, "worktime/minutes"), malloc_stringf("%d", _worktime.minutes), CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, true, true);
    #endif // CONFIG_MQTT_TIME_AS_PLAIN

    #if CONFIG_MQTT_TIME_AS_JSON
      char * json = malloc_stringf("{\"time\":\"%s\",\"date\":\"%s\",\"weekday\":\"%s\",\"timeday\":\"%s\",\"datetime1\":\"%s\",\"datetime2\":\"%s\",\"year\":%d,\"month\":%d,\"day\":%d,\"hour\":%d,\"min\":%d,\"wday\":%d,\"yday\":%d,\"worktime\":{\"days\":%d,\"hours\":%d,\"minutes\":%d}}", 
        _strTime, _strDate,  _strWeekDay, _strTimeDay, _strDatetime1, _strDatetime2, 
        timeinfo->tm_year+1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_wday, timeinfo->tm_yday,
        _worktime.days, _worktime.hours, _worktime.minutes);
      if (json) mqttPublish(_mqttTopicTime, json, CONFIG_MQTT_TIME_QOS, CONFIG_MQTT_TIME_RETAINED, false, true);
    #endif // CONFIG_MQTT_TIME_AS_JSON
  } else {
    rlog_w(logTAG, "Failed to publish date and time - MQTT is not connected");
  }
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
  _mqttTopicSysInfo = mqttGetTopicDevice1(primary, CONFIG_MQTT_SYSINFO_LOCAL, CONFIG_MQTT_SYSINFO_TOPIC);
  if (_mqttTopicSysInfo) {
    rlog_i(logTAG, "Generated topic for publishing system info: [ %s ]", _mqttTopicSysInfo);
  } else {
    rlog_e(logTAG, "Failed to generate topic for publishing system info");
  };
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
  nvs_stats_t nvs_stats;
  nvs_get_stats(NULL, &nvs_stats);
  rtc_cpu_freq_config_t cpu;
  rtc_clk_cpu_freq_get_config(&cpu);
  
  // rlog_d(logTAG, "Heap total: %.2f kB, free: %.2f kB (%.0f%%), minimum: %.2f kB (%.0f%%)",
  //   heap_total, heap_free, 100.0*heap_free/heap_total, heap_min, 100.0*heap_min/heap_total);

  if (statesMqttIsEnabled()) {
    rlog_d(logTAG, "System information publishing...");
    char * s_status = malloc_stringf("%.2d : %.2d : %.2d\nRSSI: %d dBi\n%.0f%% %d %.0f%%",
      _worktime.days, _worktime.hours, _worktime.minutes, wifi_info.rssi, 
      100.0*heap_free/heap_total, heapAllocFailedCount(),
      100.0*nvs_stats.free_entries/nvs_stats.total_entries);
      
    if (s_status) {
      #if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO
        if (mqttTopicStatusGet()) {
            #if CONFIG_MQTT_STATUS_ONLINE_SYSINFO
              mqttPublish(mqttTopicStatusGet(), malloc_stringf("%s", s_status), 
                CONFIG_MQTT_STATUS_QOS, CONFIG_MQTT_STATUS_RETAINED, false, true);
            #else
              #if CONFIG_MQTT_STATUS_ONLINE
                mqttPublish(mqttTopicStatusGet(), (char*)CONFIG_MQTT_STATUS_ONLINE_PAYLOAD, 
                  CONFIG_MQTT_STATUS_QOS, CONFIG_MQTT_STATUS_RETAINED, false, false);
              #endif // CONFIG_MQTT_STATUS_ONLINE
            #endif // CONFIG_MQTT_STATUS_ONLINE_SYSINFO
        };
      #endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO
      
      #if CONFIG_MQTT_SYSINFO_ENABLE
        if (esp_heap_free_check() && _mqttTopicSysInfo) {
          char * s_wifi = malloc_stringf("{\"ssid\":\"%s\",\"rssi\":%d,\"ip\":\"%d.%d.%d.%d\",\"mask\":\"%d.%d.%d.%d\",\"gw\":\"%d.%d.%d.%d\"}",
            wifi_info.ssid, wifi_info.rssi,
            ip[0], ip[1], ip[2], ip[3], mask[0], mask[1], mask[2], mask[3], gw[0], gw[1], gw[2], gw[3]);
          char * s_work = malloc_stringf("{\"days\":%d,\"hours\":%d,\"minutes\":%d}",
            _worktime.days, _worktime.hours, _worktime.minutes);
          char * s_heap = malloc_stringf("{\"total\":%.1f,\"errors\":%d,\"free\":%.1f,\"free_percents\":%.1f,\"free_min\":%.1f,\"free_min_percents\":%.1f}",
            heap_total, heapAllocFailedCount(), 
            heap_free, 100.0*heap_free/heap_total, 
            heap_min, 100.0*heap_min/heap_total);
          char * s_nvs = malloc_stringf("{\"total_entries\":%d,\"used_entries\":%d,\"used_percents\":%.1f,\"free_entries\":%d,\"free_percents\":%.1f,\"namespaces\":%d}",
            nvs_stats.total_entries, 
            nvs_stats.used_entries, 100.0*nvs_stats.used_entries/nvs_stats.total_entries,
            nvs_stats.free_entries, 100.0*nvs_stats.free_entries/nvs_stats.total_entries,
            nvs_stats.namespace_count);
          
          #if defined(CONFIG_MQTT_SYSINFO_SYSTEM_FLAGS) && CONFIG_MQTT_SYSINFO_SYSTEM_FLAGS
            char * s_sys_errors = statesGetErrorsJson();
            char * s_sys_flags = statesGetJson();
            char * s_wifi_flags = wifiStatusGetJson();

            if ((s_wifi) && (s_work) && (s_heap) && (s_nvs) && (s_sys_errors) && (s_sys_flags) && (s_wifi_flags)) {
              char * json = malloc_stringf("{\"firmware\":\"%s\",\"cpu_mhz\":%d,\"wifi\":%s,\"worktime\":%s,\"heap\":%s,\"nvs\":%s,\"errors\":%s,\"sys_flags\":%s,\"wifi_flags\":%s}", 
                APP_VERSION, cpu.freq_mhz, s_wifi, s_work, s_heap, s_nvs, s_sys_errors, s_sys_flags, s_wifi_flags);
              if (json) mqttPublish(_mqttTopicSysInfo, json, 
                CONFIG_MQTT_SYSINFO_QOS, CONFIG_MQTT_SYSINFO_RETAINED, false, true);
            };

            if (s_wifi_flags) free(s_wifi_flags);
            if (s_sys_flags) free(s_sys_flags);
            if (s_sys_errors) free(s_sys_errors);
          #else
            if ((s_wifi) && (s_work) && (s_heap) && (s_nvs)) {
              char * json = malloc_stringf("{\"firmware\":\"%s\",\"cpu_mhz\":%d,\"wifi\":%s,\"worktime\":%s,\"heap\":%s,\"nvs\":%s}", 
                APP_VERSION, cpu.freq_mhz, s_wifi, s_work, s_heap, s_nvs);
              if (json) mqttPublish(_mqttTopicSysInfo, json, 
                CONFIG_MQTT_SYSINFO_QOS, CONFIG_MQTT_SYSINFO_RETAINED, false, true);
            };
          #endif // CONFIG_MQTT_SYSINFO_SYSTEM_FLAGS
          
          if (s_nvs)  free(s_nvs);
          if (s_heap) free(s_heap);
          if (s_work) free(s_work);
          if (s_wifi) free(s_wifi);
        };
      #endif // CONFIG_MQTT_SYSINFO_ENABLE

      free(s_status);
    };
  } else {
    rlog_w(logTAG, "Failed to publish system information - MQTT is not connected");
  }
};
#endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE

// -----------------------------------------------------------------------------------------------------------------------
// --------------------------------------------- Publish tasklist information --------------------------------------------
// -----------------------------------------------------------------------------------------------------------------------
#if CONFIG_MQTT_TASKSHOW_ENABLE

const char* sysinfoShowTaskState(eTaskState state)
{
  switch (state) {
    case eRunning:   return "R";
    case eReady:     return "W";
    case eBlocked:   return "B";
    case eSuspended: return "S";
    case eDeleted:   return "D";
    default:         return "I";
  };
}

void sysinfoShowTaskList(void* arg)
{
  TaskStatus_t *pxTaskStatusArray = nullptr;
  volatile UBaseType_t uxArraySize;
  uint32_t ulTotalRunTime;

  // Take a snapshot of the number of tasks in case it changes while this function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();
  // Allocate a TaskStatus_t structure for each task.  An array could be allocated statically at compile time.
  pxTaskStatusArray = (TaskStatus_t*)esp_malloc(uxArraySize * sizeof(TaskStatus_t));
  if (pxTaskStatusArray) {
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
    rlog_w("TASK", "+----+-------------+---+---+----+----+----------+------+");
    rlog_w("TASK", "| id | task name   | c | s | cp | bp |    stack | mark |");
    rlog_w("TASK", "+----+-------------+---+---+----+----+----------+------+");
    // For each populated position in the pxTaskStatusArray array
    for (UBaseType_t x=0; x<uxArraySize; x++) {
      rlog_w("TASK", "| %02d | %11.11s | %d | %1.1s | %02d | %02d | %08x | %04d |",
        pxTaskStatusArray[x].xTaskNumber, 
        pxTaskStatusArray[x].pcTaskName, 
        #ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
        pxTaskStatusArray[x].xCoreID > 1 ? 8 : pxTaskStatusArray[x].xCoreID, 
        #else
        8,
        #endif // CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
        sysinfoShowTaskState(pxTaskStatusArray[x].eCurrentState), 
        pxTaskStatusArray[x].uxCurrentPriority, pxTaskStatusArray[x].uxBasePriority,
        pxTaskStatusArray[x].pxStackBase, pxTaskStatusArray[x].usStackHighWaterMark);
    };
    rlog_w("TASK", "+----+-------------+---+---+----+----+----------+------+");
    // The array is no longer needed, free the memory it consumes
    vPortFree(pxTaskStatusArray);
  };
}

esp_err_t sysinfoShowTaskStartTimer()
{
  static esp_timer_handle_t _timerTasks;
  esp_timer_create_args_t timer_args;
  memset(&timer_args, 0, sizeof(esp_timer_create_args_t));
  timer_args.callback = &sysinfoShowTaskList;
  timer_args.name = "show_tasks";
  RE_ERROR_CHECK(esp_timer_create(&timer_args, &_timerTasks));
  RE_ERROR_CHECK(esp_timer_start_periodic(_timerTasks, CONFIG_MQTT_TASKSHOW_INTERVAL));
  return ESP_OK;
}

#endif // CONFIG_MQTT_TASKSHOW_ENABLE

#if CONFIG_MQTT_TASKLIST_ENABLE

static char* _mqttTopicTaskList = NULL;

char* mqttTopicTaskListCreate(const bool primary)
{
  if (_mqttTopicTaskList) free(_mqttTopicTaskList);
  _mqttTopicTaskList = mqttGetTopicDevice1(primary, CONFIG_MQTT_TASKLIST_LOCAL, CONFIG_MQTT_TASKLIST_TOPIC);
  rlog_i(logTAG, "Generated topic for publishing task list: [ %s ]", _mqttTopicTaskList);
  return _mqttTopicTaskList;
}

char* mqttTopicTaskListGet()
{
  return _mqttTopicTaskList;
}

void mqttTopicTaskListFree()
{
  if (_mqttTopicTaskList) free(_mqttTopicTaskList);
  _mqttTopicTaskList = nullptr;
  rlog_d(logTAG, "Topic for publishing task list has been scrapped");
}

const char* sysinfoTaskListState(eTaskState state)
{
  switch (state) {
    case eRunning:   return "running";
    case eReady:     return "ready";
    case eBlocked:   return "blocked";
    case eSuspended: return "suspended";
    case eDeleted:   return "deleted";
    default:         return "invalid";
  };
}

void sysinfoPublishTaskList()
{
  if (esp_heap_free_check() && statesMqttIsEnabled() && (_mqttTopicTaskList)) {
    TaskStatus_t *pxTaskStatusArray = nullptr;
    volatile UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime;

    // Take a snapshot of the number of tasks in case it changes while this function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();
    // Allocate a TaskStatus_t structure for each task.  An array could be allocated statically at compile time.
    pxTaskStatusArray = (TaskStatus_t*)esp_malloc(uxArraySize * sizeof(TaskStatus_t));
    if (pxTaskStatusArray) {
      char * json_task = nullptr;
      char * json_summary = nullptr;
      // Generate raw status information about each task.
      uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
      // For each populated position in the pxTaskStatusArray array
      for (UBaseType_t x=0; x<uxArraySize; x++) {
        if (ulTotalRunTime > 0) {
          json_task = malloc_stringf("{\"id\":%d,\"name\":\"%s\",\"core\":%d,\"state\":\"%s\",\"current_priority\":%d,\"base_priority\":%d,\"run_time_counter\":%d,\"run_time\":%.2f,\"stack_base\":\"%x\",\"stack_minimum\":%d}",
            pxTaskStatusArray[x].xTaskNumber, 
            pxTaskStatusArray[x].pcTaskName, 
            #ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
            pxTaskStatusArray[x].xCoreID > 1 ? -1 : pxTaskStatusArray[x].xCoreID, 
            #else
            -1,
            #endif // CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
            sysinfoTaskListState(pxTaskStatusArray[x].eCurrentState), 
            pxTaskStatusArray[x].uxCurrentPriority, pxTaskStatusArray[x].uxBasePriority,
            pxTaskStatusArray[x].ulRunTimeCounter, (float)pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime,
            pxTaskStatusArray[x].pxStackBase, pxTaskStatusArray[x].usStackHighWaterMark);
        } else {
          json_task = malloc_stringf("{\"id\":%d,\"name\":\"%s\",\"core\":%d,\"state\":\"%s\",\"current_priority\":%d,\"base_priority\":%d,\"run_time_counter\":%d,\"stack_base\":\"%x\",\"stack_minimum\":%d}",
            pxTaskStatusArray[x].xTaskNumber, 
            pxTaskStatusArray[x].pcTaskName, 
            #ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
            pxTaskStatusArray[x].xCoreID > 1 ? -1 : pxTaskStatusArray[x].xCoreID, 
            #else
            -1,
            #endif // CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
            sysinfoTaskListState(pxTaskStatusArray[x].eCurrentState), 
            pxTaskStatusArray[x].uxCurrentPriority, pxTaskStatusArray[x].uxBasePriority,
            pxTaskStatusArray[x].ulRunTimeCounter,
            pxTaskStatusArray[x].pxStackBase, pxTaskStatusArray[x].usStackHighWaterMark);
        };
        if (json_task) {
          if (json_summary) {
            char* json_temp = json_summary;
            json_summary = malloc_stringf("%s,%s", json_temp, json_task);
            free(json_temp);
          } else {
            json_summary = malloc_string(json_task);
          };
          free(json_task);
        };
      };
      // Publish data
      if (json_summary) {
        mqttPublish(_mqttTopicTaskList, malloc_stringf("[%s]", json_summary), 
          CONFIG_MQTT_TASKLIST_QOS, CONFIG_MQTT_TASKLIST_RETAINED, false, true);
        free(json_summary);
        json_summary = nullptr;
      };
      // The array is no longer needed, free the memory it consumes
      vPortFree(pxTaskStatusArray);
    };
  } else {
    rlog_w(logTAG, "Failed to publish task list - MQTT is not connected");
  }
}

#endif // CONFIG_MQTT_TASKLIST_ENABLE

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

    #if CONFIG_MQTT_TASKLIST_ENABLE
    mqttTopicTaskListCreate(data->primary);
    #endif // CONFIG_MQTT_TASKLIST_ENABLE

    #if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE
    sysinfoPublishSysInfo();
    #endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE

    #if CONFIG_WIFI_DEBUG_ENABLE
    char* jsonWiFiDebug = wifiGetDebugInfo();
    if (jsonWiFiDebug) {
      mqttPublish(
        mqttGetTopicDevice1(data->primary, CONFIG_MQTT_WIFI_DEBUG_LOCAL, CONFIG_MQTT_WIFI_DEBUG_TOPIC),
        jsonWiFiDebug, 
        CONFIG_MQTT_WIFI_DEBUG_QOS,
        CONFIG_MQTT_WIFI_DEBUG_RETAINED,
        true, true);
    };
    #endif // CONFIG_WIFI_DEBUG_ENABLE
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

    #if CONFIG_MQTT_TASKLIST_ENABLE
    mqttTopicTaskListFree();
    #endif // CONFIG_MQTT_TASKLIST_ENABLE
  };
}

bool sysinfoEventHandlerRegister()
{
  #if CONFIG_MQTT_TASKSHOW_ENABLE
    sysinfoShowTaskStartTimer();
  #endif // CONFIG_MQTT_TASKSHOW_ENABLE
  
  return eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONNECTED, &sysinfoMqttEventHandler, nullptr)
      && eventHandlerRegister(RE_MQTT_EVENTS, RE_MQTT_CONN_LOST, &sysinfoMqttEventHandler, nullptr);
};

bool sysinfoEventHandlerUnregister()
{
  return eventHandlerUnregister(RE_MQTT_EVENTS, RE_MQTT_CONNECTED, &sysinfoMqttEventHandler)
      && eventHandlerUnregister(RE_MQTT_EVENTS, RE_MQTT_CONN_LOST, &sysinfoMqttEventHandler);
};