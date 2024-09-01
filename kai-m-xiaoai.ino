#define BLINKER_WIFI
#define BLINKER_MIOT_MULTI_OUTLET
#include <Blinker.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <time.h>

char auth[] = "20f6bd964a10";
char ssid[] = "GSJ";
char pswd[] = "82583485";

// 继电器控制引脚
int relay1 = 13;
int relay2 = 12;
int relay3 = 14;
int LED_PIN = 2;
const int BLINK_COUNT = 3; // 设置闪烁次数
const int BLINK_DELAY = 100; // 设置闪烁间隔（毫秒）

// 新建组件对象
BlinkerButton Button1("btn-1");
BlinkerButton Button2("btn-2");
BlinkerButton Button3("btn-3");

// 定义NTP客户端获取时间
WiFiUDP ntpUDP;
long utcOffsetInSeconds = 8 * 60 * 60;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", utcOffsetInSeconds); 

// 定义每周对时的Ticker
Ticker weeklyTimeSync;

// 时间记录数组和索引
String timeRecords[10];
int timeRecordIndex = 0;

// 按钮回调函数
void button1_callback(const String &state) {
    digitalWrite(relay1, HIGH);
    blinkLED(BLINK_COUNT, BLINK_DELAY); // 闪烁LED
    BlinkerMIOT.powerState("on", 1);
    BlinkerMIOT.print();
    Blinker.notify("-----大门-----打开-----");
    Blinker.delay(800);
    digitalWrite(relay1, LOW);
    BlinkerMIOT.powerState("off", 1);
    BlinkerMIOT.print();
    Serial.println("-----大门-----打开-----");
    recordTime("开门");
}

void button2_callback(const String &state) {
    digitalWrite(relay2, HIGH);
    blinkLED(BLINK_COUNT, BLINK_DELAY); // 闪烁LED
    BlinkerMIOT.powerState("on", 2);
    BlinkerMIOT.print();
    Blinker.notify("-----大门-----暂停-----");
    Blinker.delay(800);
    digitalWrite(relay2, LOW);
    BlinkerMIOT.powerState("off", 2);
    BlinkerMIOT.print();
    Serial.println("-----大门-----暂停-----");
    recordTime("暂停");
}

void button3_callback(const String &state) {
    digitalWrite(relay3, HIGH);
    blinkLED(BLINK_COUNT, BLINK_DELAY); // 闪烁LED
    BlinkerMIOT.powerState("on", 3);
    BlinkerMIOT.print();
    Blinker.notify("-----大门-----关闭-----");
    Blinker.delay(800);
    digitalWrite(relay3, LOW);
    BlinkerMIOT.powerState("off", 3);
    BlinkerMIOT.print();
    Serial.println("-----大门-----关闭-----");
    recordTime("关门");
}

// 记录时间和按键信息的函数
void recordTime(const String &button) {
    time_t now = timeClient.getEpochTime() - 8 * 3600; // 手动调整为北京时间
    struct tm *timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    String currentTime = String(buffer);
    timeRecords[timeRecordIndex] = button + " - " + currentTime;
    timeRecordIndex = (timeRecordIndex + 1) % 10; // 环形数组
}

// 小爱同学控制回调函数
void miotPowerState(const String &state, uint8_t num) {
    // 根据传入的编号执行相应的按钮回调函数
    if (state == "on") {
        switch (num) {
            case 1:
                button1_callback(state); // 执行 button1_callback
                break;
            case 2:
                button2_callback(state); // 执行 button2_callback
                break;
            case 3:
                button3_callback(state); // 执行 button3_callback
                break;
            default:
                // 如果编号不是1、2、3，则不执行任何操作
                break;
        }
    } else {
        // 如果状态不是 "on"，则不执行任何操作
    }
}

// 对时函数
void syncTime() {
    timeClient.forceUpdate(); // 强制更新时间
    time_t now = timeClient.getEpochTime() - 8 * 3600; // 手动调整为北京时间
    struct tm *timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
    String currentTime = String(buffer);
    String syncResult = "时间同步完成: " + currentTime;
    Serial.println(syncResult);
    Blinker.print(syncResult);
}

// 计算下一次对时的秒数
unsigned long calculateNextTimeSync() {
    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = gmtime(&now);
    // 设置下一个周日凌晨3点对时的时间
    timeinfo->tm_hour = 3;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;
    timeinfo->tm_wday = 0; // 周日
    time_t nextSync = mktime(timeinfo);
    // 如果已经过了这个时间，则加上一周的秒数
    if (nextSync <= now) {
        nextSync += 7 * 24 * 60 * 60;
    }
    return nextSync - now;
}

// 处理历史记录请求
void dataRead(const String &data) {
    if (data == "历史记录") {
        String history = "最近10次按钮执行时间:\n";
        for (int i = 0; i < 10; i++) {
            int index = (timeRecordIndex - 1 - i + 10) % 10;
            if (timeRecords[index].length() > 0) {
                history += String(i + 1) + ". " + timeRecords[index] + "\n";
            } else {
                history += String(i + 1) + ". (空)\n"; // 添加调试信息
            }
        }
        Serial.println("历史记录: " + history); // 添加调试信息
        sendHistory(history);
    } else if (data == "同步时间") {
        syncTime();
    } else if (data == "运行时间") {
        printUptime();
    }
}
// 分行发送历史记录
void sendHistory(const String &history) {
    int length = history.length();
    int start = 0;
    int end;
    while (start < length) {
        end = history.indexOf('\n', start);
        if (end == -1) {
            end = length;
        }
        Blinker.print(history.substring(start, end));
        start = end + 1;
    }
}

// 打印系统运行时间
void printUptime() {
    unsigned long millisec = millis();
    unsigned long days = millisec / (1000 * 60 * 60 * 24);
    millisec %= (1000 * 60 * 60 * 24);
    unsigned long hours = millisec / (1000 * 60 * 60);
    millisec %= (1000 * 60 * 60);
    unsigned long minutes = millisec / (1000 * 60);
    millisec %= (1000 * 60);
    unsigned long seconds = millisec / 1000;

    String uptime = "系统已运行时间: " + String(days) + "天 " + String(hours) + "小时 " + String(minutes) + "分钟 " + String(seconds) + "秒";
    Serial.println(uptime);
    Blinker.print(uptime);
}

void setup() {
    // 初始化继电器引脚
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);
    pinMode(relay3, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    digitalWrite(relay3, LOW);
    digitalWrite(LED_PIN, HIGH);

    // 初始化Blinker
    Serial.begin(115200);
    Blinker.begin(auth, ssid, pswd);
    Button1.attach(button1_callback);
    Button2.attach(button2_callback);
    Button3.attach(button3_callback);
    BlinkerMIOT.attachPowerState(miotPowerState);

    // 初始化NTP客户端
    timeClient.begin();
    timeClient.setTimeOffset(utcOffsetInSeconds); 
   
    // 安排每周对时，例如每周日凌晨3点对时
    unsigned long secondsUntilSync = calculateNextTimeSync();
    weeklyTimeSync.attach(secondsUntilSync, syncTime);

    // 注册调试窗口数据处理函数
    Blinker.attachData(dataRead);
}

void blinkLED(int count, int delayTime) {
  for (int i = 0; i < count; i++) {
    digitalWrite(LED_PIN, LOW); // 打开LED
    delay(delayTime); // 等待一段时间
    digitalWrite(LED_PIN, HIGH); // 关闭LED
    delay(delayTime); // 等待一段时间
  }
}

void loop() {
    // 更新NTP客户端时间
    timeClient.update();

    // 运行Blinker
    Blinker.run();
}
