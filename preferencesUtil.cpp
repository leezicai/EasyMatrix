#include <Preferences.h>
#include "common.h"
#include "net.h"
#include "light.h"

Preferences prefs; // 声明Preferences对象

// 上电时读取Wifi账号、密码、默认颜色等一系列信息
void getInfos(){
  prefs.begin("matrix");
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  int r = prefs.getInt("red", 0);
  int g = prefs.getInt("green", 250);
  int b = prefs.getInt("blue", 250);
  apConfig = prefs.getBool("apConfig", true);
  timePage = prefs.getInt("timePage",TIME_H_M_S);
  rhythmPage = prefs.getInt("rhythmPage",RHYTHM_MODEL1);
  animPage = prefs.getInt("animPage",ANIM_MODEL1);
  rhythmBandsModel = prefs.getInt("bandsModel",RHYTHM_BANDS_MODEL1);
  brightModel = prefs.getInt("brightModel",BRIGHT_MODEL_MANUAL);
  timeModel = prefs.getInt("timeModel",TIME_MODEL_ANIM);
  clockH = prefs.getInt("clockH",0);
  clockM = prefs.getInt("clockM",0);
  clockBellNum = prefs.getInt("clockBellNum",0);
  clockOpen = prefs.getBool("clockOpen",false);
  clockColor[0] = r;
  clockColor[1] = g;
  clockColor[2] = b;
  mainColor = matrix.Color(clockColor[0], clockColor[1], clockColor[2]);
  // 时间下面的星期条颜色，最大的RGB值不变，其他两个取反色
  int maxRGB = max(r, max(g, b)); // 求出RGB三色里的最大值
  bool findMax = false; // 是否找到最大值，防止3个值一样大，都不取反
  int weekR, weekG, weekB;
  if(r == maxRGB){
    findMax = true;
    weekR = r;
  }else{
    weekR = 255 - r;
  }
  if(g == maxRGB && !findMax){
    findMax = true;
    weekG = g;
  }else{
    weekG = 255 - g;
  }
  if(b == maxRGB && !findMax){
    findMax = true;
    weekB = b;
  }else{
    weekB = 255 - b;
  }
  weekColor = matrix.Color(weekR, weekG, weekB); 
  brightness = prefs.getInt("brightness", BRIGHTNESS);
  prefs.end();
}

// 用户配网后写入Wifi账号、密码以及时钟颜色
void recordInfos(String ssid,String pass,int r,int g,int b,bool apConfig){
  prefs.begin("matrix");
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putInt("red", r);
  prefs.putInt("green", g);
  prefs.putInt("blue", b);
  prefs.putBool("apConfig", apConfig);
  prefs.end();
}

// 写入亮度
void recordBrightness(){
  prefs.begin("matrix");
  prefs.putInt("brightness", brightness);
  prefs.end();
}

// 获取亮度
void getBrightness(){
  prefs.begin("matrix");
  brightness = prefs.getInt("brightness", BRIGHTNESS);
  prefs.end();
}

// 写入启动时是否需要配网的标志位
void setApConfigWhenStart(bool apConfig){
  prefs.begin("matrix");
  prefs.putBool("apConfig",apConfig);
  prefs.end();
}

// 写入小页面和其他页面模式
void recordExtensionPage(){
  prefs.begin("matrix");
  prefs.putInt("timePage", timePage);
  prefs.putInt("rhythmPage", rhythmPage);
  prefs.putInt("animPage", animPage);
  prefs.putInt("bandsModel", rhythmBandsModel);
  prefs.putInt("brightModel", brightModel);
  prefs.putInt("timeModel", timeModel);
  prefs.end();
}

// 写入闹钟相关的数据
void recordClockPage(){
  prefs.begin("matrix");
  prefs.putInt("clockH", clockH);
  prefs.putInt("clockM", clockM);
  prefs.putInt("clockBellNum", clockBellNum);
  prefs.putBool("clockOpen", clockOpen);
  prefs.end();
}
