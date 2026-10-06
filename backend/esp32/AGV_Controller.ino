// #include <WiFi.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>


// // ======================================================
// // WIFI
// // ======================================================

// const char* WIFI_SSID = "SHANKS";
// const char* WIFI_PASSWORD = "shashank1234";


// // ======================================================
// // MQTT
// // ======================================================

// const char* MQTT_SERVER = "e61e359e1edb4ae3ab97dce7981729b8.s1.eu.hivemq.cloud";

// const int MQTT_PORT = 8883;

// const char* AGV_ID = "AGV_01";

// String commandTopic =
//     "agv/" + String(AGV_ID) + "/command";

// String statusTopic =
//     "agv/" + String(AGV_ID) + "/status";

// String telemetryTopic =
//     "agv/" + String(AGV_ID) + "/telemetry";

// String eventTopic =
//     "agv/" + String(AGV_ID) + "/event";


// WiFiClient espClient;

// PubSubClient mqttClient(espClient);


// // ======================================================
// // MOTOR PINS
// // ======================================================

// #define LEFT_IN1 5
// #define LEFT_IN2 7
// #define LEFT_PWM 6

// #define RIGHT_IN1 8
// #define RIGHT_IN2 9
// #define RIGHT_PWM 10


// // ======================================================
// // HALL SENSORS
// // ======================================================

// #define HALL_LEFT 2
// #define HALL_RIGHT 3

// volatile long hallLeftCount = 0;
// volatile long hallRightCount = 0;


// // ======================================================
// // 3 PRECISION IR SENSORS
// // ======================================================

// #define IR_LEFT 32
// #define IR_CENTER 33
// #define IR_RIGHT 27


// // ======================================================
// // ULTRASONIC SENSOR
// // ======================================================

// #define ULTRASONIC_TRIG 4
// #define ULTRASONIC_ECHO 11


// // ======================================================
// // ULTRASONIC POWER
// // ======================================================

// #define ULTRASONIC_POWER 13


// // ======================================================
// // MOTOR SETTINGS
// // ======================================================

// const int BASE_SPEED = 170;

// const int TURN_SPEED = 150;


// // ======================================================
// // WHEEL SETTINGS
// // ======================================================

// const float WHEEL_DIAMETER = 0.064;

// const float PULSES_PER_REV = 20.0;

// const float WHEEL_CIRCUMFERENCE =
//     PI * WHEEL_DIAMETER;

// // ======================================================
// // PATH STATE
// // ======================================================

// DynamicJsonDocument currentPath(4096);

// bool pathRunning = false;
// bool pathPaused = false;

// int currentSegment = 0;

// String currentObject = "";
// String currentPathId = "";


// // ======================================================
// // STATUS
// // ======================================================

// String agvState = "IDLE";


// // ======================================================
// // INTERRUPTS
// // ======================================================

// void IRAM_ATTR hallLeftISR() {

//     hallLeftCount++;

// }


// void IRAM_ATTR hallRightISR() {

//     hallRightCount++;

// }


// // ======================================================
// // MOTOR CONTROL
// // ======================================================

// void leftMotor(int speed) {

//     speed = constrain(speed, -255, 255);


//     if (speed > 0) {

//         digitalWrite(LEFT_IN1, HIGH);

//         digitalWrite(LEFT_IN2, LOW);

//         analogWrite(LEFT_PWM, speed);

//     }

//     else if (speed < 0) {

//         digitalWrite(LEFT_IN1, LOW);

//         digitalWrite(LEFT_IN2, HIGH);

//         analogWrite(
//             LEFT_PWM,
//             abs(speed)
//         );

//     }

//     else {

//         digitalWrite(LEFT_IN1, LOW);

//         digitalWrite(LEFT_IN2, LOW);

//         analogWrite(LEFT_PWM, 0);

//     }

// }


// void rightMotor(int speed) {

//     speed = constrain(speed, -255, 255);


//     if (speed > 0) {

//         digitalWrite(RIGHT_IN1, HIGH);

//         digitalWrite(RIGHT_IN2, LOW);

//         analogWrite(RIGHT_PWM, speed);

//     }

//     else if (speed < 0) {

//         digitalWrite(RIGHT_IN1, LOW);

//         digitalWrite(RIGHT_IN2, HIGH);

//         analogWrite(
//             RIGHT_PWM,
//             abs(speed)
//         );

//     }

//     else {

//         digitalWrite(RIGHT_IN1, LOW);

//         digitalWrite(RIGHT_IN2, LOW);

//         analogWrite(RIGHT_PWM, 0);

//     }

// }


// void stopMotors() {

//     leftMotor(0);

//     rightMotor(0);

// }


// // ======================================================
// // HALL DISTANCE
// // ======================================================

// float pulsesToDistance(long pulses) {

//     float revolutions =
//         pulses / PULSES_PER_REV;

//     return revolutions *
//            WHEEL_CIRCUMFERENCE;

// }


// float averageDistance() {

//     float leftDistance =
//         pulsesToDistance(hallLeftCount);

//     float rightDistance =
//         pulsesToDistance(hallRightCount);


//     return
//         (leftDistance + rightDistance)
//         / 2.0;

// }


// // ======================================================
// // RESET HALL
// // ======================================================

// void resetHallCounters() {

//     noInterrupts();

//     hallLeftCount = 0;

//     hallRightCount = 0;

//     interrupts();

// }


// // ======================================================
// // ULTRASONIC
// // ======================================================

// float readUltrasonic(
//     int trigPin,
//     int echoPin
// ) {

//     digitalWrite(
//         trigPin,
//         LOW
//     );

//     delayMicroseconds(2);


//     digitalWrite(
//         trigPin,
//         HIGH
//     );

//     delayMicroseconds(10);


//     digitalWrite(
//         trigPin,
//         LOW
//     );


//     long duration =
//         pulseIn(
//             echoPin,
//             HIGH,
//             30000
//         );


//     if (duration == 0) {

//         return 999;

//     }


//     return
//         duration * 0.0343 / 2.0;

// }


// // ======================================================
// // OBSTACLE DETECTION
// // ======================================================

// bool obstacleDetected() {

//     float front =
//         readUltrasonic(
//             FRONT_TRIG,
//             FRONT_ECHO
//         );


//     if (front < 25) {

//         return true;

//     }


//     return false;

// }


// // ======================================================
// // IR SENSOR
// // ======================================================

// int readIR(int pin) {

//     return digitalRead(pin);

// }


// // ======================================================
// // LINE FOLLOWING
// // ======================================================

// void lineFollow() {

//     int left =
//         readIR(IR_LEFT);

//     int center =
//         readIR(IR_CENTER);

//     int right =
//         readIR(IR_RIGHT);


//     /*
//        Assuming:

//        1 = line detected
//        0 = no line

//        Change these if your IR
//        sensor works opposite.
//     */


//     if (
//         center == HIGH &&
//         left == LOW &&
//         right == LOW
//     ) {

//         leftMotor(BASE_SPEED);

//         rightMotor(BASE_SPEED);

//     }


//     else if (
//         left == HIGH &&
//         center == LOW
//     ) {

//         // Line is towards left

//         leftMotor(100);

//         rightMotor(190);

//     }


//     else if (
//         right == HIGH &&
//         center == LOW
//     ) {

//         // Line is towards right

//         leftMotor(190);

//         rightMotor(100);

//     }


//     else if (
//         left == HIGH &&
//         center == HIGH
//     ) {

//         leftMotor(90);

//         rightMotor(180);

//     }


//     else if (
//         right == HIGH &&
//         center == HIGH
//     ) {

//         leftMotor(180);

//         rightMotor(90);

//     }


//     else {

//         /*
//            Lost line.
//            Stop instead of blindly
//            continuing.
//         */

//         stopMotors();

//     }

// }


// // ======================================================
// // STRAIGHT MOVEMENT
// // ======================================================

// bool moveStraight(float targetDistance) {

//     resetHallCounters();


//     while (true) {


//         if (pathPaused) {

//             stopMotors();

//             delay(50);

//             continue;

//         }


//         if (obstacleDetected()) {

//             stopMotors();

//             agvState =
//                 "OBSTACLE";

//             publishStatus();

//             delay(100);

//             continue;

//         }


//         float distance =
//             averageDistance();


//         if (
//             distance >=
//             targetDistance
//         ) {

//             stopMotors();

//             return true;

//         }


//         /*
//            Use the three IR sensors
//            for precision path following.
//         */

//         lineFollow();


//         delay(10);

//     }

// }


// // ======================================================
// // TURN
// // ======================================================

// void turnRobot(
//     String direction,
//     float angle
// ) {

//     resetHallCounters();


//     /*
//        This is a simple timed/hall based
//        turning implementation.

//        It should be calibrated on the
//        actual AGV.
//     */


//     float targetPulses =
//         5.0 * (angle / 90.0);


//     if (
//         direction ==
//         "RIGHT"
//     ) {

//         leftMotor(TURN_SPEED);

//         rightMotor(-TURN_SPEED);

//     }

//     else {

//         leftMotor(-TURN_SPEED);

//         rightMotor(TURN_SPEED);

//     }


//     while (
//         abs(hallLeftCount) <
//         targetPulses &&
//         abs(hallRightCount) <
//         targetPulses
//     ) {

//         if (obstacleDetected()) {

//             stopMotors();

//             agvState =
//                 "OBSTACLE";

//             while (
//                 obstacleDetected()
//             ) {

//                 delay(100);

//             }

//             agvState =
//                 "RUNNING";

//         }


//         delay(5);

//     }


//     stopMotors();

// }


// // ======================================================
// // EXECUTE CURRENT SEGMENT
// // ======================================================

// void executeCurrentSegment() {

//     JsonObject segment =
//         currentPath["segments"]
//             [currentSegment];


//     String type =
//         segment["type"];


//     if (
//         type ==
//         "STRAIGHT"
//     ) {

//         float distance =
//             segment["distance"];


//         moveStraight(distance);

//     }


//     else if (
//         type ==
//         "TURN"
//     ) {

//         String direction =
//             segment["direction"];


//         float angle =
//             segment["angle"];


//         turnRobot(
//             direction,
//             angle
//         );

//     }


// }


// // ======================================================
// // EXECUTE PATH
// // ======================================================

// void executePath() {

//     if (!pathRunning) {

//         return;

//     }


//     int totalSegments =
//         currentPath["segments"].size();


//     while (
//         currentSegment <
//         totalSegments
//     ) {

//         if (!pathRunning) {

//             stopMotors();

//             return;

//         }


//         agvState =
//             "RUNNING";


//         publishStatus();


//         executeCurrentSegment();


//         if (!pathRunning) {

//             return;

//         }


//         currentSegment++;


//         publishStatus();

//     }


//     stopMotors();


//     pathRunning = false;


//     agvState =
//         "COMPLETED";


//     publishEvent(
//         "PATH_COMPLETED"
//     );


//     publishStatus();

// }


// // ======================================================
// // MQTT CALLBACK
// // ======================================================

// void mqttCallback(
//     char* topic,
//     byte* payload,
//     unsigned int length
// ) {


//     String message;


//     for (
//         unsigned int i = 0;
//         i < length;
//         i++
//     ) {

//         message +=
//             (char)payload[i];

//     }


//     Serial.println(
//         "MQTT:"
//     );

//     Serial.println(message);


//     DynamicJsonDocument doc(4096);


//     DeserializationError error =
//         deserializeJson(
//             doc,
//             message
//         );


//     if (error) {

//         Serial.println(
//             "JSON ERROR"
//         );

//         return;

//     }


//     String command =
//         doc["command"];


//     // ----------------------------------------
//     // START PATH
//     // ----------------------------------------

//     if (
//         command ==
//         "START_PATH"
//     ) {

//         currentPath.clear();


//         currentPath.set(doc);


//         currentObject =
//             doc["object"].as<String>();


//         currentPathId =
//             doc["pathId"].as<String>();


//         currentSegment = 0;

//         pathRunning = true;

//         pathPaused = false;

//         agvState =
//             "STARTING";


//         resetHallCounters();


//         publishEvent(
//             "PATH_STARTED"
//         );


//         publishStatus();

//     }


//     // ----------------------------------------
//     // STOP
//     // ----------------------------------------

//     else if (
//         command ==
//         "STOP"
//     ) {

//         pathRunning = false;

//         pathPaused = true;

//         stopMotors();

//         agvState =
//             "STOPPED";


//         publishEvent(
//             "AGV_STOPPED"
//         );


//         publishStatus();

//     }


//     // ----------------------------------------
//     // RESUME
//     // ----------------------------------------

//     else if (
//         command ==
//         "RESUME"
//     ) {

//         pathPaused = false;


//         if (currentSegment <
//             currentPath["segments"].size()) {

//             pathRunning = true;

//             agvState =
//                 "RESUMING";

//         }


//         publishStatus();

//     }


//     // ----------------------------------------
//     // RESET
//     // ----------------------------------------

//     else if (
//         command ==
//         "RESET"
//     ) {

//         pathRunning = false;

//         pathPaused = false;

//         currentSegment = 0;

//         currentObject = "";

//         currentPathId = "";

//         resetHallCounters();

//         stopMotors();


//         agvState =
//             "IDLE";


//         publishStatus();

//     }


//     // ----------------------------------------
//     // MANUAL
//     // ----------------------------------------

//     else if (
//         command ==
//         "MANUAL"
//     ) {

//         String action =
//             doc["action"];


//         pathRunning = false;


//         if (
//             action ==
//             "FORWARD"
//         ) {

//             leftMotor(160);

//             rightMotor(160);

//         }


//         else if (
//             action ==
//             "BACKWARD"
//         ) {

//             leftMotor(-160);

//             rightMotor(-160);

//         }


//         else if (
//             action ==
//             "LEFT"
//         ) {

//             leftMotor(-120);

//             rightMotor(120);

//         }


//         else if (
//             action ==
//             "RIGHT"
//         ) {

//             leftMotor(120);

//             rightMotor(-120);

//         }


//         else {

//             stopMotors();

//         }


//         agvState =
//             "MANUAL";


//         publishStatus();

//     }

// }


// // ======================================================
// // STATUS
// // ======================================================

// void publishStatus() {

//     DynamicJsonDocument doc(1024);


//     doc["state"] =
//         agvState;


//     doc["object"] =
//         currentObject;


//     doc["path"] =
//         currentPathId;


//     doc["segment"] =
//         currentSegment;


//     doc["distance"] =
//         averageDistance();


//     doc["hallLeft"] =
//         hallLeftCount;


//     doc["hallRight"] =
//         hallRightCount;


//     doc["irLeft"] =
//         readIR(IR_LEFT);


//     doc["irCenter"] =
//         readIR(IR_CENTER);


//     doc["irRight"] =
//         readIR(IR_RIGHT);


//     doc["ultrasonicFront"] =
//         readUltrasonic(
//             FRONT_TRIG,
//             FRONT_ECHO
//         );


//     doc["ultrasonicLeft"] =
//         readUltrasonic(
//             LEFT_TRIG,
//             LEFT_ECHO
//         );


//     doc["ultrasonicRight"] =
//         readUltrasonic(
//             RIGHT_TRIG,
//             RIGHT_ECHO
//         );


//     String output;


//     serializeJson(
//         doc,
//         output
//     );


//     mqttClient.publish(
//         statusTopic.c_str(),
//         output.c_str()
//     );

// }


// // ======================================================
// // EVENT
// // ======================================================

// void publishEvent(
//     String event
// ) {

//     DynamicJsonDocument doc(512);


//     doc["event"] =
//         event;


//     doc["object"] =
//         currentObject;


//     doc["path"] =
//         currentPathId;


//     doc["timestamp"] =
//         millis();


//     String output;


//     serializeJson(
//         doc,
//         output
//     );


//     mqttClient.publish(
//         eventTopic.c_str(),
//         output.c_str()
//     );

// }


// // ======================================================
// // WIFI
// // ======================================================

// void connectWiFi() {

//     Serial.print(
//         "Connecting WiFi"
//     );


//     WiFi.begin(
//         WIFI_SSID,
//         WIFI_PASSWORD
//     );


//     while (
//         WiFi.status() !=
//         WL_CONNECTED
//     ) {

//         delay(500);

//         Serial.print(".");

//     }


//     Serial.println();

//     Serial.println(
//         "WiFi connected"
//     );


//     Serial.println(
//         WiFi.localIP()
//     );

// }


// // ======================================================
// // MQTT CONNECTION
// // ======================================================

// void connectMQTT() {

//     while (
//         !mqttClient.connected()
//     ) {

//         Serial.println(
//             "Connecting MQTT..."
//         );


//         String clientId =
//             "AGV_ESP32_" +
//             String(random(0xffff), HEX);


//         if (
//             mqttClient.connect(
//                 clientId.c_str()
//             )
//         ) {

//             Serial.println(
//                 "MQTT connected"
//             );


//             mqttClient.subscribe(
//                 commandTopic.c_str()
//             );


//             publishEvent(
//                 "ESP32_CONNECTED"
//             );


//             publishStatus();

//         }

//         else {

//             Serial.print(
//                 "MQTT failed: "
//             );

//             Serial.println(
//                 mqttClient.state()
//             );


//             delay(2000);

//         }

//     }

// }


// // ======================================================
// // SETUP
// // ======================================================

// void setup() {

//     Serial.begin(115200);


//     // Motors

//     pinMode(
//         LEFT_IN1,
//         OUTPUT
//     );

//     pinMode(
//         LEFT_IN2,
//         OUTPUT
//     );

//     pinMode(
//         RIGHT_IN1,
//         OUTPUT
//     );

//     pinMode(
//         RIGHT_IN2,
//         OUTPUT
//     );


//     pinMode(
//         LEFT_PWM,
//         OUTPUT
//     );

//     pinMode(
//         RIGHT_PWM,
//         OUTPUT
//     );


//     // Hall

//     pinMode(
//         HALL_LEFT,
//         INPUT
//     );

//     pinMode(
//         HALL_RIGHT,
//         INPUT
//     );


//     attachInterrupt(
//         digitalPinToInterrupt(
//             HALL_LEFT
//         ),
//         hallLeftISR,
//         RISING
//     );


//     attachInterrupt(
//         digitalPinToInterrupt(
//             HALL_RIGHT
//         ),
//         hallRightISR,
//         RISING
//     );


//     // IR

//     pinMode(
//         IR_LEFT,
//         INPUT
//     );

//     pinMode(
//         IR_CENTER,
//         INPUT
//     );

//     pinMode(
//         IR_RIGHT,
//         INPUT
//     );


//     // Ultrasonic

//     pinMode(
//         FRONT_TRIG,
//         OUTPUT
//     );

//     pinMode(
//         FRONT_ECHO,
//         INPUT
//     );


//     pinMode(
//         LEFT_TRIG,
//         OUTPUT
//     );

//     pinMode(
//         LEFT_ECHO,
//         INPUT
//     );


//     pinMode(
//         RIGHT_TRIG,
//         OUTPUT
//     );

//     pinMode(
//         RIGHT_ECHO,
//         INPUT
//     );


//     stopMotors();


//     connectWiFi();


//     mqttClient.setServer(
//         MQTT_SERVER,
//         MQTT_PORT
//     );


//     mqttClient.setCallback(
//         mqttCallback
//     );

// }


// // ======================================================
// // LOOP
// // ======================================================

// void loop() {


//     if (
//         WiFi.status() !=
//         WL_CONNECTED
//     ) {

//         connectWiFi();

//     }


//     if (
//         !mqttClient.connected()
//     ) {

//         connectMQTT();

//     }


//     mqttClient.loop();


//     if (
//         pathRunning &&
//         !pathPaused
//     ) {

//         executePath();

//     }


//     static unsigned long lastStatus =
//         0;


//     if (
//         millis() -
//         lastStatus >
//         1000
//     ) {

//         publishStatus();

//         lastStatus =
//             millis();

//     }


//     delay(5);

// }