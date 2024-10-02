#include <WiFi.h>
#include <FirebaseESP32.h>

// Inisialisasi kredensial WiFi
#define WIFI_SSID "Hanafi"
#define WIFI_PASSWORD "1sampai2"

// Konfigurasi Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// URL dan secret database Firebase
#define FIREBASE_HOST "https://realtime-data-e4a69-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "TMwibhstO5klvuzUDaychElBayXpq5zb681nuRo1"
#define API_KEY "AIzaSyAaD7G017FZu9YqSO5r8X2P3Yt2dYu_Uy8"

// Pin sensor
#define PIN_SENSOR 35

void setup() {
  Serial.begin(115200);

  // Menghubungkan ke WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");

  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 20) {  // Batas 20 kali percobaan
    delay(1000);
    Serial.print(".");
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nTerhubung ke WiFi!");
  } else {
    Serial.println("\nGagal terhubung ke WiFi");
  }

  // Konfigurasi Firebase
  config.database_url = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  config.api_key = API_KEY; // Tambahkan API Key ke konfigurasi

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Mengatur maksimal retry
  Firebase.setMaxRetry(fbdo, 3);  // Tambahkan objek fbdo sebagai parameter
}

void loop() {
  // Membaca kelembaban dari sensor
  int tingkatKelembaban = analogRead(PIN_SENSOR);
  
  // Konversi nilai kelembaban ke persentase
  int kelembabanPersen = map(tingkatKelembaban, 0, 4095, 0, 100);
  
  // Menampilkan nilai kelembaban yang sudah dikonversi
  Serial.print("Tingkat Kelembaban: ");
  Serial.print(kelembabanPersen);
  Serial.println("%");


  // Mengirim data ke Firebase
  if (!Firebase.setInt(fbdo, "/moisture", kelembabanPersen)) {
    Serial.print("Gagal mengirim data ke Firebase: ");
    Serial.println(fbdo.errorReason());  // Menampilkan error
  } else {
    Serial.println("Data berhasil dikirim ke Firebase!");
  }

  delay(5000);
}

