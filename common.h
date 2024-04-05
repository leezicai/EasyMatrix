#ifndef __COMMON_H
#define __COMMON_H

#include "img/timeAnim0.h"
#include "img/timeAnim1.h"
#include "img/timeAnim2.h"
#include "img/timeAnim3.h"
#include "img/timeAnim4.h"
#include "img/timeAnim5.h"
#include "img/timeAnim6.h"
#include "img/timeAnim7.h"
#include "img/timeAnim8.h"
#include "img/animHack.h"
#include "img/bell.h"
#include "img/checkingTime.h"

#define DATAPIN               6 // 灯珠矩阵输入引脚
#define LIGHTCOUNT            256 // 灯珠个数
#define BTN1                  5 // 按钮1
#define BTN2                  4 // 按钮2
#define BTN3                  8 // 按钮3
#define BUZZER                0 // 蜂鸣器
#define AUDIO_IN_PIN          1 // 拾音器
#define BRIGHTNESS            45 // 默认亮度
#define MATRIX_SIDE           8 //每个矩阵的边长
#define MATRIX_WIDTH          32 //矩阵总宽度
#define MATRIX_COUNT          4 //矩阵数量
#define BRIGHTNESS_SPACING    20 //每次调节亮度的间隔大小
#define RANDOM_SEED_PIN       11 // 随机数种子引脚
#define NTP3                  "ntp4.ntsc.ac.cn"
#define NTP2                  "ntp3.ict.ac.cn"
#define NTP1                  "ntp2.aliyun.com"
#define SAMPLES               256  //采样个数
#define SAMPLING_FREQ         10000 //采样频率
#define AMPLITUDE             1000  //声音强度调整倍率（柱状高度倍率）
#define NUM_BANDS             32  //频段个数 
#define NOISE                 1770   //噪音
#define BAR_WIDTH             1 //每个频段的宽度
#define ONE_DAY_SECONDS       24 * 60 * 60 // 一天的秒数
#define ANIM_INTERVAL         200 // 时钟页面每帧动画间隔（ms）
#define TIME_CHECK_INTERVAL   18000 // NTP对时间隔（s）, 18000秒即为5小时

// 时钟页面下的小页面
const int TIME_H_M_S = 1;
const int TIME_H_M = 2;
const int TIME_DATE = 3;
// 节奏灯页面下的小页面
const int RHYTHM_MODEL1 = 1;
const int RHYTHM_MODEL2 = 2;
const int RHYTHM_MODEL3 = 3;
const int RHYTHM_MODEL4 = 4;
// 动画页面下的小页面
const int ANIM_MODEL1 = 1;
const int ANIM_MODEL2 = 2;
const int ANIM_MODEL3 = 3;
// 闹钟页面选中项
const int CLOCK_H = 1;
const int CLOCK_M = 2;
const int CLOCK_BELL = 3;

// 定义页面枚举 SETTING-配网页面  TIME-时间页面  RHYTHM-节奏灯页面  ANIM-动画页面  CLOCK-闹钟设置页面  BRIGHT-亮度调节
enum CurrentPage{
  SETTING, TIME, RHYTHM, ANIM, CLOCK, BRIGHT
};


// 配置WiFi的网页代码
const String ROOT_HTML_PAGE1 PROGMEM = R"rawliteral(
  <!DOCTYPE html><html lang='zh'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <link href='favicon.ico' rel='shortcut icon'>
    <title>EasyMatrix配置页面</title>
    <style type='text/css'>
        #titleDiv{
            margin-top: 20px;
            height: 10%;
            width: 100%;
            text-align: center;
            font-size: 2rem;
            font-weight: bold;
        }
        .titleOption{
            text-align: center;
            margin-top: 30px;
            height: 40px;
            background-color: dodgerblue; 
            position: relative;
            color: #ffffff;
            border-radius: 5px;
            line-height: 40px;
        }
        #selectDiv {
            margin-top: 20px;
            height: 40px;
            border-radius: 5px;
            box-shadow: 0 0 5px #ccc;
            position: relative;   
        }
        select {
            border: none;
            outline: none;
            width: 100%;
            height: 40px;
            line-height: 40px;
            appearance: none;
            -webkit-appearance: none;
            -moz-appearance: none;
            text-align: center;
            font-size: 1rem;
        }
        .passAndCity{
            border: none;
            margin-top: 20px;
            height: 40px;
            border-radius: 5px;
            box-shadow: 0 0 5px #ccc;
            font-size: 1rem;
            position: relative;
            text-align: center;
        }
        #rgbDiv{
            margin-top: 25px;
            position: relative;
            text-align: center;
        }
        #rgbDiv input{
            height: 35px;
            border-radius: 5px;
            box-shadow: 0 0 5px #ccc;
            font-size: 1rem;
        }
        .rgb{
            margin-left: 5px;
            width: 26%;
        }
        #sub{
            text-align: center;
            margin-top: 50px;
            height: 40px;
            background-color: dodgerblue; 
            position: relative;
            color: #ffffff;
            border-radius: 5px;
            line-height: 40px;
            cursor: pointer;
        }
        #tail{
            font-size: 0.9rem;
            margin-top: 5px;
            width: 100%;
            text-align: center;
            color: #757575;
        }
    </style>
</head>
<body>
    <div id='titleDiv'>EasyMatrix像素钟</div>
    <div id='tail'>呈杰希工作室&nbsp&nbsp&nbsp&nbsp出品</div>
    <form action='configwifi' method='post' id='form' accept-charset="UTF-8">
        <div class='titleOption commonWidth'>WiFi名称</div>
        <div id='selectDiv' class='commonWidth'>
            <select name='ssid' id='ssid'>
                <option value=''></option>
)rawliteral";
const String ROOT_HTML_PAGE2 PROGMEM = R"rawliteral(
  </select>
        </div>
        <div class='titleOption commonWidth'>WiFi密码</div>
)rawliteral";
const String ROOT_HTML_PAGE3 PROGMEM = R"rawliteral(
  <div class='titleOption commonWidth'>时钟颜色(RGB数值)</div>
        <div class='commonWidth' id="rgbDiv">
)rawliteral";
const String ROOT_HTML_PAGE4 PROGMEM = R"rawliteral(
  </div>
        <div class='commonWidth' style="background-color: '0 20 23';"></div>
        <div id='sub' onclick='doSubmit()'>提交</div>
    </form>
    <script type='text/javascript'>
        function doSubmit(){
            var select = document.getElementById('ssid');
            var selectValue = select.options[select.selectedIndex].value;
            if(selectValue == ''){
                alert('请选择要连接的WiFi');
                return;
            }
            if(document.getElementById('pass').value == ''){
                alert('请输入该WiFi的密码');
                return;
            }
            var red = document.getElementById('red').value;
            var green = document.getElementById('green').value;
            var blue = document.getElementById('blue').value;
            if(red == '' || green == '' || blue == ''){
                alert('请输入完整的RGB颜色数值');
                return;
            }
            if(red.indexOf('.') > 0 || green.indexOf('.') > 0 || blue.indexOf('.') > 0){
                alert('RGB数值必须是0到255之间的整数');
                return;
            }
            if(parseInt(red) < 0 || parseInt(red) > 255){
                alert('RGB数值必须是0到255之间的整数');
                document.getElementById('red').value = '';
                return;
            }
            if(parseInt(green) < 0 || parseInt(green) > 255){
                alert('RGB数值必须是0到255之间的整数');
                document.getElementById('green').value = '';
                return;
            }
            if(parseInt(blue) < 0 || parseInt(blue) > 255){
                alert('RGB数值必须是0到255之间的整数');
                document.getElementById('blue').value = '';
                return;
            }
            document.getElementById('form').submit();
        }
        var nodes = document.getElementsByClassName('commonWidth');
        var rgbNodes = document.getElementsByClassName('rgb');
        var node = document.getElementById('sub');
        var screenWidth = window.screen.width;
        function setWidth(width){
            nodes[0].setAttribute('style',width);
            nodes[1].setAttribute('style',width);
            nodes[2].setAttribute('style',width);
            nodes[3].setAttribute('style',width);
            nodes[4].setAttribute('style',width);
            nodes[5].setAttribute('style',width);
        }
        if(screenWidth > 1000){
            setWidth('width: 40%;left: 30%;');
            node.setAttribute('style','width: 14%;left: 43%;');
        }else if(screenWidth > 800 && screenWidth <= 1000){
            setWidth('width: 50%;left: 25%;');
            node.setAttribute('style','width: 16%;left: 42%;');
        }else if(screenWidth > 600 && screenWidth <= 800){
            setWidth('width: 60%;left: 20%;');
            node.setAttribute('style','width: 20%;left: 40%;');
        }else if(screenWidth > 400 && screenWidth <= 600){
            setWidth('width: 74%;left: 13%;');
            node.setAttribute('style','width: 26%;left: 37%;');
        }else{
            setWidth('width: 90%;left: 5%;');
            node.setAttribute('style','width: 40%;left: 30%;');
        }
    </script>
</body>
</html>
)rawliteral";
#endif

