#include "common.h"
#include "preferencesUtil.h"
#include "light.h"
#include "buzzer.h"
#include "net.h"
#include "task.h"

/**
EasyMatrix像素时钟  版本1.4

本次更新内容：

1.在之前的节奏灯页面中，外界音量大到一定程度时，会发生好几个频段同时冲顶的情况，观赏性大打折扣。
在这个版本中，我优化了算法，系统会动态调整柱状条的高度。
2.增加了时间滑动动画，时间变化不再是呆板的直接跳变。时间显示页面长按按键1可在两种时间跳变模式之间切换。
3.重新绘制了海豚动画，并优化重构了海豚动画的显示逻辑。
4.由于时间页面的按键1长按功能被使用，现在只能在动画页面长按按键1才可以重新配置网络和颜色。

注意：烧录前请将 Erase All Flash 选项选为 Enable，烧录完再重新配置网络，否则程序可能会有错误。
*/

unsigned long prevDisplay = 0;
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
    Serial.println("开始对时");
    long start = millis(); // 记录开始对时的时间
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
        if((millis() - prevDisplay) >= 50 || prevDisplay > millis()){
          // 绘制时间
          drawTime();
          prevDisplay = millis();
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


