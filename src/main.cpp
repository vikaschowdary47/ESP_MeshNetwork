#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <esp_now.h>
// #include <M5Core2.h>

// #ifdef M5CORE2
// #include <M5Core2.h>
// #elif defined(XIAO_ESP32S3)
// #include <U8x8lib.h>
// #endif

// #if defined(XIAO_ESP32S3)
// #define PIN_WIRE_SCL D5
// #define PIN_WIRE_SDA D4
// U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* clock=*/PIN_WIRE_SCL, /* data=*/PIN_WIRE_SDA, /* reset=*/U8X8_PIN_NONE); // OLEDs without Reset of the Display
// #endif

#if defined(XIAO_ESP32S3)
const int buttonPin = 1; // the number of the pushbutton pin
int buttonState = 0;     // variable for reading the pushbutton status
#endif

void readUserButton()
{
  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH)
  {
    // turn LED on:
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("button clicked");
  }
  else
  {
    // turn LED off:
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("button not clicked");
  }
}
// Node in peer MAC linked list
typedef struct peerNode
{
  uint8_t addr[6];
  peerNode *next = NULL;
} peerNode;

// Head of peer linked list
peerNode *peerList;

// Function to convert uint8_t array MAC to string
void formatMACAddress(const uint8_t *macAddr, char *buffer, int maxLength)
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void dumpPeers(peerNode *cur)
{
  if (cur == NULL)
  {
    Serial.println("NO MAC address's in the LL");
    return;
  }

  // char macStr[18];
  // formatMACAddress(cur->addr, macStr, 18);
  Serial.print("MAC Addresses in the Linked List: ");
  // Serial.println(macStr);
  Serial.printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                cur->addr[0], cur->addr[1], cur->addr[2], cur->addr[3], cur->addr[4], cur->addr[5]);

  // Recursively call dumpPeers for the next node
  dumpPeers(cur->next);
}

void broadcast(const String &message, const uint8_t targetAddr[6])
{
  uint8_t destinationAddress[6];

  if (targetAddr != NULL)
  {
    // If targetAddr is provided, use it as the destination address
    memcpy(destinationAddress, targetAddr, 6);
  }
  else
  {
    // If targetAddr is NULL, use the default broadcast address
    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    memcpy(destinationAddress, broadcastAddress, 6);
  }
  // this will Broadcast a message to everyone in range
  // uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, destinationAddress, 6);
  if (!esp_now_is_peer_exist(destinationAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  esp_err_t result = esp_now_send(destinationAddress, (const uint8_t *)message.c_str(), message.length());
  if (result == ESP_OK)
  {
    Serial.print(" >> Broadcast message success || ");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESPNOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}
// Recursive function to print MAC addresses in linked list to the serial port

int isMACInList(const uint8_t targetAddr[6])
{
  peerNode *current = peerList;
  peerNode *prev = NULL;
  while (current != NULL)
  {
    if (memcmp(current->addr, targetAddr, 6) == 0)
    {
      // MAC address found in the list
      Serial.print("MAC Address found in the LL || ");
      return 1;
    }
    prev = current;
    current = current->next;
  }

  // MAC address not found in the list, add it to the list
  peerNode *newNode = (peerNode *)malloc(sizeof(peerNode));
  if (newNode == NULL)
  {
    // Handle memory allocation failure
    fprintf(stderr, "Error: Unable to allocate memory for a new node.\n");
    exit(EXIT_FAILURE);
  }
  // Copy the MAC address to the new node
  memcpy(newNode->addr, targetAddr, 6);
  newNode->next = NULL;

  // If the list is empty, set the new node as the head of the list
  if (prev == NULL)
  {
    peerList = newNode;
  }
  else
  {
    // Otherwise, add the new node to the end of the list
    prev->next = newNode;
  }
  broadcast("Welcome", targetAddr);
  dumpPeers(peerList);
  // uint8_t broadcastAddress[] = {0x30, 0xAE, 0xA4, 0x15, 0xC7, 0xFC};
  // MAC address not found in the list
  // macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]
  Serial.print("MAC Address NOT found in the LL ");
  return 0;
}

// Callback function for send process. Prints message success/failure message.
void OnDataSent(const uint8_t *macAddr, esp_now_send_status_t status)
{
  // Serial.println("xPortGetCoreID");
  // Serial.println(xPortGetCoreID());
  char macStr[18];
  formatMACAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  // Serial.print("Last Packet Send Status: ");
  // #if defined(XIAO_ESP32S3)
  //   u8x8.setCursor(0, 0);
  //   u8x8.print("Hello World!");
  // #endif
  // #ifdef M5CORE2
  //   // M5.Lcd.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  // #elif defined(XIAO_ESP32S3)
  //   u8x8.setCursor(0, 0);
  //   u8x8.print("Hello World!");
  // #endif

  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  // Print message on send
}

// Callback function for receive process. Records peer if new. Responds on "Hello" messages.
void OnDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
  // Serial.println(macAddr);
  isMACInList(macAddr);
  // printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
  //        macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  // Serial.println("receiveCallback running");
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, len);
  strncpy(buffer, (const char *)incomingData, msgLen);
  buffer[msgLen] = 0;
  char macStr[18];
  formatMACAddress(macAddr, macStr, 18);
  Serial.printf("Received message from: %s = %s\n", macStr, buffer);

  // Process all incoming messages
}

String msg = "Hello";

void setup()
{

  // Set up Serial Monitor
  Serial.begin(115200);
  // #if defined(XIAO_ESP32S3)
  //   u8x8.begin();
  //   u8x8.setCursor(0, 0);
  //   u8x8.print("Hello World!");
  // #endif
  delay(1000);
  // initialize the LED pin as an output:
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);
  // while (!Serial)
  //   ;
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }
  dumpPeers(peerList);
  // Broadcast 5 "Hello" messages
}

int initialMsgCount = 0;
void initialSend()
{
  while (initialMsgCount <= 5)
  {
    Serial.print("initialMsgCount : ");
    Serial.print(initialMsgCount);
    broadcast(msg, NULL);
    initialMsgCount++;
    delay(1000);
  }
}
void loop()
{
  initialSend();
  //   #if defined(XIAO_ESP32S3)
  //  readUserButton();   // variable for reading the pushbutton status
  // #endif

  // Do Nothing
}
