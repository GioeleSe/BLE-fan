#include <NimBLEDevice.h>

#define SERVICE_UUID "4fa4c201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define FAN_PWM_PIN A0
#define FAN_RELAY_PIN A1
#define PWM_FREQ 25000
#define PWM_RES 8

void stopFanSystem()
{
    ledcWrite(FAN_PWM_PIN, 0);
    digitalWrite(FAN_RELAY_PIN, LOW);
    Serial.println("System Safe: Fan Stopped");
}

class MyServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        Serial.println("Client connected");
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        Serial.println("Client disconnected");
        stopFanSystem();
        NimBLEDevice::startAdvertising();
    }
};

class MyCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        NimBLEAttValue value = pCharacteristic->getValue();

        if (value.length() > 0)
        {
            String inString = String(value.c_str());
            inString.trim();

            int percentage = inString.toInt();
            Serial.printf("Command received: %d%%\n", percentage);

            if (percentage > 0 && percentage <= 100)
            {
                int targetPWM = map(percentage, 1, 100, 3, 255);

                digitalWrite(FAN_RELAY_PIN, HIGH);
                ledcWrite(FAN_PWM_PIN, targetPWM);
            }
            else
            {
                stopFanSystem();
            }
        }
    }
};

static MyServerCallbacks serverCallbacks;
static MyCharacteristicCallbacks characteristicCallbacks;

void setup()
{
    Serial.begin(115200);

    pinMode(FAN_PWM_PIN, OUTPUT);
    pinMode(FAN_RELAY_PIN, OUTPUT);
    ledcAttach(FAN_PWM_PIN, PWM_FREQ, PWM_RES);

    stopFanSystem();

    NimBLEDevice::init("ESP32_Fan_Controller");
    NimBLEServer *pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(&serverCallbacks);
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        NIMBLE_PROPERTY::WRITE);
    pCharacteristic->setCallbacks(&characteristicCallbacks);

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setName("ESP32_Fan_Controller");
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->enableScanResponse(true);
    NimBLEDevice::startAdvertising();
    Serial.println("Fan Controller Ready.");
}

void loop()
{
    delay(5000);
}