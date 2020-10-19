#include <WiFi.h>
#include <WiFiClient.h>
#include <NeoPixelBus.h>
#include <WebSocketsServer.h>

const char *ssid = "xxx";
const char *password = "xxx";

WebSocketsServer webSocket = WebSocketsServer(1337);

// check your own pins!
#define PIN 25
#define PIN2 22

const int LEDS = 60;
const int STRIPCOUNT = 2;

NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Ws2812xMethod> strip(LEDS, PIN);
NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt1Ws2812xMethod> strip2(LEDS, PIN2);
 
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) 
{
  int counter = 0;
  bool drawPixels = false;
  int offset = 0;
  int i;

  bool updateStrip1 = false;
  bool updateStrip2 = false;
      
  // Figure out the type of WebSocket event
  switch(type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;

    // here to write the pixels
    case WStype_BIN:

      // used for scanning the received data
      i = 0;
      
      while(i < length)
      {
        // when the counter == 0, we expect a new block.
        if(counter == 0)
        {
          // highest bit is used to check if the block contains pixels or we should skip some.
          drawPixels = (payload[i] & 128) == 128;
          counter = payload[i] & 127;
          i++;
        }
        else
        {
          // do we expect pixels?, write them increase the datapointer.
          if(drawPixels)
          {
            // cheap-ass check which strip the pixel should be set.
            if(offset<LEDS)
            {
              strip.SetPixelColor(offset, RgbwColor(payload[i],payload[i+1],payload[i+2],payload[i+3]));
              // the boolean is used to check if the strip needs to be updated
              updateStrip1 = true;
            }
            else
            {
              strip2.SetPixelColor(offset-LEDS, RgbwColor(payload[i],payload[i+1],payload[i+2],payload[i+3]));
              updateStrip2 = true;
            }
  
            i+=4;
          }
          offset++;
          counter--;
        }
        
        if(offset>=STRIPCOUNT*LEDS)
          break;
      }

      // is pixels are changed, update them
      if(updateStrip1) strip.Show();
      if(updateStrip2) strip2.Show();
  
      break;

    case WStype_TEXT:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void setup(void) 
{
  Serial.begin(115200);

  Serial.println("Init first strip");
  strip.Begin();
  strip.ClearTo(RgbColor(0, 0, 0));
  strip.Show(); // Initialize all pixels to 'off'

  Serial.println("Init second strip");
  strip2.Begin();
  strip2.ClearTo(RgbColor(0, 0, 0));
  strip2.Show(); // Initialize all pixels to 'off'

  
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(ssid, password);
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  
  Serial.println("WebSocket server started");
}

void loop(void) 
{
  webSocket.loop();
}
