#ifndef WIFI_H
#define WIFI_H

#define WL_CONNECTED 3
#define WIFI_STA 1

class IPAddress {
public:
    const char* localIP() { return "192.168.1.1"; }
};

class WiFiMock {
public:
    void mode(int m) {}
    void begin(const char* ssid, const char* pass) {}
    int status() { return WL_CONNECTED; }
    IPAddress localIP() { return IPAddress(); }
};

extern WiFiMock WiFi;

#endif
