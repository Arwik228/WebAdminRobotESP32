#include "string.h"

#include "WiFi.h"

#include "SPIFFS.h"

#include "AsyncTCP.h"

#include "esp32-hal-ledc.h"

#include "ESPAsyncWebServer.h"

/*
   Set a username and password for your home network.
   If everything is correct, the site will be available at 192.168.1.199
*/
const char* ssid = "ASUS";
const char* password = "123456789";
/*
    Set logical pins that will be responsible for one direction or another for
   each of the wheelsю
*/
#define leftWheelForwardPIN 27
#define rightWheelForwardPIN 12
#define leftWheelBackwardPIN 14
#define rightWheelBackwardPIN 26
/*
    If the body of the machine was assembled incorrectly and it drifts to one
   side.
    It is necessary to accelerate the wheel of the side to which it is moving.
    The motor speed scale is 0/255, the base machine speed is 100 for 5 volts,
   so the speed of the skid side will be higher.
    Note: Only 1 value out of 2 is allowed. Acceleration is matched to your
   voltage and base speed
*/
#define randomTurnLeft 25 //%
#define randomTurnRight 0 //%
/*
   Install the pins of the collision sensors, the code is for 3 sensors.
   left, center, right sensors
*/
#define laserSensorLeftPIN 16
#define laserSensorCenterPIN 17
#define laserSensorRightPIN 5
/*
   Default speed 0-255, 150/180
   Const 8 holes whell sensor
   Wheel Circumference
   Pin counter rotate L, R
*/
#define defaultSpeed 80
#define whellSensor 16
#define wheelCircumference 20.4 // cm
#define countRotationLeftPIN 18
#define countRotationRightPIN 19
/*
    If you have a buzzer with an installed generator, then use this pin.
    Else set this variable to false
*/
#define speakerPIN 23
/*
   Some esp32 boards have touch button outputs.
   You can wake up from sleep mode by clicking on it
   Else set this variable to false
   D4  = T0 | D2  = T2 | D15 = T3
   D13 = T4 | D12 = T5 | D14 = T6
   D27 = T7 | D33 = T8 | D32 = T9
*/
#define touchPIN T0
/*
   The sensors can not always work,
   for this reason you need to connect
   the head-on collision button.
   She will deploy the technique as a last resort
*/
#define bumperButtonPIN 22

//*********************************************\\

File file;
int counterLeft = 0, counterRight = 0, speedLeft = 0, speedRight = 0,
    touchValue = 100, threshold = 55, speed = 0, drive = 0, cursorArray = 0;
bool driveState = false, oldValueTouch = false, speakerState = false,
     counter = false;
int vectors[2048][2];
AsyncWebSocketClient * globalClient = NULL;

IPAddress local_IP(192, 168, 1, 199);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

AsyncWebServer server(80);
AsyncWebSocket ws("/websocket");

void
speaker()
{
  for (int i = 0; i < 10; i++) {
    digitalWrite(speakerPIN, HIGH);
    delay(100);
    digitalWrite(speakerPIN, LOW);
    delay(100);
  }
  digitalWrite(speakerPIN, LOW);
  speakerState = false;
}

void
stop()
{
  counter = false;
  ledcWrite(2, 0);
  ledcWrite(3, 0);
  ledcWrite(0, 0);
  ledcWrite(1, 0);
}

void
onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
          AwsEventType type, void* arg, uint8_t* data, size_t len)
{
  if (type == WS_EVT_CONNECT) {
    client->text("Websocket client connection received");
    globalClient = client;
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
    globalClient = NULL;
  } else if (type == WS_EVT_DATA) {
    if (!strncmp((const char*)data, "find", 4)) {
      if (speakerPIN) {
        speakerState = true;
      }
    } else if (!strncmp((const char*)data, "goAuto", 6)) {
      driveState = counter = true;
    } else if (!strncmp((const char*)data, "stopAuto", 8)) {
      driveState = false;
      stop();
    } else if (!strncmp((const char*)data, "goHome", 6)) {
      drive = 4;
    } else if (!strncmp((const char*)data, "clearStory", 10)) {
      cursorArray = 0;
      if (SPIFFS.remove("/config.ini")) {
        client->text("Clear file story");
      } else {
        Serial.print("Deleting file failed!");
      }
    } else if (!strncmp((const char*)data, "alg-random", 10)) {
      drive = 1;
    } else if (!strncmp((const char*)data, "alg-contour", 11)) {
      drive = 2;
    } else if (!strncmp((const char*)data, "alg-snake", 9)) {
      drive = 3;
    } else if (!strncmp((const char*)data, "u", 1)) {
      counter = true;
      speed = data[1] - '0';
      speed = speed > 5 ? 5 : (speed < 1 ? 1 : speed);
      ledcWrite(1, (255 / 100 * (100 - randomTurnLeft) / 5 * speed));
      ledcWrite(2, 0);
      ledcWrite(3, 0);
      ledcWrite(0, (255 / 100 * (100 - randomTurnRight) / 5 * speed));
    } else if (!strncmp((const char*)data, "l", 1)) {
      counter = true;
      speed = data[1] - '0';
      speed = speed > 5 ? 5 : (speed < 1 ? 1 : speed);
      ledcWrite(1, (255 / 100 * (100 - randomTurnLeft) / 5 * speed));
      ledcWrite(2, 0);
      ledcWrite(3, (255 / 100 * (100 - randomTurnRight) / 5 * speed));
      ledcWrite(0, 0);
    } else if (!strncmp((const char*)data, "r", 1)) {
      counter = true;
      speed = data[1] - '0';
      speed = speed > 5 ? 5 : (speed < 1 ? 1 : speed);
      ledcWrite(1, 0);
      ledcWrite(2, (255 / 100 * (100 - randomTurnLeft) / 5 * speed));
      ledcWrite(3, 0);
      ledcWrite(0, (255 / 100 * (100 - randomTurnRight) / 5 * speed));
    } else if (!strncmp((const char*)data, "d", 1)) {
      counter = true;
      speed = data[1] - '0';
      speed = speed > 5 ? 5 : (speed < 1 ? 1 : speed);
      ledcWrite(1, 0);
      ledcWrite(2, (255 / 100 * (100 - randomTurnLeft) / 5 * speed));
      ledcWrite(3, (255 / 100 * (100 - randomTurnRight) / 5 * speed));
      ledcWrite(0, 0);
    } else if (!strncmp((const char*)data, "stopJoy", 7)) {
      stop();
    }
  }
}

void IRAM_ATTR
countLeftFunction()
{
  if (counter) {
    counterLeft++;
  }
}

void IRAM_ATTR
countRightFunction()
{
  if (counter) {
    counterRight++;
  }
}

void
initPIN()
{
  ledcSetup(0, 50, 8);
  ledcAttachPin(leftWheelForwardPIN, 0);
  ledcAttachPin(rightWheelForwardPIN, 1);
  ledcAttachPin(leftWheelBackwardPIN, 2);
  ledcAttachPin(rightWheelBackwardPIN, 3);
  pinMode(leftWheelForwardPIN, OUTPUT);
  pinMode(rightWheelForwardPIN, OUTPUT);
  pinMode(leftWheelBackwardPIN, OUTPUT);
  pinMode(rightWheelBackwardPIN, OUTPUT);
  pinMode(bumperButtonPIN, INPUT);
  pinMode(laserSensorLeftPIN, INPUT);
  pinMode(laserSensorCenterPIN, INPUT);
  pinMode(laserSensorRightPIN, INPUT);
  pinMode(countRotationLeftPIN, INPUT_PULLUP);
  pinMode(countRotationRightPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(countRotationLeftPIN),
                  countLeftFunction, FALLING);
  attachInterrupt(digitalPinToInterrupt(countRotationRightPIN),
                  countRightFunction, FALLING);
  if (speakerPIN) {
    pinMode(speakerPIN, OUTPUT);
  }
  speedLeft = 80 + (80 / 100 * randomTurnLeft);
  speedRight = 80 + (80 / 100 * randomTurnRight);
}

void
setup()
{
  Serial.begin(115200);
  initPIN();
  if (randomTurnLeft ^ randomTurnRight) {
    if (randomTurnLeft) {
      speedLeft = defaultSpeed;
      speedRight = defaultSpeed - (defaultSpeed / 100 * randomTurnLeft);
    } else {
      speedRight = defaultSpeed;
      speedLeft = defaultSpeed - (defaultSpeed / 100 * randomTurnRight);
    }
  } else {
    Serial.println("You can specify only one speed correction out of 2");
  }
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  file = SPIFFS.open("/config.ini", "w+");
  if (!file) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  WiFi.begin(ssid, password);
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  server.on("/main.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/main.js", "text/javascript");
  });
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/favicon.png", "image/png");
  });
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  delay(4000);
}

void trigger() {
  if (globalClient != NULL && globalClient->status() == WS_CONNECTED) {
    char str[255];
    if (cursorArray <= 2047) {
      vectors[cursorArray][0] = counterLeft * (wheelCircumference / whellSensor);
      vectors[cursorArray][1] = counterRight * (wheelCircumference / whellSensor);
    }
    int left = digitalRead(laserSensorLeftPIN) == LOW ? 1 : 0;
    int center = digitalRead(laserSensorCenterPIN) == LOW ? 1 : 0;
    int  right = digitalRead(laserSensorRightPIN) == LOW ? 1 : 0;
    int bumper = digitalRead(bumperButtonPIN) == HIGH ? 1 : 0;
    sprintf(str, "{\"left\":%i,\"center\":%i,\"right\":%i,\"bumper\":%i,\"segment\":%i,\"leftCounter\":%i,\"rightCounter\":%i}",
            left, center, right, bumper, cursorArray, vectors[cursorArray][0], vectors[cursorArray][1]);
    globalClient->text(str);
    counterLeft = counterRight = 0;
    cursorArray++;
  }
}

void loop()
{
  if (speakerState) {
    speaker();
  }

  touchValue = touchRead(touchPIN);
  if (touchValue < threshold) {
    if (!oldValueTouch) {
      if (driveState) {
        stop();
        driveState = counter = false;
      } else {
        driveState = counter = true;
      }
      delay(500);
    }
    oldValueTouch = true;
  } else {
    oldValueTouch = false;
  }

  if (driveState) {
    if (digitalRead(laserSensorLeftPIN) == LOW) {
      trigger();
      counter = true;
      ledcWrite(1, 0);
      ledcWrite(3, speedLeft);
      ledcWrite(0, 0);
      ledcWrite(2, speedRight);
      delay(500);
      ledcWrite(1, 0);
      ledcWrite(3, 0);
      ledcWrite(0, speedLeft);
      ledcWrite(2, speedRight);
      delay(700);
    } else if (digitalRead(laserSensorCenterPIN) == LOW ||
               digitalRead(bumperButtonPIN) == HIGH) {
      trigger();
      counter = true;
      ledcWrite(1, 0);
      ledcWrite(3, speedLeft);
      ledcWrite(0, 0);
      ledcWrite(2, speedRight);
      delay(1000);
      ledcWrite(1, 0);
      ledcWrite(3, 0);
      ledcWrite(0, speedLeft);
      ledcWrite(2, speedRight);
      delay(700);
    } else if (digitalRead(laserSensorRightPIN) == LOW) {
      trigger();
      counter = true;
      ledcWrite(1, 0);
      ledcWrite(3, speedLeft);
      ledcWrite(0, 0);
      ledcWrite(2, speedRight);
      delay(500);
      counter = true;
      ledcWrite(0, 0);
      ledcWrite(2, 0);
      ledcWrite(3, speedLeft);
      ledcWrite(1, speedRight);
      delay(700);
    } else {
      counter = true;
      ledcWrite(2, 0);
      ledcWrite(3, 0);
      ledcWrite(0, speedLeft);
      ledcWrite(1, speedRight);
    }
  }
}
