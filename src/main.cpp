#include "main.h"

void setup() {
  // Set the RTC watchdog timer timeout to 10 seconds
  esp_task_wdt_init(10, true);
  Serial.println("Starting GameSword...");
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
  // Keeps scanning until a bluetooth connection is formed or an error occurs.
  if (!bt.is_bt_connected && !bt.is_error) {
    // Does a 5 second scan for a server.
    btClient.pBLEScan->start(5);
    Serial.println("Scanning...");
    delay(5000);
    btClient.pBLEScan->stop();
    
    // If we found a server, attempt to connect.
    if (!btClient.pBLEClient->connect(&btClient.advertisedDevice)) {
      return;
    }
    // Collects the service and characteristic from the server.
    btClient.pRemoteService = btClient.pBLEClient->getService(UUID_SERVICE);
    if (!(btClient.pRemoteService = 
          btClient.pBLEClient->getService(UUID_SERVICE))) {
      return;
    }
    // Collects the characteristic from the server.
    if (!(btClient.pRemoteCharacteristic = 
        btClient.pRemoteService->getCharacteristic(UUID_CHARACTERISTIC))) {
      return;
    }
    // Reads the sent mac address from the server.
    const uint8_t* rawData = (uint8_t*) btClient.pRemoteCharacteristic->readValue().c_str();
    sprintf(bt.server_mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", rawData[0], rawData[1], rawData[2], rawData[3], rawData[4], rawData[5]);
    // If we made it here, we are connected.
    bt.is_bt_connected = true;
  // If we are connected, keep printing so until it's false.
  } else if (bt.is_bt_connected && !bt.is_error) {
    Serial.println("Connected to Server");
  }
}