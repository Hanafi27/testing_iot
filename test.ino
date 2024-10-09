#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <time.h>  // Library untuk menampilkan waktu
#include <SD.h>

// Ganti dengan kredensial Firebase dan WiFi Anda
#define FIREBASE_HOST "data-test-abec7-default-rtdb.asia-southeast1.firebasedatabase.app" // Removed "https://"
#define FIREBASE_API_KEY "AIzaSyCNyw8q2Rb4SNnHopHp8WB0WBNJXmeyQXc"
#define WIFI_SSID "You_WIFI"
#define WIFI_PASSWORD "You_WIFI_PASSWORD"
#define FIREBASE_EMAIL "You_EMAIL"
#define FIREBASE_PASSWORD "You_EMAIL_PASSWORD"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define PHPIN 2          // Pin untuk sensor pH
#define DHTPIN 4         // Pin untuk DHT22
#define DHTTYPE DHT22    // Tipe DHT
#define MOISTURE_PIN 34  // Pin untuk kelembaban tanah 1
#define MOISTURE_PIN2 32 // Pin untuk kelembaban tanah 2
#define MOISTURE_PIN3 35 // Pin untuk kelembaban tanah 3
#define WATER_TEMP_PIN 33// Pin untuk suhu air
#define RAIN_SENSOR_PIN 27// Pin untuk sensor hujan

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHTPIN, DHTTYPE);

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Konstanta Kalibrasi pH
const float pH_Slope = 0.00353;      // Slope dari kalibrasi
const float pH_Intercept = 0.999;    // Intercept dari kalibrasi

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

// Konfigurasi NTP untuk mendapatkan waktu
void configureTime() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");  // GMT+7 untuk Indonesia
  Serial.println("Waktu telah dikonfigurasi.");
}

String getTime() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("Gagal mendapatkan waktu");
    return "00:00";
  }
  
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &timeInfo);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();

  // Konfigurasi Firebase
  firebaseConfig.api_key = FIREBASE_API_KEY;
  firebaseConfig.database_url = FIREBASE_HOST; 
  firebaseAuth.user.email = FIREBASE_EMAIL;
  firebaseAuth.user.password = FIREBASE_PASSWORD;
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Pastikan alamat I2C benar
    Serial.println(F("SSD1306 allocation failed"));
    while (true); // Menghentikan eksekusi jika OLED gagal diinisialisasi
  }

  display.clearDisplay();
  display.setTextSize(1); // Menggunakan ukuran teks yang lebih besar untuk keterbacaan
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initializing..."));
  display.display();

  // Inisialisasi sensor DHT
  dht.begin();

  // Inisialisasi pin rain sensor
  pinMode(RAIN_SENSOR_PIN, INPUT_PULLUP);
  
  // Konfigurasi NTP
  configureTime();

  // Menginisialisasi random seed
  randomSeed(analogRead(0)); // Menginisialisasi seed untuk random
}

// Fungsi untuk membaca pH dari sensor
float readSoilPH(float &hasil) {
    int sensorValue = analogRead(PHPIN);
    Serial.print("Raw sensor value: ");
    Serial.println(sensorValue); // Tampilkan nilai mentah ke serial monitor

    // Menghitung pH berdasarkan rumus
    hasil = (-0.0139 * sensorValue) + 7.851;

    // Tambahkan variasi acak dengan batasan untuk tetap dalam rentang 1-14
    float randomVariation = random(-3, 4) / 10.0; // Variasi antara -0.3 hingga 0.3
    hasil += randomVariation;

    // Pastikan nilai tetap dalam rentang 1-14
    hasil = constrain(hasil, 1.0, 14.0);

    // Debugging tambahan
    Serial.print("Calculated pH: ");
    Serial.println(hasil);

    return hasil; // Kembalikan nilai pH yang dibaca
}

void readSensors(float &humidity, float &airTemp, float &moisture1, 
                 float &moisture2, float &moisture3, 
                 float &soilTempC, float &soilPHValue, float &hasil, String &rainSensor) {

  // Membaca kelembaban dan suhu udara
  humidity = dht.readHumidity();
  airTemp = dht.readTemperature();

  // Pastikan nilai kelembaban dan suhu valid
  if (isnan(humidity) || humidity < 0 || humidity > 100) {
    humidity = 0;  // Atur kelembaban ke 0 jika tidak valid
    Serial.println("Humidity reading is invalid, set to 0");
  }

  if (isnan(airTemp) || airTemp < -40 || airTemp > 80) {
    airTemp = 0;  // Atur suhu udara ke 0 jika tidak valid
    Serial.println("Air temperature reading is invalid, set to 0");
  }

  // Membaca nilai pH tanah
  soilPHValue = readSoilPH(hasil); // Panggil fungsi dengan parameter hasil

  // Membaca nilai kelembaban tanah
  int rawMoistureValue = analogRead(MOISTURE_PIN);
  int rawMoistureValue2 = analogRead(MOISTURE_PIN2);
  int rawMoistureValue3 = analogRead(MOISTURE_PIN3);

  int maxMoistureValue = 4095;
  moisture1 = constrain(map(maxMoistureValue - rawMoistureValue, 0, maxMoistureValue, 0, 100), 0, 100);
  moisture2 = constrain(map(maxMoistureValue - rawMoistureValue2, 0, maxMoistureValue, 0, 100), 0, 100);
  moisture3 = constrain(map(maxMoistureValue - rawMoistureValue3, 0, maxMoistureValue, 0, 100), 0, 100);

  // Membaca suhu air
  int soilTempValue = analogRead(WATER_TEMP_PIN);
  soilTempC = constrain(map(soilTempValue, 0, 4095, 0, 80), 0, 80); // Rentang suhu dari 0 hingga 80

  // Membaca status rain sensor
  int rainSensorValue = digitalRead(RAIN_SENSOR_PIN);
  rainSensor = (rainSensorValue == LOW) ? "Rain" : "NoRain";
}


void updateDisplay(float humidity, float airTemp, float moisture1, float moisture2, float moisture3, 
                  float soilPHValue, float soilTempC, String rainSensorStatus, float hasil) { // Tambahkan parameter untuk hasil
  
  String currentTime = getTime(); // Mendapatkan waktu sekarang

  display.clearDisplay();
  display.setCursor(0, 0);
  
  display.print(F("Time: "));
  display.println(currentTime); 

  display.print(F("Humidity: "));
  display.print((int)humidity); 
  display.println(F("%"));

  display.print(F("Air Temp: "));
  display.print((int)airTemp); 
  display.println(F("C"));

  display.print(F("Moisture1: "));
  display.print((int)moisture1); 
  display.println(F("%"));

  display.print(F("Moisture2: "));
  display.print((int)moisture2); 
  display.println(F("%"));

  display.print(F("Moisture3: "));
  display.print((int)moisture3); 
  display.println(F("%"));

  display.print(F("Soil pH: "));
  // Format pH with comma
  String soilPHStr = String(hasil, 2);
  soilPHStr.replace('.', ',');
  display.print(soilPHStr); 
  display.println();

  display.print(F("SoilMoisture: "));
  display.print((int)soilTempC); 
  display.println(F("%"));

  display.print(F("Rain: "));
  display.println(rainSensorStatus); 

  display.display();
}

void uploadToFirebase(float humidity, float airTemp, float moisture1, float moisture2,
                      float moisture3, float soilPH, float soilTempC, String rainSensor) {
  
  if (WiFi.status() == WL_CONNECTED) {
    String path = "/node1/";

    // Mengupload data sensor ke Firebase
    Firebase.setFloat(firebaseData, path + "Humidity", humidity);
    Firebase.setFloat(firebaseData, path + "AirTemperature", airTemp);
    Firebase.setFloat(firebaseData, path + "Moisture1EP000468", moisture1);
    Firebase.setFloat(firebaseData, path + "Moisture2YL69", moisture2);
    Firebase.setFloat(firebaseData, path + "Moisture3CapasitiveV1_2", moisture3);
    Firebase.setFloat(firebaseData, path + "Moisture4StickADC", soilTempC);
    Firebase.setString(firebaseData, path + "RainSensor", rainSensor);
    
    // Upload current time
    String currentTime = getTime();
    Firebase.setString(firebaseData, path + "Time", currentTime);

    // Format soil pH with comma
    String soilPHStr = String(soilPH, 2);
    soilPHStr.replace('.', ',');
    Firebase.setString(firebaseData, path + "SoilPH", soilPHStr);
  }
}

void loop() {
  float humidity, airTemp, moisture1, moisture2, moisture3, soilTempC, soilPHValue, hasil; // Tambahkan variabel untuk hasil
  String rainSensor;

  readSensors(humidity, airTemp, moisture1, moisture2, moisture3, soilTempC, soilPHValue, hasil, rainSensor); // Panggil readSensors dengan parameter hasil

  updateDisplay(humidity, airTemp, moisture1, moisture2, moisture3, soilPHValue, soilTempC, rainSensor, hasil); // Panggil updateDisplay dengan parameter hasil

  uploadToFirebase(humidity, airTemp, moisture1, moisture2, moisture3, hasil, soilTempC, rainSensor);

  delay(5000); // Interval 5 detik sebelum membaca ulang sensor
}
