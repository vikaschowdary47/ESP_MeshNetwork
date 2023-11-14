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

int isMACInList(const uint8_t targetAddr[6])
{
  peerNode *current = peerList;
  peerNode *prev = NULL;
  while (current != NULL)
  {
    if (memcmp(current->addr, targetAddr, 6) == 0)
    {
      // MAC address found in the list
      Serial.print("MAC Address found in the LL ");
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
  // MAC address not found in the list
  Serial.print("MAC Address NOT found in the LL ");
  return 0;
}

void broadcast(const String &message)
{
  // this will Broadcast a message to everyone in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Serial.print(peerInfo);
  // Serial.print(peerInfo);
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
  // and this will send a message to a specific device
  /*uint8_t peerAddress[] = {0x3C, 0x71, 0xBF, 0x47, 0xA5, 0xC0};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, peerAddress, 6);
  if (!esp_now_is_peer_exist(peerAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  esp_err_t result = esp_now_send(peerAddress, (const uint8_t *)message.c_str(), message.length());*/
  if (result == ESP_OK)
  {
    Serial.println("Broadcast message success");
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
void dumpPeers(peerNode *cur)
{

  // Recursively print peers to Serial
}

// Callback function for send process. Prints message success/failure message.
void OnDataSent(const uint8_t *macAddr, esp_now_send_status_t status)
{
  char macStr[18];
  formatMACAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  // M5.Lcd.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  // Print message on send
}

// Callback function for receive process. Records peer if new. Responds on "Hello" messages.
void OnDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
  // Serial.println(macAddr);
  isMACInList(macAddr);
  printf("MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
         macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
  Serial.println("receiveCallback running");
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, len);
  strncpy(buffer, (const char *)incomingData, msgLen);
  buffer[msgLen] = 0;
  char macStr[18];
  formatMACAddress(macAddr, macStr, 18);
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);

  // Process all incoming messages
}

String msg = "Hello";
void setup()
{

  // Set up Serial Monitor
  Serial.begin(115200);
  delay(1000);
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

  // Broadcast 5 "Hello" messages
}

int initialMsgCount = 0;
void initialSend()
{
  while (initialMsgCount <= 5)
  {
    Serial.print("initialMsgCount : ");
    Serial.print(initialMsgCount);
    broadcast(msg);
    initialMsgCount++;
    delay(1000);
  }
}
void loop()
{
  initialSend();
  // Serial.println("working");
  // delay(3000);
  // Do Nothing
}

// bool buttonDown = false;
// bool ledOn = false;

// void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// {
//   snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
// }

// void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
// {
//   Serial.println("receiveCallback running");
//   // only allow a maximum of 250 characters in the message + a null terminating byte
//   char buffer[ESP_NOW_MAX_DATA_LEN + 1];
//   int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
//   strncpy(buffer, (const char *)data, msgLen);
//   // make sure we are null terminated
//   buffer[msgLen] = 0;
//   // format the mac address
//   char macStr[18];
//   formatMacAddress(macAddr, macStr, 18);
//   // debug log the message to the serial port
//   Serial.printf("Received message from: %s - %s\n", macStr, buffer);
//   M5.Lcd.println("Received message");
//   // M5.Lcd.println(macAddr);
// }

// // callback when data is sent
// void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// {
//   char macStr[18];
//   formatMacAddress(macAddr, macStr, 18);
//   Serial.print("Last Packet Sent to: ");
//   Serial.println(macStr);
//   Serial.print("Last Packet Send Status: ");
//   // M5.Lcd.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
//   Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
//   M5.Lcd.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void broadcast(const String &message)
// {
//   // this will Broadcast a message to everyone in range
//   uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//   esp_now_peer_info_t peerInfo = {};
//   memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
//   if (!esp_now_is_peer_exist(broadcastAddress))
//   {
//     esp_now_add_peer(&peerInfo);
//   }
//   // Serial.print(peerInfo);
//   // Serial.print(peerInfo);
//   esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
//   // and this will send a message to a specific device
//   /*uint8_t peerAddress[] = {0x3C, 0x71, 0xBF, 0x47, 0xA5, 0xC0};
//   esp_now_peer_info_t peerInfo = {};
//   memcpy(&peerInfo.peer_addr, peerAddress, 6);
//   if (!esp_now_is_peer_exist(peerAddress))
//   {
//     esp_now_add_peer(&peerInfo);
//   }
//   esp_err_t result = esp_now_send(peerAddress, (const uint8_t *)message.c_str(), message.length());*/
//   if (result == ESP_OK)
//   {
//     Serial.println("Broadcast message success");
//   }
//   else if (result == ESP_ERR_ESPNOW_NOT_INIT)
//   {
//     Serial.println("ESPNOW not Init.");
//   }
//   else if (result == ESP_ERR_ESPNOW_ARG)
//   {
//     Serial.println("Invalid Argument");
//   }
//   else if (result == ESP_ERR_ESPNOW_INTERNAL)
//   {
//     Serial.println("Internal Error");
//   }
//   else if (result == ESP_ERR_ESPNOW_NO_MEM)
//   {
//     Serial.println("ESP_ERR_ESPNOW_NO_MEM");
//   }
//   else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
//   {
//     Serial.println("Peer not found.");
//   }
//   else
//   {
//     Serial.println("Unknown error");
//   }
// }

// void setup()
// {
//   M5.begin(true, false, true);
//   Serial.begin(9600);
//   delay(1000);
//   // Set device in STA mode to begin with
//   WiFi.mode(WIFI_STA);
//   Serial.println("ESPNow Example");
//   // Output my MAC address - useful for later
//   Serial.print("My MAC Address is: ");
//   Serial.println(WiFi.macAddress());
//   // M5.Lcd.print(WiFi.macAddress());
//   // shut down wifi
//   WiFi.disconnect();
//   // startup ESP Now
//   if (esp_now_init() == ESP_OK)
//   {
//     Serial.println("ESPNow Init Success");
//     esp_now_register_recv_cb(receiveCallback);
//     esp_now_register_send_cb(sentCallback);
//   }
//   else
//   {
//     Serial.println("ESPNow Init Failed");
//     delay(3000);
//     ESP.restart();
//   }
//   // use the built in button
//   // pinMode(0, INPUT_PULLUP);
//   // pinMode(2, OUTPUT);
// }

// void loop()
// {
//   broadcast("helloFromM5core2");
//   Serial.println("working");
//   Serial.println(millis());

//   // M5.Lcd.println("printing");
//   delay(10000);
//   // if (digitalRead(0))
//   // {
//   //   // detect the transition from low to high
//   //   if (!buttonDown)
//   //   {
//   //     buttonDown = true;
//   //     // toggle the LED state
//   //     ledOn = !ledOn;
//   //     digitalWrite(2, ledOn);
//   //     // send a message to everyone else
//   //     if (ledOn)
//   //     {
//   //       broadcast("on");
//   //     }
//   //     else
//   //     {
//   //       broadcast("off");
//   //     }
//   //   }
//   //   // delay to avoid bouncing
//   //   delay(500);
//   // }
//   // else
//   // {
//   //   // reset the button state
//   //   buttonDown = false;
//   // }
// }