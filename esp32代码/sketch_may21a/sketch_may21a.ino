#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

// WiFi配置
const char* ssid = "Man";
const char* password = "hjnb0404";

// 华为云 IoTDA配置
const char* mqttServer = "d82876d880.st1.iotda-device.cn-north-4.myhuaweicloud.com";
const int   mqttPort = 8883;
const char* mqttClientId = "682dab1284adf27cda5af276_GPS_DHT11_GM_0_0_2025052110";
const char* mqttUser = "682dab1284adf27cda5af276_GPS_DHT11_GM";
const char* mqttPassword = "00089d9595e92077d30a54a20c0372fe979b4c0802d41044e543aa1f6bdc0c38";
const char* mqttTopic = "$oc/devices/682dab1284adf27cda5af276_GPS_DHT11_GM/sys/properties/report";

// ----------- DHT11配置 -----------
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ----------- LDR配置 -------------
#define LDR_PIN 34 // 用D2（GPIO2，ADC1_CH2）

// ----------- GPS配置 -------------
#define GPS_RX 17      // GPS模块TX -> ESP32 RX2 (GPIO17)
#define GPS_TX 16      // GPS模块RX -> ESP32 TX2 (不用接)
#define GPS_BAUD 38400
HardwareSerial GPSSerial(2);
TinyGPSPlus gps;

WiFiClientSecure espClient;
PubSubClient client(espClient);

// 跳过证书校验（仅测试用，生产要加根证书）
void setup_wifi() {
  delay(100);
  Serial.println();
  Serial.print("连接WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi已连接");
  Serial.print("IP地址: "); Serial.println(WiFi.localIP());
  // 不校验证书
  espClient.setInsecure();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("连接到MQTT...");
    if (client.connect(mqttClientId, mqttUser, mqttPassword)) {
      Serial.println("已连接");
    } else {
      Serial.print("失败, rc=");
      Serial.print(client.state());
      Serial.println(" 5秒后重试");
      delay(5000);
    }
  }
}

void publishProperties(float temperature, float humidity, int light, double longitude, double latitude, int satellites) {
  StaticJsonDocument<512> doc;
  JsonArray services = doc.createNestedArray("services");
  JsonObject obj = services.createNestedObject();
  obj["service_id"] = "GPS_DHT11_GM"; // 与云端功能定义一致
  JsonObject props = obj.createNestedObject("properties");
  props["temperature"] = temperature;
  props["humidity"] = humidity;
  props["light"] = light;
  props["longitude"] = longitude;
  props["latitude"] = latitude;
  props["satellites"] = satellites;

  char buf[512];
  size_t n = serializeJson(doc, buf);
  Serial.print("上报JSON: ");
  Serial.println(buf);
  client.publish(mqttTopic, buf, n);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 采集数据
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int ldrValue = analogRead(LDR_PIN);

  // 读取GPS
  while (GPSSerial.available()) {
    gps.encode(GPSSerial.read());
  }
  double longitude = gps.location.isValid() ? gps.location.lng() : 0.0;
  double latitude = gps.location.isValid() ? gps.location.lat() : 0.0;
  int satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;

  // 本地串口输出
  Serial.println("====== 本地环境与位置数据 ======");
  if (!isnan(temp) && !isnan(hum)) {
    Serial.print("温度: "); Serial.print(temp);
    Serial.print(" ℃  湿度: "); Serial.print(hum); Serial.println(" %");
  } else {
    Serial.println("DHT11 传感器读取失败");
  }
  Serial.print("光照强度（ADC）: "); Serial.println(ldrValue);
  if (gps.location.isValid()) {
    Serial.print("经度: "); Serial.print(longitude, 6);
    Serial.print("  纬度: "); Serial.println(latitude, 6);
  } else {
    Serial.println("GPS 正在定位...");
  }
  Serial.print("卫星数: "); Serial.println(satellites);

  // 每30秒上报一次
  publishProperties(temp, hum, ldrValue, longitude, latitude, satellites);

  delay(30000); // 30秒上报一次
}
