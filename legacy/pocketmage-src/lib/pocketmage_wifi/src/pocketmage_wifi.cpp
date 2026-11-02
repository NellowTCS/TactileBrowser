#include "pocketmage_wifi.h"

#include <esp_task_wdt.h>

#include <cstring>

static const char* TAG = "PocketMageWifi";

PocketMageWifi& PocketMageWifi::getInstance() {
  static PocketMageWifi instance;
  return instance;
}

PocketMageWifi& P_WIFI = PocketMageWifi::getInstance();

PocketMageWifi::PocketMageWifi()
    : _mutex(xSemaphoreCreateRecursiveMutex()),
      _state(WifiRadioState::Off),
      _scanResults(nullptr),
      _scanResultCount(0),
      _staNetif(nullptr),
      _wifiEventHandler(nullptr),
      _ipEventHandler(nullptr),
      _initialized(false),
      _autoConnectEnabled(true),
      _lastScanTime(0),
      _eventCallback(nullptr) {
  _statusMessage[0] = 0;
  _connectedSSID[0] = 0;
  _ipAddress[0] = 0;
  _pendingSSID[0] = 0;
  _pendingPassword[0] = 0;
  _pendingSave = false;
}

PocketMageWifi::~PocketMageWifi() {
  stop();
  if (_mutex)
    vSemaphoreDelete(_mutex);
}

void PocketMageWifi::begin() {
  if (_initialized)
    return;
  
  // Initialize ESP-IDF networking stack ONCE per boot (static guard)
  static bool netif_initialized = false;
  if (!netif_initialized) {
    Serial.println("WiFi: Calling esp_netif_init()...");
    esp_err_t err = esp_netif_init();
    Serial.printf("WiFi: esp_netif_init returned: %s\n", esp_err_to_name(err));
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      Serial.println("WiFi: CRITICAL - esp_netif_init failed!");
      return;
    }
    
    Serial.println("WiFi: Calling esp_event_loop_create_default()...");
    err = esp_event_loop_create_default();
    Serial.printf("WiFi: esp_event_loop_create_default returned: %s\n", esp_err_to_name(err));
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
      Serial.println("WiFi: CRITICAL - esp_event_loop_create_default failed!");
      return;
    }
    
    netif_initialized = true;
    Serial.println("WiFi: Network stack initialized");
  } else {
    Serial.println("WiFi: Network stack already initialized (static guard)");
  }
  
  // Create network interface once per instance
  if (!_staNetif) {
    Serial.println("WiFi: Creating default WiFi STA interface...");
    _staNetif = esp_netif_create_default_wifi_sta();
    Serial.printf("WiFi: _staNetif = %p\n", _staNetif);
  } else {
    Serial.printf("WiFi: _staNetif already exists: %p\n", _staNetif);
  }
  
  _initialized = true;
  Serial.println("WiFi: begin() complete");
}

void PocketMageWifi::stop() {
  if (_scanResults) {
    free(_scanResults);
    _scanResults = nullptr;
  }
  _initialized = false;
}

void PocketMageWifi::enable() {
  // Call directly - no task needed
  doEnable();
}

void PocketMageWifi::disable() {
  doDisable();
}

void PocketMageWifi::scan() {
  doScan();
}

void PocketMageWifi::connect(const char* ssid, const char* password, bool save) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
  strncpy(_pendingPassword, password, sizeof(_pendingPassword));
  _pendingSave = save;
  xSemaphoreGiveRecursive(_mutex);
  doConnect();
}

void PocketMageWifi::disconnect() {
  doDisconnect();
}

void PocketMageWifi::reconnect() {
  doAutoConnect();
}

WifiRadioState PocketMageWifi::getState() const {
  return _state;
}

bool PocketMageWifi::isConnected() const {
  return _state == WifiRadioState::Connected;
}

bool PocketMageWifi::isScanning() const {
  return _state == WifiRadioState::Scanning;
}

String PocketMageWifi::getStatusMessage() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String msg = String(_statusMessage);
  xSemaphoreGiveRecursive(_mutex);
  return msg;
}

String PocketMageWifi::getConnectedSSID() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ssid = String(_connectedSSID);
  xSemaphoreGiveRecursive(_mutex);
  return ssid;
}

String PocketMageWifi::getIpAddress() const {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  String ip = String(_ipAddress);
  xSemaphoreGiveRecursive(_mutex);
  return ip;
}

int PocketMageWifi::getRssi() const {
  wifi_ap_record_t info;
  if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
    return info.rssi;
  }
  return 0;
}

uint16_t PocketMageWifi::getScanResultCount() const {
  return _scanResultCount;
}

bool PocketMageWifi::getScanResult(uint16_t index, WifiApInfo& out) const {
  if (index >= _scanResultCount || !_scanResults)
    return false;
  strncpy(out.ssid, (const char*)_scanResults[index].ssid, sizeof(out.ssid));
  out.ssid[32] = 0;
  out.rssi = _scanResults[index].rssi;
  out.channel = _scanResults[index].primary;
  out.authmode = _scanResults[index].authmode;
  return true;
}

bool PocketMageWifi::hasSavedCredentials(const char* ssid) const {
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    bool found = _prefs.isKey(ssid);
    _prefs.end();
    return found;
  }
  return false;
}

bool PocketMageWifi::loadSavedCredentials(const char* ssid, char* password, size_t maxLen) const {
  if (_prefs.begin(PREFS_NAMESPACE, true)) {
    String pass = _prefs.getString(ssid, "");
    _prefs.end();
    if (pass.length() > 0) {
      strncpy(password, pass.c_str(), maxLen);
      password[maxLen - 1] = 0;
      return true;
    }
  }
  return false;
}

void PocketMageWifi::clearSavedCredentials(const char* ssid) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.remove(ssid);
    _prefs.end();
  }
}

void PocketMageWifi::setEventCallback(WifiEventCallback cb) {
  _eventCallback = cb;
}

void PocketMageWifi::espEventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
  PocketMageWifi* self = static_cast<PocketMageWifi*>(arg);
  if (base == WIFI_EVENT)
    self->handleWifiEvent(id, data);
  else if (base == IP_EVENT)
    self->handleIpEvent(id, data);
}

void PocketMageWifi::handleWifiEvent(int32_t id, void* data) {
  switch (id) {
    case WIFI_EVENT_STA_START:
      Serial.println("WiFi EVENT: STA_START");
      setStatus("WiFi started");
      break;
    case WIFI_EVENT_STA_CONNECTED:
      Serial.println("WiFi EVENT: STA_CONNECTED");
      setStatus("WiFi connected");
      _state = WifiRadioState::Connected;
      publishEvent();
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi EVENT: STA_DISCONNECTED");
      setStatus("WiFi disconnected");
      _state = WifiRadioState::On;
      publishEvent();
      break;
    case WIFI_EVENT_SCAN_DONE:
      Serial.println("WiFi EVENT: SCAN_DONE");
      setStatus("Scan done");
      {
        uint16_t num = 0;
        esp_wifi_scan_get_ap_num(&num);
        // Clamp num to MAX_SCAN_RESULTS before allocation
        if (num > MAX_SCAN_RESULTS)
          num = MAX_SCAN_RESULTS;
        
        if (_scanResults)
          free(_scanResults);
        _scanResults = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * num);
        if (_scanResults) {
          esp_err_t err = esp_wifi_scan_get_ap_records(&num, _scanResults);
          Serial.printf("WiFi: Got %d APs from scan (%s)\n", num, esp_err_to_name(err));
          _scanResultCount = num;
        } else {
          Serial.println("WiFi: ERROR - Failed to allocate scan results buffer");
          _scanResultCount = 0;
        }
      }
      _state = WifiRadioState::On;
      publishEvent();
      break;
    default:
      break;
  }
}

void PocketMageWifi::handleIpEvent(int32_t id, void* data) {
  if (id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*)data;
    snprintf(_ipAddress, sizeof(_ipAddress), "%d.%d.%d.%d", IP2STR(&event->ip_info.ip));
    setStatus("Got IP");
    _state = WifiRadioState::Connected;
    publishEvent();
  }
}

void PocketMageWifi::doEnable() {
  if (_state == WifiRadioState::Off || _state == WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOn;
    setStatus("Enabling WiFi...");
    Serial.println("WiFi: doEnable() called");
    
    // Don't recreate netif - it's created once in begin()
    if (!_staNetif) {
      Serial.println("WiFi: ERROR - _staNetif is null!");
      setStatus("WiFi init failed");
      _state = WifiRadioState::Off;
      return;
    }
    
    // Check if WiFi is already initialized (e.g., after disable/enable cycle)
    wifi_mode_t mode;
    esp_err_t err = esp_wifi_get_mode(&mode);
    Serial.printf("WiFi: esp_wifi_get_mode returned: %s\n", esp_err_to_name(err));
    
    if (err == ESP_OK) {
      // WiFi already initialized, just start it
      Serial.printf("WiFi: Already initialized (mode=%d), just starting\n", mode);
      err = esp_wifi_start();
      Serial.printf("WiFi: esp_wifi_start returned: %s\n", esp_err_to_name(err));
      
      if (err == ESP_OK) {
        _state = WifiRadioState::On;
        setStatus("WiFi enabled");
      } else {
        _state = WifiRadioState::Off;
        setStatus("WiFi start failed");
      }
      publishEvent();
      return;
    }
    
    // WiFi not initialized yet, do full init
    Serial.println("WiFi: Not initialized yet, doing FULL init");
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    Serial.println("WiFi: Calling esp_wifi_init()...");
    err = esp_wifi_init(&cfg);
    Serial.printf("WiFi: esp_wifi_init returned: %s\n", esp_err_to_name(err));
    if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
      Serial.printf("WiFi: ERROR - esp_wifi_init failed: %s\n", esp_err_to_name(err));
      setStatus("WiFi init failed");
      _state = WifiRadioState::Off;
      publishEvent();
      return;
    }
    
    Serial.println("WiFi: Registering event handlers...");
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &PocketMageWifi::espEventHandler, this, &_wifiEventHandler);
    Serial.printf("WiFi: WIFI_EVENT handler registered: %s\n", esp_err_to_name(err));
    
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &PocketMageWifi::espEventHandler, this, &_ipEventHandler);
    Serial.printf("WiFi: IP_EVENT handler registered: %s\n", esp_err_to_name(err));
    
    Serial.println("WiFi: Setting mode to STA...");
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    Serial.printf("WiFi: esp_wifi_set_mode returned: %s\n", esp_err_to_name(err));
    if (err != ESP_OK) {
      Serial.printf("WiFi: ERROR - esp_wifi_set_mode failed: %s\n", esp_err_to_name(err));
      setStatus("WiFi mode failed");
      _state = WifiRadioState::Off;
      publishEvent();
      return;
    }
    
    Serial.println("WiFi: Starting WiFi stack...");
    err = esp_wifi_start();
    Serial.printf("WiFi: esp_wifi_start returned: %s\n", esp_err_to_name(err));
    
    if (err == ESP_OK) {
      _state = WifiRadioState::On;
      setStatus("WiFi enabled");
      Serial.println("WiFi: Successfully enabled!");
    } else {
      Serial.printf("WiFi: ERROR - esp_wifi_start failed: %s\n", esp_err_to_name(err));
      _state = WifiRadioState::Off;
      setStatus("WiFi start failed");
    }
    publishEvent();
  }
}

void PocketMageWifi::doDisable() {
  if (_state != WifiRadioState::Off && _state != WifiRadioState::TurningOff) {
    _state = WifiRadioState::TurningOff;
    setStatus("Disabling WiFi...");
    Serial.println("WiFi: doDisable() called");
    
    esp_err_t err = esp_wifi_stop();
    Serial.printf("WiFi: esp_wifi_stop returned: %s\n", esp_err_to_name(err));
    
    err = esp_wifi_deinit();
    Serial.printf("WiFi: esp_wifi_deinit returned: %s\n", esp_err_to_name(err));
    
    // Don't destroy netif - we'll reuse it
    
    _state = WifiRadioState::Off;
    setStatus("WiFi disabled");
    publishEvent();
  }
}

void PocketMageWifi::doScan() {
  if (_state == WifiRadioState::On || _state == WifiRadioState::Connected) {
    _state = WifiRadioState::Scanning;
    setStatus("Scanning...");
    Serial.println("WiFi: Starting scan...");
    wifi_scan_config_t scanConf = {};
    scanConf.ssid = nullptr;
    scanConf.bssid = nullptr;
    scanConf.channel = 0;
    scanConf.show_hidden = true;
    
    esp_err_t err = esp_wifi_scan_start(&scanConf, false);
    Serial.printf("WiFi: esp_wifi_scan_start returned: %s\n", esp_err_to_name(err));
    
    publishEvent();
  }
}

void PocketMageWifi::doConnect() {
  if (_pendingSSID[0] == 0) {
    setStatus("No SSID");
    return;
  }
  setStatus("Connecting...");
  Serial.printf("WiFi: Connecting to SSID: %s\n", _pendingSSID);
  wifi_config_t config = {};
  strncpy((char*)config.sta.ssid, _pendingSSID, sizeof(config.sta.ssid));
  strncpy((char*)config.sta.password, _pendingPassword, sizeof(config.sta.password));
  config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  config.sta.pmf_cfg.capable = true;
  
  esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &config);
  Serial.printf("WiFi: esp_wifi_set_config returned: %s\n", esp_err_to_name(err));
  
  err = esp_wifi_connect();
  Serial.printf("WiFi: esp_wifi_connect returned: %s\n", esp_err_to_name(err));
  
  if (_pendingSave)
    saveCredentials(_pendingSSID, _pendingPassword);
  _state = WifiRadioState::Connecting;
  publishEvent();
}

void PocketMageWifi::doDisconnect() {
  esp_err_t err = esp_wifi_disconnect();
  Serial.printf("WiFi: esp_wifi_disconnect returned: %s\n", esp_err_to_name(err));
  
  setStatus("Disconnecting...");
  _state = WifiRadioState::On;
  publishEvent();
}

void PocketMageWifi::doAutoConnect() {
  // Try to find a saved network in scan results
  char ssid[33] = {0};
  char password[65] = {0};
  if (findSavedNetwork(ssid, password)) {
    strncpy(_pendingSSID, ssid, sizeof(_pendingSSID));
    strncpy(_pendingPassword, password, sizeof(_pendingPassword));
    _pendingSave = false;
    doConnect();
  }
}

// Call this periodically from your main loop to handle auto-scan/auto-connect
void PocketMageWifi::process() {
  if (!_initialized) return;
  
  // Auto-scan/auto-connect if enabled
  if (_autoConnectEnabled && _state == WifiRadioState::On) {
    unsigned long now = millis();
    if (now - _lastScanTime > AUTO_SCAN_INTERVAL) {
      _lastScanTime = now;
      Serial.println("WiFi: Auto-scan triggered");
      doScan();
      // Auto-connect will happen when scan completes (in event handler)
    }
  }
}

void PocketMageWifi::setStatus(const char* msg) {
  xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
  strncpy(_statusMessage, msg, sizeof(_statusMessage));
  _statusMessage[sizeof(_statusMessage) - 1] = 0;
  xSemaphoreGiveRecursive(_mutex);
  publishEvent();
}

void PocketMageWifi::publishEvent() {
  if (_eventCallback)
    _eventCallback();
}

void PocketMageWifi::saveCredentials(const char* ssid, const char* password) {
  if (_prefs.begin(PREFS_NAMESPACE, false)) {
    _prefs.putString(ssid, password);
    _prefs.end();
  }
}

bool PocketMageWifi::findSavedNetwork(char* ssid, char* password) {
  if (!_scanResults || _scanResultCount == 0)
    return false;
  for (uint16_t i = 0; i < _scanResultCount; ++i) {
    if (hasSavedCredentials((const char*)_scanResults[i].ssid)) {
      strncpy(ssid, (const char*)_scanResults[i].ssid, 33);
      loadSavedCredentials(ssid, password, 65);
      return true;
    }
  }
  return false;
}
