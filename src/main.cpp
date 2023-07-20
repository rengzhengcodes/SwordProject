// Flag to minimize library size.
#pragma GCC optimize ("O3")

// Imports the ESP32 BLE Library, ESP32 WiFi Library, and ESP32 ESP-Now Library
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>

// Imports the Arduino Standard Library
#include <Arduino.h>

// Bluetooth Client vars storage.
struct BTClient {
  BLEScan* pBLEScan;
  BLEClient* pBLEClient;
  BLERemoteService* pRemoteService;
  BLERemoteCharacteristic* pRemoteCharacteristic;
  BLEAdvertisedDevice advertisedDevice;
};

BTClient btClient = {
  .pBLEScan = BLEDevice::getScan(),
  .pBLEClient = BLEDevice::createClient(),
  .pRemoteService = nullptr,
  .pRemoteCharacteristic = nullptr,
  .advertisedDevice = BLEAdvertisedDevice()
};

// Bluetooth Server Vars
struct BTServer {
  BLEServer *bleServer;
  BLEService *bleService;
  BLECharacteristic *bleCharacteristic;
  BLEAdvertising *bleAdvertising;
};

BTServer btServer = {
  .bleServer = BLEDevice::createServer(),
  .bleService = nullptr,
  .bleCharacteristic = nullptr,
  .bleAdvertising = nullptr
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

// Bluetooth Connection Constants
#define UUID_SERVICE "734c019d-0055-4922-a5e1-943d57c8281a"
#define UUID_CHARACTERISTIC "671acaa6-7c79-4722-9ad0-4bdf3118695a"

// Creates a global BTState inherent to this sword
BTState bt = {
  .is_bt_connected = false,
  .is_en_connected = false,
  .is_error = false,
  .delay = 0,
  .mac_address = { 0 },
  .server_mac_address = { 0 }
};

/// @brief Handles communication when devices try to connect to the server.
class BluetoothServerCallbacks : public BLEServerCallbacks {
  /**
   * Event-driven instruction for when a esp32 device connects to the server.
   * 
   * @param pServer The server that the device connects to (this server).
   */
  void onConnect(BLEServer* pServer) {
    // Modifies state to reflect a connection.
    bt.is_bt_connected = true;
  }
  /**
   * Event-driven instruction for when a esp32 device disconnects from the server.
   * 
   * @param pServer The server that the device disconnects from (this server).
   */
  void onDisconnect(BLEServer* pServer) {
    // Modifies state to reflect a disconnection.
    bt.is_bt_connected = false;
    bt.is_error = true;
  }
};

/// @brief Handles advertisement communications when an ESP32 is scanning.
class BluetoothClientCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Event-driven instruction for when an advertising device is discovered.
   * 
   * @param advertisedDevice The device that was discovered.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {    
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

void setup() {
  BLEDevice::init("GameSword");
  Serial.begin(115200);

  // Initialize Bluetooth Client
  btClient.pBLEScan->setAdvertisedDeviceCallbacks(new BluetoothClientCallbacks());
  btClient.pBLEScan->setActiveScan(true);
  btClient.pBLEScan->setInterval(100);
  btClient.pBLEScan->setWindow(99);

  // Initialize Bluetooth Server
  btServer.bleServer = BLEDevice::createServer();
  btServer.bleServer->setCallbacks(new BluetoothServerCallbacks());
  btServer.bleService = btServer.bleServer->createService(UUID_SERVICE);
  btServer.bleCharacteristic = btServer.bleService->createCharacteristic(UUID_CHARACTERISTIC, BLECharacteristic::PROPERTY_READ);

  // Starts advertising the server.
  btServer.bleService->start();
  btServer.bleAdvertising = BLEDevice::getAdvertising();
  BLEDevice::startAdvertising();
  Serial.println("Advertising...");


  // Keeps scanning until a bluetooth connection is formed or an error occurs.
  while(!bt.is_bt_connected && !bt.is_error) {
    // Does a 5 second scan for a server.
    btClient.pBLEScan->start(5);
    Serial.println("Scanning...");
    delay(5000);
    btClient.pBLEScan->stop();
    
    // If we found a server, attempt to connect.
    if (!btClient.pBLEClient->connect(&btClient.advertisedDevice)) {
      continue;
    }
    // Collects the service and characteristic from the server.
    btClient.pRemoteService = btClient.pBLEClient->getService(UUID_SERVICE);
    if (!(btClient.pRemoteService = 
          btClient.pBLEClient->getService(UUID_SERVICE))) {
      continue;
    }
    // Collects the characteristic from the server.
    if (!(btClient.pRemoteCharacteristic = 
        btClient.pRemoteService->getCharacteristic(UUID_CHARACTERISTIC))) {
      continue;
    }
    // Reads the sent mac address from the server.
    const uint8_t* rawData = (uint8_t*) btClient.pRemoteCharacteristic->readValue().c_str();
    sprintf(bt.server_mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5]);
    // If we made it here, we are connected.
    bt.is_bt_connected = true;
  }

  // If we are connected, keep printing so until it's false.
  while(bt.is_bt_connected) {
    Serial.println("Connected to Server");
  }
}
  // If you made it here, that probably worked
  // Hi Reng, it's 6am, and I need to start moving, so here's the plan
  // - Follow this to the tee https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/
  // - If we're a bt server `is_bt_server`, then we just wait for an ESP Now Connection
  // - If we're a bt client `!is_bt_server`, then use the mac address hopefully in `server_mac_address` to connect using ESP now
  // - Tom has most of the code completed for motion detecting, so grab that from him.
  // - Uh, I guess what I leave to you is sending the record of the impact to the server sword, comparing time stamps, and determining if a life point was lost
  // - You're a much better programmer than I, so I have full faith you'll finish this in under 5 hours
  // - Don't forget to unit test lol
  // - Thanks a bunch bestie :)

void loop() {

}