#include <WiFi.h>
#include "ESPAsyncWebServer.h"

#define reedSwitch 4
#define buzzer  2

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Detects whenever the door changed state
bool changeState = false;

// Holds reedswitch state (0 = closed, 1 = open)
bool state;
String doorState; 

const char* ssid = "Android";
const char* password = "password";
const char* host = "maker.ifttt.com";
const char* apiKey = "mksnH5Hj0vKPnXlt26CV4SefTY-DdfDOAuodPN9skLw";
const char* event = "Door_Monitor";
unsigned long previousMillis = 0;
unsigned long interval = 30000;

void send_email();

// Runs whenever the reedswitch changes state
ICACHE_RAM_ATTR void changeDoorStatus() {
  Serial.println("State changed");
  changeState = true;
}


String readDoorStatus() {
  int reed_status;
  reed_status = digitalRead(reedSwitch);
  if (reed_status == 1){
    return String("Opened");
  }
  else{
    return String("Closed");
  }
  delay(1000);
  return String(reed_status);
}


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>ESP32 Intruder Alert!</h2>
  <p>
    <span class="dht-labels">Door Status</span> 
    <span id="doorStatus">%DOORSTATUS%</span>
  </p>
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("doorStatus").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/doorStatus", true);
  xhttp.send();
}, 5000 ) ;
</script>
</html>)rawliteral";


String processor(const String& var){
if(var == "DOORSTATUS"){
    return readDoorStatus();
  }
  return String();
}


void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Read the current door state
  pinMode(reedSwitch, INPUT_PULLUP);
  state = digitalRead(reedSwitch);

  //Set buzzer state to match door state
  pinMode(buzzer, OUTPUT);
  digitalWrite(reedSwitch, INPUT);

  // Set the reedswitch pin as interrupt, assign interrupt function and set CHANGE mode
  attachInterrupt(digitalPinToInterrupt(reedSwitch), changeDoorStatus, CHANGE);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");  

   
     // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/doorStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDoorStatus().c_str());
  });

  // Start server
  server.begin();
}

void loop() {
unsigned long currentMillis = millis();
// if WiFi is down, try reconnecting
if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
  Serial.print(millis());
  Serial.println("Reconnecting to WiFi...");
  WiFi.disconnect();
  Serial.println("Disconnected from WiFi...");
  WiFi.reconnect();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi Again...");
  Serial.println(WiFi.localIP());
  previousMillis = currentMillis;
}
  if (changeState){ 
      state = !state;
      if(state) {
        doorState = "open";
        //Turn on buzzer if door is open
        digitalWrite(buzzer, HIGH);
      }
      else{
        //Do not turn on the buzzer if door is closed
        digitalWrite(buzzer, LOW);
        doorState = "close";
      }

      changeState = false;
      Serial.println(state);
      Serial.println("Door is: ");
      Serial.println(doorState);

      //Send email
      if(doorState == "open"){
      Serial.println("Notifying owner..");
      send_email();
      }
    }  
 }

void send_email(){
      Serial.print("connecting to ");
      Serial.println(host);
      WiFiClient client;
      const int httpPort = 80;
      if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;
      }
      
    String url = "/trigger/";
    url += event;
    url += "/with/key/";
    url += apiKey;
  Serial.print("Requesting URL: ");
  Serial.println(url);
     // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
while(client.connected())
  {
    if(client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    } 
    else {
      // No data yet, wait a bit
      delay(50);
    };
  }
  Serial.println();
  Serial.println("closing connection");
  client.stop();
}
