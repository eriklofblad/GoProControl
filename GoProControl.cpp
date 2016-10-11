#include "GoProControl.h"

GoProControl::GoProControl(char* ssid, char* password, char* GoProIP, int GoProPort, bool debug) {
    _ssid      = ssid;
    _password  = password;
    _GoProIP   = GoProIP;
    _GoProPort = GoProPort;
    _debug     = debug;

    if (_debug) {
      //Om debug är true aktiveras debug/info utskrifter
      #ifdef WiFi101
      Serial.begin(9600);
      #endif // WiFi101
      #ifdef WiFi8266
      Serial.begin(115200);
      #endif // WiFi8266
      
    }
}

bool GoProControl::connect() {
  //Anslut till GoPro hotspot
    
#ifdef WiFi101
  WiFi.setPins(8,7,4,2);
#endif
  WiFi.begin(_ssid, _password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (_debug) {
      Serial.print("Trying to connect, current status: ");
      Serial.println(WiFi.status());
      }
    }
  if (WiFi.status() == WL_CONNECTED) {
    if (_debug) {Serial.println("Connected");}
    return true;
  }
  else {
    if (_debug) {Serial.println("Failed to connect");}
    return false;
  }
}

void GoProControl::wake() {
  //Skickar WakeOnLan(wol) signal

  WiFiUDP udp; //wol skickas via UDP

  if (udp.begin(_GoProPort) == 1) { //Är önskad port tillgänglig?
    if (_debug) {Serial.println("UDP port open");}
    
    /*
    Bygg wol "magicpackage"
    
    "6 bytes of all 255 (FF FF FF FF FF FF in hexadecimal),
    followed by sixteen repetitions of the target computer's 48-bit MAC address,
    for a total of 102 bytes."

    GoPro behöver dock inte MAC adressen för att vakna. Därför skickas enbart första delen.
    */
    char magicP[102];
    //6 Bytes with 0xFF
    for (int i=0;i<6;i++)
        magicP[i] = 0xff;
        if (_debug) {Serial.println("InSide UDP magicPackage loop");}
    
    udp.beginPacket(_GoProIP, _GoProPort);
    udp.write(magicP);
    udp.endPacket();
    udp.stop();
  }
  else {
    if (_debug) {Serial.println("Failed to open UDP port");}
  }
}

#ifdef WiFi8266
bool GoProControl::httpGET(String url) {
  //Skicka GET request via http
  
  int httpCode = 0;
  HTTPClient client;
  client.begin("http://" + String(_GoProIP) + url);

  while (httpCode < 1 || httpCode == 500) {
    httpCode = client.GET();
    if (_debug) {
      Serial.print(url+" ");
      Serial.println(httpCode);}
    delay(100);
  }
  client.end();
  if (httpCode == 200) {
    return true;
  }
  else {
    return false;
  }
}
#endif

#ifdef WiFi101
bool GoProControl::httpGET(String url) {
  WiFiClient client;
  if (client.connect("10.5.5.9", 80)) {
  Serial.println("connected to server");
  // Make a HTTP request:
  client.println("GET " + url + " HTTP/1.1");
  client.println("Host: 10.5.5.9");
  client.println("Connection: close");
  client.println();
  }
  while(client.available()){
    bool c = client.find("200 OK"); //Möjlig väg för att kunna plocka ut svarskoden?
    Serial.println(c);
  }
  return true;
}
#endif

bool GoProControl::status() {
  return httpGET("/gp/gpControl/status");
}

bool GoProControl::videoModeOn() {
  return httpGET("/gp/gpControl/command/sub_mode?mode=0&sub_mode=0");
}

bool GoProControl::videoModeOff() {
  return httpGET("/gp/gpControl/setting/10/0");
}

bool GoProControl::trigger() {
  return httpGET("/gp/gpControl/command/shutter?p=1");
}

bool GoProControl::stop() {
  return httpGET("/gp/gpControl/command/shutter?p=0");
}

bool GoProControl::sleep() {
  return httpGET("http://" + String(_GoProIP) + "/gp/gpControl/command/system/sleep");
}