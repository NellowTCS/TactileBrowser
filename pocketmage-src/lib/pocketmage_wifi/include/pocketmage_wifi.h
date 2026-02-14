#pragma once

#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <Preferences.h>
#include <vector>
#include <functional>

// WiFi radio state machine
enum class WifiRadioState {
    Off,            // Radio is off
    TurningOn,      // Radio is initializing
    On,             // Radio is on but not connected
    Scanning,       // Scanning for networks
    Connecting,     // Connection attempt in progress
    Connected,      // Connected to an AP
    TurningOff      // Radio is shutting down
};

// Scanned AP record (simplified)
struct WifiApInfo {
    char ssid[33];
    int8_t rssi;
    uint8_t channel;
    wifi_auth_mode_t authmode;
};

// Event callback type
typedef std::function<void(void)> WifiEventCallback;

class PocketMageWifi {
public:
    static PocketMageWifi& getInstance();
    
    // Lifecycle
    void begin();   // Call once at startup to initialize the service
    void stop();    // Call at shutdown to cleanly stop the service
    
    // Control methods (all non-blocking, dispatch to task)
    void enable();                                              // Turn on WiFi radio
    void disable();                                             // Turn off WiFi radio
    void scan();                                                // Start a scan
    void connect(const char* ssid, const char* password, bool save = true);  // Connect to AP
    void disconnect();                                          // Disconnect from current AP
    void reconnect();                                           // Retry connection with saved creds
    
    // Status queries (thread-safe)
    WifiRadioState getState() const;
    bool isConnected() const;
    bool isScanning() const;
    String getStatusMessage() const;
    String getConnectedSSID() const;
    String getIpAddress() const;
    int getRssi() const;
    
    // Scan results
    uint16_t getScanResultCount() const;
    bool getScanResult(uint16_t index, WifiApInfo& out) const;
    
    // Saved credentials management
    bool hasSavedCredentials(const char* ssid) const;
    bool loadSavedCredentials(const char* ssid, char* password, size_t maxLen) const;
    void clearSavedCredentials(const char* ssid);
    
    // Event callback (called when state changes: use to trigger display update)
    void setEventCallback(WifiEventCallback cb);
    
private:
    PocketMageWifi();
    ~PocketMageWifi();
    PocketMageWifi(const PocketMageWifi&) = delete;
    PocketMageWifi& operator=(const PocketMageWifi&) = delete;
    
    // Task function
    static void wifiTaskFunc(void* param);
    void taskLoop();
    
    // ESP event handler
    static void espEventHandler(void* arg, esp_event_base_t base, int32_t id, void* data);
    void handleWifiEvent(int32_t id, void* data);
    void handleIpEvent(int32_t id, void* data);
    
    // Internal operations (run in task context)
    void doEnable();
    void doDisable();
    void doScan();
    void doConnect();
    void doDisconnect();
    void doAutoConnect();
    
    // Helpers
    void setStatus(const char* msg);
    void publishEvent();
    void saveCredentials(const char* ssid, const char* password);
    bool findSavedNetwork(char* ssid, char* password);
    
    // Command queue types
    enum class Command : uint8_t {
        None = 0,
        Enable,
        Disable,
        Scan,
        Connect,
        Disconnect,
        Reconnect,
        CheckAutoConnect
    };
    
    // Thread-safe state
    mutable SemaphoreHandle_t _mutex;
    volatile WifiRadioState _state;
    char _statusMessage[64];
    char _connectedSSID[33];
    char _ipAddress[16];
    
    // Pending connection data
    char _pendingSSID[33];
    char _pendingPassword[65];
    bool _pendingSave;
    
    // Scan results
    wifi_ap_record_t* _scanResults;
    uint16_t _scanResultCount;
    static constexpr uint16_t MAX_SCAN_RESULTS = 20;
    
    // FreeRTOS handles
    TaskHandle_t _taskHandle;
    QueueHandle_t _commandQueue;
    EventGroupHandle_t _eventGroup;
    
    // ESP handles
    esp_netif_t* _staNetif;
    esp_event_handler_instance_t _wifiEventHandler;
    esp_event_handler_instance_t _ipEventHandler;
    
    // Preferences
    mutable Preferences _prefs;
    static constexpr const char* PREFS_NAMESPACE = "pmwifi";
    
    // Event callback
    WifiEventCallback _eventCallback;
    
    // Event bits
    static constexpr uint32_t BIT_CONNECTED = BIT0;
    static constexpr uint32_t BIT_DISCONNECTED = BIT1;
    static constexpr uint32_t BIT_SCAN_DONE = BIT2;
    static constexpr uint32_t BIT_CONNECT_FAIL = BIT3;
    
    // Flags
    bool _initialized;
    bool _autoConnectEnabled;
    unsigned long _lastScanTime;
    static constexpr unsigned long AUTO_SCAN_INTERVAL = 15000; // 15 seconds
};

// Global accessor for convenience
extern PocketMageWifi& P_WIFI;
