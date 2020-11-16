
#include <Adafruit_DHT_Particle.h>
#include <Adafruit_SSD1306_RK.h>
#include <Adafruit_GFX_RK.h>
#include <Wire.h>

/// pin for PIR sensor
#define PIR_MOTION_SENSOR A2

/// pin for user button
#define USER_BUTTON_PIN D2


/// pin for Humidity and Temp sensor
#define DHTPIN D4     
#define DHTTYPE DHT11		// DHT 11 
// #define DHTTYPE DHT22		// DHT 22 (AM2302)
//#define DHTTYPE DHT21		// DHT 21 (AM2301)

DHT dht_sensor(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
// SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


static unsigned int _loop_count = 0;
static float _celsius = 0.0f;
static float _pct_humidity = 0.0f;
static float _heat_index_c = 0.0f;
static float _dewpoint_c = 0.0f;
// static float _fahrenheit = 0.0f;
// static int _light_lux = 0;

void setup() {
	Serial.begin(115200); 
  Particle.syncTime();

	Particle.publish("state", "begin");

  pinMode(PIR_MOTION_SENSOR, INPUT);

  display_setup();

  Time.zone(-8.0);
	dht_sensor.begin();

	delay(2000);
}

void display_setup() {

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32 // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
  }

  // Clear the display buffer
  display.clearDisplay();
  display.display();
  delay(2000);

}

// sensor readings locally
void render_to_oled() {
  display.clearDisplay();

  int xoff = 0;
  int yoff = 0;
  display.setTextColor(WHITE);

  // time display
  display.setCursor(xoff, yoff);             
  display.setTextSize(1);       
  time_t time = Time.now(); 
  String time_str = Time.format(time, "%r");
  display.println(time_str);
  yoff += 14;

  // temp 
  display.setCursor(xoff, yoff);   
  display.setTextSize(4);                  
  display.println(String::format("%2.0fc",_celsius));
  xoff += 80;
  // humidity
  display.setCursor(xoff, yoff);   
  display.setTextSize(2);                  
  display.println(String::format("%2.0f%%", _pct_humidity));

  // misc
  xoff = 0; yoff = (SCREEN_HEIGHT - 14);
  display.setCursor(xoff, yoff);             
  display.setTextSize(1);        
  display.println(String::format("%2.0f dp %2.0f hi ",_dewpoint_c, _heat_index_c));
  yoff += 14;
    
  // flush to oled
  display.display();
    
  //ensure we write to the display
  delay(2000);
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds stale
  float pct_humidity = dht_sensor.getHumidity();
  float celsius = dht_sensor.getTempCelcius();

  // Check if any reads failed and exit early (to try again).
  if (isnan(pct_humidity) || isnan(celsius) || (celsius > 100.0)) {
    Serial.println("DHT sensor read failed");
    delay(5000); // give the sensor some time to recover
    return;
  }

  // read ambient light sensor
  // _light_lux = analogRead(A2);

  //update globals

  // _fahrenheit = dht_sensor.getTempFarenheit();
  _celsius = celsius;
  _pct_humidity = pct_humidity;
  _heat_index_c = dht_sensor.getHeatIndex();
  _dewpoint_c = dht_sensor.getDewPoint();

  String report = String::format("{\"hum_p\": %4.2f, \"temp_c\": %4.2f, \"dewp_c\": %4.2f, \"heati_c\": %4.2f}", 
    pct_humidity, celsius, _dewpoint_c, _heat_index_c);
  Particle.publish("sample", report);
  render_to_oled();

  //Serial.println(Time.timeStr());
  Serial.println(report);

	_loop_count++;
	if (_loop_count > 5) {
	  Particle.publish("state", "sleep 60s");
    Serial.println("sleeping...");
	  delay(5000);

    SystemSleepConfiguration config;
    config.mode(SystemSleepMode::ULTRA_LOW_POWER)
      .duration(60s)
      .gpio(USER_BUTTON_PIN, RISING)
      .gpio(PIR_MOTION_SENSOR, CHANGE)
      // .network(NETWORK_INTERFACE_WIFI_STA)
      .flag(SystemSleepFlag::WAIT_CLOUD);
    SystemSleepResult sleep_result = System.sleep(config);
    Particle.publish("state", String::format("wakeup %d",sleep_result.wakeupReason()));
	}
}

