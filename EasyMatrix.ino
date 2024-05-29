#include "common.h"
#include "preferencesUtil.h"
#include "light.h"
#include "buzzer.h"
#include "net.h"
#include "task.h"

/**
EasyMatrix像素时钟  版本1.3

本次更新内容：

1.音乐频谱灯本来只有左高频，右低频一种亮灯模式，新版本增加了中间低频，两边高频的模式。
在节奏灯页面，长按按键 1，可以在两种模式之间切换。
2.亮度调节在这个版本支持两种方式，手动调节和自动调节。
在亮度页面，长按按键1，就可以在两种模式之间切换。电路接线图在1.3版本的更新视频简介中。
3.由于节奏灯页面的按键 1 长按功能被切换模式占用，所以1.3版本中只能在时钟页面和动画页面长按按键 1，才能重新配网。


注意：烧录前请将 Erase All Flash 选项选为 Enable，烧录完再重新配置网络，否则程序可能会有错误。
*/

unsigned int prevDisplay = 0;
unsigned long prevSampling = 0;

void setup() {
  Serial.begin(115200);
  // 从NVS中获取信息
  getInfos();
  // 初始化点阵屏(包含拾音器)
  initMatrix();
  // 创建加载动画任务
  createShowTextTask("START");
  // 初始化按键
  btnInit();
  Serial.println("各外设初始化成功");
  // nvs中没有WiFi信息，进入配置页面
  if(apConfig){
    currentPage = SETTING; // 将页面置为配网页面
    wifiConfigBySoftAP(); // 开启SoftAP配置WiFi
  }else{
    // 连接WiFi,30秒超时后显示wifi连接失败的图案
    connectWiFi(30); 
    // 如果连接上了wifi,就进行NTP对时,超过30秒对时失败，就进入节奏灯页面
    if(wifiConnected){
      checkTime(30);
      if(RTCSuccess){
        // 开启对时任务
        startTickerCheckTime();
        // RTC对时成功，并且闹钟开启后，设置闹钟倒计时
        if(clockOpen){
          startTickerClock(getClockRemainSeconds());
        }
        // 将页面置为时间页面
        currentPage = TIME; 
        // 停止启动加载文字的动画
        vTaskDelete(showTextTask);
        delay(300);
        // 清屏
        clearMatrix();
      }
      // 关闭wifi
      disConnectWifi();
    }
  }
}

void loop() { 
  watchBtn();
  if(brightModel == BRIGHT_MODEL_AUTO && ((millis() - prevSampling) >= 1000 || prevSampling > millis())){
    brightSamplingValue+=analogRead(LIGHT_ADC);
    brightSamplingTime++;
    prevSampling = millis();
    if(brightSamplingTime >= BRIGHT_SAMPLING_TIMES){ // 每轮采样N次重新计算一次亮度值
      calculateBrightnessValue();
      clearBrightSampling();
    }
  }
  if(isCheckingTime){ // 对时中
    bool stopAnim = false; // 记录是否中断了动画
    Serial.println("开始对时");
    long start = millis(); // 记录开始对时的时间
    // 停止动画
    if(tickerAnim.active()){
      tickerAnim.detach();
      stopAnim = true;
    }
    // 绘制对时提示文字
    drawCheckTimeText();
    // 执行对时逻辑
    checkTimeTicker();
    // 将对时标志置为false
    isCheckingTime = false;
    // 让整个对时过程持续超过4秒，不然时间太短，提示文字一闪而过，让人感觉鬼畜了
    while((millis() - start) < 4000){
      delay(200);
    }
    // 如果中断了动画，则重新开始动画
    if(stopAnim){
      startTickerAnim();
    }
    // 清屏
    clearMatrix();
    Serial.println("结束对时");
    // 结束对时后，重新绘制之前的页面
    if(currentPage == CLOCK){
      drawClock();
    }else if(currentPage == BRIGHT){
      drawBright();
    }else if(currentPage == ANIM){
      lightedCount = 0;
      memset(matrixArray,0,sizeof(matrixArray));
    }
  }else{
    switch(currentPage){
      case SETTING:  // 配置页面
        doClient(); // 监听客户端配网请求
        break;
      case TIME: // 时钟页面
        time_t now;
        time(&now);
        if(now != prevDisplay){ // 每秒更新一次时间显示
          prevDisplay = now;
          // 绘制时间
          drawTime();
        }
        break;
      case RHYTHM: // 节奏灯页面
        drawRHYTHM();
        break;
      case ANIM: // 动画页面
        drawAnim();
        break;
      case CLOCK: // 闹钟设置页面
        drawClock();
        break;
      case BRIGHT: // 亮度调节页面
        drawBright();
        break;  
      default:
        break;
    }
  } 
}


