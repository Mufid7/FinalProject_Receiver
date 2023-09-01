/*********
  Modified from the examples of the Arduino LoRa library
  More resources: https://randomnerdtutorials.com
*********/
#include <SoftwareSerial.h>
#include <BluetoothSerial.h>
#include <SPI.h>
#include <LoRa.h>
BluetoothSerial bt;

// Define the pins used by the transceiver module
#define ss 5
#define rst 14
#define dio0 2
// #define buzzer 12

int counter = 0;

void setup() {
  //inisialisation
  bt.begin(9600);
  //initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("LoRa Receiver");

  //Set Pin Lora
  LoRa.setPins(ss, rst, dio0);

  ////Set Frekuensi Lora 915E6
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  // pinMode(buzzer, OUTPUT);
  // digitalWrite(buzzer, HIGH);
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String receivedData = "";

    while (LoRa.available()) {
      char c = LoRa.read();
      receivedData += c;
    }

    // Memisahkan data menjadi tiga string berdasarkan karakter pemisah (,)
    int separatorIndex1 = receivedData.indexOf(',');
    int separatorIndex2 = receivedData.indexOf(',', separatorIndex1 + 1);


    if (separatorIndex1 != -1 && separatorIndex2 != -1) {
      String data1 = receivedData.substring(0, separatorIndex1);
      String data2 = receivedData.substring(separatorIndex1 + 1, separatorIndex2);
      String data3 = receivedData.substring(separatorIndex2 + 1);

      // Mencetak tiga string yang diterima
      Serial.println("Data 1: " + data1);
      Serial.println("Data 2: " + data2);
      Serial.println("Data 3: " + data3);

    // Kirim Data Lewat Bluetooth
      bt.print(data1);
      bt.print(";");
      bt.print(data2);
      bt.print(";");
      bt.print(0);
      bt.println(";");
    }
  }
}