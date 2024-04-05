#include <OneButton.h>
#include "Ticker.h"
#include "net.h"
#include "common.h"
#include "light.h"
#include "buzzer.h"
#include "preferencesUtil.h"

enum CurrentPage currentPage = SETTING;
int animIndex = 0; // 时钟页面动画索引
bool playingMusic = false; // 是否正在播放音乐
bool belling = false; // 是否正在播放铃声
bool isCheckingTime = false; // 是否正在NTP对时中

void startShowText(void *param);
void startShowIp(void *param);
void startPlaySongs(void *param);
void startPlayBell(void *param);
void startTickerClock(int32_t seconds);
int32_t getClockRemainSeconds();
void btn1click();
void btn2click();
void btn3click();
void btn1LongClick();
void btn2LongClick();
void btn3LongClick();
void createPlaySongsTask();
void createBellTask();
void cancelBell();

///////////////////////////////////Freertos区域///////////////////////////////////////
// 按钮
OneButton button1(BTN1, true);
OneButton button2(BTN2, true);
OneButton button3(BTN3, true);
//闹钟响铃任务句柄
TaskHandle_t bellTask;
//播放歌曲的任务句柄(闹钟页面使用)
TaskHandle_t playSongsTask;
//系统启动动画的任务句柄
TaskHandle_t showTextTask;
//循环播放配网ip地址的任务句柄
TaskHandle_t showIpTask;
//创建闹铃任务
void createBellTask(){
  xTaskCreate(startPlayBell, "startPlayBell", 1000, NULL, 1, &bellTask);
}
//创建音乐播放任务(闹钟页面使用)
void createPlaySongsTask(){
  xTaskCreate(startPlaySongs, "startPlaySongs", 1000, NULL, 1, &playSongsTask);
}
//创建文字显示任务
void createShowTextTask(char *text){
  xTaskCreate(startShowText, "startShowText", 1000, (void *)text, 1, &showTextTask);
}
//创建轮播配网的ip地址的任务
void createShowIpTask(){
  xTaskCreate(startShowIp, "startShowIp", 1000, NULL, 1, &showIpTask);
}
// 启动文字显示，CONFIG不用了，改为采用显示ip地址的形式
void startShowText(void *param){
  String text = (char *)param;
  int x;
  if(text.equals("START")){
    x = 2;
  }else if(text.equals("CONFIG")){
    x = 0;
  }
  int index = 0;
  while (true) {
    if(index % 3 == 0){
      drawText(x, 6, text + ".");
    }else if(index % 3 == 1){
      drawText(x, 6, text + "..");
    }else if(index % 3 == 2){
      drawText(x, 6, text + "...");
    }
    vTaskDelay(1000);
    index++;
  }
  Serial.println("退出启动文字显示");
}
// 轮播IP
void startShowIp(void *param){
  while (true) {
    showIp();
  }
  Serial.println("退出轮播配网IP动画");
}
// 播放歌曲(闹钟页面使用)
void startPlaySongs(void *param){
  while (true) {
    playSong(false);
    vTaskDelay(3000);
  }
  Serial.println("退出音乐播放");
}
// 响铃
void startPlayBell(void *param){
  while (true) {
    playSong(true);
    vTaskDelay(3000);
  }
  Serial.println("退出响铃任务");
}
//////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////定时器区域///////////////////////////////////////
Ticker tickerClock;
Ticker tickerCheckTime;
Ticker tickerAnim;
// NTP对时
void checkTime(){
  // 只要将对时标志置为true即可，在主循环中进行对时操作
  isCheckingTime = true;
}
// 时钟页面显示动画
void showAnim(){
  switch(animIndex){
      case 0:
        matrix.drawRGBBitmap(0,0,timeAnim0,11,8);
        break;
      case 1:
        matrix.drawRGBBitmap(0,0,timeAnim1,11,8);
        break;
      case 2:
        matrix.drawRGBBitmap(0,0,timeAnim2,11,8);
        break;
      case 3:
        matrix.drawRGBBitmap(0,0,timeAnim3,11,8);
        break;
      case 4:
        matrix.drawRGBBitmap(0,0,timeAnim4,11,8);
        break;
      case 5:
        matrix.drawRGBBitmap(0,0,timeAnim5,11,8);
        break;
      case 6:
        matrix.drawRGBBitmap(0,0,timeAnim6,11,8);
        break;
      case 7:
        matrix.drawRGBBitmap(0,0,timeAnim7,11,8);
        break;
      case 8:
        matrix.drawRGBBitmap(0,0,timeAnim8,11,8);
        break;
      case 9:
        matrix.drawRGBBitmap(0,0,timeAnim0,11,8);
        break;  
      default:
        animIndex = 9;
        break;
    }
    matrix.show();
    animIndex++;
    if(animIndex == 10){
      animIndex = 0;
    } 
}
// 根据NVS中的闹钟时间计算出定时器需要多久之后触发
int32_t getClockRemainSeconds(){
  // 获取RTC时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)){
    Serial.println("获取RTC时间失败");
    return -1;
  }
  int32_t nowTime = (timeinfo.tm_hour * 60 + timeinfo.tm_min) * 60 + timeinfo.tm_sec;
  int32_t clockTime = (clockH * 60 + clockM) * 60;
  if(clockTime > nowTime){ // 闹钟时间在当天
    return (clockTime - nowTime);
  }else{ // 闹钟时间在第二天
    return (clockTime + ONE_DAY_SECONDS - nowTime);
  }
}
// 奏响闹铃
void ringingBell() {
  // 配置页面和闹钟设置页面不响铃
  if(currentPage == SETTING || currentPage == CLOCK){
    return;
  }
  // 进入时钟页面
  if(currentPage == RHYTHM){
    // 将二维数组重置为0
    memset(matrixArray,0,sizeof(matrixArray));
    // 将已点亮灯的个数置零
    lightedCount = 0;
  }
  currentPage = TIME;
  // 创建播放音乐的任务
  createBellTask();
  belling = true;
  // 将定时器重置
  tickerClock.detach();
  startTickerClock(getClockRemainSeconds());
}
// 开启定时器
void startTickerClock(int32_t seconds){
  // seconds秒后，执行一次
  tickerClock.once(seconds, ringingBell);
}
void startTickerAnim(){
  // 每隔一段时间播放一帧动画
  tickerAnim.attach_ms(ANIM_INTERVAL, showAnim);
}
void startTickerCheckTime(){
  // 每隔一段时间进行一次NTP对时
  tickerCheckTime.attach(TIME_CHECK_INTERVAL, checkTime);
}
// 在闹钟响铃时，任何按键按下，都取消闹铃
void cancelBell(){
  vTaskDelete(bellTask);
  delay(300);
  belling = false;
}
//////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////// 按键区////////////////////////////////////////
// 初始化各按键
void btnInit(){
  button1.attachClick(btn1click);
  button1.setDebounceMs(10); //设置消抖时长 
  button2.attachClick(btn2click);
  button2.setDebounceMs(10); //设置消抖时长 
  button3.attachClick(btn3click);
  button3.setDebounceMs(10); //设置消抖时长 
  button1.attachLongPressStart(btn1LongClick);
  button1.setPressMs(1200); //设置长按时间
  button2.attachLongPressStart(btn2LongClick);
  button2.setPressMs(1200); //设置长按时间
  button3.attachLongPressStart(btn3LongClick);
  button3.setPressMs(1500); //设置长按时间
}
// 监控按键
void watchBtn(){
  button1.tick();
  button2.tick();
  button3.tick();
}
// 按键方法
// >按键
void btn1click(){
  if(belling){
    cancelBell();
    return;
  }
  switch(currentPage){
    case TIME:
      if(timePage == TIME_H_M_S){
        timePage = TIME_H_M;
      }else if(timePage == TIME_H_M){
        timePage = TIME_DATE;
      }else{
        tickerAnim.detach();
        timePage = TIME_H_M_S;
      }
      // 清屏
      clearMatrix();
      // 保存设置
      recordExtensionPage();
      // 绘制时间
      drawTime();
      break;
    case RHYTHM:
      if(rhythmPage == RHYTHM_MODEL1){
        rhythmPage = RHYTHM_MODEL2;
      }else if(rhythmPage == RHYTHM_MODEL2){
        rhythmPage = RHYTHM_MODEL3;
      }else if(rhythmPage == RHYTHM_MODEL3){
        rhythmPage = RHYTHM_MODEL4;
      }else if(rhythmPage == RHYTHM_MODEL4){
        rhythmPage = RHYTHM_MODEL1;
      }
      break;
    case ANIM:
      if(animPage == ANIM_MODEL1){
        animPage = ANIM_MODEL2;
      }else if(animPage == ANIM_MODEL2){
        // 将已点亮灯的个数置零
        lightedCount = 0;
        animPage = ANIM_MODEL3;
      }else if(animPage == ANIM_MODEL3){       
        animPage = ANIM_MODEL1;
      }
      // 将二维数组重置为0
      memset(matrixArray,0,sizeof(matrixArray));
      // 保存设置
      recordExtensionPage();
      // 清屏
      clearMatrix();
      break;
    case CLOCK:
      if(!clockOpen){
        return;
      }
      if(clockChoosed == CLOCK_H){
        tmpClockH++;
        if(tmpClockH == 24){
          tmpClockH = 0;
        }
        drawClock();
      }else if(clockChoosed == CLOCK_M){
        tmpClockM++;
        if(tmpClockM == 60){
          tmpClockM = 0;
        }
        drawClock();
      }else if(clockChoosed == CLOCK_BELL){
        tmpClockBellNum++;
        if(tmpClockBellNum == songCount){
          tmpClockBellNum = 0;
        }
        // 停止音乐播放
        vTaskDelete(playSongsTask);
        delay(300);
        // 开始音乐播放
        createPlaySongsTask();       
      }
      break;
    case BRIGHT:
      if(brightness >= 145){
        Serial.println("已达最大亮度");
        return;
      }
      brightness+=BRIGHTNESS_SPACING;
      Serial.print("当前亮度：");Serial.println(brightness);
      recordBrightness();
      drawBright();
      break;
    default:
      break;
  }
}
// <按键
void btn2click(){
  if(belling){
    cancelBell();
    return;
  }
  switch(currentPage){
    case TIME:     
      if(timePage == TIME_H_M_S){
        timePage = TIME_DATE;
      }else if(timePage == TIME_H_M){
        tickerAnim.detach();
        timePage = TIME_H_M_S;
      }else{
        timePage = TIME_H_M;
      }
      // 清屏
      clearMatrix();
      // 保存设置
      recordExtensionPage();
      // 绘制时间
      drawTime();
      break;
    case RHYTHM:
      if(rhythmPage == RHYTHM_MODEL1){
        rhythmPage = RHYTHM_MODEL4;
      }else if(rhythmPage == RHYTHM_MODEL2){
        rhythmPage = RHYTHM_MODEL1;
      }else if(rhythmPage == RHYTHM_MODEL3){
        rhythmPage = RHYTHM_MODEL2;
      }else if(rhythmPage == RHYTHM_MODEL4){
        rhythmPage = RHYTHM_MODEL3;
      }
      break;
    case ANIM:
      if(animPage == ANIM_MODEL1){
        // 将已点亮灯的个数置零
        lightedCount = 0;
        animPage = ANIM_MODEL3;
      }else if(animPage == ANIM_MODEL2){
        animPage = ANIM_MODEL1;
      }else if(animPage == ANIM_MODEL3){
        animPage = ANIM_MODEL2;
      }
      // 将二维数组重置为0
      memset(matrixArray,0,sizeof(matrixArray));
      // 保存设置
      recordExtensionPage();
      // 清屏
      clearMatrix();
      break;
    case CLOCK:
      if(!clockOpen){
        return;
      }
      if(clockChoosed == CLOCK_H){
        tmpClockH--;
        if(tmpClockH == -1){
          tmpClockH = 23;
        }
        drawClock();
      }else if(clockChoosed == CLOCK_M){
        tmpClockM--;
        if(tmpClockM == -1){
          tmpClockM = 59;
        }
        drawClock();
      }else if(clockChoosed == CLOCK_BELL){
        tmpClockBellNum--;
        if(tmpClockBellNum == -1){
          tmpClockBellNum = (songCount - 1);
        }
        // 停止音乐播放
        vTaskDelete(playSongsTask);
        delay(300);
        // 开始音乐播放
        createPlaySongsTask();
      }
      break;
    case BRIGHT:
      if(brightness <= 5){
        Serial.println("已达最小亮度");
        return;
      }
      brightness-=BRIGHTNESS_SPACING;
      Serial.print("当前亮度：");Serial.println(brightness);
      recordBrightness();
      drawBright();
      break;
    default:
      break;
  }
}
void btn3click(){
  if(belling){
    cancelBell();
    return;
  }
  switch(currentPage){
    case SETTING:
      // 关闭显示IP的动画任务
      vTaskDelete(showIpTask);
      delay(300);
      // 清空屏幕
      clearMatrix();
      // 查看是否曾经配置过网络
      if(ssid.equals("")){ // 没有配置过，进入节奏灯页面
        // 至节奏灯页面
        currentPage = RHYTHM;
      }else{  //有配置过，尝试进行连接
        // 将页面置为时间页面
        currentPage = TIME; 
        // 将重新配网标志位置为false
        setApConfigWhenStart(false);
        // 连接WiFi,30秒超时后显示wifi连接失败的图案
        connectWiFi(30); 
        // 如果连接上了wifi,就进行NTP对时,超过30秒对时失败，就进入节奏灯页面
        if(wifiConnected){
          checkTime(30);
        }
      }
      break;
    case TIME:
      // 关闭动画
      if(tickerAnim.active()){
        tickerAnim.detach();
      }
      // 清屏
      clearMatrix();
      currentPage = RHYTHM;
      break;
    case RHYTHM:
      // 将二维数组重置为0
      memset(matrixArray,0,sizeof(matrixArray));
      // 将已点亮灯的个数置零
      lightedCount = 0;
      // 清屏
      clearMatrix();
      currentPage = ANIM;
      break;
    case ANIM:
      if (!RTCSuccess){
        // 至亮度调节页面
        currentPage = BRIGHT;
        drawBright();
      }else{
        currentPage = CLOCK;
        resetTmpClockData();
        drawClock();
      }  
      break;
    case CLOCK:
      // 保存当前设置的闹钟信息
      clockH = tmpClockH;
      clockM = tmpClockM;
      clockBellNum = tmpClockBellNum;
      // 记录闹钟配置信息
      recordClockPage();
      // 将定时器重置
      tickerClock.detach();
      if(clockOpen){
        startTickerClock(getClockRemainSeconds());
      }
      // 如果正在播放音乐，则停止音乐播放
      if(playingMusic){
        vTaskDelete(playSongsTask);
        delay(300);
      }
      playingMusic = false;
      currentPage = BRIGHT;
      drawBright();
      break;
    case BRIGHT:
      if (!RTCSuccess){
        // 至节奏灯页面
        currentPage = RHYTHM;
      }else{
        currentPage = TIME;
      }
      break; 
    default:
      break;
  }
}
void btn1LongClick(){
  if(belling){
    cancelBell();
    return;
  }
  if(currentPage != CLOCK || !clockOpen){
    return;
  }
  if(clockChoosed == CLOCK_H){
    clockChoosed = CLOCK_M;
    drawClock();
  }else if(clockChoosed == CLOCK_M){
    clockChoosed = CLOCK_BELL;
    drawClock();
    // 演奏当前铃声
    createPlaySongsTask();
    playingMusic = true;
  }else if(clockChoosed == CLOCK_BELL){
    // 停止演奏
    vTaskDelete(playSongsTask);
    delay(300);
    playingMusic = false;
    clockChoosed = CLOCK_H;
    drawClock();
  }
}
void btn2LongClick(){
  if(belling){
    cancelBell();
    return;
  }
  if(currentPage != CLOCK || !clockOpen){
    return;
  }
  if(clockChoosed == CLOCK_H){
    clockChoosed = CLOCK_BELL;
    drawClock();
    // 演奏当前铃声
    createPlaySongsTask();
    playingMusic = true;
  }else if(clockChoosed == CLOCK_M){
    clockChoosed = CLOCK_H;
    drawClock();
  }else if(clockChoosed == CLOCK_BELL){
    // 停止演奏
    vTaskDelete(playSongsTask);
    delay(300);
    playingMusic = false;
    clockChoosed = CLOCK_M;
    drawClock();
  }
}
void btn3LongClick(){
  if(belling){
    cancelBell();
    return;
  }
  if(currentPage == SETTING){
    return;
  }
  if(currentPage == CLOCK){ // 时钟页面长按，关闭/开启闹钟
    if(clockChoosed == CLOCK_BELL){
      if(playingMusic){
        // 停止演奏
        vTaskDelete(playSongsTask);
        delay(300);
        playingMusic = false;
      }else{
        // 演奏当前铃声
        createPlaySongsTask();
        playingMusic = true;
      }
    }
    clockOpen = !clockOpen;
    drawClock();
    recordClockPage();
  }else{ // 其他页面长按，重启并配网
    Serial.println("重启并配网");
    setApConfigWhenStart(true);
    ESP.restart();
  } 
}
///////////////////////////////////////////////////////////////

