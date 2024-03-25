#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "UPB-Guest";
const char* password = "";

const int echoPin = D5;
const int trigPin = D3;
const int yellowLedPin = D1;
const int greenLedPin = D2;
const int blueLedPin = D6;

class DistanceSensor {
public:
  DistanceSensor(int trigPin, int echoPin) : trigPin(trigPin), echoPin(echoPin) {}

  void setup() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
  }

  double measureDistance() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duration = pulseIn(echoPin, HIGH);
    return (duration * 0.034 / 2) / 100;
  }

private:
  int trigPin;
  int echoPin;
  long duration;
};

class LEDController {
public:
  LEDController(int yellowPin, int greenPin, int bluePin) : yellowPin(yellowPin), greenPin(greenPin), bluePin(bluePin) {}

  void setup() {
    pinMode(yellowPin, OUTPUT);
    pinMode(greenPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
  }

  void setLEDColor(int obj) {
    digitalWrite(yellowPin, LOW);
    digitalWrite(greenPin, LOW);
    digitalWrite(bluePin, LOW);

    if (obj == 3) {
      digitalWrite(yellowPin, HIGH);  // centimetri
    } else if (obj == 2) {
      digitalWrite(greenPin, HIGH);   // decimetri
    } else {
      digitalWrite(bluePin, HIGH);    // metri
    }
  }

private:
  int yellowPin;
  int greenPin;
  int bluePin;
};

class DistanceServer {
public:
  DistanceServer(int serverPort, DistanceSensor& sensor, LEDController& ledController) : serverPort(serverPort), sensor(sensor), ledController(ledController) {}

  void setup() {
    server.on("/", std::bind(&DistanceServer::handleRoot, this));
    server.on("/refresh", std::bind(&DistanceServer::handleRefresh, this));

    server.begin();
    Serial.println("HTTP server started");
  }

  void handleClient() {
    server.handleClient();
  }

private:
  ESP8266WebServer server;
  int serverPort;
  DistanceSensor& sensor;
  LEDController& ledController;

  static const char* page;
  
  void handleRoot() {
    server.send(200, "text/html", page);
  }

  void handleRefresh() {
    double distance = sensor.measureDistance();
    char messageFinal[100];
    sprintf(messageFinal, "%.2f", distance);
/*
    Serial.print("Distance: ");
    Serial.println(distance);
*/
    server.send(200, "application/javascript", messageFinal);

    int obj;
    if (distance > 1) {
      obj = 1;
    } else if (distance > 0.1) {
      obj = 2;
    } else {
      obj = 3;
    }

    ledController.setLEDColor(obj);
  }
};

const char* DistanceServer::page = R"(
<html>  
  <head> 
    <script src='https://code.jquery.com/jquery-3.3.1.min.js'></script>
    <title>Distance measurement</title> 
  </head> 

  <body> 
    <h2>Here is the distance you wish to measure!
    
    </h2> 
    <table style='font-size:20px'>  
      <tr>  
        <td> 
          <div>Distance:  </div>
        </td>
        <td> 
          <div id='Distance'></div> 
        </td>
      </tr> 
    </table>  
  </body> 
  
  <script>
  $(document).ready(function () {
    setInterval(refreshFunction, 100);
  });

  function refreshFunction() {
    $.get('/refresh', function (result) {
      // Decide unitatea de masura in functie de valoarea distantei
      var distanceValue = parseFloat(result);
      var unit = '';

      if (distanceValue > 1) {
         unit = ' m';
         obj = 1;
      } else if (distanceValue > 0.1) {
         unit = ' dm';
         distanceValue *= 10;  // Convertim în decimetri
         obj = 2;
      } else {
         unit = ' cm';
         distanceValue *= 100; // Convertim in centimetri
         obj = 3;
      }
      
      $('#Distance').html(distanceValue.toFixed(2) + unit);
    });
  }
  </script>
</html> 
)";

void connectToWiFi() {
  Serial.println("Connecting to the WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Waiting for connection");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

DistanceSensor distanceSensor(trigPin, echoPin);
LEDController ledController(yellowLedPin, greenLedPin, blueLedPin);
DistanceServer distanceServer(80, distanceSensor, ledController);

void setup() {
  Serial.begin(115200);
  delay(1000);

  connectToWiFi();
  distanceSensor.setup();
  ledController.setup();
  distanceServer.setup();
}

void loop() {
  distanceServer.handleClient();
  delay(500);
}

