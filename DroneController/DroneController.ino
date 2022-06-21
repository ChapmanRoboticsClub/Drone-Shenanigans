#include <WiFi101.h>
#include <WiFiUDP.h>

WiFiClient client;
WiFiUDP droneClient;

char ssid[] = "Drone Shenanigans";        // your network SSID (name)
char pass[] = "NotDroneShenanigans";    // your network password (use for WPA, or use as key for WEP)

char telloID[14] = "TELLO-??????";

// targetIP is ddd.ddd.ddd.ddd
IPAddress targetIP(192, 168, 103, 2);
char message[100];


void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT); // Debug Output

  WiFi.setPins(8, 7, 4, 2);

  Serial.begin(9600);
  // attempt to connect to WiFi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to connect");
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);
  }
  Serial.println("Connected to wifi");

  if (client.connect(targetIP, 9090)) {
    Serial.println("connected to server");
    // Say I'm a Drone (Controller):
    client.print("D");
  }

  delay(100); // Delaying so that telloID can be sent through

  int len = client.read((uint8_t*) telloID, 13);
  Serial.printf("Len: %d\nTello ID: %s\n", len, telloID);

  // Failed tello connect; works if you don't connect to the main network ahead of time but not if you do.
  
  //  delay(500);
  //
  //  WiFi.disconnect();
  //
  //  delay(3000);
  
  //  WiFi.begin(telloID);
  //  Serial.print("SSID: ");
  //  Serial.println(WiFi.SSID());
  //  droneClient.begin(2390);
  //  // Enter command mode
  //  droneClient.beginPacket("192.168.10.1", 8889);
  //  droneClient.write("command");
  //  droneClient.endPacket();
  //  delay(500);
  //  Serial.println("Command sent");
  //  droneClient.beginPacket("192.168.10.1", 8889);
  //  droneClient.write("takeoff");
  //  droneClient.endPacket();
  //  Serial.println("Takeoff sent");
  //  delay(500);
  //
  //  WiFi.begin(ssid, pass);
  //  delay(500);

}

void loop() {
  // put your main code here, to run repeatedly:
  // Message Send: client.printf("%d%d%d", b1, b2, b3);
  // NOTE: Only receive one character at a time?  False
  
  int len = client.read((uint8_t*) message, 100);
  if (len > 0) {
    Serial.printf("Read Message: %s\n", message);
    if(strcmp(message, "takeoff") == 0) {
      // Takeoff!
      Serial.println("Takeoff!");
    } else if (strcmp(message, "somethi") == 0) {
      // Somethi!
      Serial.println("Somethi!");
    } else if (strcmp(message, "rotate") == 0) {
      Serial.println("Rotate!");
    }
  }
}
