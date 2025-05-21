
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <HTTPClient.h> 

const char* ssid = "NoPo Nanotechnologies";
const char* password = "thefutureishere";

// Teams webhook URL
const String teamsWebhookUrl = "https://noponano.webhook.office.com/webhookb2/c2055065-06e4-402e-9c98-30d60e67d82c@32da1b04-ebe3-4fd5-bf4b-44a21c40a8c2/IncomingWebhook/b71e3d184c524a97bc282869fbbaeb26/5e1c2639-7dab-4755-834a-7fa134351c94/V2tHhZMCl0WFhHSZyE99HuDZ2v4zbnu7LTQCADdU5lraw1";  // Replace with your actual webhook URL

WebServer server(80);
Preferences preferences;

float flow0 = 0, flow1 = 0, flow2 = 0, flow3 = 0;
float leakFlow0 = 0, leakFlow1 = 0, leakFlow2 = 0, leakFlow3 = 0;
float cumulativeTot0 = 0, cumulativeTot1 = 0, cumulativeTot2 = 0, cumulativeTot3 = 0;
float prevTot0 = 0, prevTot1 = 0, prevTot2 = 0, prevTot3 = 0;
String leakMessage = "";
String ipAddress;  // Global variable to store the IP address
unsigned long lastMessageTime = 0; // Time of last message sent
unsigned long interval = 43200000; // 24 hours in milliseconds (24 * 60 * 60 * 1000)
float dailyTotalFlow0 = 0, dailyTotalFlow1 = 0, dailyTotalFlow2 = 0, dailyTotalFlow3 = 0;
float dailyFlow0 = 0, dailyFlow1 = 0, dailyFlow2 = 0, dailyFlow3 = 0;

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to Wi-Fi");

  // Get and store the IP address
  ipAddress = WiFi.localIP().toString();
  Serial.print("IP Address: ");
  Serial.println(ipAddress);

  SPIFFS.begin();

  preferences.begin("waterTotals", false);
  cumulativeTot0 = preferences.getFloat("cumulativeTot0", 0);
  cumulativeTot1 = preferences.getFloat("cumulativeTot1", 0);
  cumulativeTot2 = preferences.getFloat("cumulativeTot2", 0);
  cumulativeTot3 = preferences.getFloat("cumulativeTot3", 0);
  prevTot0 = preferences.getFloat("prevTot0", 0);
  prevTot1 = preferences.getFloat("prevTot1", 0);
  prevTot2 = preferences.getFloat("prevTot2", 0);
  prevTot3 = preferences.getFloat("prevTot3", 0);
  preferences.end();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  if (currentMillis - lastMessageTime >= interval) {
    sendDailyConsumptionToTeams(); // Send today's consumption data
    lastMessageTime = currentMillis; // Reset the timer
  }

  if (Serial2.available() > 0) {
    String data = Serial2.readStringUntil('\n');
    parseData(data);
  }
}

void parseData(String data) {
  if (data.startsWith("Flow1:")) {
    flow1 = data.substring(6).toFloat();
  } else if (data.startsWith("Total1:")) {
    addToCumulativeTotal(data.substring(7).toFloat(), cumulativeTot1, prevTot1, "cumulativeTot1", "prevTot1");
  } else if (data.startsWith("Flow2:")) {
    flow2 = data.substring(6).toFloat();
  } else if (data.startsWith("Total2:")) {
    addToCumulativeTotal(data.substring(7).toFloat(), cumulativeTot2, prevTot2, "cumulativeTot2", "prevTot2");
  } else if (data.startsWith("Flow3:")) {
    flow3 = data.substring(6).toFloat();
  } else if (data.startsWith("Total3:")) {
    addToCumulativeTotal(data.substring(7).toFloat(), cumulativeTot3, prevTot3, "cumulativeTot3", "prevTot3");
  }else if (data.startsWith("Flow0:")) {
    flow0 = data.substring(6).toFloat();
  } else if (data.startsWith("Total0:")) {
    addToCumulativeTotal(data.substring(7).toFloat(), cumulativeTot0, prevTot0, "cumulativeTot0", "prevTot0"); 
  }else if (data.startsWith("LEAK DETECTED in Flow Sensor 1")) {
    leakFlow1 = data.substring(data.indexOf("Leak Flow: ") + 11).toFloat();
    leakMessage = String("NoPo Office/VRISVA SPACE (Cumulative Liters): ") + 
                  "**" + String(cumulativeTot1) + " L** " + 
                  " <--> Overwater was consumed over 10 min at 1st floor, Flowed Liters: " + 
                  "**" + String(leakFlow1) + " L** " + 
                  " <--> Server IP: " + 
                  "**" + ipAddress + "**";
    sendToTeams(leakMessage);  // Send leak message to Teams
  } else if (data.startsWith("LEAK DETECTED in Flow Sensor 2")) {
    leakFlow2 = data.substring(data.indexOf("Leak Flow: ") + 11).toFloat();
    leakMessage = String("VIGA Entertainment (Cumulative Liters): ") + 
                  "**" + String(cumulativeTot2) + " L** " + 
                  " <--> Overwater was consumed over 10 minutes at 2nd floor, Flowed Liters: " + 
                  "**" + String(leakFlow2) + " L** " + 
                  " <--> Server IP: " + 
                  "**" + ipAddress + "**";
    sendToTeams(leakMessage);  // Send leak message to Teams
  } else if (data.startsWith("LEAK DETECTED in Flow Sensor 3")) {
    leakFlow3 = data.substring(data.indexOf("Leak Flow: ") + 11).toFloat();
    leakMessage = String("Cafeteria (Cumulative Liters): ") + 
                  "**" + String(cumulativeTot3) + " L** " + 
                  " <--> Overwater was consumed over 10 minutes at 3rd floor, Flowed Liters: " + 
                  "**" + String(leakFlow3) + " L** " + 
                  " <--> Server IP: " + 
                  "**" + ipAddress + "**";
    sendToTeams(leakMessage);  // Send leak message to Teams
  } else if (data.startsWith("LEAK DETECTED in Flow Sensor 0")) {
    leakFlow0 = data.substring(data.indexOf("Leak Flow: ") + 11).toFloat();
    leakMessage = String("NoPo Nanotecnologies (Cumulative Liters): ") + 
                  "**" + String(cumulativeTot0) + " L** " + 
                  " <--> Overwater was consumed over 10 minutes at Gnd floor, Flowed Liters: " + 
                  "**" + String(leakFlow0) + " L** " + 
                  " <--> Server IP: " + 
                  "**" + ipAddress + "**";
    sendToTeams(leakMessage);  // Send leak message to Teams
  } else {
    leakMessage = "";  
  }
}
void sendDailyConsumptionToTeams() {
  String message = String("Daily Water Consumption Report:\n\n") +
                   "NoPo Nanotechnologies (Ground floor): " + "**" + String(dailyTotalFlow0) + " L**" +
                   " <--> Nopo Office/VRISVA SpaceTech. (1st floor): " + "**" + String(dailyTotalFlow1) + " L**" +
                   " <--> VIGA Entertainment (2nd floor): " + "**" + String(dailyTotalFlow2) + " L**" +
                   " <--> Cafeteria (3rd floor): " + "**" + String(dailyTotalFlow3) + " L**" +
                   " <--> Server IP: " + "**" + ipAddress + "**";

  sendToTeams(message);  // Send the message to Teams
    dailyTotalFlow0 = 0;
    dailyTotalFlow1 = 0;
    dailyTotalFlow2 = 0;
    dailyTotalFlow3 = 0;

}

void addToCumulativeTotal(float arduinoTotal, float &cumulativeTotal, float &prevTotal, const char* key, const char* prevKey) {
    if (arduinoTotal != prevTotal) {
        float increment = (arduinoTotal < prevTotal) ? arduinoTotal : (arduinoTotal - prevTotal);
        cumulativeTotal += increment;
        prevTotal = arduinoTotal;

        // Update daily total as well
        if (key == String("cumulativeTot0")) dailyTotalFlow0 += increment;
        if (key == String("cumulativeTot1")) dailyTotalFlow1 += increment;
        if (key == String("cumulativeTot2")) dailyTotalFlow2 += increment;
        if (key == String("cumulativeTot3")) dailyTotalFlow3 += increment;

        preferences.begin("waterTotals", false);
        preferences.putFloat(key, cumulativeTotal);
        preferences.putFloat(prevKey, prevTotal);
        preferences.end();
    }
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Flow Meter Data</h1>";
  html += "<p>IP Address: " + ipAddress + "</p>";
  html += "<p>Gnd floor (L/min): <input type='text' id='flow0' readonly></p>";
  html += "<p>1st floor (L/min): <input type='text' id='flow1' readonly></p>";
  html += "<p>2nd floor (L/min): <input type='text' id='flow2' readonly></p>";
  html += "<p>3rd floor (L/min): <input type='text' id='flow3' readonly></p>";
  html += "<p>NoPo Nanotechnologies (Cumulative Liters): <input type='text' id='tot0' readonly></p>";
  html += "<p>Nopo Office/VRISVA SpaceTech. (Cumulative Liters): <input type='text' id='tot1' readonly></p>";
  html += "<p>VIGA Entertainment (Cumulative Liters): <input type='text' id='tot2' readonly></p>";
  html += "<p>Cafeteria (Cumulative Liters): <input type='text' id='tot3' readonly></p>";

  html += "<p id='leak'></p>";  // Leak message placeholder

  html += "<script>setInterval(() => {";
  html += "fetch('/data').then(response => response.json()).then(data => {";
  html += "document.getElementById('flow0').value = data.flow0;";
  html += "document.getElementById('flow1').value = data.flow1;";
  html += "document.getElementById('flow2').value = data.flow2;";
  html += "document.getElementById('flow3').value = data.flow3;";
  html += "document.getElementById('tot0').value = data.cumulativeTot0;";
  html += "document.getElementById('tot1').value = data.cumulativeTot1;";
  html += "document.getElementById('tot2').value = data.cumulativeTot2;";
  html += "document.getElementById('tot3').value = data.cumulativeTot3;";
  html += "document.getElementById('leak').innerText = data.leakMessage;";
  html += "});}, 1000);</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{\"flow0\":" + String(flow0) + ",\"flow1\":" + String(flow1) + ",\"flow2\":" + String(flow2) + 
                ",\"flow3\":" + String(flow3) + ",\"cumulativeTot0\":" + String(cumulativeTot0) + ",\"cumulativeTot1\":" + String(cumulativeTot1) + 
                ",\"cumulativeTot2\":" + String(cumulativeTot2) + ",\"cumulativeTot3\":" + String(cumulativeTot3) + 
                ",\"leakMessage\":\"" + leakMessage + "\"}";
  server.send(200, "application/json", json);
}

void sendToTeams(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(teamsWebhookUrl);
    http.addHeader("Content-Type", "application/json");
    String jsonMessage = "{\"text\":\"" + message + "\"}";
    int httpResponseCode = http.POST(jsonMessage);
    Serial.print("Teams response: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    Serial.println("Wi-Fi disconnected, message not sent.");
  }
}

