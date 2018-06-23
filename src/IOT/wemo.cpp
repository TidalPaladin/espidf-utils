#include "wemo.h"

IPAddress ipMulti(239, 255, 255, 250);


void esp_wemo_prep_id() {

  char uuid[64];
  sprintf(uuid, PSTR("38323636-4558-4dda-9188-cda0e6%02x%02x%02x"),
        (uint16_t) ((chipId >> 16) & 0xff),
        (uint16_t) ((chipId >>  8) & 0xff),
        (uint16_t)   chipId        & 0xff);

  serial = String(uuid);
  persistent_uuid = "Socket-1_0-" + serial;
  device_name = "WemoEm["+String(ESP.getChipId())+("]");
}

void respondToSearch() {

    ESP_LOGI(ESP_WEMO_TAG, "Responding to search");

   


    ESP_LOGI(ESP_WEMO_TAG, "Sent query response");
}

void startHttpServer() {
    HTTP.on("/index.html", HTTP_GET, [](){
      Serial.println("Got Request index.html ...\n");
      HTTP.send(200, "text/plain", "Hello World!");
    });

    HTTP.on("/upnp/control/basicevent1", HTTP_POST, []() {
      Serial.println("########## Responding to  /upnp/control/basicevent1 ... ##########");

      //for (int x=0; x <= HTTP.args(); x++) {
      //  Serial.println(HTTP.arg(x));
      //}

      String request = HTTP.arg(0);
      Serial.print("request:");
      Serial.println(request);

      if(request.indexOf("<BinaryState>1</BinaryState>") > 0) {
          Serial.println("Got Turn on request");
          turnOn();
      }

      if(request.indexOf("<BinaryState>0</BinaryState>") > 0) {
          Serial.println("Got Turn off request");
          turnOff();
      }

      HTTP.send(200, "text/plain", "");
    });

    HTTP.on("/eventservice.xml", HTTP_GET, [](){
      Serial.println(" ########## Responding to eventservice.xml ... ########\n");
      

      HTTP.send(200, "text/plain", eventservice_xml.c_str());
    });

    HTTP.on("/setup.xml", HTTP_GET, [](){
      Serial.println(" ########## Responding to setup.xml ... ########\n");

      IPAddress localIP = WiFi.localIP();
      char s[16];
      sprintf(s, "%d.%d.%d.%d", localIP[0], localIP[1], localIP[2], localIP[3]);

    

        HTTP.send(200, "text/xml", setup_xml.c_str());

        Serial.print("Sending :");
        Serial.println(setup_xml);
    });

    HTTP.begin();
    Serial.println("HTTP Server started ..");
}







void setup() {

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);


  // Make sure the builtin RED led is off
  pinMode(D0,OUTPUT);
  digitalWrite(D0,HIGH);

  // Make sure the builtin BLUE led is off
  pinMode(2,OUTPUT);
  digitalWrite(2,HIGH);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword((const char *)OTA_PASSWORD);

  prepareIds();

  // Delay so we dont draw too much current right at boot
  delay(1000);

  // Try to connect to wifi, set up AP if unable
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect(AP_SSID);

  configTime(5 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT+5 = central
  ArduinoOTA.begin();

  udpConnected = connectUDP();
  if (udpConnected){
    startHttpServer();
  }
}

void loop() {
  // Handle OTA, RemoteDebug, websocket, and HTTP server
  ArduinoOTA.handle();
  HTTP.handleClient();
  delay(1);


  if(WiFi.status() == WL_CONNECTED){
    if(udpConnected){
      // if there’s data available, read a packet
      int packetSize = UDP.parsePacket();

      if(packetSize) {
        Serial.println("");
        Serial.print("Received packet of size ");
        Serial.println(packetSize);
        Serial.print("From ");
        IPAddress remote = UDP.remoteIP();

        for (int i =0; i < 4; i++) {
          Serial.print(remote[i], DEC);
          if (i < 3) {
            Serial.print(".");
          }
        }

        Serial.print(", port ");
        Serial.println(UDP.remotePort());

        int len = UDP.read(packetBuffer, 255);

        if (len > 0) {
            packetBuffer[len] = 0;
        }

        String request = packetBuffer;
        //Serial.println("Request:");
        //Serial.println(request);

        if(request.indexOf('M-SEARCH') > -1) {
            if(request.indexOf("urn:Belkin:device:**") > -1) {
                Serial.println("Responding to search request ...");
                respondToSearch();
            }
        }
      }
      delay(10);
    }
  }
  else {
    wifiManager.autoConnect(AP_SSID);
  }
}