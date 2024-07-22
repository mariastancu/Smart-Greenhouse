#include <WiFi.h>
#include <FirebaseESP32.h>
#include "DHT.h"
#include <WiFiUdp.h>


#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""

#define WIFI_SSID ""
#define WIFI_PASSWORD ""


const int soilHumidityPin = A3;
const int airQualityPin = A1;
const int lightIntensityPin = A2;
const int UVPin = A0;
#define DHTPIN 2     
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200; 
const int   daylightOffset_sec = 3600; 
WiFiUDP udp;
unsigned int localPort = 2390;      

const int NTP_PACKET_SIZE = 48;     
byte packetBuffer[NTP_PACKET_SIZE]; 


void sendNTPpacket(const char * address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  packetBuffer[0] = 0b11100011;   
  packetBuffer[1] = 0;            
  packetBuffer[2] = 6;            
  packetBuffer[3] = 0xEC;        

  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  udp.beginPacket(address, 123); 
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

String getDateTime() {
  udp.begin(localPort);
  sendNTPpacket(ntpServer);

  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("No packet yet");
    return "";
  } else {
    udp.read(packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;

    time_t rawtime = epoch + gmtOffset_sec + daylightOffset_sec;
    struct tm * ti;
    ti = localtime(&rawtime);

    char datetime[20];
    strftime(datetime, 20, "%Y%m%d_%H%M%S", ti);
    return String(datetime);
  }
}

void setup() {
  Serial.begin(115200);
  
 
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("Connected to WiFi");

 
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  
  dht.begin();
}
void loop() {
  
  String dateTime = getDateTime();

  
  int soilHumidityValue = analogRead(soilHumidityPin);
  float voltage = soilHumidityValue * (3.3 / 4095.0); 
  float humidity = (voltage / 3.3) * 100; 

  Serial.print("Soil Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  int airQualityValue = analogRead(airQualityPin);

  Serial.print("Air Quality Value: ");
  Serial.println(airQualityValue);

  float airHumidity = dht.readHumidity();
  float airTemperature = dht.readTemperature();

  Serial.print(F("Air Humidity: "));
  Serial.print(airHumidity);
  Serial.print(F("%  Air Temperature: "));
  Serial.print(airTemperature);
  Serial.println(F("°C"));

  int lightIntensity = analogRead(lightIntensityPin);

  Serial.print("Light Intensity: ");
  Serial.println(lightIntensity);

  int UVIntensity = analogRead(UVPin);

  Serial.print("UV Intensity: ");
  Serial.println(UVIntensity);

  if (Firebase.setFloat(firebaseData, "/soilHumidity/" + dateTime, humidity)) {
    Serial.println("Soil humidity data sent to Firebase");
  } else {
    Serial.print("Error sending soil humidity data: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setInt(firebaseData, "/airQuality/" + dateTime, airQualityValue)) {
    Serial.println("Air quality data sent to Firebase");
  } else {
    Serial.print("Error sending air quality data: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setFloat(firebaseData, "/airHumidity/" + dateTime, airHumidity)) {
    Serial.println("Air humidity data sent to Firebase");
  } else {
    Serial.print("Error sending air humidity data: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setFloat(firebaseData, "/airTemperature/" + dateTime, airTemperature)) {
    Serial.println("Air temperature data sent to Firebase");
  } else {
    Serial.print("Error sending air temperature data: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setInt(firebaseData, "/lightIntensity/" + dateTime, lightIntensity)) {
    Serial.println("Light intensity data sent to Firebase");
  } else {
    Serial.print("Error sending light intensity data: ");
    Serial.println(firebaseData.errorReason());
  }

  if (Firebase.setInt(firebaseData, "/UVIntensity/" + dateTime, UVIntensity)) {
    Serial.println("UV intensity data sent to Firebase");
  } else {
    Serial.print("Error sending UV intensity data: ");
    Serial.println(firebaseData.errorReason());
  }

  delay(10000);
}
