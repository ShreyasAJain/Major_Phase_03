// ============================================================
// AGV PHASE 3
// ESP32-WROOM-32
//`
// MQTT + HiveMQ
// 8-Array IR Line Following
// 2 x BTS7960
// HX711 Load Cell
// Front + Rear Ultrasonic
//
// SEQUENCE
//
// START
//   ↓
// WAIT FOR >= 2 KG
//   ↓
// FORWARD LINE FOLLOWING
//   ↓
// DESTINATION = 00000000
//   ↓
// WAIT FOR LOAD REMOVAL <= 0.5 KG
//   ↓
// REVERSE STRAIGHT FOR 1 SECOND
//   ↓
// STOP 100 ms
//   ↓
// BACKWARD LINE FOLLOWING
//   ↓
// SOURCE = 00000000
//   ↓
// STOP
//
// No sharp/pivot turns.
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "HX711.h"

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID =
    "Nothing Phone (2)_1932";

const char* WIFI_PASSWORD =
    "12345678";

// ============================================================
// HIVEMQ
// ============================================================

const char* MQTT_HOST =
    "e61e359e1edb4ae3ab97dce7981729b8.s1.eu.hivemq.cloud";

const int MQTT_PORT = 8883;

const char* MQTT_USERNAME =
    "Major_AGV";

const char* MQTT_PASSWORD =
    "Major_AGV";

// ============================================================
// MQTT TOPICS
// ============================================================

const char* COMMAND_TOPIC =
    "agv/AGV_01/command";

const char* STATUS_TOPIC =
    "agv/AGV_01/status";

const char* TELEMETRY_TOPIC =
    "agv/AGV_01/telemetry";

const char* EVENT_TOPIC =
    "agv/AGV_01/event";

// ============================================================
// IR SENSOR PINS
// ============================================================

const int IR_PINS[8] = {
    14,
    27,
    26,
    25,
    33,
    32,
    35,
    34
};

// ============================================================
// MOTOR PINS
// ============================================================

const int LEFT_RPWM = 19;
const int LEFT_LPWM = 18;

const int RIGHT_RPWM = 4;
const int RIGHT_LPWM = 15;

// ============================================================
// PWM
// ============================================================

const int PWM_FREQUENCY = 20000;
const int PWM_RESOLUTION = 8;

// ============================================================
// MOTOR SPEEDS
// ============================================================

const int STRAIGHT_LEFT_PWM = 75;
const int STRAIGHT_RIGHT_PWM = 75;

const int SMALL_RIGHT_LEFT_PWM = 70;
const int SMALL_RIGHT_RIGHT_PWM = 62;

const int MEDIUM_RIGHT_LEFT_PWM = 85;
const int MEDIUM_RIGHT_RIGHT_PWM = 45;

const int STRONG_RIGHT_LEFT_PWM = 90;
const int STRONG_RIGHT_RIGHT_PWM = 35;

const int SMALL_LEFT_LEFT_PWM = 62;
const int SMALL_LEFT_RIGHT_PWM = 70;

const int MEDIUM_LEFT_LEFT_PWM = 45;
const int MEDIUM_LEFT_RIGHT_PWM = 85;

const int STRONG_LEFT_LEFT_PWM = 35;
const int STRONG_LEFT_RIGHT_PWM = 90;

// ============================================================
// HX711
// ============================================================

#define HX711_DT 23
#define HX711_SCK 22

HX711 scale;

#define CALIBRATION_FACTOR 17.9

// ============================================================
// WEIGHT
// ============================================================

// Weight is in KG in this program.

#define LOAD_REQUIRED_KG 2.0

// Reference backtracking logic:
// load is considered removed below 0.5 kg.

#define LOAD_REMOVED_KG 0.5

// ============================================================
// ULTRASONIC
// ============================================================

#define ULTRASONIC_TRIG 5

// IMPORTANT:
// These match the reference code you supplied.

#define FRONT_ECHO 13
#define REAR_ECHO 12

#define OBSTACLE_DISTANCE 30

// ============================================================
// BACKTRACKING
// ============================================================

#define INITIAL_REVERSE_TIME 1000
#define REVERSE_STOP_TIME 100

// ============================================================
// ROBOT STATES
// ============================================================

enum RobotState
{
    WAITING_FOR_LOAD,

    MOVING_FORWARD,

    WAITING_FOR_REMOVAL,

    MOVING_BACKWARD,

    COMPLETE
};

RobotState state =
    WAITING_FOR_LOAD;

// ============================================================
// NETWORK
// ============================================================

WiFiClientSecure secureClient;

PubSubClient mqttClient(
    secureClient
);

// ============================================================
// SENSOR DATA
// ============================================================

int sensorValues[8] = {
    0, 0, 0, 0,
    0, 0, 0, 0
};

float currentWeight = 0.0;

long frontDistance = 999;

long rearDistance = 999;

float lastLinePosition = 0;

// ============================================================
// FLAGS
// ============================================================

bool cycleActive = false;

bool lineFollowing = false;

// ============================================================
// TIMERS
// ============================================================

unsigned long lastMQTTAttempt = 0;

unsigned long lastStatus = 0;

unsigned long lastTelemetry = 0;

unsigned long lastUltrasonic = 0;

// ============================================================
// FUNCTION DECLARATIONS
// ============================================================

void connectWiFi();

bool connectMQTT();

void maintainMQTT();

void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
);

void processCommand(
    String command
);

void publishEvent(
    const char* event
);

void publishStatus();

void publishTelemetry();

void readIRSensors();

float readWeight();

long getDistance(
    int echoPin
);

bool allWhite();

void lineFollowForward();

void lineFollowBackward();

void stopMotors();

void drive(
    int left,
    int right
);

void setLeftMotor(
    int speed
);

void setRightMotor(
    int speed
);

void manualForward();

void manualBackward();

void manualLeft();

void manualRight();

String getStateText();

// ============================================================
// MOTOR
// ============================================================

void setLeftMotor(
    int speed
)
{
    speed = constrain(
        speed,
        -255,
        255
    );

    if (speed > 0)
    {
        ledcWrite(
            LEFT_RPWM,
            speed
        );

        ledcWrite(
            LEFT_LPWM,
            0
        );
    }
    else if (speed < 0)
    {
        ledcWrite(
            LEFT_RPWM,
            0
        );

        ledcWrite(
            LEFT_LPWM,
            -speed
        );
    }
    else
    {
        ledcWrite(
            LEFT_RPWM,
            0
        );

        ledcWrite(
            LEFT_LPWM,
            0
        );
    }
}

// ============================================================

void setRightMotor(
    int speed
)
{
    speed = constrain(
        speed,
        -255,
        255
    );

    if (speed > 0)
    {
        ledcWrite(
            RIGHT_RPWM,
            speed
        );

        ledcWrite(
            RIGHT_LPWM,
            0
        );
    }
    else if (speed < 0)
    {
        ledcWrite(
            RIGHT_RPWM,
            0
        );

        ledcWrite(
            RIGHT_LPWM,
            -speed
        );
    }
    else
    {
        ledcWrite(
            RIGHT_RPWM,
            0
        );

        ledcWrite(
            RIGHT_LPWM,
            0
        );
    }
}

// ============================================================

void drive(
    int left,
    int right
)
{
    setLeftMotor(left);
    setRightMotor(right);
}

// ============================================================

void stopMotors()
{
    drive(0, 0);
}

// ============================================================
// IR
// ============================================================

void readIRSensors()
{
    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        sensorValues[i] =
            digitalRead(
                IR_PINS[i]
            );
    }
}

// ============================================================
// ALL WHITE
//
// Your reference backtracking code uses:
//
// 00000000
//
// as the destination/source marker.
// ============================================================

bool allWhite()
{
    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        if (
            sensorValues[i] != 0
        )
        {
            return false;
        }
    }

    return true;
}

// ============================================================
// WEIGHT
// ============================================================

float readWeight()
{
    if (
        !scale.is_ready()
    )
    {
        return currentWeight;
    }

    float weight =
        scale.get_units(5);

    if (
        weight < 0
    )
    {
        weight = 0;
    }

    currentWeight =
        weight;

    return currentWeight;
}

// ============================================================
// ULTRASONIC
// ============================================================

long getDistance(
    int echoPin
)
{
    digitalWrite(
        ULTRASONIC_TRIG,
        LOW
    );

    delayMicroseconds(2);

    digitalWrite(
        ULTRASONIC_TRIG,
        HIGH
    );

    delayMicroseconds(10);

    digitalWrite(
        ULTRASONIC_TRIG,
        LOW
    );

    long duration =
        pulseIn(
            echoPin,
            HIGH,
            30000
        );

    if (
        duration == 0
    )
    {
        return 999;
    }

    return (
        duration *
        0.0343 /
        2
    );
}

// ============================================================
// FORWARD LINE FOLLOWING
//
// Uses the SAME weighted correction logic.
// ============================================================

void lineFollowForward()
{
    readIRSensors();

    // --------------------------------------------------------
    // DESTINATION MARKER
    //
    // 00000000
    // --------------------------------------------------------

    if (
        allWhite()
    )
    {
        stopMotors();

        lineFollowing =
            false;

        state =
            WAITING_FOR_REMOVAL;

        Serial.println();
        Serial.println(
            "================================"
        );
        Serial.println(
            "DESTINATION REACHED"
        );
        Serial.println(
            "REMOVE LOAD"
        );
        Serial.println(
            "================================"
        );

        publishEvent(
            "DESTINATION REACHED - REMOVE LOAD"
        );

        return;
    }

    // --------------------------------------------------------
    // WEIGHTED POSITION
    // --------------------------------------------------------

    const int weights[8] = {
        -7,
        -5,
        -3,
        -1,
         1,
         3,
         5,
         7
    };

    int activeCount = 0;

    int weightedSum = 0;

    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        if (
            sensorValues[i] == HIGH
        )
        {
            activeCount++;

            weightedSum +=
                weights[i];
        }
    }

    // --------------------------------------------------------
    // NO LINE
    // --------------------------------------------------------

    if (
        activeCount == 0
    )
    {
        stopMotors();

        lineFollowing =
            false;

        Serial.println(
            "NO LINE"
        );

        return;
    }

    float position =
        (float)weightedSum /
        activeCount;

    lastLinePosition =
        position;

    // --------------------------------------------------------
    // CENTER
    // --------------------------------------------------------

    if (
        position >= -1 &&
        position <= 1
    )
    {
        drive(
            STRAIGHT_LEFT_PWM,
            STRAIGHT_RIGHT_PWM
        );
    }

    // --------------------------------------------------------
    // RIGHT
    // --------------------------------------------------------

    else if (
        position < -1 &&
        position >= -2
    )
    {
        drive(
            SMALL_RIGHT_LEFT_PWM,
            SMALL_RIGHT_RIGHT_PWM
        );
    }

    else if (
        position < -2 &&
        position >= -4
    )
    {
        drive(
            MEDIUM_RIGHT_LEFT_PWM,
            MEDIUM_RIGHT_RIGHT_PWM
        );
    }

    else if (
        position < -4
    )
    {
        drive(
            STRONG_RIGHT_LEFT_PWM,
            STRONG_RIGHT_RIGHT_PWM
        );
    }

    // --------------------------------------------------------
    // LEFT
    // --------------------------------------------------------

    else if (
        position > 1 &&
        position <= 2
    )
    {
        drive(
            SMALL_LEFT_LEFT_PWM,
            SMALL_LEFT_RIGHT_PWM
        );
    }

    else if (
        position > 2 &&
        position <= 4
    )
    {
        drive(
            MEDIUM_LEFT_LEFT_PWM,
            MEDIUM_LEFT_RIGHT_PWM
        );
    }

    else if (
        position > 4
    )
    {
        drive(
            STRONG_LEFT_LEFT_PWM,
            STRONG_LEFT_RIGHT_PWM
        );
    }
}

// ============================================================
// BACKWARD LINE FOLLOWING
//
// IMPORTANT:
//
// This is NOT a pivot turn.
//
// It uses the same weighted IR correction,
// but both motors run in reverse.
// ============================================================

void lineFollowBackward()
{
    readIRSensors();

    // --------------------------------------------------------
    // SOURCE MARKER
    //
    // 00000000
    // --------------------------------------------------------

    if (
        allWhite()
    )
    {
        stopMotors();

        lineFollowing =
            false;

        cycleActive =
            false;

        state =
            COMPLETE;

        Serial.println();
        Serial.println(
            "================================"
        );
        Serial.println(
            "SOURCE REACHED"
        );
        Serial.println(
            "BACKTRACKING COMPLETE"
        );
        Serial.println(
            "AGV STOPPED"
        );
        Serial.println(
            "================================"
        );

        publishEvent(
            "SOURCE REACHED - AGV STOPPED"
        );

        return;
    }

    // --------------------------------------------------------
    // WEIGHTED POSITION
    // --------------------------------------------------------

    const int weights[8] = {
        -7,
        -5,
        -3,
        -1,
         1,
         3,
         5,
         7
    };

    int activeCount = 0;

    int weightedSum = 0;

    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        if (
            sensorValues[i] == HIGH
        )
        {
            activeCount++;

            weightedSum +=
                weights[i];
        }
    }

    // --------------------------------------------------------
    // NO LINE
    // --------------------------------------------------------

    if (
        activeCount == 0
    )
    {
        stopMotors();

        lineFollowing =
            false;

        return;
    }

    float position =
        (float)weightedSum /
        activeCount;

    lastLinePosition =
        position;

    // --------------------------------------------------------
    // CENTER
    // --------------------------------------------------------

    if (
        position >= -1 &&
        position <= 1
    )
    {
        drive(
            -STRAIGHT_LEFT_PWM,
            -STRAIGHT_RIGHT_PWM
        );
    }

    // --------------------------------------------------------
    // RIGHT
    // --------------------------------------------------------

    else if (
        position < -1 &&
        position >= -2
    )
    {
        drive(
            -SMALL_RIGHT_LEFT_PWM,
            -SMALL_RIGHT_RIGHT_PWM
        );
    }

    else if (
        position < -2 &&
        position >= -4
    )
    {
        drive(
            -MEDIUM_RIGHT_LEFT_PWM,
            -MEDIUM_RIGHT_RIGHT_PWM
        );
    }

    else if (
        position < -4
    )
    {
        drive(
            -STRONG_RIGHT_LEFT_PWM,
            -STRONG_RIGHT_RIGHT_PWM
        );
    }

    // --------------------------------------------------------
    // LEFT
    // --------------------------------------------------------

    else if (
        position > 1 &&
        position <= 2
    )
    {
        drive(
            -SMALL_LEFT_LEFT_PWM,
            -SMALL_LEFT_RIGHT_PWM
        );
    }

    else if (
        position > 2 &&
        position <= 4
    )
    {
        drive(
            -MEDIUM_LEFT_LEFT_PWM,
            -MEDIUM_LEFT_RIGHT_PWM
        );
    }

    else if (
        position > 4
    )
    {
        drive(
            -STRONG_LEFT_LEFT_PWM,
            -STRONG_LEFT_RIGHT_PWM
        );
    }
}

// ============================================================
// MANUAL
// ============================================================

void manualForward()
{
    drive(
        100,
        100
    );
}

void manualBackward()
{
    drive(
        -100,
        -100
    );
}

void manualLeft()
{
    drive(
        35,
        100
    );
}

void manualRight()
{
    drive(
        100,
        35
    );
}

// ============================================================
// MQTT EVENT
// ============================================================

void publishEvent(
    const char* event
)
{
    if (
        !mqttClient.connected()
    )
    {
        return;
    }

    String payload =
        "{\"event\":\"";

    payload += event;

    payload += "\"}";

    mqttClient.publish(
        EVENT_TOPIC,
        payload.c_str(),
        false
    );
}

// ============================================================
// STATE TEXT
// ============================================================

String getStateText()
{
    switch (
        state
    )
    {
        case WAITING_FOR_LOAD:
            return "WAITING_FOR_LOAD";

        case MOVING_FORWARD:
            return "MOVING_FORWARD";

        case WAITING_FOR_REMOVAL:
            return "WAITING_FOR_REMOVAL";

        case MOVING_BACKWARD:
            return "MOVING_BACKWARD";

        case COMPLETE:
            return "COMPLETE";
    }

    return "UNKNOWN";
}

// ============================================================
// STATUS
// ============================================================

void publishStatus()
{
    if (
        !mqttClient.connected()
    )
    {
        return;
    }

    String payload = "{";

    payload +=
        "\"state\":\"";

    payload +=
        getStateText();

    payload +=
        "\",";

    payload +=
        "\"lineFollowing\":";

    payload +=
        lineFollowing
        ? "true"
        : "false";

    payload +=
        ",\"cycleActive\":";

    payload +=
        cycleActive
        ? "true"
        : "false";

    payload +=
        ",\"weight\":";

    payload +=
        String(
            currentWeight,
            2
        );

    payload +=
        ",\"weightKg\":";

    payload +=
        String(
            currentWeight,
            2
        );

    payload +=
        ",\"loadReady\":";

    payload +=
        currentWeight >=
        LOAD_REQUIRED_KG
        ? "true"
        : "false";

    payload +=
        ",\"backtracking\":";

    payload +=
        state ==
        MOVING_BACKWARD
        ? "true"
        : "false";

    payload +=
        ",\"ultrasonicFront\":";

    payload +=
        String(
            frontDistance
        );

    payload +=
        ",\"ultrasonicRear\":";

    payload +=
        String(
            rearDistance
        );

    payload +=
        ",\"ir\":{";

    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        payload +=
            "\"S";

        payload +=
            String(
                i + 1
            );

        payload +=
            "\":";

        payload +=
            String(
                sensorValues[i]
            );

        if (
            i < 7
        )
        {
            payload += ",";
        }
    }

    payload +=
        "}}";

    mqttClient.publish(
        STATUS_TOPIC,
        payload.c_str(),
        false
    );
}

// ============================================================
// TELEMETRY
// ============================================================

void publishTelemetry()
{
    if (
        !mqttClient.connected()
    )
    {
        return;
    }

    String payload = "{";

    payload +=
        "\"state\":\"";

    payload +=
        getStateText();

    payload +=
        "\",";

    payload +=
        "\"weight\":";

    payload +=
        String(
            currentWeight,
            2
        );

    payload +=
        ",\"weightKg\":";

    payload +=
        String(
            currentWeight,
            2
        );

    payload +=
        ",\"loadReady\":";

    payload +=
        currentWeight >=
        LOAD_REQUIRED_KG
        ? "true"
        : "false";

    payload +=
        ",\"backtracking\":";

    payload +=
        state ==
        MOVING_BACKWARD
        ? "true"
        : "false";

    payload +=
        ",\"ultrasonicFront\":";

    payload +=
        String(
            frontDistance
        );

    payload +=
        ",\"ultrasonicRear\":";

    payload +=
        String(
            rearDistance
        );

    payload +=
        ",\"linePosition\":";

    payload +=
        String(
            lastLinePosition,
            2
        );

    payload +=
        ",\"ir\":{";

    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        payload +=
            "\"S";

        payload +=
            String(
                i + 1
            );

        payload +=
            "\":";

        payload +=
            String(
                sensorValues[i]
            );

        if (
            i < 7
        )
        {
            payload += ",";
        }
    }

    payload +=
        "}}";

    mqttClient.publish(
        TELEMETRY_TOPIC,
        payload.c_str(),
        false
    );
}

// ============================================================
// MQTT CALLBACK
// ============================================================

void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
)
{
    String message = "";

    for (
        unsigned int i = 0;
        i < length;
        i++
    )
    {
        message +=
            (char)payload[i];
    }

    Serial.print(
        "MQTT COMMAND: "
    );

    Serial.println(
        message
    );

    if (
        String(topic) ==
        COMMAND_TOPIC
    )
    {
        processCommand(
            message
        );
    }
}

// ============================================================
// COMMAND PROCESSING
// ============================================================

void processCommand(
    String command
)
{
    command.trim();

    String upper =
        command;

    upper.toUpperCase();

    // ========================================================
    // START
    // ========================================================

    if (
        upper == "START" ||
        upper.indexOf(
            "\"COMMAND\":\"START\""
        ) >= 0
    )
    {
        stopMotors();

        cycleActive =
            true;

        lineFollowing =
            false;

        state =
            WAITING_FOR_LOAD;

        Serial.println();
        Serial.println(
            "START RECEIVED"
        );
        Serial.println(
            "WAITING FOR 2 KG"
        );

        publishEvent(
            "START RECEIVED - WAITING FOR 2 KG"
        );

        return;
    }

    // ========================================================
    // STOP
    // ========================================================

    if (
        upper == "STOP" ||
        upper.indexOf(
            "\"COMMAND\":\"STOP\""
        ) >= 0
    )
    {
        cycleActive =
            false;

        lineFollowing =
            false;

        stopMotors();

        publishEvent(
            "AGV STOPPED"
        );

        return;
    }

    // ========================================================
    // RESUME
    // ========================================================

    if (
        upper == "RESUME" ||
        upper.indexOf(
            "\"COMMAND\":\"RESUME\""
        ) >= 0
    )
    {
        cycleActive =
            true;

        publishEvent(
            "AGV RESUMED"
        );

        return;
    }

    // ========================================================
    // RESET
    // ========================================================

    if (
        upper == "RESET" ||
        upper.indexOf(
            "\"COMMAND\":\"RESET\""
        ) >= 0
    )
    {
        stopMotors();

        cycleActive =
            false;

        lineFollowing =
            false;

        state =
            WAITING_FOR_LOAD;

        currentWeight =
            readWeight();

        publishEvent(
            "AGV RESET - WAITING FOR 2 KG"
        );

        return;
    }

    // ========================================================
    // MANUAL FORWARD
    // ========================================================

    if (
        upper == "MANUAL_FORWARD" ||
        upper == "FORWARD"
    )
    {
        cycleActive =
            false;

        lineFollowing =
            false;

        manualForward();

        return;
    }

    // ========================================================
    // MANUAL BACKWARD
    // ========================================================

    if (
        upper == "MANUAL_BACKWARD" ||
        upper == "BACKWARD"
    )
    {
        cycleActive =
            false;

        lineFollowing =
            false;

        manualBackward();

        return;
    }

    // ========================================================
    // MANUAL LEFT
    // ========================================================

    if (
        upper == "MANUAL_LEFT" ||
        upper == "LEFT"
    )
    {
        cycleActive =
            false;

        lineFollowing =
            false;

        manualLeft();

        return;
    }

    // ========================================================
    // MANUAL RIGHT
    // ========================================================

    if (
        upper == "MANUAL_RIGHT" ||
        upper == "RIGHT"
    )
    {
        cycleActive =
            false;

        lineFollowing =
            false;

        manualRight();

        return;
    }

    // ========================================================
    // MANUAL STOP
    // ========================================================

    if (
        upper == "MANUAL_STOP"
    )
    {
        stopMotors();

        lineFollowing =
            false;

        return;
    }
}

// ============================================================
// WIFI
// ============================================================

void connectWiFi()
{
    Serial.println();
    Serial.println(
        "Connecting to WiFi..."
    );

    WiFi.mode(
        WIFI_STA
    );

    WiFi.setSleep(
        false
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    int attempts = 0;

    while (
        WiFi.status() !=
        WL_CONNECTED &&
        attempts < 40
    )
    {
        delay(500);

        Serial.print(
            "."
        );

        attempts++;
    }

    Serial.println();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println(
            "WiFi connected"
        );

        Serial.print(
            "IP: "
        );

        Serial.println(
            WiFi.localIP()
        );
    }
    else
    {
        Serial.println(
            "WiFi connection failed"
        );
    }
}

// ============================================================
// MQTT
// ============================================================

bool connectMQTT()
{
    if (
        mqttClient.connected()
    )
    {
        return true;
    }

    String clientID =
        "AGV_01_";

    clientID +=
        String(
            (uint32_t)
            ESP.getEfuseMac(),
            HEX
        );

    Serial.println(
        "Connecting to HiveMQ..."
    );

    bool connected =
        mqttClient.connect(
            clientID.c_str(),
            MQTT_USERNAME,
            MQTT_PASSWORD
        );

    if (
        connected
    )
    {
        Serial.println(
            "HiveMQ connected"
        );

        mqttClient.subscribe(
            COMMAND_TOPIC,
            1
        );

        Serial.print(
            "Subscribed to: "
        );

        Serial.println(
            COMMAND_TOPIC
        );

        publishEvent(
            "ESP32 CONNECTED"
        );

        return true;
    }

    Serial.print(
        "MQTT failed: "
    );

    Serial.println(
        mqttClient.state()
    );

    return false;
}

// ============================================================
// MQTT MAINTENANCE
// ============================================================

void maintainMQTT()
{
    if (
        mqttClient.connected()
    )
    {
        mqttClient.loop();

        return;
    }

    if (
        millis() -
        lastMQTTAttempt <
        3000
    )
    {
        return;
    }

    lastMQTTAttempt =
        millis();

    connectMQTT();
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(
        115200
    );

    delay(1000);

    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        "AGV PHASE 3 START"
    );
    Serial.println(
        "================================"
    );

    // --------------------------------------------------------
    // IR
    // --------------------------------------------------------

    for (
        int i = 0;
        i < 8;
        i++
    )
    {
        pinMode(
            IR_PINS[i],
            INPUT
        );
    }

    // --------------------------------------------------------
    // MOTOR PWM
    // --------------------------------------------------------

    ledcAttach(
        LEFT_RPWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    ledcAttach(
        LEFT_LPWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    ledcAttach(
        RIGHT_RPWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    ledcAttach(
        RIGHT_LPWM,
        PWM_FREQUENCY,
        PWM_RESOLUTION
    );

    stopMotors();

    // --------------------------------------------------------
    // ULTRASONIC
    // --------------------------------------------------------

    pinMode(
        ULTRASONIC_TRIG,
        OUTPUT
    );

    pinMode(
        FRONT_ECHO,
        INPUT
    );

    pinMode(
        REAR_ECHO,
        INPUT
    );

    digitalWrite(
        ULTRASONIC_TRIG,
        LOW
    );

    // --------------------------------------------------------
    // HX711
    // --------------------------------------------------------

    scale.begin(
        HX711_DT,
        HX711_SCK
    );

    scale.set_scale(
        CALIBRATION_FACTOR
    );

    delay(1500);

    scale.tare();

    Serial.println(
        "HX711 ready"
    );

    // --------------------------------------------------------
    // WIFI
    // --------------------------------------------------------

    connectWiFi();

    // --------------------------------------------------------
    // MQTT TLS
    // --------------------------------------------------------

    secureClient.setInsecure();

    mqttClient.setServer(
        MQTT_HOST,
        MQTT_PORT
    );

    mqttClient.setCallback(
        mqttCallback
    );

    mqttClient.setKeepAlive(
        30
    );

    mqttClient.setBufferSize(
        1024
    );

    Serial.println(
        "AGV READY"
    );

    Serial.println(
        "Waiting for START..."
    );
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // ========================================================
    // WIFI
    // ========================================================

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        stopMotors();

        lineFollowing =
            false;

        connectWiFi();

        return;
    }

    // ========================================================
    // MQTT
    // ========================================================

    maintainMQTT();

    if (
        !mqttClient.connected()
    )
    {
        stopMotors();

        lineFollowing =
            false;
    }

    // ========================================================
    // WEIGHT
    // ========================================================

    currentWeight =
        readWeight();

    // ========================================================
    // ULTRASONIC
    // ========================================================

    if (
        millis() -
        lastUltrasonic >=
        300
    )
    {
        lastUltrasonic =
            millis();

        frontDistance =
            getDistance(
                FRONT_ECHO
            );

        rearDistance =
            getDistance(
                REAR_ECHO
            );
    }

    // ========================================================
    // AUTOMATIC CYCLE
    // ========================================================

    if (
        cycleActive &&
        mqttClient.connected()
    )
    {
        // ====================================================
        // WAIT FOR 2 KG
        // ====================================================

        if (
            state ==
            WAITING_FOR_LOAD
        )
        {
            stopMotors();

            lineFollowing =
                false;

            if (
                currentWeight >=
                LOAD_REQUIRED_KG
            )
            {
                state =
                    MOVING_FORWARD;

                lineFollowing =
                    true;

                Serial.println();
                Serial.println(
                    "================================"
                );
                Serial.println(
                    "2 KG LOAD DETECTED"
                );
                Serial.println(
                    "FORWARD LINE FOLLOWING STARTED"
                );
                Serial.println(
                    "================================"
                );

                publishEvent(
                    "2 KG LOAD DETECTED - FORWARD START"
                );

                delay(300);
            }
        }

        // ====================================================
        // FORWARD
        // ====================================================

        else if (
            state ==
            MOVING_FORWARD
        )
        {
            lineFollowing =
                true;

            if (
                frontDistance <=
                OBSTACLE_DISTANCE
            )
            {
                stopMotors();

                lineFollowing =
                    false;

                publishEvent(
                    "FRONT OBSTACLE - AGV STOPPED"
                );
            }
            else
            {
                lineFollowForward();
            }
        }

        // ====================================================
        // WAIT FOR LOAD REMOVAL
        // ====================================================

        else if (
            state ==
            WAITING_FOR_REMOVAL
        )
        {
            stopMotors();

            lineFollowing =
                false;

            // ------------------------------------------------
            // IMPORTANT:
            //
            // Reference logic uses <= 0.5 kg.
            // ------------------------------------------------

            if (
                currentWeight <=
                LOAD_REMOVED_KG
            )
            {
                Serial.println();
                Serial.println(
                    "================================"
                );
                Serial.println(
                    "LOAD REMOVED"
                );
                Serial.println(
                    "REVERSE STRAIGHT 1 SECOND"
                );
                Serial.println(
                    "================================"
                );

                publishEvent(
                    "WEIGHT REMOVED - REVERSING"
                );

                // --------------------------------------------
                // INITIAL REVERSE
                // --------------------------------------------

                drive(
                    -STRAIGHT_LEFT_PWM,
                    -STRAIGHT_RIGHT_PWM
                );

                delay(
                    INITIAL_REVERSE_TIME
                );

                // --------------------------------------------
                // SHORT STOP
                // --------------------------------------------

                stopMotors();

                delay(
                    REVERSE_STOP_TIME
                );

                // --------------------------------------------
                // BACKWARD LFR
                // --------------------------------------------

                state =
                    MOVING_BACKWARD;

                lineFollowing =
                    true;

                Serial.println();
                Serial.println(
                    "================================"
                );
                Serial.println(
                    "BACKTRACKING STARTED"
                );
                Serial.println(
                    "================================"
                );

                publishEvent(
                    "BACKTRACKING STARTED"
                );
            }
        }

        // ====================================================
        // BACKTRACKING
        // ====================================================

        else if (
            state ==
            MOVING_BACKWARD
        )
        {
            lineFollowing =
                true;

            if (
                rearDistance <=
                OBSTACLE_DISTANCE
            )
            {
                stopMotors();

                lineFollowing =
                    false;

                publishEvent(
                    "REAR OBSTACLE - AGV STOPPED"
                );
            }
            else
            {
                lineFollowBackward();
            }
        }

        // ====================================================
        // COMPLETE
        // ====================================================

        else if (
            state ==
            COMPLETE
        )
        {
            stopMotors();

            lineFollowing =
                false;

            cycleActive =
                false;
        }
    }

    // ========================================================
    // STATUS
    // ========================================================

    if (
        millis() -
        lastStatus >=
        500
    )
    {
        lastStatus =
            millis();

        readIRSensors();

        publishStatus();
    }

    // ========================================================
    // TELEMETRY
    // ========================================================

    if (
        millis() -
        lastTelemetry >=
        250
    )
    {
        lastTelemetry =
            millis();

        readIRSensors();

        publishTelemetry();
    }

    delay(2);
}