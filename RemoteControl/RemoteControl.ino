//GDanovski's tank remote

#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("b8c78f30-57ae-11ea-82b4-0242ac130003");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("c1569f89-7936-4a43-9cf1-0f9690223454");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
////////////////////////////////////////////////////////////////////////////////////////////////////
//setup uART ports
#define RXD0 3
#define TXD0 1
#define RXD1 16
#define TXD1 17
////////////////////////////////////////////////////////////////////////////////////////////////////
//my pins
const int S1 = 36;
const int S2 = 39;
const int S3 = 34;
const int S4 = 35;
const int M1 = 33;
const int M2 = 32;
const int LED = 2;
//values

const double S1_min = 30;
const double S2_min = 75;
const double S3_min = 60;
const double S4_min = 30;

const double S1_max = 125;
const double S2_max = 120;
const double S3_max = 110;
const double S4_max = 100;

double S1_Const = 4095;
double S2_Const = 4095;
double S3_Const = 4095;
double S4_Const = 4095;
double M_Const = 4095;

const double Potentiometer_Const = 4095;
//Variables
int MotorsStatus = 5;

int S1_value = 0;
int S2_value = 0;
int S3_value = 0;
int S4_value = 0;

int M_speed_value = 1024;
int M1_value = 0;
int M2_value = 0;
//temp variable
double currentValue = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////
//my methods
void Setup_Potentiometer(){
  S1_Const = (S1_max-S1_min)/Potentiometer_Const;
  S2_Const = (S2_max-S2_min)/Potentiometer_Const;
  S3_Const = (S3_max-S3_min)/Potentiometer_Const;
  S4_Const = (S4_max-S4_min)/Potentiometer_Const;
  
  //configurate pins
  pinMode (S1, INPUT); 
  pinMode (S2, INPUT); 
  pinMode (S3, INPUT); 
  pinMode (S4, INPUT); 
  pinMode (M1, INPUT); 
  pinMode (M2, INPUT);  
  
  pinMode (LED, OUTPUT);
  digitalWrite(LED, LOW);  
}
int ReadPotentiometer(int Pin, double Const,double Min, int OldValue){
  // Reading potentiometer value
  currentValue = analogRead(Pin);  
  currentValue = Min+(currentValue*Const);
  
  if(OldValue+2 < (int)currentValue || OldValue-2 > (int)currentValue){
    OldValue = (int)currentValue;    
  }
  
  return OldValue;
}
int CheckMotorIndex(int M_value){
  if(M_value<1650){
    return -1;
  }
  else if (M_value>2150){
    return 1;
  }
  else{
    return 0;
  }
}
void CalcMotorSpeed(){
  double mid = 2047;
  double M1_speed = abs(M1_value - mid);
  double M2_speed = abs(M2_value - mid);
  
  if(M1_speed<M2_speed)
  {
    M1_speed = M2_speed;
  }
  
  M1_speed = M1_speed/2;
  
  if(M1_speed<1000){
    M1_speed = 400;
  }
  
  if(M_speed_value+2 < (int)M1_speed || M_speed_value-2 > (int)M1_speed){
    M_speed_value = (int)M1_speed;    
  }
}
void MeasureValues(){
  //read analog values
  S1_value = ReadPotentiometer(S1, S1_Const, S1_min, S1_value);
  S2_value = ReadPotentiometer(S2, S2_Const, S2_min, S2_value);
  S3_value = ReadPotentiometer(S3, S3_Const, S3_min, S3_value);
  S4_value = ReadPotentiometer(S4, S4_Const, S4_min, S4_value);
 
  M1_value = analogRead(M1);
  M2_value = analogRead(M2);
  CalcMotorSpeed();
  //check motor status
  int M1_index = CheckMotorIndex(M1_value);
  int M2_index = CheckMotorIndex(M2_value);
  String M_string = String(String(M1_index)+"|"+String(M2_index));

  if (M_string == "-1|-1"){
    MotorsStatus = 1;
  }
  else if (M_string == "-1|0"){
    MotorsStatus = 2;
  }
  else if (M_string == "-1|1"){
    MotorsStatus = 3;
  }
  else if (M_string == "0|-1"){
    MotorsStatus = 6;
  }
  else if (M_string == "0|0"){
    MotorsStatus = 5;
  }
  else if (M_string ==  "0|1"){
    MotorsStatus = 4;
  }
  else if (M_string == "1|-1"){
    MotorsStatus = 7;
  }
  else if (M_string == "1|0"){
    MotorsStatus = 8;
  }
  else if (M_string ==  "1|1"){
    MotorsStatus = 9;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
//BLE methods
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {   
    Serial1.print("Notify callback for characteristic ");
    Serial1.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial1.print(" of data length ");
    Serial1.println(length);
    Serial1.print("data: ");
    Serial1.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    digitalWrite(LED, HIGH);
    Serial1.println("Connected");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    digitalWrite(LED, LOW);
    Serial1.println("Disconnected");
  }
};

bool connectToServer() {
    Serial1.print("Forming a connection to ");
    Serial1.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial1.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial1.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial1.print("Failed to find our service UUID: ");
      Serial1.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial1.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial1.print("Failed to find our characteristic UUID: ");
      Serial1.println(charUUID.toString().c_str());
      
      pClient->disconnect();
      return false;
    }
    Serial1.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial1.print("The characteristic value was: ");
      Serial1.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial1.print("BLE Advertised Device found: ");
    Serial1.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks
void SetUpBLEClient(){
  BLEDevice::init("GDanovski's tank remote");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial1.begin(115200, SERIAL_8N1, RXD0, TXD0);
  //Serial2.begin(115200, SERIAL_8N1, RXD1, TXD1);  
  Serial1.println("Starting Arduino BLE Client application...");
  //setup my pins
   Setup_Potentiometer();
  //setup BLE client
   SetUpBLEClient();
} // End of setup.

// This is the Arduino main loop function.
void loop() {
  //read uART1
  //while (Serial1.available()) {
    //Serial.print(char(Serial1.read()));
  //}
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial1.println("We are now connected to the BLE Server.");
    } else {
      Serial1.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    MeasureValues();
    delay(10);
    // MotorsSpeed | MotorsStatus | Servo1 | Servo2 | Servo3|Servo4|
    String newValue = "" + String(M_speed_value) + "|" + String(MotorsStatus) + "|" +String(S1_value)+ "|" + String(S2_value) + "|" + String(S3_value) + "|" + String(S4_value) + "|;";
    Serial1.println("Output: \"" + newValue + "\"");
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(100); // Delay between loops.
} // End of loop
