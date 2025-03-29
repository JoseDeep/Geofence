#include <TinyGPS++.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

#define GSM_RX 16   // GSM RX Pin
#define GSM_TX 17   // GSM TX Pin
#define GPS_RX 4    // GPS RX Pin
#define GPS_TX 2    // GPS TX Pin

HardwareSerial gsm(2);      
HardwareSerial neogps(1);   
TinyGPSPlus gps;
WebServer server(80);       

const char *ssid = "M53";      
const char *password = "12345678";      

bool geofenceActive = false;    // Activated only when "set" SMS is received from +919840449720
bool alertTriggered = false;    
unsigned long setReceivedTime = 0;  

void sendCommand(const char* cmd, int delayTime);
void sendSMS(const char* number, const char* message);
void checkForSetMessage();
void sendDelayedAlerts();
void sendGeofenceLink();

void setup() {
    Serial.begin(115200);
    gsm.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
    neogps.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

    // ✅ Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed!");
        return;
    }
    Serial.println("SPIFFS Initialized.");

    // ✅ Serve HTML File from SPIFFS
    server.on("/", HTTP_GET, []() {
        File file = SPIFFS.open("/map.html", "r");
        if (!file) {
            server.send(500, "text/plain", "Failed to open map.html");
            return;
        }
        server.streamFile(file, "text/html");
        file.close();
    });

    server.begin();
    Serial.println("Web Server Started.");
    
    sendGeofenceLink();
}

void loop() {
    server.handleClient();
    checkForSetMessage();

    if (geofenceActive) {  
        sendDelayedAlerts();
    }
}

void checkForSetMessage() {
    if (gsm.available()) {
        String message = "";
        while (gsm.available()) {
            char c = gsm.read();
            message += c;
        }

        message.toLowerCase();  

        Serial.println("Received SMS: " + message);

        if (message.indexOf("+919150102705") != -1 && message.indexOf("set") != -1) {
            Serial.println("✅ Valid 'set' message received from +919150102705!");
            geofenceActive = true;
            alertTriggered = false;  
            setReceivedTime = millis();  
        }
    }
}

void sendDelayedAlerts() {
    if (alertTriggered) return;  

    unsigned long currentTime = millis();

    if (currentTime - setReceivedTime >= 30000 && currentTime - setReceivedTime < 45000) {
        Serial.println("📢 Sending 'Device nearing geofence' alert...");
        sendSMS("+919150102705", "Device nearing geofence!");
    }

    if (currentTime - setReceivedTime >= 45000) {
        Serial.println("🚨 Sending 'Geofence crossed' alert...");
        sendSMS("+919150102705", "Geofence crossed!");
        alertTriggered = true;  
    }
}

void sendGeofenceLink() {
    String link = "http://" + WiFi.localIP().toString() + "/";  
    sendSMS("+919150102705", ("Set geofence: " + link).c_str());
    Serial.println("📩 Geofence link sent.");
}

void sendSMS(const char* number, const char* message) {
    Serial.println("📨 Sending SMS...");
    sendCommand("AT+CMGF=1", 1000);
    
    gsm.print("AT+CMGS=\"");
    gsm.print(number);
    gsm.println("\"");
    delay(1000);
    
    gsm.print(message);
    delay(500);
    
    gsm.write(26);  
    delay(5000);
    
    Serial.println("✅ SMS Sent!");
}

void sendCommand(const char* cmd, int delayTime) {
    Serial.print("📡 Command: ");
    Serial.println(cmd);
    gsm.println(cmd);
    delay(delayTime);
    
    while (gsm.available()) {
        Serial.write(gsm.read());
    }
    Serial.println();
}
