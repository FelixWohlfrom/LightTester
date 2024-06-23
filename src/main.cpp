#include <stdint.h>

/* Configuration */
const uint16_t MAX_NUM_LEDS = 150; // Overall maximum number of leds that you want to try
const uint8_t LED_PIN = 5;         // GPIO Pin of your led stripe

// Replace with your network credentials
const char *ssid = "YOUR_WIFI_NAME_HERE";
const char *password = "YOUR_WIFI_PASSWORD_HERE";

// Light
#include <FastLED.h>

CRGB leds[MAX_NUM_LEDS];
bool leds_on[MAX_NUM_LEDS] = {};

// Global variables for led settings
uint8_t hue = 30;
uint8_t saturation = 255;
uint8_t value = 30;
uint16_t num_leds = 6;

// Load Wi-Fi library
#include <ESP8266WiFi.h>

// Set web server port number to 80
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

void send_index();
void update();
void update_leds();

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on(F("/"), send_index);
  server.on("/update", HTTP_POST, update);

  server.begin();

  // Light init
  Serial.println("Init LEDs");
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, MAX_NUM_LEDS);
  update_leds();
  // Need to set twice to make sure it's reset properly.
  // FastLED somehow seems to ignore the first call.
  update_leds();
  Serial.println("All done :)");
}

void loop() {
  server.handleClient();
}

void send_index() {
  String led_checkboxes = "";
  for (uint16_t i = 0; i < num_leds; i++) {
    String checked = "";
    if (leds_on[i]) {
      checked = "checked=\"checked\"";
    }
    led_checkboxes = led_checkboxes + "<input type=\"checkbox\" class=\"light\" name=\"leds[]\" value=\"" + i + "\" " + checked + " onclick=\"all_selected()\"/>";

    if (i % 10 == 9) {
      led_checkboxes = led_checkboxes + "<span class=\"divider\">|</span></div><div class=\"nobreak\">";
    }
  }

  server.send(200, "text/html",
    "<!DOCTYPE html><html>"
    "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<link rel=\"icon\" href=\"data:,\">"
    "<style>"
      "body { text-align: center; }\n"
      "input { min-width: 20%; margin: 3px }\n"
      ".light, #all_lights { min-width: 0%; float: left }\n"
      ".divider { float: left; }\n"
      ".nobreak { float: left; white-space: nowrap; }\n"
      "button { margin-top: 15px; }"
    "</style>"
    "<script>"
    "function select_all() {\n"
    "  const checked = document.getElementById('all_lights').checked;\n"
    "  document.querySelectorAll('.light').forEach(checkbox => {\n"
    "    checkbox.checked = checked;\n"
    "  });\n"
    "};\n"
    "function all_selected() {\n"
    "  const checked_items = document.querySelectorAll('.light:checked').length;\n"
    "  const unchecked_items = document.querySelectorAll('.light:not(:checked)').length;\n"
    "  const all_items = document.querySelectorAll('.light').length;\n"
    "  if (checked_items == all_items) {\n"
    "    document.getElementById('all_lights').checked = true;\n"
    "    document.getElementById('all_lights').indeterminate = false;\n"
    "  } else if (unchecked_items == all_items) {\n"
    "    document.getElementById('all_lights').checked = false;\n"
    "    document.getElementById('all_lights').indeterminate = false;\n"
    "  } else {\n"
    "    document.getElementById('all_lights').indeterminate = true;\n"
    "  }\n"
    "}"
    "</script>"
    "</head>"
    "<body onload=\"all_selected()\">"
      "<h1>LED Settings</h1>"
      "<form action=\"update\" method=\"post\" id=\"settings\">"
      "<label for=\"hue\">Hue:</label><input type=\"number\" id=\"hue\" name=\"hue\" min=\"0\" max=\"255\" value=\"" + String(hue) + "\" /><br />"
      "<label for=\"saturation\">Saturation:</label><input type=\"number\" id=\"saturation\" name=\"saturation\" min=\"0\" max=\"255\" value=\"" + String(saturation) + "\" /><br />"
      "<label for=\"value\">Value:</label><input type=\"number\" id=\"value\" name=\"value\" min=\"0\" max=\"255\" value=\"" + String(value) + "\" /><br />"
      "<label for=\"num_leds\">LED Count:</label><input type=\"number\" id=\"num_leds\" name=\"num_leds\" min=\"0\" max=\"" + String(MAX_NUM_LEDS) + "\" value=\"" + String(num_leds) + "\" /><br />"
      "<div style=\"width: fit-content; margin: 3px auto\"><label class=\"divider\">Lights:</label>"
        "<input type=\"checkbox\" id=\"all_lights\" name=\"all_lights\" value=\"all\" onclick=\"select_all()\"><span class=\"divider\">All</span></input><span class=\"divider\">|</span>"
        "<div class=\"nobreak\">" + led_checkboxes + "</div>"
      "</div><div style=\"clear: both\"></div>"
      "</form>"
      "<button type=\"submit\" form=\"settings\">UPDATE</button>"
    "</body></html>");
}

// Will update the settings from given arguments
void update() {
  hue = server.arg(0).toInt();
  saturation = server.arg(1).toInt();
  value = server.arg(2).toInt();
  num_leds = server.arg(3).toInt();

  for (uint8_t i = 0; i < MAX_NUM_LEDS; i++) {
    leds_on[i] = false;
  }
  for (uint8_t i = 4; i < server.args(); i++) {
    // Skip the "all" checkbox if checked
    if (server.arg(i) == "all") {
      continue;
    }

    uint8_t number = server.arg(i).toInt();
    if (number < num_leds) {
      leds_on[number] = true;
    }
  }

  Serial.printf("Hue: %i, saturation: %i, value: %i, num_leds: %i\n", hue, saturation, value, num_leds);
  update_leds();

  server.sendHeader("Location", "/");
  server.send(303);
}

// Will update all leds with the stored hue, saturation and value
void update_leds() {
  for (uint16_t i = 0; i < MAX_NUM_LEDS; i++) {
    if (leds_on[i]) {
      leds[i].setHSV(hue, saturation, value);
    } else {
      leds[i].setHSV(hue, saturation, 0);
    }
  }
  FastLED.show();
}