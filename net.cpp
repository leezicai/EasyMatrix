#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "common.h"
#include "preferencesUtil.h"
#include "light.h"
#include "task.h"

void sendNTPpacket(IPAddress &address);
void connectWiFi(int timeOut_s);
void startAP();
void startServer();
void scanWiFi();
void getHTML();
void handleNotFound();
void handleRoot();
void handleConfigWifi();

// Wifi相关
String ssid;  //WIFI名称
String pass;  //WIFI密码
String WifiNames; // 根据搜索到的wifi生成的option字符串
String PassHTML; // 根据密码生成的HTML字符串
String RGBColors; // 根据查询到的NVS中的值生成的RGBinput字符串
bool apConfig; // 系统启动时是否需要配网
// SoftAP相关
const char *APssid = "EasyMatrix";
IPAddress staticIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);
// 是否顺利连接上wifi
bool wifiConnected = false;
// 是否NTP对时成功
bool RTCSuccess = false;

// 开启SoftAP进行配网
void wifiConfigBySoftAP(){
  // 开启AP模式，如果开启失败，重启系统
  startAP();
  // 扫描WiFi,并将扫描到的WiFi组成option选项字符串
  scanWiFi();
  // 将从NVS中获取到的颜色信息组合成字符串,将wifi密码组合成字符串
  getHTML();
  // 启动服务器
  startServer();
  // 重新生成动画任务,提示用户配网
  vTaskDelete(showTextTask);
  delay(300);
  createShowIpTask();
}

// 处理服务器请求
void doClient(){
  server.handleClient();
}
// 处理404情况的函数'handleNotFound'
void handleNotFound(){
  handleRoot();//访问不存在目录则返回配置页面
}
// 处理网站根目录的访问请求
void handleRoot(){
  server.send(200,"text/html", ROOT_HTML_PAGE1 + WifiNames + ROOT_HTML_PAGE2 + PassHTML + ROOT_HTML_PAGE3 + RGBColors + ROOT_HTML_PAGE4);
}
// 提交数据后的提示页面
void handleConfigWifi(){
  //判断是否有WiFi名称
  if (server.hasArg("ssid")){
    Serial.print("获得WiFi名称:");
    ssid = server.arg("ssid");
    Serial.println(ssid);
  }else{
    Serial.println("错误, 没有发现WiFi名称");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现WiFi名称");
    return;
  }
  //判断是否有WiFi密码
  if (server.hasArg("pass")){
    Serial.print("获得WiFi密码:");
    pass = server.arg("pass");
    Serial.println(pass);
  }else{
    Serial.println("错误, 没有发现WiFi密码");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现WiFi密码");
    return;
  }
  // 判断是否有RGB数据
  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")){
    Serial.print("获得RGB数据:");
    clockColor[0] = server.arg("red").toInt();
    clockColor[1] = server.arg("green").toInt();
    clockColor[2] = server.arg("blue").toInt();
    Serial.print(clockColor[0]);Serial.print(" ");Serial.print(clockColor[1]);Serial.print(" ");Serial.println(clockColor[2]);
  }else{
    Serial.println("错误, 没有发现RGB颜色数据");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现RGB颜色数据");
    return;
  }
  // 将信息存入nvs中
  recordInfos(ssid, pass, clockColor[0], clockColor[1], clockColor[2], false);
  // 获得了所需要的一切信息，给客户端回复
  server.send(200, "text/html", "<meta charset='UTF-8'><style type='text/css'>body {font-size: 2rem;}</style><br/><br/>WiFi: " + ssid + "<br/>密码: " + pass + "<br/>RGB颜色: " + clockColor[0] + " " + clockColor[1] + " " + clockColor[2] + " " + "<br/>已取得相关信息,正在尝试连接,请手动关闭此页面。");
  ESP.restart();
}

// 连接WiFi
void connectWiFi(int timeOut_s){
  int connectTime = 0; //用于连接计时，如果长时间连接不成功，则提示失败
  Serial.print("正在连接网络");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    connectTime++;
    if (connectTime > 2 * timeOut_s){ //长时间连接不上，提示连接失败
      Serial.println("网络连接失败...");
      // 停止启动加载动画
      vTaskDelete(showTextTask);
      delay(300);
      // 屏幕显示wifi连接失败
      drawFailed(4, 24, "WIFI");
      delay(2000);
      // 清空屏幕
      clearMatrix();
      // 至节奏灯页面
      currentPage = RHYTHM;
      // 跳出循环
      return;
    }
  }
  wifiConnected = true;
  Serial.println("网络连接成功");
  Serial.print("本地IP： ");
  Serial.println(WiFi.localIP());
}

// 启动服务器
void startServer(){
  // 当浏览器请求服务器根目录(网站首页)时调用自定义函数handleRoot处理，设置主页回调函数，必须添加第二个参数HTTP_GET，否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);
  // 当浏览器请求服务器/configwifi(表单字段)目录时调用自定义函数handleConfigWifi处理
  server.on("/configwifi", HTTP_POST, handleConfigWifi);
  // 当浏览器请求的网络资源无法在服务器找到时调用自定义函数handleNotFound处理   
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("服务器启动成功！");
}
// 开启AP模式，如果开启失败，重启系统
void startAP(){
  Serial.println("开启AP模式...");
  WiFi.enableAP(true); // 使能AP模式
  //传入参数静态IP地址,网关,掩码
  WiFi.softAPConfig(staticIP, gateway, subnet);
  if (!WiFi.softAP(APssid)) {
    Serial.println("AP模式启动失败");
    ESP.restart(); // Ap模式启动失败，重启系统
  }  
  Serial.println("AP模式启动成功");
  Serial.print("IP地址: ");
  Serial.println(WiFi.softAPIP());
}
// 扫描WiFi,并将扫描到的Wifi组成option选项字符串
void scanWiFi(){
  Serial.println("开始扫描WiFi");
  int n = WiFi.scanNetworks();
  if (n){
    Serial.print("扫描到");
    Serial.print(n);
    Serial.println("个WIFI");
    WifiNames = "";
    bool hasSavedSSID = false; // 记录是否扫描到已经存储在NVS中的wifi
    for (size_t i = 0; i < n; i++){
      int32_t rssi = WiFi.RSSI(i);
      String signalStrength;
      if(rssi >= -35){
        signalStrength = " (信号极强)";
      }else if(rssi >= -50){
        signalStrength = " (信号强)";
      }else if(rssi >= -70){
        signalStrength = " (信号中)";
      }else{
        signalStrength = " (信号弱)";
      }
      WifiNames += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + signalStrength + "</option>";
      if(WiFi.SSID(i).equals(ssid) && !hasSavedSSID){
        hasSavedSSID = true;
        WifiNames += "<option selected value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + signalStrength + "</option>";
      }else{
        WifiNames += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + signalStrength + "</option>";
      }
    }
    if(!hasSavedSSID && !ssid.equals("")){ // 如果扫描到的wifi中，没有已经存储的wifi，则手动添加进去
      WifiNames += "<option selected value='" + ssid + "'>" + ssid + "</option>";
    }
  }else{
    Serial.println("没扫描到WIFI");
  }
}
// 将从NVS中获取到的颜色信息组合成字符串,将wifi密码组合成字符串
void getHTML(){
  RGBColors = "";
  RGBColors += "R<input type='number' placeholder='0-255' name='red' id='red' class='rgb' value='" + String(clockColor[0]) + "'>";
  RGBColors += "G<input type='number' placeholder='0-255' name='green' id='green'  class='rgb' value='" + String(clockColor[1]) + "'>";
  RGBColors += "B<input type='number' placeholder='0-255' name='blue' id='blue'  class='rgb' value='" + String(clockColor[2]) + "'>";
  PassHTML = "<input type='text' placeholder='请输入WiFi密码' name='pass' id='pass' class='passAndCity commonWidth' value='" + pass + "'>";
}
// 获取NTP并同步RTC时间
void getNTPTime(){
  // 8 * 3600 东八区时间修正
  // 使用夏令时 daylightOffset_sec 就填写3600，否则就填写0；
  Serial.println("执行NTP对时");
  configTime( 8 * 3600, 0, NTP1, NTP2, NTP3);
}

// 系统启动时进行NTP对时
void checkTime(int limitTime){
  // 记录此时的时间，在NTP对时时，超过一定的时间，就直接重启
  time_t start;
  time(&start);
  // 获取NTP并同步至RTC,第一次同步失败，就一直尝试同步
  getNTPTime();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)){
    time_t end;
    time(&end);
    if((end - start) > limitTime){
      Serial.println("时钟对时超时...");
      // RTCSuccess初始化时就是false，所以只需要在对时成功后赋值true就可以
      // 这样也可以在启动对时成功，之后每小时一次对时失败时，不会将RTCSuccess置为false，影响某些程序运行
      // RTCSuccess = false;
      // 停止启动加载动画
      vTaskDelete(showTextTask);
      delay(300);
      // 屏幕显示TIME获取失败
      drawFailed(4, 24, "TIME");
      delay(2000);
      // 清空屏幕
      clearMatrix();
      // 至节奏灯页面
      currentPage = RHYTHM;
      // 跳出循环
      return;
    }
    Serial.println("时钟对时失败...");
    getNTPTime();
  }
  // 到了这一步，说明对时成功
  RTCSuccess = true;
}

// 关闭wifi
void disConnectWifi(){
  WiFi.disconnect();
}

// 定时对时
void checkTimeTicker(){
  // 重新连接wifi
  Serial.println("重新连接网络...");
  int connectTime = 0; //连接计时
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    connectTime++;
    if (connectTime > 20){ //循环20次（10秒）连接不上，就退出
      Serial.println("网络连接失败...");
      wifiConnected = false;
      // 跳出循环
      break;
    }
  }
  // wifi连接成功，进行对时
  if(wifiConnected){
    // 只对一次，不管是否成功，因为启动时已经成功进行了对时，这次无关紧要
    getNTPTime();
    // 对时后如果闹钟开启，就重置闹钟时间
    if(clockOpen){
      tickerClock.detach();
      startTickerClock(getClockRemainSeconds());
    }
  }
  // 关闭wifi功能
  disConnectWifi();
}



