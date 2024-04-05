#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/MyFont.h>
#include <Arduino.h>
#include <arduinoFFT.h>
#include "common.h"
#include "preferencesUtil.h"
#include "task.h"

int clockColor[3];
uint16_t mainColor;
uint16_t weekColor;
int brightness;
int showIpIndex = MATRIX_SIDE * 4;
int weekChromatism = 50; // 绘制星期时的色差基础值
// 闹钟页面
bool clockOpen; // 闹钟是否开启
int clockH,clockM,clockBellNum; // 闹钟时、分、闹铃编号
int tmpClockH,tmpClockM,tmpClockBellNum; // 用来显示的临时闹钟时、分、闹铃编号
int clockChoosed = CLOCK_H; // 默认选中时
// 记录分页面
int timePage;
int rhythmPage;
int animPage;
// 动画页相关
int matrixArray[13][32]; // 点阵二维数组，用来记录一些数据
int animInterval1 = 60; // 动画1每一帧动画间隔
int animInterval2 = 80; // 动画2每一帧动画间隔
int animInterval3 = 100; // 动画3每一帧动画间隔
int hackAnimProbability = 8; // 骇客动画产生的概率，数值越大，新产生的下坠动画就越少
int lightedCount = 0; // 随机点动画已点亮的个数
bool increasing = true; // 随机点动画正在增加状态 
unsigned long animTime; // 记录上一次动画的时间
// 节奏灯相关
uint16_t hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value);
unsigned int sampling_period_us; //采样周期
byte peak[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int oldBarHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int bandValues[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long starTime;
unsigned long peekDecayTime;
unsigned long changeColorTime;
int colorTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);
int model2ColorArray[2][3] = {{0, 220, 255}, {240, 45, 255}};
int model4ColorArrar[8][3] = {{240, 45, 255},{253, 98, 248},{253, 169, 205},{255, 196, 123},{253, 214, 200},{253, 192, 255},{249, 175, 255},{0, 220, 255}};

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(MATRIX_SIDE, MATRIX_SIDE, MATRIX_COUNT, 1, DATAPIN,
NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE + 
NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS + NEO_TILE_PROGRESSIVE, NEO_GRB + NEO_KHZ800);

// 初始化矩阵
void initMatrix(){
  matrix.begin();
  matrix.setFont(&MyFont);
  matrix.setTextWrap(false);
  // 计算采样周期（单位：uS）
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
  // 初始化拾音器
  pinMode(AUDIO_IN_PIN, INPUT);
}

// 清空屏幕
void clearMatrix(){
  matrix.fillScreen(0);
  matrix.show();
}

// 绘制对时文字
void drawCheckTimeText(){
  matrix.fillScreen(0);
  matrix.setBrightness(brightness);
  matrix.drawBitmap(0, 0, checkingTime, 32, 8, mainColor);
  matrix.show();
}

// 绘制文字
void drawText(int x, int y, String text){
  matrix.setBrightness(brightness);
  matrix.setFont(&MyFont);
  matrix.fillScreen(0);
  matrix.setCursor(x,y);
  matrix.setTextColor(mainColor);
  matrix.print(text);
  matrix.show();
}

// 绘制wifi连接错误的文字和图案
void drawFailed(int textX, int failedX, String text){
  drawText(textX, 6, text);
  matrix.setCursor(failedX,6);
  matrix.setTextColor(matrix.Color(255, 0, 0));
  matrix.print("X");
  matrix.show();
}

// 轮播配网IP地址
void showIp(){
  matrix.fillScreen(0);
  matrix.setTextColor(mainColor);
  matrix.setCursor(showIpIndex, 6);
  matrix.print("192.168.1.1");
  matrix.show();
  delay(100);
  showIpIndex--;
  if(showIpIndex < -36){
    showIpIndex = MATRIX_SIDE * 4;
  }
}

// 绘制时间页面
void drawTime(){
  // 获取RTC时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)){
    Serial.println("获取RTC时间失败");
    return;
  }
  matrix.setBrightness(brightness);
  matrix.setFont(&MyFont);
  matrix.setTextColor(mainColor);
  if(timePage == TIME_H_M_S){
    // 清屏
    matrix.fillScreen(0);
    // 绘制时间
    matrix.setCursor(2, 5);
    String h;
    if(timeinfo.tm_hour < 10){
      h = "0" + String(timeinfo.tm_hour);
    }else{
      h = String(timeinfo.tm_hour);
    }
    String m;
    if(timeinfo.tm_min < 10){
      m = "0" + String(timeinfo.tm_min);
    }else{
      m = String(timeinfo.tm_min);
    }
    String s;
    if(timeinfo.tm_sec < 10){
      s = "0" + String(timeinfo.tm_sec);
    }else{
      s = String(timeinfo.tm_sec);
    }
    matrix.print(h + ":" + m + ":" + s);
    // 绘制星期
    int weekNum = timeinfo.tm_wday;
    if(weekNum == 0){
      weekNum = 7;
    }
    // 绘制6条直线(除了当天的)
    for(int i = 1; i <= 7; i++){
      if(i == weekNum){
        continue;
      }
      matrix.drawFastHLine(2 + (i - 1) * 4, 7, 3, weekColor);
    }
    matrix.drawFastHLine(2 + (weekNum - 1) * 4, 7, 3, mainColor);
  }else if(timePage == TIME_H_M){
    if(!tickerAnim.active()){
      startTickerAnim();
    }
    // 将除了动画的其他地方都清空
    matrix.fillRect(12, 0, 19, 8, 0);
    // 写入时间
    matrix.setCursor(14, 5);
    String h;
    if(timeinfo.tm_hour < 10){
      h = "0" + String(timeinfo.tm_hour);
    }else{
      h = String(timeinfo.tm_hour);
    }
    String m;
    if(timeinfo.tm_min < 10){
      m = "0" + String(timeinfo.tm_min);
    }else{
      m = String(timeinfo.tm_min);
    }
    matrix.print(h + ":" + m);
    // 绘制星期
    int weekNum = timeinfo.tm_wday;
    if(weekNum == 0){
      weekNum = 7;
    }
    // 绘制6条直线(除了当天的)
    for(int i = 1; i <= 7; i++){
      if(i == weekNum){
        continue;
      }
      matrix.drawFastHLine(12 + (i - 1) * 3, 7, 2, weekColor);
    }
    matrix.drawFastHLine(12 + (weekNum - 1) * 3, 7, 2, mainColor);
  }else if(timePage == TIME_DATE){
    if(!tickerAnim.active()){
      startTickerAnim();
    }
    // 将除了动画的其他地方都清空
    matrix.fillRect(12, 0, 19, 8, 0);
    // 写入日期
    matrix.setCursor(12, 5);
    String month = timeinfo.tm_mon < 9 ? "0" : "";
    month = month + (timeinfo.tm_mon + 1);
    String day = timeinfo.tm_mday < 10 ? "0" : "";
    day = day + timeinfo.tm_mday;
    matrix.print(month + "-" + day);
    // 绘制星期
    int weekNum = timeinfo.tm_wday;
    if(weekNum == 0){
      weekNum = 7;
    }
    // 绘制6条直线(除了当天的)
    for(int i = 1; i <= 7; i++){
      if(i == weekNum){
        continue;
      }
      matrix.drawFastHLine(12 + (i - 1) * 3, 7, 2, weekColor);
    }
    matrix.drawFastHLine(12 + (weekNum - 1) * 3, 7, 2, mainColor);
  }
  matrix.show();
}

// 绘制亮度调节页面
void drawBright(){
  matrix.setBrightness(brightness);
  matrix.setFont(&MyFont);
  matrix.fillScreen(0);
  matrix.setTextColor(mainColor);
  // 亮度图标
  matrix.drawFastVLine(13, 3, 2, mainColor);
  matrix.drawPixel(14, 2, mainColor);
  matrix.drawPixel(14, 5, mainColor);
  matrix.drawPixel(15, 1, mainColor);
  matrix.drawPixel(15, 6, mainColor);
  matrix.drawFastVLine(16, 1, 6, mainColor);
  matrix.drawFastVLine(17, 2, 4, mainColor);
  matrix.drawFastVLine(18, 3, 2, mainColor);
  // +=号
  if(brightness > 5){
    matrix.setCursor(3, 6);
    matrix.print("-");
  }
  if(brightness < 145){
    matrix.setCursor(26, 6);
    matrix.print("+");
  }
  matrix.show();
}

// 绘制动画页面
void drawAnim(){
  if(animPage == ANIM_MODEL1){
    if((millis() - animTime) < animInterval1) return;
    matrix.fillScreen(0);
    randomSeed(analogRead(RANDOM_SEED_PIN));
    for(int i = 0; i < MATRIX_COUNT * MATRIX_SIDE; i++){
      int animCount = 0; // 记录往下移一格后这一列还有几条动画存在（矩阵高度为8，动画高度为6，所以一列最多存在两条动画）
      int index1 = 0; // 记录往下移一格后第一个动画的头部位置
      int index2 = 0; // 记录往下移一格后第二个动画的头部位置
      // 根据矩阵二维数组判断这一列是否已经有动画，动画的起始位置（最上端）的值为1
      for(int j = 0; j < 13; j++){
        if(matrixArray[j][i] == 1){ // 这个位置是有动画的
          // 把这个动画往下移一格
          if((j + 1) <= 12){
            matrix.drawRGBBitmap(i,(j + 1 - 5),animHack,1,6);
            animCount++;
            if(animCount == 1){
              index1 = j + 1;
            }
            if(animCount == 2){
              index2 = j + 1;
            }
          }
        }
        // 将这个位置置为0
        matrixArray[j][i] = 0;
      }
      // 将新位置的值置为1
      if(animCount == 2){
        matrixArray[index1][i] = 1;
        matrixArray[index2][i] = 1;
      }else if(animCount == 1){
        matrixArray[index1][i] = 1;
      }
      int x = random(0, hackAnimProbability); // 随机数，如果是1则产生新动画
      if(x == 1){
        // 如果这一列没有动画，或者只有1个动画，并且这条动画与这条新生成的动画距离>=（6+1），就进入下面的判断
        if(animCount == 0 || (animCount == 1 && index1 >= 7)){
          // 判断左边同一行有没有动画，有的话也不生成新动画
          if(i == 0 || matrixArray[0][i - 1] == 0){
            matrix.drawRGBBitmap(i,-5,animHack,1,6);
            matrixArray[0][i] = 1;
          }
        }
      }
    }
  }else if(animPage == ANIM_MODEL2){
    if((millis() - animTime) < animInterval2) return;
    // matrixArray[0][0]记录大步骤，0：红色→黄色 1：黄色→绿色 2：绿色→蓝色 3：蓝色→紫色 4：紫色→红色
    // matrixArray[0][1]记录每一个大步骤的小步,每一种颜色渐变过程持续50步
    // matrixArray[0][2]记录颜色R
    // matrixArray[0][3]记录颜色G
    // matrixArray[0][4]记录颜色B
    if(matrixArray[0][0] == 0 && matrixArray[0][1] == 0){ // 刚开始设置为红色，初始值250
      matrixArray[0][2] = 250;
    }
    // 循环绘制竖线
    int tmpR = matrixArray[0][2];
    int tmpG = matrixArray[0][3];
    int tmpB = matrixArray[0][4];
    int tmpStep = matrixArray[0][0];
    for(int i = 0; i < MATRIX_SIDE * MATRIX_COUNT; i++){
      matrix.drawFastVLine(i, 0, 8, matrix.Color(tmpR, tmpG, tmpB));
      // 计算出下一个循环的颜色
      if(i != MATRIX_SIDE * MATRIX_COUNT - 1){
        if(tmpStep == 0){ // 红色→黄色
          tmpG = tmpG + 5;
          if(tmpG == 250){
            tmpStep++;
          }
        }else if(tmpStep == 1){ // 黄色→绿色
          tmpR = tmpR - 5;
          if(tmpR == 0){
            tmpStep++;
          }
        }else if(tmpStep == 2){ // 绿色→蓝色
          tmpG = tmpG - 5;
          tmpB = tmpB + 5;
          if(tmpG == 0){
            tmpStep++;
          }
        }else if(tmpStep == 3){ // 蓝色→紫色
          tmpR = tmpR + 5;
          if(tmpR == 250){
            tmpStep++;
          }
        }else if(tmpStep == 4){ // 紫色→红色
          tmpB = tmpB - 5;
          if(tmpB == 0){
            tmpStep = 0;
          }
        }
      }      
    }   
    if(matrixArray[0][0] == 0){ // 红色→黄色
      matrixArray[0][3] = matrixArray[0][3] + 5;
    }else if(matrixArray[0][0] == 1){ // 黄色→绿色
      matrixArray[0][2] = matrixArray[0][2] - 5;
    }else if(matrixArray[0][0] == 2){ // 绿色→蓝色
      matrixArray[0][3] = matrixArray[0][3] - 5;
      matrixArray[0][4] = matrixArray[0][4] + 5;
    }else if(matrixArray[0][0] == 3){ // 蓝色→紫色
      matrixArray[0][2] = matrixArray[0][2] + 5;
    }else if(matrixArray[0][0] == 4){ // 紫色→红色
      matrixArray[0][4] = matrixArray[0][4] - 5;
    }
    matrixArray[0][1] = matrixArray[0][1] + 1;
    if(matrixArray[0][1] == 50){
      matrixArray[0][1] = 0;
      matrixArray[0][0] = matrixArray[0][0] + 1;
      if(matrixArray[0][0] == 5){
        matrixArray[0][0] = 0;
      }
    } 
  }else if(animPage == ANIM_MODEL3){
    if((millis() - animTime) < animInterval3) return;
    // 计算是填充过程还是消失过程
    if(lightedCount == 0){
      increasing = true;
    }else if(lightedCount == MATRIX_SIDE * MATRIX_SIDE * MATRIX_COUNT){
      increasing = false;
    }
    randomSeed(analogRead(RANDOM_SEED_PIN));
    if(increasing){
      int num = random(1, 4); // 每次随机填充num个点
      if(num + lightedCount > MATRIX_SIDE * MATRIX_SIDE * MATRIX_COUNT){ // 不能超过灯珠总数量
        num = MATRIX_SIDE * MATRIX_SIDE * MATRIX_COUNT - lightedCount;
      }
      for(int i = 0; i < num; i++){
        int x = random(0, 32);
        int y = random(0, 8);
        while(matrixArray[y][x] == 1){ // 这个随机生成的坐标已经点亮
          x = random(0, 32);
          y = random(0, 8);
        }
        matrixArray[y][x] = 1;
        // 已点亮灯的个数 + 1
        lightedCount++;
        int r = random(10, 256);
        int g = random(10, 256);
        int b = random(10, 256);
        matrix.drawPixel(x, y, matrix.Color(r, g, b));
      }
    }else{
      int num = random(1, 4); // 每次随机消失num个点
      if(num > lightedCount){ // 不能大于剩余已点亮数量
        num = lightedCount;
      }
      for(int i = 0; i < num; i++){
        int x = random(0, 32);
        int y = random(0, 8);
        while(matrixArray[y][x] == 0){ // 这个随机生成的坐标已经熄灭
          x = random(0, 32);
          y = random(0, 8);
        }
        matrixArray[y][x] = 0;
        // 已点亮灯的个数 - 1
        lightedCount--;
        matrix.drawPixel(x, y, 0);
      }
    }
  }
  matrix.show();
  animTime = millis();
}

// 绘制节奏灯页面
void drawRHYTHM(){
  matrix.clear();
  // 重置频率块的值
  for (int i = 0; i < NUM_BANDS; i++){
    bandValues[i] = 0;
  }
  // 采样SAMPLES次
  for (int i = 0; i < SAMPLES; i++) {
    starTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN);
    vImag[i] = 0;
    // Serial.println(micros() - starTime);
    while ((micros() - starTime) < sampling_period_us) { /* chill */ }
    // Serial.println(vReal[i]);
  }
  // 进行FFT计算
  FFT.DCRemoval();
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();
  // 解析计算结果
  for (int i = 2; i < (SAMPLES/2); i++){
    if (vReal[i] > NOISE) {
      // Serial.println(vReal[i]);
      // 去除前面6段低频杂音和一些高频尖叫
      if (i>6    && i<=9   ) bandValues[0]   += (int)vReal[i];
      if (i>9    && i<=11  ) bandValues[1]   += (int)vReal[i];
      if (i>11   && i<=13  ) bandValues[2]   += (int)vReal[i];
      if (i>13   && i<=15  ) bandValues[3]   += (int)vReal[i];
      if (i>15   && i<=17  ) bandValues[4]   += (int)vReal[i];
      if (i>17   && i<=19  ) bandValues[5]   += (int)vReal[i];
      if (i>19   && i<=21  ) bandValues[6]   += (int)vReal[i];
      if (i>21   && i<=23  ) bandValues[7]   += (int)vReal[i];
      if (i>23   && i<=25  ) bandValues[8]   += (int)vReal[i];
      if (i>25   && i<=27  ) bandValues[9]   += (int)vReal[i];
      if (i>27   && i<=29  ) bandValues[10]  += (int)vReal[i];
      if (i>29   && i<=31  ) bandValues[11]  += (int)vReal[i];
      if (i>31   && i<=33  ) bandValues[12]  += (int)vReal[i];
      if (i>33   && i<=35  ) bandValues[13]  += (int)vReal[i];
      if (i>35   && i<=38  ) bandValues[14]  += (int)vReal[i];
      if (i>38   && i<=41  ) bandValues[15]  += (int)vReal[i];
      if (i>41   && i<=44  ) bandValues[16]  += (int)vReal[i];
      if (i>44   && i<=47  ) bandValues[17]  += (int)vReal[i];
      if (i>47   && i<=50  ) bandValues[18]  += (int)vReal[i];
      if (i>50   && i<=53  ) bandValues[19]  += (int)vReal[i];
      if (i>53   && i<=56  ) bandValues[20]  += (int)vReal[i];
      if (i>56   && i<=59  ) bandValues[21]  += (int)vReal[i];
      if (i>59   && i<=62  ) bandValues[22]  += (int)vReal[i];
      if (i>62   && i<=65  ) bandValues[23]  += (int)vReal[i];
      if (i>65   && i<=68  ) bandValues[24]  += (int)vReal[i];
      if (i>68   && i<=71  ) bandValues[25]  += (int)vReal[i];
      if (i>71   && i<=74  ) bandValues[26]  += (int)vReal[i];
      if (i>74   && i<=77  ) bandValues[27]  += (int)vReal[i];
      if (i>77   && i<=80  ) bandValues[28]  += (int)vReal[i];
      if (i>80   && i<=83  ) bandValues[29]  += (int)vReal[i];
      if (i>83   && i<=87  ) bandValues[30]  += (int)vReal[i];
      if (i>87   && i<=91  ) bandValues[31]  += (int)vReal[i];
    }
  }
  // 将FFT数据处理为条形高度  
  int color = 0;
  int r, g, b;
  for (byte band = 0; band < NUM_BANDS; band++) {
    // 根据倍率缩放条形图高度
    int barHeight = bandValues[band] / AMPLITUDE;
    if (barHeight > MATRIX_SIDE) barHeight = MATRIX_SIDE;
    // 旧的高度值和新的高度值平均一下
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;
    // 如果条形的高度大于顶点高度，则调整顶点高度
    if (barHeight > peak[band]) {
      peak[band] = min(MATRIX_SIDE - 0, barHeight);
    }
    // 绘制操作
    switch (rhythmPage){
      case RHYTHM_MODEL1:
        // 绘制条形
        matrix.drawFastVLine((MATRIX_WIDTH - 1 - band), (MATRIX_SIDE - barHeight), barHeight, hsv2rgb(color,100,100));
        color += 360 / (NUM_BANDS + 4);
        // 绘制顶点
        matrix.drawPixel((MATRIX_WIDTH - 1 - band), MATRIX_SIDE - peak[band] - 1, matrix.Color(150,150,150));
        break;  
      case RHYTHM_MODEL2:
        // 绘制条形
        r = model2ColorArray[0][0];
        g = model2ColorArray[0][1];
        b = model2ColorArray[0][2];
        for (int y = MATRIX_SIDE; y >= MATRIX_SIDE - barHeight; y--) {
          matrix.drawPixel((MATRIX_WIDTH - 1 - band), y, matrix.Color(r, g, b));
          r+=(model2ColorArray[1][0] - model2ColorArray[0][0]) / barHeight;
          g+=(model2ColorArray[1][1] - model2ColorArray[0][1]) / barHeight;
          b+=(model2ColorArray[1][2] - model2ColorArray[0][2]) / barHeight;
        }
        // 绘制顶点
        matrix.drawPixel((MATRIX_WIDTH - 1 - band), MATRIX_SIDE - peak[band] - 1, matrix.Color(150,150,150));
        break;
      case RHYTHM_MODEL3:
        // 绘制条形,此模式不绘制顶点
        for (int y = MATRIX_SIDE; y >= MATRIX_SIDE - barHeight; y--) {
          matrix.drawPixel((MATRIX_WIDTH - 1 - band), y, hsv2rgb(y * (255 / MATRIX_WIDTH / 5) + colorTime, 100, 100));
        }
        break;
      case RHYTHM_MODEL4:
        // 此模式下，只绘制顶点      
        matrix.drawPixel((MATRIX_WIDTH - 1 - band), (MATRIX_SIDE - peak[band] - 1), 
        matrix.Color(model4ColorArrar[peak[band]][0], model4ColorArrar[peak[band]][1], model4ColorArrar[peak[band]][2]));
        break;  
      default:
        break;
    }   
    // 将值记录到oldBarHeights
    oldBarHeights[band] = barHeight;
  }
  // 70毫秒降低一次顶点
  if((millis() - peekDecayTime) >= 70){
    for (byte band = 0; band < NUM_BANDS; band++){
      if (peak[band] > 0) peak[band] -= 1;
    }
    colorTime++;
    peekDecayTime = millis();
  }
  // 10毫秒变换一次颜色
  if((millis() - changeColorTime) >= 10){
    colorTime++;
    changeColorTime = millis();
  }
  matrix.show();
}

// HSV转RGB格式
uint16_t hsv2rgb(uint16_t hue, uint8_t saturation, uint8_t value){
  uint8_t r, g, b;
  uint16_t h = (hue / 60) % 6;
  uint16_t F = 100 * hue / 60 - 100 * h;
  uint16_t P = value * (100 - saturation) / 100;
  uint16_t Q = value * (10000 - F * saturation) / 10000;
  uint16_t T = value * (10000 - saturation * (100 - F)) / 10000;
  switch (h){
    case 0:
        r = value;
        g = T;
        b = P;
        break;
    case 1:
        r = Q;
        g = value;
        b = P;
        break;
    case 2:
        r = P;
        g = value;
        b = T;
        break;
    case 3:
        r = P;
        g = Q;
        b = value;
        break;
    case 4:
        r = T;
        g = P;
        b = value;
        break;
    case 5:
        r = value;
        g = P;
        b = Q;
        break;
    default:
      return matrix.Color(255, 0, 0);
  }
  r = r * 255 / 100;
  g = g * 255 / 100;
  b = b * 255 / 100;
  return matrix.Color(r, g, b);
}

// 重新设置闹钟相关的临时数据
void resetTmpClockData(){
  tmpClockH = clockH;
  tmpClockM = clockM;
  tmpClockBellNum = clockBellNum;
  clockChoosed = CLOCK_H;
}

// 绘制闹钟页面
void drawClock(){
  // 清屏
  matrix.fillScreen(0);
  if(clockOpen){
    // 绘制时间
    matrix.setTextColor(mainColor);
    matrix.setCursor(2, 5);
    String h;
    if(tmpClockH < 10){
      h = "0" + String(tmpClockH);
    }else{
      h = String(tmpClockH);
    }
    String m;
    if(tmpClockM < 10){
      m = "0" + String(tmpClockM);
    }else{
      m = String(tmpClockM);
    }
    matrix.print(h + ":" + m);
    // 绘制闹钟图标
    matrix.drawBitmap(22, 1, bell, 8, 6, mainColor);
    // 绘制指示线
    if(clockChoosed == CLOCK_H){
      matrix.drawFastHLine(2, 7, 7, weekColor);
    }else if(clockChoosed == CLOCK_M){
      matrix.drawFastHLine(12, 7, 7, weekColor);
    }else if(clockChoosed == CLOCK_BELL){
      matrix.drawFastHLine(22, 7, 8, weekColor);
    }
  }else{
    matrix.drawBitmap(8, 1, bell, 8, 6, mainColor);
    matrix.setCursor(20, 6);
    matrix.setTextColor(matrix.Color(255, 0, 0));
    matrix.print("X");  
  } 
  matrix.show();
}

