#include <random>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <WiFi.h>

// Bluetooth Client Vars
BLEScan* pBLEScan;
BLEClient* pBLEClient;
BLERemoteService* pRemoteService;
BLERemoteCharacteristic* pRemoteCharacteristic;
BLEAdvertisedDevice _advertisedDevice;

// Bluetooth Server Vars
BLEServer *bleServer;
BLEService *bleService;
BLECharacteristic *bleCharacteristic;
BLEAdvertising *bleAdvertising;

// State
bool is_server = true;
bool is_bt_connected = false;
bool is_en_connected = false; // Is ESP-Now Connected
bool is_error = false;
int32_t delay_length;
uint8_t mac_address[6];
char server_mac_address[18]; // Only used if this device becomes client

// Bluetooth Connection Constants
const char* uuid_service = "734c019d-0055-4922-a5e1-943d57c8281a";
const char* uuid_characteristic = "671acaa6-7c79-4722-9ad0-4bdf3118695a";

// Flip `is_server` and set `delay` to a random milisecond value
void next_discovery() {
  is_server = !is_server;
  auto gen = std::bind(std::uniform_int_distribution<int32_t>(1000,5000),std::default_random_engine());
  delay_length = gen();
}

class BluetoothServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    is_bt_connected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    is_bt_connected = false;
    is_error = true;
  }
}

class BluetoothClientCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {      
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().toString() == uuid_service) {
        Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
        _advertisedDevice = advertisedDevice;
        is_bt_connected = true;
        return;
      }
    }
};

void setup() {
  BLEDevice::init("Gamesword")
  Serial.begin(115200);
  // Grab MAC Address
  WiFi.mode(WIFI_MODE_STA);
  esp_wifi_get_mac(WIFI_IF_STA, mac_address);

  // Initialize Bluetooth Server
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new BluetoothServerCallbacks());
  bleService = bleServer->createService(uuid_service);
  bleCharacteristic = bleService->createCharacteristic(uuid_characteristic, BLECharacteristic::PROPERTY_READ);

  // Initialize Bluetooth Client
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new BluetoothClientCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // Keep looping until a bluetooth connection is formed or an error occurs
  while(!is_bt_connected && !is_error) {
    // Attempt to masquerade as a server
    if(is_server) {
      bleService->start();
      bleAdvertising = BLEDevice::getAdvertising();
      BLEDevice::startAdvertising();
      delay(delay_length);
      bleService->stop();
    } else {
      clientConnected = pBLEClient->connect(&_advertisedDevice);
      if (!clientConnected) {
        continue;
      }
      pRemoteService = pBLEClient->getService(uuid_service);
      if (pRemoteService == nullptr) {
        continue;
      }
      pRemoteCharacteristic = pRemoteService->getCharacteristic(uuid_characteristic);
      
      if (pRemoteCharacteristic == nullptr) {
        continue;
      }
      server_mac_address = { 0 };
      const char* rawData = pRemoteCharacteristic->readValue().c_str();
      sprintf(server_mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5]);
      // If we made it this far, I guess we're connected now, yay!
      is_bt_connected = true;
    }
    next_discovery();
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
}

void loop() {

}