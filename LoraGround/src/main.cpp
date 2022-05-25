#include <SPI.h>
#include <RH_RF95.h>
#include <RH_Serial.h>
#include <ESP8266WiFi.h>

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

const char* ssid = "YourWIFISSID";
const char* password = "YourWIFIPassword";
const int networkport = 23;

WiFiServer localServer(networkport);
WiFiClient localClient;

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

  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }
  
  // Start UART and the server
  localServer.begin();
  localServer.setNoDelay(true);

  Serial.print("Ready! Use 'Uart-WiFi Bridge ");
  Serial.print(WiFi.localIP());
  Serial.println(" to connect");

}

uint8_t buf[RH_SERIAL_MAX_MESSAGE_LEN];
void loop()
{
   // Check if there are any new clients
  if (localServer.hasClient()){
    if (!localClient.connected()){
      if(localClient) localClient.stop();
      localClient = localServer.available();
      Serial.println("Client available");  
    }
  }
  else
  {
    Serial.println("Client not available"); 
  }

  // Check a client for data
  if (localClient && localClient.connected()){
    if(localClient.available()){
      Serial.println("Client data available ");
      size_t len = localClient.available();
      localClient.readBytes(buf, len);
      serial.send(buf, len);      
    }
  }
  else
  {
    Serial.println("Client data not available"); 
  }

  // Check UART for data
  if(serial.available()){
    Serial.println("Serial available ");
    uint8_t len = Serial.available();
    serial.recv(buf,&len);
    rf95.setHeaderTo(serial.headerTo());
    rf95.setHeaderFrom(serial.headerFrom());
    rf95.setHeaderId(serial.headerId());
    rf95.setHeaderFlags(serial.headerFlags(), 0xFF); // Must clear all flags
    Serial.println("Sending serial to client");
    rf95.send(buf, len);

    if (localClient && localClient.connected()){
      localClient.write(buf, len);
    }
    else
    {
      Serial.println("Serial was sent to Lora but not wifi client");
    }
  }
  else
  {
    Serial.println("Serial not available"); 
  }

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

}
