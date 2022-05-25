#include <SPI.h>
#include <RH_RF95.h>
#include <RH_Serial.h>

#define RFM95_CS  2    // "E"
#define RFM95_RST 16   // "D"
#define RFM95_INT 15   // "B"

// Lora AIR Unit
// Singleton instance of the serial driver which relays all messages
// via Serial to another RadioHead RH_Serial driver, perhaps on a Unix host.
RH_Serial serial(Serial);

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 868.0
// Default to low baudrate for Lora
#define BAUDRATE 9600

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(BAUDRATE);

 while (!Serial) {
    delay(1);
  }

   // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

 while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("SetFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); 
  Serial.println(RF95_FREQ);
  
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  rf95.setPromiscuous(true);
  
  if (!serial.init())
    Serial.println("Serial init failed");  
  serial.setPromiscuous(true);
}

uint8_t buf[RH_SERIAL_MAX_MESSAGE_LEN];
void loop()
{
  if (rf95.available())
  {    
    Serial.println("Radio available");
    uint8_t len = sizeof(buf);
    rf95.recv(buf, &len);
    serial.setHeaderTo(rf95.headerTo());
    serial.setHeaderFrom(rf95.headerFrom());
    serial.setHeaderId(rf95.headerId());
    serial.setHeaderFlags(rf95.headerFlags(), 0xFF); // Must clear all flags
    serial.send(buf, len);
  }
  else
  {
     Serial.println("Radio not available");
  }

  if (serial.available())
  {
    Serial.println("Serial available");
    uint8_t len = sizeof(buf);
    serial.recv(buf, &len);
    rf95.setHeaderTo(serial.headerTo());
    rf95.setHeaderFrom(serial.headerFrom());
    rf95.setHeaderId(serial.headerId());
    rf95.setHeaderFlags(serial.headerFlags(), 0xFF); // Must clear all flags
    Serial.println("Sending serial to client");
    rf95.send(buf, len);
  }
  else
  {
      Serial.println("Serial not available");
  }
}
