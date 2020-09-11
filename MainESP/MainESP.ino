//GDanovski's tank code
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
///////////////////////////////////////////////////////////////
//BLE variables
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string output = ";";
String input = "";
String current = "";
int ind = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "b8c78f30-57ae-11ea-82b4-0242ac130003" 
#define CHARACTERISTIC_UUID "c1569f89-7936-4a43-9cf1-0f9690223454"
////////////////////////////////////////////////////////////////
//setup uART ports
#define RXD0 3
#define TXD0 1
#define RXD1 16
#define TXD1 17
////////////////////////////////////////////////////////////////////////////////////////////////////
//GPIO indexing
const int M1A = 32;
const int M1B = 33;
const int M2A = 25;
const int M2B = 26;

const int S1 = 27;
const int S2 = 14;
const int S3 = 12;
const int S4 = 13;

const int LED = 2;

const int RX1 = 3;
const int TX1 = 1;
const int RX2 = 16;
const int TX2 = 17;
//PWN channels
const int M1A_channel = 1;
const int M1B_channel = 2;
const int M2A_channel = 3;
const int M2B_channel = 4;

const int S1_channel = 5;
const int S2_channel = 6;
const int S3_channel = 7;
const int S4_channel = 8;
   
// setting PWM properties
const int Servo_freq = 50;
const int Motor_freq = 500;    
const int resolution = 10;//max 1024
//Borders
const int M_min = 0;
const int M_max = 600;
//Big tank
/*
const int S1_min = 30;
const int S2_min = 75;
const int S3_min = 60;
const int S4_min = 30;

const int S1_max = 125;
const int S2_max = 120;
const int S3_max = 110;
const int S4_max = 100;
*/
//Small Tank

const int S1_min = 30;
const int S2_min = 30;
const int S3_min = 30;
const int S4_min = 30;

const int S1_max = 125;
const int S2_max = 125;
const int S3_max = 125;
const int S4_max = 125;

//Variables
int MotorsStatus = 5;

double S1_const = 0;
double S2_const = 0;
double S3_const = 0;
double S4_const = 0;

int S1_value = 0;
int S2_value = 0;
int S3_value = 0;
int S4_value = 0;

int M_speed = 0;
///////////////////////////////////////////////////////////
//My methods
//Functions
void ConfigChannels(){
  // initialize digital pin LED as an output.
  Serial1.println("Starting pin configuration...");
  pinMode(LED, OUTPUT);    
  digitalWrite(LED, LOW); 
  // configure LED PWM functionalitites
  pinMode(M1A, OUTPUT);
  pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT);
  pinMode(M2B, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(S4, OUTPUT);
  
  ledcSetup(M1A_channel, Motor_freq, resolution);
  ledcSetup(M1B_channel, Motor_freq, resolution);
  ledcSetup(M2A_channel, Motor_freq, resolution);
  ledcSetup(M2B_channel, Motor_freq, resolution);

  ledcSetup(S1_channel, Servo_freq, resolution);
  ledcSetup(S2_channel, Servo_freq, resolution);
  ledcSetup(S3_channel, Servo_freq, resolution);
  ledcSetup(S4_channel, Servo_freq, resolution);
  
   // attach the channel to the GPIO to be controlled
   ledcAttachPin(M1A, M1A_channel);
   ledcAttachPin(M1B, M1B_channel);
   ledcAttachPin(M2A, M2A_channel);
   ledcAttachPin(M2B, M2B_channel);

   ledcAttachPin(S1, S1_channel);
   ledcAttachPin(S2, S2_channel);
   ledcAttachPin(S3, S3_channel);
   ledcAttachPin(S4, S4_channel);

   //setup initial values
   ledcWrite(M1A_channel,  M_min);
   ledcWrite(M1B_channel,  M_min);
   ledcWrite(M2A_channel,  M_min);
   ledcWrite(M2B_channel,  M_min);

   ledcWrite(S1_channel,  S1_value);
   ledcWrite(S2_channel,  S2_value);
   ledcWrite(S3_channel,  S3_value);
   ledcWrite(S4_channel,  S4_value);

   S1_const = ((double)(S1_max-S1_min))/100.0;
   S2_const = ((double)(S2_max-S2_min))/100.0;
   S3_const = ((double)(S3_max-S3_min))/100.0;
   S4_const = ((double)(S4_max-S4_min))/100.0;
}
void DecodeInput(){
  Serial1.println("Recieved: " + input);
  if(!CheckSizeOfInput()){ 
    Serial1.println("Error: decoding! " + input);   
    return;
  }
  // MotorsSpeed | MotorsStatus | Servo1 | Servo2 | Servo3|Servo4|
  //reset
  current = "";
  ind = 0;
  int curValue = 0;
  //decode values
   if (input.length() > 0) {        
      for (int i = 0; i < input.length(); i++){
        if(input[i]=='|'){
          curValue = current.toInt();
          current = "";
          switch(ind){
            case 0:
            //MotorSpeed
              if(M_speed!=curValue){
                M_speed = curValue;
                if(M_speed>M_max){
                  M_speed = M_max;
                }
               // SetMotors();
              }
              break;
            case 1:
            //MotorsStatus
              if(MotorsStatus!=curValue){
                MotorsStatus = curValue;
                //SetMotors();
                SetmotorsGently();
              }
              break;
            case 2:
              //S1
              curValue = S1_const*curValue + S1_min;
              
              if(curValue != S1_value){
              if(curValue<S1_min){
                S1_value=S1_min;
              }
              else if(curValue>S1_max){
                S1_value=S1_max;
              }
              else{
                S1_value=curValue;
              }
              ledcWrite(S1_channel,  S1_value);
              }
              break;
            case 3:
              //S2
              curValue = S2_const*curValue + S2_min;
              
              if(curValue != S2_value){
              if(curValue<S2_min){
                S2_value=S2_min;
              }
              else if(curValue>S2_max){
                S2_value=S2_max;
              }
              else{
                S2_value=curValue;
              }
              ledcWrite(S2_channel,  S2_value);
              }
              break;
            case 4:
              //S3
              curValue = S3_const*curValue + S3_min;
              
              if(curValue != S3_value){
              if(curValue<S3_min){
                S3_value=S3_min;
              }
              else if(curValue>S3_max){
                S3_value=S3_max;
              }
              else{
                S3_value=curValue;
              }
              ledcWrite(S3_channel,  S3_value);
              }
              break;
            case 5:
              //S4     
              curValue = S4_const*curValue + S4_min;
              if(curValue != S4_value ){         
              if(curValue<S4_min){
                S4_value=S4_min;
              }
              else if(curValue>S4_max){
                S4_value=S4_max;
              }
              else{
                S4_value=curValue;
              }
              ledcWrite(S4_channel,  S4_value);
              }
              break;
          }
          ind++;
        }
        else{  
          current+=input[i]; 
        }
      }
   }
   //clear
   current = "";
    ind = 0;
}
void SetmotorsGently(){
  if(M_speed == M_max){
    for(M_speed = 150; M_speed<=450; M_speed+=10){
      SetMotors();
      delay(40);
    }
  }
  else if(M_speed == 0){
    SetMotors();
  }
  else{
    for(M_speed = 150; M_speed<=250; M_speed+=10){
      SetMotors();
      delay(30);
    }
  }
}
void SetMotors(){
  switch(MotorsStatus){
    case 1:
      ledcWrite(M1A_channel,  M_min);
      delay(10);
      ledcWrite(M1B_channel,  M_speed);
      ledcWrite(M2A_channel,  M_min);
      ledcWrite(M2B_channel,  M_min);
      break;
    case 2:
      ledcWrite(M1A_channel,  M_min);
      delay(10);
      ledcWrite(M1B_channel,  M_speed);
      ledcWrite(M2A_channel,  M_min);
      delay(10);
      ledcWrite(M2B_channel,  M_speed);
      break;
    case 3:
      ledcWrite(M1A_channel,  M_min);
      ledcWrite(M1B_channel,  M_min);
      ledcWrite(M2A_channel,  M_min);
      delay(10);
      ledcWrite(M2B_channel,  M_speed);
      break;
    case 4:
      ledcWrite(M1A_channel,  M_min);
      delay(10);
      ledcWrite(M1B_channel,  M_speed);
      ledcWrite(M2B_channel,  M_min);
      delay(10);
      ledcWrite(M2A_channel,  M_speed);
      break;
    case 5:
      ledcWrite(M1A_channel,  M_min);
      ledcWrite(M1B_channel,  M_min);
      ledcWrite(M2A_channel,  M_min);
      ledcWrite(M2B_channel,  M_min);
      break;
    case 6:
      ledcWrite(M1B_channel,  M_min);
      delay(10);
      ledcWrite(M1A_channel,  M_speed);
      ledcWrite(M2A_channel,  M_min);
      delay(10);
      ledcWrite(M2B_channel,  M_speed);
      break;
    case 7:
      ledcWrite(M1B_channel,  M_min);
      delay(10);
      ledcWrite(M1A_channel,  M_speed);
      ledcWrite(M2A_channel,  M_min);
      ledcWrite(M2B_channel,  M_min);
      break;
    case 8:
      ledcWrite(M1B_channel,  M_min);
      delay(10);
      ledcWrite(M1A_channel,  M_speed);
      ledcWrite(M2B_channel,  M_min);
      delay(10);
      ledcWrite(M2A_channel,  M_speed);
      break;
    case 9:
      ledcWrite(M1A_channel,  M_min);
      ledcWrite(M1B_channel,  M_min);
      ledcWrite(M2B_channel,  M_min);
      delay(10);
      ledcWrite(M2A_channel,  M_speed);
      break;
  }
}
bool CheckSizeOfInput(){

if (input.length() == 0){return false;}
int count = 0;
 for (int i = 0; i < input.length(); i++){
        if(input[i] == '|'){ count++; }      
   }
   if(count == 6){
    return true;
   }
   else{
    return false;
   }
}
void StopMotors(){
  MotorsStatus = 5;
  ledcWrite(M1A_channel,  M_min);
  ledcWrite(M1B_channel,  M_min);
  ledcWrite(M2A_channel,  M_min);
  ledcWrite(M2B_channel,  M_min);
}
////////////////////////////////////////////////////////////////
//BLE Methods
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
      for (int i = 0; i < value.length(); i++){
         if(value[i] == ';'){     
          DecodeInput();
          //Serial.println("Recieved: " + input);
          //Serial1.println("Recieved: " + input);
          input = "";
         }
         else{
          input+=value[i];          
         }
      }
    }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      digitalWrite(LED, HIGH);
      
      Serial1.println("Connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      digitalWrite(LED, LOW);
      
      Serial1.println("Disconnected");
    }
};
void BLEServerSetUp(){
  Serial1.println("Starting BLE server!");
  // Create the BLE Device
  BLEDevice::init("GDanovski's tank");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Welcome, I'm ready to rumble!");

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial1.println("Waiting a client connection to notify...");
}
/////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial1.begin(115200, SERIAL_8N1, RXD0, TXD0);
  //Serial2.begin(115200, SERIAL_8N1, RXD1, TXD1);
  Serial1.println("Setup GDanovski's tank");
  //setup GPIO
  ConfigChannels();
  //setup server
  BLEServerSetUp();  
}

void loop() {
  //read uART1
 // while (Serial1.available()) {
  //  Serial.print(char(Serial1.read()));
  //}
  // notify changed value
  if (deviceConnected) {
      pCharacteristic->setValue(output);
      pCharacteristic->notify();
      delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
     delay(500); // give the bluetooth stack the chance to get things ready
     pServer->startAdvertising(); // restart advertising
     Serial1.println("start advertising");
     oldDeviceConnected = deviceConnected;
     StopMotors();
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
     // do stuff here on connecting
     oldDeviceConnected = deviceConnected;
  }
  delay(100);
}
