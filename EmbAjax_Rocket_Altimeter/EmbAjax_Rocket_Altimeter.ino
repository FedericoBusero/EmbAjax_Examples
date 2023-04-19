#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_BMP280.h>

// #define DEBUG_SERIAL Serial

#define LED_PIN LED_BUILTIN

#define LED_ON HIGH
#define LED_OFF LOW

bool bmp280_status;
Adafruit_BMP280 bmp; // I2C

#define PRESSURE_CALC 1013.0

float current_pressure;
float current_temperature;

float current_altitude;
float max_altitude;
float min_altitude;

#define WIFI_SOFTAP_CHANNEL 1 // 1-13
const char ssid[] = "rocket-";
const char password[] = "12345678";

// EmbAjax: can be installed from Arduino IDE or https://github.com/tfry-git/EmbAJAX
#ifdef ESP32
#include <AsyncTCP.h>
#include <EmbAJAXOutputDriverESPAsync.h>
#endif
#include <EmbAJAX.h> 

#include <DNSServer.h>
DNSServer dnsServer;

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDriverWebServerClass is a
// convenience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

#define BUFLEN 100

EmbAJAXMutableSpan bmpsensor_status_d("bmpsensor_status_d");

EmbAJAXMutableSpan temperature_d("temperature_d");
char temperature_d_buf[BUFLEN];

EmbAJAXMutableSpan pressure_d("pressure_d");
char pressure_d_buf[BUFLEN];

EmbAJAXMutableSpan altitude_d("altitude_d");
char altitude_d_buf[BUFLEN];

void buttonPressed(EmbAJAXPushButton*) {
  min_altitude = current_altitude;
  max_altitude = current_altitude;
}
EmbAJAXPushButton button("button", "Reset Min&Max Altitude", buttonPressed);
EmbAJAXMutableSpan button_d("button_d");
char button_d_buf[BUFLEN];

EmbAJAXMomentaryButton m_button("m_button", "LED");
EmbAJAXMutableSpan m_button_d("m_button_d");

EmbAJAXStatic nextCell("</td><td>&nbsp;</td><td><b>");
EmbAJAXStatic nextRow("</b></td></tr><tr><td>");

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "Rocket - Altimeter - EmbAJAX", "",
                 new EmbAJAXStatic("<table cellpadding=\"10\"><tr><td>"),

                 new EmbAJAXStatic("BMP280 sensor status:"),
                 &nextCell,
                 &bmpsensor_status_d,
                 &nextRow,

                 &button,
                 &nextCell,
                 &button_d,
                 &nextRow,

                 new EmbAJAXStatic("Temperature:"),
                 &nextCell,
                 &temperature_d,
                 &nextRow,

                 new EmbAJAXStatic("Pressure:"),
                 &nextCell,
                 &pressure_d,
                 &nextRow,

                 new EmbAJAXStatic("Altitude:"),
                 &nextCell,
                 &altitude_d,
                 &nextRow,

                 &m_button,
                 &nextCell,
                 &m_button_d,
                 &nextRow,

                 new EmbAJAXStatic("Server status:"),
                 &nextCell,
                 new EmbAJAXConnectionIndicator(),
                 new EmbAJAXStatic("</b></td></tr></table>")
                )

void updateAltitude()
{
  if (!bmp280_status)
  {
    return;
  }
  current_altitude = bmp.readAltitude(PRESSURE_CALC);

  max_altitude = max(current_altitude, max_altitude);
  /*
    DEBUG_SERIAL.println("MAX,CURRENT");
    DEBUG_SERIAL.print(max_altitude);
    DEBUG_SERIAL.print(',');
    DEBUG_SERIAL.print(current_altitude);
    DEBUG_SERIAL.println();
  */
}

void updatePressure()
{
  static unsigned long timeout_millis_pressure = millis();

  if (!bmp280_status)
  {
    return;
  }

  if (millis() >= timeout_millis_pressure)
  {
    // DEBUG_SERIAL.println("updatePressure");
    current_pressure = bmp.readPressure() / 100.0;
    timeout_millis_pressure += 1000UL;
  }
}

void updateTemperature()
{
  static unsigned long timeout_millis_temp = millis();

  if (!bmp280_status)
  {
    return;
  }

  if (millis() >= timeout_millis_temp)
  {
    // DEBUG_SERIAL.println("updateTemperature");
    current_temperature = bmp.readTemperature();
    timeout_millis_temp += 1000UL;
  }
}

void show_state_serial()
{
  static unsigned long timeout_millis_serial = millis();

  if (millis() > timeout_millis_serial)
  {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.print(F("Temperature = "));
    DEBUG_SERIAL.print(current_temperature);
    DEBUG_SERIAL.println(" °C");

    DEBUG_SERIAL.print(F("Pressure = "));
    DEBUG_SERIAL.print(current_pressure);
    DEBUG_SERIAL.println(" hPa");


    DEBUG_SERIAL.print(F("Min Altitude = "));
    DEBUG_SERIAL.print(min_altitude);
    DEBUG_SERIAL.println(" m");

    DEBUG_SERIAL.print(F("Corrected Max Altitude = "));
    DEBUG_SERIAL.print(max_altitude - min_altitude);
    DEBUG_SERIAL.println(" m");

    DEBUG_SERIAL.print(F("Corrected Current Altitude = "));
    DEBUG_SERIAL.print(current_altitude - min_altitude);
    DEBUG_SERIAL.println(" m");

    DEBUG_SERIAL.println();

#endif
    timeout_millis_serial += 200UL;
  }
}

void updateWebUI() {
  // Update UI. Note that you could simply do this inside the loop. However,
  // placing it here makes the client UI more responsive (try it).
  bmpsensor_status_d.setValue((bmp280_status) ? "OK" : "NOT OK!");
  button_d.setValue(dtostrf(max_altitude - min_altitude, 0, 2, button_d_buf));
  temperature_d.setValue(dtostrf(current_temperature, 0, 2, temperature_d_buf));
  pressure_d.setValue(dtostrf(current_pressure, 0, 2, pressure_d_buf));
  altitude_d.setValue(dtostrf(current_altitude - min_altitude, 0, 2, altitude_d_buf));
  m_button_d.setValue((m_button.status() == EmbAJAXMomentaryButton::Pressed) ? "On" : "Off");
  if (m_button.status() == EmbAJAXMomentaryButton::Pressed)
  {
    digitalWrite(LED_PIN, LED_ON);
  }
  else
  {
    digitalWrite(LED_PIN, LED_OFF);
  }
}


void setup() {
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.begin(115200);
  delay(1500);
  DEBUG_SERIAL.println(F("\nRocket Altitude EmbAjax"));
#endif

  // Flash LED 2x to show there's a reboot
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LED_ON);
  delay(10);
  digitalWrite(LED_PIN, LED_OFF);
  delay(100);
  digitalWrite(LED_PIN, LED_ON);
  delay(10);
  digitalWrite(LED_PIN, LED_OFF);

  // Init BMP280
  // Wire.begin(SDA_PIN,SCL_PIN); // SDA_PIN,SCL_PIN
  bmp280_status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);

  if (!bmp280_status) {
#ifdef DEBUG_SERIAL
    DEBUG_SERIAL.println(F("Could not find a valid BMP280 sensor, check wiring or "
                           "try a different address!"));
    DEBUG_SERIAL.print("SensorID was: 0x"); Serial.println(bmp.sensorID(), 16);
    DEBUG_SERIAL.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    DEBUG_SERIAL.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    DEBUG_SERIAL.print("        ID of 0x60 represents a BME 280.\n");
    DEBUG_SERIAL.print("        ID of 0x61 represents a BME 680.\n");
#endif
    min_altitude = 0.0;
  }
  else
  {
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X8,
                    // Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X4,
                    // Adafruit_BMP280::FILTER_X16,      /* Filtering, uitgezet om vertraging tegen te gaan. */
                    //                  Adafruit_BMP280::STANDBY_MS_500
                    Adafruit_BMP280::STANDBY_MS_63); /* Standby time. */

    min_altitude = bmp.readAltitude(PRESSURE_CALC);
  }
  max_altitude = min_altitude;
  current_altitude = 0.0;

  current_pressure = 0.0;
  current_temperature = 0.0;

  // Wifi setup
  WiFi.persistent(true);

  uint8_t macAddr[6];
  WiFi.macAddress(macAddr);
  WiFi.disconnect();

  WiFi.mode(WIFI_AP);
  // WiFi.softAPConfig (IPAddress (192, 168, 4, 1), IPAddress (0, 0, 0, 0), IPAddress (255, 255, 255, 0));
  // WiFi.softAP("EmbAJAXTest", "12345678");
  // ssidmac = ssid + 4 last hexadecimal values of MAC address
  char ssidmac[33];
  sprintf(ssidmac, "%s%02X%02X", ssid, macAddr[4], macAddr[5]);
  WiFi.softAP(ssidmac, password, WIFI_SOFTAP_CHANNEL);
  IPAddress apIP = WiFi.softAPIP();
#ifdef DEBUG_SERIAL
  DEBUG_SERIAL.print(F("SoftAP SSID="));
  DEBUG_SERIAL.println(ssidmac);
  DEBUG_SERIAL.print(F("IP: "));
  DEBUG_SERIAL.println(apIP);
#endif
  //  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(53, "r.be", apIP);

  // Tell the server to serve our EmbAJAX test page on root
  // installPage() abstracts over the (trivial but not uniform) WebServer-specific instructions to do so
  driver.installPage(&page, "/", updateWebUI);
  server.begin();

  updateWebUI(); // init displays
}

void loop() {

  static unsigned long timeout_millis_web_ui = millis();

  dnsServer.processNextRequest();

  if (millis() > timeout_millis_web_ui)
  {
    updateWebUI();
    timeout_millis_web_ui += 1000UL;
  }
  driver.loopHook();   // handle network. loopHook() simply calls server.handleClient(), in most but not all server implementations.

  updateAltitude();
  updateTemperature();
  updatePressure();
  show_state_serial();
}
