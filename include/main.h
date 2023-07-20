// Flag to minimize library size.
#pragma once

// Imports the ESP32 BLE Library, ESP32 WiFi Library, and ESP32 ESP-Now Library
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_wifi.h>
#include <esp_task_wdt.h>
#include <esp_now.h>
#include <WiFi.h>

// Imports the Arduino Standard Library
#include <Arduino.h>

// Bluetooth Connection Constants
#define UUID_SERVICE "734c019d-0055-4922-a5e1-943d57c8281a"
#define UUID_CHARACTERISTIC "671acaa6-7c79-4722-9ad0-4bdf3118695a"

// Bluetooth Client vars storage.
struct BTClient {
  BLEScan* pBLEScan;
  BLEClient* pBLEClient;
  BLERemoteService* pRemoteService;
  BLERemoteCharacteristic* pRemoteCharacteristic;
  BLEAdvertisedDevice advertisedDevice;
};

// Bluetooth Server Vars
struct BTServer {
  BLEServer *bleServer;
  BLEService *bleService;
  BLECharacteristic *bleCharacteristic;
  BLEAdvertising *bleAdvertising;
};

// Tracks the bluetooth state
struct BTState {
  bool is_bt_connected;       // If a BT device was connected
  bool is_en_connected;       // Is ESP-Now Connected
  bool is_error;              // If a BTError occurred
  uint16_t delay;             // Delay length for next discovery phase shift
  uint8_t mac_address[6];     // MAC Address of this device
  char server_mac_address[18];// Only used if this device becomes client
};

// Instantiates the bluetooth client related variables.
BTClient btClient;
// Instantiates the bluetooth server related variables.
BTServer btServer;
// Instantiates a global BTState inherent to this sword
BTState bt;

class BluetoothServerCallbacks : public BLEServerCallbacks {
    inline void onConnect(BLEServer* pServer) 
    { bt.is_bt_connected = true; }
    inline void onDisconnect(BLEServer* pServer) 
    { bt.is_bt_connected = false; bt.is_error = true; }
};
/// @brief Handles advertisement communications when an ESP32 is scanning.
class BluetoothClientCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Event-driven instruction for when an advertising device is discovered.
   * 
   * @param advertisedDevice The device that was discovered.
   */
  inline void onResult(BLEAdvertisedDevice advertisedDevice) {    
    // If the device has a service UUID, and it matches the UUID we're looking for, then we found the server.  
    if (advertisedDevice.haveServiceUUID() && 
        advertisedDevice.getServiceUUID().toString() == UUID_SERVICE) {
      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
      btClient.advertisedDevice = advertisedDevice;
      bt.is_bt_connected = true;
      return;
    }
  }
};

void setup();
void loop();