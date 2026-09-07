/* Start User Editable Parameters */

// WiFi credentials
const char* ssid = "xxxxxx"; // <- Edit this with your SSID
const char* password = "xxxxxx"; // <- Edit this with your Password

/* End Of User Editable Parameters */


// Based on work done by Rui Santos & Sara Santos - Random Nerd Tutorials
/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-async-web-server-espasyncwebserver-library/
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/



/* Libraries */

// WiFi library
#include <WiFi.h>

// AsyncWebServer
#include <AsyncTCP.h>

// Web server library
#include <ESPAsyncWebServer.h>



/* Pins */

// Define the output pin
#define LIGHT_PIN 7  

// Define the input pin
#define BUTTON_PIN 6



/* Web Server */

// URL parameter
const char* PARAM_STATE = "state";

// Create web server on port 80
AsyncWebServer server(80);

// Create Events channel for live updates
AsyncEventSource events("/events");



/* Debounce */

// Store current button state
bool buttonState = HIGH;

// Store last button state
bool lastButtonState = HIGH;

// Store time since last button state change
unsigned long lastDebounceTime = 0;

// Debounce delay 
const unsigned long debounceDelay = 50;



/* HTML PAGE */


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Gears ESP NPF Light Switch</title>
  
  <style>

    /* Basic page styling */
    html {
      font-family: Arial;
      text-align: center;
    }

    body {
      margin-top: 50px;
    }

    /* Slider container */
    .switch {
      position: relative;
      display: inline-block;
      width: 120px;
      height: 68px;
    }

    /* Hide default checkbox */
    .switch input {
      display: none;
    }

    /* Slider background */
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: .5s;
      border-radius: 6px;
    }

    /* Slider knob */
    .slider:before {
      position: absolute;
      content: "";
      height: 52px;
      width: 52px;
      left: 8px;
      bottom: 8px;
      background-color: white;
      transition: .5s;
      border-radius: 3px;
    }

    /* Background color */
    input:checked + .slider {
      background-color: #b30000;
    }

    /* Move slider */
    input:checked + .slider:before {
      transform: translateX(52px);
    }

  </style>
</head>
<body>

<h2>ESP32 NPF Light Switch</h2>

<!-- Display current state -->
<p>State: <span id="state">--</span></p>


<!-- Slider switch -->
<label class="switch">
  <input type="checkbox" id="toggle" onchange="toggle(this)">
  <span class="slider"></span>
</label>


<script>

// Send request when user toggles switch
function toggle(el) {
  var state = el.checked ? 1 : 0;
  fetch(`/update?state=${state}`);
}

// Listen for live updates from ESP32
var source = new EventSource('/events');

// When ESP sends new state, update UI
source.addEventListener('state', function(e) {
  let isOn = (e.data === "1");

  document.getElementById("toggle").checked = isOn;
  document.getElementById("state").innerText = isOn ? "ON" : "OFF";
});

// Get initial state when page loads
fetch('/state')
  .then(r => r.text())
  .then(data => {
    let isOn = (data === "1");
    document.getElementById("toggle").checked = isOn;
    document.getElementById("state").innerText = isOn ? "ON" : "OFF";
  });

</script>

<h4><a href=\state>View Output Status Page</a></h4>
<h4><a href=\update?state=1>View Output Update Page (ON)</a></h4>
<h4><a href=\update?state=0>View Output Update Page (OFF)</a></h4>


</body>
</html>
)rawliteral";


// Current light state as "0" or "1" 
String lightStateStr() {
  return String(digitalRead(LIGHT_PIN));
}

// Send current LIGHT_PIN state to all connected browsers
void notifyClients() {
  events.send(lightStateStr().c_str(), "state", millis());
}



void setup() {

  // Set LIGHT_PIN as output
  pinMode(LIGHT_PIN, OUTPUT);

  // Start with light OFF
  digitalWrite(LIGHT_PIN, LOW);

  // Set BUTTON_PIN as input
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Connect to WiFi
  WiFi.begin(ssid, password);

  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }


 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });


  // Endpoint to SET the light state
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){

    // Check required parameter exists
    if (request->hasParam(PARAM_STATE)) {

      // Get desired state
      int state = request->getParam(PARAM_STATE)->value().toInt();

      // Set the pin HIGH or LOW
      digitalWrite(LIGHT_PIN, state);

      // Notify all connected browsers
      notifyClients();
    
      // Return "0" or "1"
      request->send(200, "text/plain", String(state));
    }
  });


  // Endpoint to READ the light state
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", lightStateStr());
  });


  // When browser connects to event stream
  events.onConnect([](AsyncEventSourceClient *client){

    // Send current state immediately
    client->send(lightStateStr().c_str(), "state", millis());
  });

  // Attach SSE handler to server
  server.addHandler(&events);


  // Start the web server
  server.begin();
}


void loop() {

  // Read current button state
  int reading = digitalRead(BUTTON_PIN);

  // If reading changed, reset debounce timer
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // If stable longer than debounce delay
  if ((millis() - lastDebounceTime) > debounceDelay) {

    // If actual state changed
    if (reading != buttonState) {
      buttonState = reading;

      // If button pressed (LOW because of pull-up)
      if (buttonState == LOW) {

        // Toggle light state
        digitalWrite(LIGHT_PIN, !digitalRead(LIGHT_PIN));

        // Notify browser of change
        notifyClients();
      }
    }
  }

  // Save last reading
  lastButtonState = reading;
}
