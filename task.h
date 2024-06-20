#ifndef __TASK_H
#define __TASK_H

#include "common.h"
#include "Ticker.h"

extern enum CurrentPage currentPage;
extern TaskHandle_t btnTask;
extern TaskHandle_t showTextTask;
extern TaskHandle_t showIpTask;
void watchBtn();
void createShowTextTask(char *text);
void createShowIpTask();
void createBellTask();
void createPlaySongsTask();
void btnInit();
int32_t getClockRemainSeconds();
extern Ticker tickerClock;
extern Ticker tickerCheckTime;
extern bool isCheckingTime;
void startTickerClock(int32_t seconds);
void startTickerCheckTime();

#endif

