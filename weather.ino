#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include "addons/TokenHelper.h"
#include <time.h>

/* WIFI SETTINGS */
#define WIFI_SSID "Wifi"
#define WIFI_PASSWORD "09070907"

/* FIREBASE SETTINGS */
#define API_KEY "AIzaSyCd2mUtd9bf3A8kRfDzUEMT-kS3ul7pE_k"
#define DATABASE_URL "https://weather-monitoring-e6913-default-rtdb.firebaseio.com/"

/* SENSOR PINS */
#define DHTPIN 2       // D4 GPIO2
#define RAIN_PIN 14    // D5 GPIO14
#define BUZZER_PIN 12  // D6 GPIO12

#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

/* FIREBASE OBJECTS */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* NTP SERVER */
const char* ntpServer = "pool.ntp.org";

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Starting Weather Station");

  /* PIN MODES */
  pinMode(RAIN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  /* START DHT SENSOR */
  dht.begin();

  /* WIFI SETTINGS */
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");

    retry++;

    if (retry > 40)
    {
      Serial.println();
      Serial.println("WiFi Connection Failed!");

      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status());

      return;
    }
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  /* TIME SYNC */
  configTime(0, 0, ntpServer);

  Serial.print("Syncing Time");

  time_t now = time(nullptr);

  while (now < 100000)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println();
  Serial.println("Time Synced");

  /* FIREBASE CONFIG */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = "esp32@test.com";
  auth.user.password = "12345678";

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase Ready");
}

void loop()
{
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int rainValue = digitalRead(RAIN_PIN);

  Serial.println("------------------------");

  /* CHECK SENSOR VALUES */
  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("DHT Sensor Read Failed");
    delay(2000);
    return;
  }

  /* PRINT VALUES */
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Rain Sensor: ");
  Serial.println(rainValue);

  /* BUZZER CONTROL */
  if (rainValue == LOW)
  {
    Serial.println("Rain Detected");
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    Serial.println("No Rain");
    digitalWrite(BUZZER_PIN, LOW);
  }

  /* SEND DATA TO FIREBASE */
  if (Firebase.ready())
  {
    bool t = Firebase.RTDB.setFloat(
      &fbdo,
      "/weather/temperature",
      temperature
    );

    bool h = Firebase.RTDB.setFloat(
      &fbdo,
      "/weather/humidity",
      humidity
    );

    bool r = Firebase.RTDB.setInt(
      &fbdo,
      "/weather/rain",
      rainValue
    );

    long now = time(nullptr);

    bool ts = Firebase.RTDB.setInt(
      &fbdo,
      "/weather/lastSeen",
      now
    );

    if (t && h && r && ts)
    {
      Serial.println("Firebase Updated Successfully");
    }
    else
    {
      Serial.println("Firebase Update Failed");
      Serial.println(fbdo.errorReason());
    }
  }
  else
  {
    Serial.println("Firebase Not Ready");
  }

  delay(5000);
}
