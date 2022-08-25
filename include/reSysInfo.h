/*
   EN: Module for collecting and publishing system statistics about the controller's operation, including date and time
   RU: Модуль для сбора и публикации системной статистики о работе контроллера, в том числе даты и времени
   --------------------------
   (с) 2021-2022 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
*/

#ifndef __RE_SYSINFO_H__
#define __RE_SYSINFO_H__

#include <time.h>
#include <stddef.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "project_config.h"
#include "def_consts.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
   uint32_t days;
   uint8_t  hours;
   int8_t   minutes;
} worktime_t;

typedef struct {
   char* date;
   char* time;
   char* weekday;
   char* timeday;
   char* datetime_s;
   char* datetime_l;
} str_datetime_t;

void sysinfoWorkTimeInc();
worktime_t getWorkTime();

void sysinfoFixDateTime(const struct tm* currTime);
str_datetime_t getDateTimeStrings();
#if CONFIG_MQTT_TIME_ENABLE
char* mqttTopicDateTimeCreate(const bool primary);
char* mqttTopicDateTimeGet();
void  mqttTopicDateTimeFree();
void  mqttPublishDateTime(const struct tm* timeinfo);
#endif // CONFIG_MQTT_TIME_ENABLE

#if CONFIG_MQTT_SYSINFO_ENABLE
char* mqttTopicSysInfoCreate(const bool primary);
char* mqttTopicSysInfoGet();
void  mqttTopicSysInfoFree();
#endif // CONFIG_MQTT_SYSINFO_ENABLE

#if CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE
void  sysinfoPublishSysInfo();
#endif // CONFIG_MQTT_STATUS_ONLINE || CONFIG_MQTT_STATUS_ONLINE_SYSINFO || CONFIG_MQTT_SYSINFO_ENABLE

#if CONFIG_MQTT_TASKSHOW_ENABLE
esp_err_t sysinfoShowTaskStartTimer();
#endif // CONFIG_MQTT_TASKSHOW_ENABLE

#if CONFIG_MQTT_TASKLIST_ENABLE
char* mqttTopicTaskListCreate(const bool primary);
char* mqttTopicTaskListGet();
void  mqttTopicTaskListFree();
void  sysinfoPublishTaskList();
#endif // CONFIG_MQTT_TASKLIST_ENABLE

bool sysinfoEventHandlerRegister();
bool sysinfoEventHandlerUnregister();

#ifdef __cplusplus
}
#endif

#endif // __RE_SYSINFO_H__

