#include <Wire.h>                     // enable I2C.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
const char* ssid = "place WLAN name here";
const char* password = "place WLAN pw here";
const char* mqtt_server = "place ip adress of mqtt server here";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

int value = 0;
char Topic[32];

char sensordata[30];                  // A 30 byte character array to hold incoming data from the sensors
byte sensor_bytes_received = 0;       // We need to know how many characters bytes have been received

byte code = 0;                        // used to hold the I2C response code.
byte in_char = 0;                     // used as a 1 byte buffer to store in bound bytes from the I2C Circuit.

#define TOTAL_CIRCUITS 4              // <-- CHANGE THIS | set how many I2C circuits are attached to the Tentacle shield(s): 1-8

int channel_ids[] = { 20,99,102, 100};// <-- CHANGE THIS.
// A list of I2C ids that you set your circuits to.
// This array should have 1-8 elements (1-8 circuits connected)

char *channel_names[] = { "RTD", "PH", "RTD2","EC"}; // <-- CHANGE THIS.
// A list of channel names (must be the same order as in channel_ids[]) 
// it's used to give a name to each sensor ID. This array should have 1-8 elements (1-8 circuits connected).
// {"PH Tank 1", "PH Tank 2", "EC Tank 1", "EC Tank2"}, or {"PH"}



void setup() {           
  Serial.begin(115200);              
  Wire.begin();                      // enable I2C port.
  pinMode(BUILTIN_LED, OUTPUT);    
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void setup_wifi() {

  delay(100);
  // Start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  } 
  Serial.println();
}


void loop() {
  // Loop through all sensors, read the values.
  // Then send the value and sensor name to the server via mqtt.
  for (int channel = 0; channel < TOTAL_CIRCUITS; channel++) {  
  
    Wire.beginTransmission(channel_ids[channel]);     // call the circuit by its ID number.
    Wire.write('r');                                  // request a reading by sending 'r'
    Wire.endTransmission();                           // end the I2C data transmission.
    
    delay(5000);  // AS circuits need a 5 second before the reading is ready

    sensor_bytes_received = 0;                        // reset data counter
    memset(sensordata, 0, sizeof(sensordata));        // clear sensordata array;

    Wire.requestFrom(channel_ids[channel], 48, 1);    // call the circuit and request 48 bytes (this is more then we need).
    code = Wire.read();

    while (Wire.available()) {          // are there bytes to receive?
      in_char = Wire.read();            // receive a byte.

      if (in_char == 0) {               // null character indicates end of command
        Wire.endTransmission();         // end the I2C data transmission.
        break;                          // exit the while loop, we're done here
      }
      else {
        sensordata[sensor_bytes_received] = in_char;      // append this byte to the sensor data array.
        sensor_bytes_received++;
      }
    }
    
    switch (code) {                       // switch case based on what the response code is.
      case 1:  
        snprintf (msg, 75, "%s",sensordata);
        snprintf (Topic,32,"Topic/%s",channel_names[channel]);
        Serial.print (Topic);
        Serial.print ('\n');
        Serial.print(channel_names[channel]);   // print channel name
        Serial.print(':');// decimal 1  means the command was successful.
        Serial.println(sensordata);       // print the actual reading
        client.publish(Topic,msg);        // publish sensordata as string with the topic (channelname)
        break;                              // exits the switch case.

      case 2:                             // decimal 2 means the command has failed.
        Serial.println("command failed");   // print the error
        break;                              // exits the switch case.

      case 254:                           // decimal 254  means the command has not yet been finished calculating.
        Serial.println("circuit not ready"); // print the error
        break;                              // exits the switch case.

      case 255:                           // decimal 255 means there is no further data to send.
        Serial.println("no data");          // print the error
        break;                              // exits the switch case.
    } 

  }

  
  // for loop 
  if (!client.connected()) {
    reconnect();
  }
 
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
  }
}

  
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Publisher")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", sensordata,true);
      // ... and resubscribe
     // client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    
  }
  
}
