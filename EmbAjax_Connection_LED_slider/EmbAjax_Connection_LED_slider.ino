/* Basic usage example for EmbAJAX library:
   The example shows the use of connection events, the LED shows the connection state.
   The LED is blinking when waiting for a connection. When there is connection the LED is switched 
   off and flashes each time something is received. The web interface only has a slider. 

   Note that ESP boards seems to be no real standard on which pin the builtin LED is on, and
   there is a very real chance that LED_BUILTIN is not defined, correctly, for your board.
   If you see no blinking, try changing the LEDPIN define (with an externally connected LED
   on a known pin being the safest option). Similarly, on and off states are sometimes reversed.

   This example code is in the public domain. */

#include <EmbAJAX.h>

#define LEDCONNECTIONPIN LED_BUILTIN
#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH
bool connection_active = false;

// Set up web server, and register it with EmbAJAX. Note: EmbAJAXOutputDriverWebServerClass is a
// convenience #define to allow using the same example code across platforms
EmbAJAXOutputDriverWebServerClass server(80);
EmbAJAXOutputDriver driver(&server);

// Define the main elements of interest as variables, so we can access to them later in our sketch.
EmbAJAXSlider slider("slider", 0, 1000, 100);   // slider, from 0 to 1000, initial value 100

// Define a page (named "page") with our elements of interest, above, interspersed by some uninteresting
// static HTML. Note: MAKE_EmbAJAXPage is just a convenience macro around the EmbAJAXPage<>-class.
MAKE_EmbAJAXPage(page, "EmbAJAX example - Connection events", "",
                 new EmbAJAXStatic("<h1>Control the slider and see the messages on the builtin LED</h1><p>"),
                 new EmbAJAXStatic("</p><p>Slider: "),
                 &slider
                )


void updateUI() {
  // Here you can add code to handle the slider events

  Serial.print("updateUI slider value=");
  Serial.println(slider.intValue());
}

void setup() {
  Serial.begin(115200);
  
  // Example WIFI setup as an access point. Change this to whatever suits you, best.
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig (IPAddress (192, 168, 4, 1), IPAddress (0, 0, 0, 0), IPAddress (255, 255, 255, 0));
  WiFi.softAP("EmbAJAXTest", "12345678");

  Serial.println("EmbAjax connection example");
  
  // Tell the server to serve our EmbAJAX test page on root
  // installPage() abstracts over the (trivial but not uniform) WebServer-specific instructions to do so
  driver.installPage(&page, "/", updateUI);
  server.begin();

  pinMode(LEDCONNECTIONPIN, OUTPUT);

}

void checkConnectionStatus()
{
  bool prev_connection_active = connection_active;

  connection_active = page.hasActiveClient(2500);
  if (connection_active != prev_connection_active)
  {
    if (prev_connection_active)
    {
      Serial.println("Disconnected");
    }
    else
    {
      Serial.println("Connected");
      digitalWrite(LEDCONNECTIONPIN, LED_BRIGHTNESS_OFF);
    }
  }
  if (connection_active) {
    // when something is received, the LED will be switched on for 15 ms
    digitalWrite(LEDCONNECTIONPIN, page.hasActiveClient(15) ? LED_BRIGHTNESS_ON : LED_BRIGHTNESS_OFF);
  }
  else {
    digitalWrite(LEDCONNECTIONPIN, (millis() % 1000) > 500 ? LED_BRIGHTNESS_ON : LED_BRIGHTNESS_OFF);
  }
}

void loop() {
  // handle network. loopHook() simply calls server.handleClient(), it can also call the connection callback function, here onConnectionEvent.
  driver.loopHook();
  checkConnectionStatus();
}
