// ======================================================
// AGV PHASE 3
// ESP32 + HiveMQ + 8 ARRAY IR + BTS7960
// ======================================================


// ======================================================
// LIBRARIES
// ======================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>


// ======================================================
// WIFI
// ======================================================

const char* WIFI_SSID =
    "Nothing Phone (2)_1932";

const char* WIFI_PASSWORD =
    "12345678";


// ======================================================
// HIVEMQ CLOUD
// ======================================================

const char* MQTT_HOST =
    "e61e359e1edb4ae3ab97dce7981729b8.s1.eu.hivemq.cloud";

const int MQTT_PORT =
    8883;

const char* MQTT_USERNAME =
    "Major_AGV";

const char* MQTT_PASSWORD =
    "Major_AGV";



// ======================================================
// MQTT TOPICS
// ======================================================

const char* COMMAND_TOPIC =
    "agv/AGV_01/command";

const char* STATUS_TOPIC =
    "agv/AGV_01/status";

const char* TELEMETRY_TOPIC =
    "agv/AGV_01/telemetry";

const char* EVENT_TOPIC =
    "agv/AGV_01/event";


// ======================================================
// IR SENSOR PINS
// ======================================================
// KEEPING YOUR EXISTING CONNECTIONS
//
// S1 = GPIO14
// S2 = GPIO27
// S3 = GPIO26
// S4 = GPIO25
// S5 = GPIO33
// S6 = GPIO32
// S7 = GPIO35
// S8 = GPIO34
//
// ======================================================

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


// ======================================================
// BTS7960 PINS
// ======================================================
// KEEPING YOUR EXISTING CONNECTIONS
//
// LEFT:
// RPWM = GPIO19
// LPWM = GPIO18
//
// RIGHT:
// RPWM = GPIO4
// LPWM = GPIO15
//
// ======================================================

const int LEFT_RPWM =
    19;

const int LEFT_LPWM =
    18;

const int RIGHT_RPWM =
    4;

const int RIGHT_LPWM =
    15;


// ======================================================
// PWM
// ======================================================

const int PWM_FREQUENCY =
    20000;

const int PWM_RESOLUTION =
    8;


// ======================================================
// LINE FOLLOWING SPEEDS
// TAKEN FROM UPLOADED LINE-FOLLOWING LOGIC
// ======================================================

// STRAIGHT

const int STRAIGHT_LEFT_PWM =
    75;

const int STRAIGHT_RIGHT_PWM =
    75;


// SMALL RIGHT

const int SMALL_RIGHT_LEFT_PWM =
    70;

const int SMALL_RIGHT_RIGHT_PWM =
    62;


// MEDIUM RIGHT

const int MEDIUM_RIGHT_LEFT_PWM =
    85;

const int MEDIUM_RIGHT_RIGHT_PWM =
    45;


// SMALL LEFT

const int SMALL_LEFT_LEFT_PWM =
    62;

const int SMALL_LEFT_RIGHT_PWM =
    70;


// MEDIUM LEFT

const int MEDIUM_LEFT_LEFT_PWM =
    45;

const int MEDIUM_LEFT_RIGHT_PWM =
    85;


// ======================================================
// SHARP TURN SPEED
// ======================================================

const int SHARP_RIGHT_FORWARD =
    110;

const int SHARP_RIGHT_REVERSE =
    -110;

const int SHARP_LEFT_REVERSE =
    -110;

const int SHARP_LEFT_FORWARD =
    110;


// ======================================================
// TIMING
// ======================================================

const unsigned long STRAIGHT_DELAY =
    20;

const unsigned long SMALL_TURN_DELAY =
    50;

const unsigned long MEDIUM_TURN_DELAY =
    100;


// SHARP TURN

const unsigned long SHARP_TURN_DELAY =
    1715;


// MOVE FORWARD AFTER SHARP TURN

const unsigned long FORWARD_SEARCH_DELAY =
    800;


// ALL WHITE

const unsigned long STOP_DELAY =
    100;


// ======================================================
// SHARP TURN LOCK
// ======================================================

bool sharpTurnAllowed =
    true;


// ======================================================
// SENSOR VALUES
// ======================================================

int sensorValues[8] = {

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0

};


// ======================================================
// LINE FOLLOWING
// ======================================================

bool lineFollowing =
    false;


// ======================================================
// LAST LINE POSITION
// ======================================================

float lastLinePosition =
    0;


// ======================================================
// NETWORK
// ======================================================

WiFiClientSecure secureClient;

PubSubClient mqttClient(
    secureClient
);


// ======================================================
// TIMERS
// ======================================================

unsigned long lastMQTTAttempt =
    0;

unsigned long lastHeartbeat =
    0;

unsigned long lastTelemetry =
    0;


// ======================================================
// FUNCTION DECLARATIONS
// ======================================================

void connectWiFi();

bool connectMQTT();

void maintainMQTT();

void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
);

void processCommand(
    String message
);

void publishEvent(
    const char* event
);

void publishStatus();

void publishTelemetry();

void readIRSensors();

void lineFollow();

void stopMotors();

void setLeftMotor(
    int speed
);

void setRightMotor(
    int speed
);

void drive(
    int leftPWM,
    int rightPWM
);

void sharpLeftTurn();

void sharpRightTurn();

void manualForward();

void manualBackward();

void manualLeft();

void manualRight();


// ======================================================
// LEFT MOTOR
// ======================================================

void setLeftMotor(
    int speed
) {

    speed =
        constrain(
            speed,
            -180,
            180
        );


    if (
        speed > 0
    ) {

        ledcWrite(
            LEFT_RPWM,
            speed
        );

        ledcWrite(
            LEFT_LPWM,
            0
        );

    }

    else if (
        speed < 0
    ) {

        ledcWrite(
            LEFT_RPWM,
            0
        );

        ledcWrite(
            LEFT_LPWM,
            -speed
        );

    }

    else {

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


// ======================================================
// RIGHT MOTOR
// ======================================================

void setRightMotor(
    int speed
) {

    speed =
        constrain(
            speed,
            -180,
            180
        );


    if (
        speed > 0
    ) {

        ledcWrite(
            RIGHT_RPWM,
            speed
        );

        ledcWrite(
            RIGHT_LPWM,
            0
        );

    }

    else if (
        speed < 0
    ) {

        ledcWrite(
            RIGHT_RPWM,
            0
        );

        ledcWrite(
            RIGHT_LPWM,
            -speed
        );

    }

    else {

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


// ======================================================
// DRIVE
// ======================================================

void drive(
    int leftPWM,
    int rightPWM
) {

    setLeftMotor(
        leftPWM
    );

    setRightMotor(
        rightPWM
    );

}


// ======================================================
// STOP
// ======================================================

void stopMotors() {

    setLeftMotor(
        0
    );

    setRightMotor(
        0
    );

}


// ======================================================
// READ IR
// ======================================================

void readIRSensors() {

    for (
        int i = 0;
        i < 8;
        i++
    ) {

        sensorValues[i] =
            digitalRead(
                IR_PINS[i]
            );

    }

}


// ======================================================
// SHARP LEFT TURN
// ======================================================
// LOGIC FROM UPLOADED FILE
//
// LEFT MOTOR  = REVERSE
// RIGHT MOTOR = FORWARD
//
// AFTER TURN:
// MOVE FORWARD
// LOCK SHARP TURN
// ======================================================

void sharpLeftTurn() {

    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "        SHARP LEFT"
    );

    Serial.println(
        "================================"
    );


    // PIVOT LEFT

    drive(
        -110,
        110
    );


    delay(
        SHARP_TURN_DELAY
    );


    // --------------------------------------------------
    // FORWARD SEARCH
    // --------------------------------------------------

    Serial.println(
        "FORWARD AFTER LEFT TURN"
    );


    drive(
        75,
        75
    );


    delay(
        FORWARD_SEARCH_DELAY
    );


    // --------------------------------------------------
    // LOCK
    // --------------------------------------------------

    sharpTurnAllowed =
        false;


    Serial.println(
        "SHARP TURN LOCKED"
    );

}


// ======================================================
// SHARP RIGHT TURN
// ======================================================
// LOGIC FROM UPLOADED FILE
//
// LEFT MOTOR  = FORWARD
// RIGHT MOTOR = REVERSE
//
// ======================================================

void sharpRightTurn() {

    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "        SHARP RIGHT"
    );

    Serial.println(
        "================================"
    );


    // PIVOT RIGHT

    drive(
        110,
        -110
    );


    delay(
        SHARP_TURN_DELAY
    );


    // --------------------------------------------------
    // FORWARD SEARCH
    // --------------------------------------------------

    Serial.println(
        "FORWARD AFTER RIGHT TURN"
    );


    drive(
        75,
        75
    );


    delay(
        FORWARD_SEARCH_DELAY
    );


    // --------------------------------------------------
    // LOCK
    // --------------------------------------------------

    sharpTurnAllowed =
        false;


    Serial.println(
        "SHARP TURN LOCKED"
    );

}


// ======================================================
// NEW LINE FOLLOWING LOGIC
// ======================================================
// THIS SECTION IS THE IMPORTANT PART.
//
// LOGIC TAKEN FROM YOUR UPLOADED FILE.
//
// CONNECTIONS ARE NOT FROM THE UPLOADED FILE.
// ======================================================

void lineFollow() {

    readIRSensors();


    int activeCount =
        0;


    // ==================================================
    // COUNT ACTIVE SENSORS
    // ==================================================

    for (
        int i = 0;
        i < 8;
        i++
    ) {

        if (
            sensorValues[i] ==
            HIGH
        ) {

            activeCount++;

        }

    }


    // ==================================================
    // RE-ARM SHARP TURN
    // ==================================================
    //
    // S4 OR S5 MUST SEE THE LINE.
    //
    // ==================================================

    if (
        sensorValues[3] ==
            HIGH
        ||
        sensorValues[4] ==
            HIGH
    ) {

        if (
            !sharpTurnAllowed
        ) {

            sharpTurnAllowed =
                true;


            Serial.println(
                "CENTER LINE FOUND"
            );

            Serial.println(
                "SHARP TURN RE-ARMED"
            );

        }

    }


    // ==================================================
    // ALL WHITE / NO LINE
    // ==================================================

    if (
        activeCount == 0
    ) {

        Serial.println(
            "NO LINE -> STOP"
        );


        stopMotors();


        delay(
            STOP_DELAY
        );


        return;

    }


    // ==================================================
    // SHARP LEFT
    // ==================================================
    //
    // S7 + S8 BOTH HIGH
    //
    // ==================================================

    if (
        sharpTurnAllowed
        &&
        sensorValues[6] ==
            HIGH
        &&
        sensorValues[7] ==
            HIGH
    ) {

        sharpLeftTurn();


        return;

    }


    // ==================================================
    // SHARP RIGHT
    // ==================================================
    //
    // S1 + S2 BOTH HIGH
    //
    // ==================================================

    if (
        sharpTurnAllowed
        &&
        sensorValues[0] ==
            HIGH
        &&
        sensorValues[1] ==
            HIGH
    ) {

        sharpRightTurn();


        return;

    }


    // ==================================================
    // POSITION WEIGHTS
    // ==================================================

    const int positionWeight[8] = {

        -7,
        -5,
        -3,
        -1,
         1,
         3,
         5,
         7

    };


    int weightedSum =
        0;


    for (
        int i = 0;
        i < 8;
        i++
    ) {

        if (
            sensorValues[i] ==
            HIGH
        ) {

            weightedSum +=
                positionWeight[i];

        }

    }


    float position =
        (float)weightedSum /
        activeCount;


    lastLinePosition =
        position;


    Serial.print(
        "SENSORS: "
    );


    for (
        int i = 0;
        i < 8;
        i++
    ) {

        Serial.print(
            sensorValues[i]
        );

    }


    Serial.print(
        "  POSITION = "
    );


    Serial.println(
        position
    );


    // ==================================================
    // STRAIGHT
    // ==================================================
    //
    // CENTER
    //
    // ==================================================

    if (
        position >= -1.0
        &&
        position <= 1.0
    ) {

        Serial.println(
            "STRAIGHT"
        );


        drive(
            STRAIGHT_LEFT_PWM,
            STRAIGHT_RIGHT_PWM
        );


        delay(
            STRAIGHT_DELAY
        );

    }


    // ==================================================
    // SMALL RIGHT
    // ==================================================

    else if (
        position < -1.0
        &&
        position >= -2.0
    ) {

        Serial.println(
            "SMALL RIGHT"
        );


        drive(
            SMALL_RIGHT_LEFT_PWM,
            SMALL_RIGHT_RIGHT_PWM
        );


        delay(
            SMALL_TURN_DELAY
        );

    }


    // ==================================================
    // MEDIUM RIGHT
    // ==================================================

    else if (
        position < -2.0
        &&
        position >= -4.0
    ) {

        Serial.println(
            "MEDIUM RIGHT"
        );


        drive(
            MEDIUM_RIGHT_LEFT_PWM,
            MEDIUM_RIGHT_RIGHT_PWM
        );


        delay(
            MEDIUM_TURN_DELAY
        );

    }


    // ==================================================
    // STRONG RIGHT
    // ==================================================
    //
    // IMPORTANT:
    // This is NOT a sharp turn.
    //
    // Sharp right only happens with S1 + S2.
    //
    // ==================================================

    else if (
        position < -4.0
    ) {

        Serial.println(
            "STRONG RIGHT - NORMAL FOLLOWING"
        );


        drive(
            90,
            35
        );


        delay(
            MEDIUM_TURN_DELAY
        );

    }


    // ==================================================
    // SMALL LEFT
    // ==================================================

    else if (
        position > 1.0
        &&
        position <= 2.0
    ) {

        Serial.println(
            "SMALL LEFT"
        );


        drive(
            SMALL_LEFT_LEFT_PWM,
            SMALL_LEFT_RIGHT_PWM
        );


        delay(
            SMALL_TURN_DELAY
        );

    }


    // ==================================================
    // MEDIUM LEFT
    // ==================================================

    else if (
        position > 2.0
        &&
        position <= 4.0
    ) {

        Serial.println(
            "MEDIUM LEFT"
        );


        drive(
            MEDIUM_LEFT_LEFT_PWM,
            MEDIUM_LEFT_RIGHT_PWM
        );


        delay(
            MEDIUM_TURN_DELAY
        );

    }


    // ==================================================
    // STRONG LEFT
    // ==================================================
    //
    // IMPORTANT:
    // This is NOT a sharp turn.
    //
    // Sharp left only happens with S7 + S8.
    //
    // ==================================================

    else if (
        position > 4.0
    ) {

        Serial.println(
            "STRONG LEFT - NORMAL FOLLOWING"
        );


        drive(
            35,
            90
        );


        delay(
            MEDIUM_TURN_DELAY
        );

    }

}


// ======================================================
// MANUAL FORWARD
// ======================================================

void manualForward() {

    drive(
        100,
        100
    );

}


// ======================================================
// MANUAL BACKWARD
// ======================================================

void manualBackward() {

    drive(
        -100,
        -100
    );

}


// ======================================================
// MANUAL LEFT
// ======================================================

void manualLeft() {

    drive(
        33,
        100
    );

}


// ======================================================
// MANUAL RIGHT
// ======================================================

void manualRight() {

    drive(
        100,
        33
    );

}


// ======================================================
// MQTT COMMAND PROCESSING
// ======================================================

void processCommand(
    String message
) {

    message.trim();


    String command =
        message;


    command.trim();

    command.toUpperCase();


    Serial.println();

    Serial.println(
        "================================"
    );

    Serial.print(
        "RAW COMMAND RECEIVED: "
    );

    Serial.println(
        message
    );


    // ==================================================
    // START
    // ==================================================

    if (
        command == "START"
        ||
        command == "START_PATH"
        ||
        message.indexOf(
            "\"command\":\"START\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"START_PATH\""
        ) >= 0
    ) {

        lineFollowing =
            true;


        lastLinePosition =
            0;


        // START = fresh run

        sharpTurnAllowed =
            true;


        Serial.println(
            "COMMAND = START"
        );

        Serial.println(
            "LINE FOLLOWING STARTED"
        );


        publishEvent(
            "LINE FOLLOWING STARTED"
        );


        return;

    }


    // ==================================================
    // RESUME
    // ==================================================

    if (
        command == "RESUME"
        ||
        message.indexOf(
            "\"command\":\"RESUME\""
        ) >= 0
    ) {

        lineFollowing =
            true;


        Serial.println(
            "COMMAND = RESUME"
        );

        Serial.println(
            "LINE FOLLOWING RESUMED"
        );


        publishEvent(
            "LINE FOLLOWING RESUMED"
        );


        return;

    }


    // ==================================================
    // STOP
    // ==================================================

    if (
        command == "STOP"
        ||
        message.indexOf(
            "\"command\":\"STOP\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        stopMotors();


        Serial.println(
            "COMMAND = STOP"
        );


        publishEvent(
            "AGV STOPPED"
        );


        return;

    }


    // ==================================================
    // RESET
    // ==================================================

    if (
        command == "RESET"
        ||
        message.indexOf(
            "\"command\":\"RESET\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        stopMotors();


        lastLinePosition =
            0;


        sharpTurnAllowed =
            true;


        Serial.println(
            "COMMAND = RESET"
        );


        publishEvent(
            "AGV RESET"
        );


        return;

    }


    // ==================================================
    // MANUAL FORWARD
    // ==================================================

    if (
        command == "FORWARD"
        ||
        command == "MANUAL_FORWARD"
        ||
        message.indexOf(
            "\"command\":\"FORWARD\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"MANUAL_FORWARD\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        manualForward();


        publishEvent(
            "MANUAL FORWARD"
        );


        return;

    }


    // ==================================================
    // MANUAL BACKWARD
    // ==================================================

    if (
        command == "BACKWARD"
        ||
        command == "MANUAL_BACKWARD"
        ||
        message.indexOf(
            "\"command\":\"BACKWARD\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"MANUAL_BACKWARD\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        manualBackward();


        publishEvent(
            "MANUAL BACKWARD"
        );


        return;

    }


    // ==================================================
    // MANUAL LEFT
    // ==================================================

    if (
        command == "LEFT"
        ||
        command == "MANUAL_LEFT"
        ||
        message.indexOf(
            "\"command\":\"LEFT\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"MANUAL_LEFT\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        manualLeft();


        publishEvent(
            "MANUAL LEFT"
        );


        return;

    }


    // ==================================================
    // MANUAL RIGHT
    // ==================================================

    if (
        command == "RIGHT"
        ||
        command == "MANUAL_RIGHT"
        ||
        message.indexOf(
            "\"command\":\"RIGHT\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"MANUAL_RIGHT\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        manualRight();


        publishEvent(
            "MANUAL RIGHT"
        );


        return;

    }


    // ==================================================
    // MANUAL STOP
    // ==================================================

    if (
        command == "MANUAL_STOP"
        ||
        command == "STOP_MANUAL"
        ||
        message.indexOf(
            "\"command\":\"MANUAL_STOP\""
        ) >= 0
        ||
        message.indexOf(
            "\"command\":\"STOP_MANUAL\""
        ) >= 0
    ) {

        lineFollowing =
            false;


        stopMotors();


        publishEvent(
            "MANUAL STOP"
        );


        return;

    }


    // ==================================================
    // UNKNOWN
    // ==================================================

    Serial.println(
        "UNKNOWN MQTT COMMAND"
    );

}


// ======================================================
// MQTT CALLBACK
// ======================================================

void mqttCallback(
    char* topic,
    byte* payload,
    unsigned int length
) {

    String message =
        "";


    for (
        unsigned int i = 0;
        i < length;
        i++
    ) {

        message +=
            (char)payload[i];

    }


    Serial.print(
        "MQTT MESSAGE: "
    );

    Serial.println(
        message
    );


    if (
        String(topic) ==
        COMMAND_TOPIC
    ) {

        processCommand(
            message
        );

    }

}


// ======================================================
// PUBLISH EVENT
// ======================================================

void publishEvent(
    const char* event
) {

    if (
        !mqttClient.connected()
    ) {

        return;

    }


    String payload =
        "{\"event\":\"";


    payload +=
        event;


    payload +=
        "\"}";


    mqttClient.publish(
        EVENT_TOPIC,
        payload.c_str(),
        false
    );

}


// ======================================================
// PUBLISH STATUS
// ======================================================

void publishStatus() {

    if (
        !mqttClient.connected()
    ) {

        return;

    }


    String payload =
        "{";


    payload +=
        "\"state\":\"";


    payload +=
        lineFollowing
            ? "LINE_FOLLOWING"
            : "IDLE";


    payload +=
        "\",";


    payload +=
        "\"lineFollowing\":";


    payload +=
        lineFollowing
            ? "true"
            : "false";


    payload +=
        ",\"ir\":{";


    for (
        int i = 0;
        i < 8;
        i++
    ) {

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
        ) {

            payload +=
                ",";

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


// ======================================================
// PUBLISH TELEMETRY
// ======================================================

void publishTelemetry() {

    if (
        !mqttClient.connected()
    ) {

        return;

    }


    String payload =
        "{";


    payload +=
        "\"state\":\"";


    payload +=
        lineFollowing
            ? "LINE_FOLLOWING"
            : "IDLE";


    payload +=
        "\",";


    payload +=
        "\"lineFollowing\":";


    payload +=
        lineFollowing
            ? "true"
            : "false";


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
    ) {

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
        ) {

            payload +=
                ",";

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


// ======================================================
// WIFI
// ======================================================

void connectWiFi() {

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


    int attempts =
        0;


    while (
        WiFi.status() !=
        WL_CONNECTED
        &&
        attempts < 40
    ) {

        delay(
            500
        );


        Serial.print(
            "."
        );


        attempts++;

    }


    Serial.println();


    if (
        WiFi.status() ==
        WL_CONNECTED
    ) {

        Serial.println(
            "WIFI CONNECTED"
        );


        Serial.print(
            "IP: "
        );


        Serial.println(
            WiFi.localIP()
        );

    }

    else {

        Serial.println(
            "WIFI CONNECTION FAILED"
        );

    }

}


// ======================================================
// MQTT CONNECT
// ======================================================

bool connectMQTT() {

    if (
        mqttClient.connected()
    ) {

        return true;

    }


    Serial.println();

    Serial.println(
        "Connecting to HiveMQ..."
    );


    String clientID =
        "AGV_01_";


    clientID +=
        String(
            (uint32_t)
            ESP.getEfuseMac(),
            HEX
        );


    bool connected =
        mqttClient.connect(
            clientID.c_str(),
            MQTT_USERNAME,
            MQTT_PASSWORD
        );


    if (
        connected
    ) {

        Serial.println(
            "HIVEMQ CONNECTED"
        );


        bool subscribed =
            mqttClient.subscribe(
                COMMAND_TOPIC,
                1
            );


        if (
            subscribed
        ) {

            Serial.println(
                "COMMAND TOPIC SUBSCRIBED"
            );

        }


        publishEvent(
            "ESP32 CONNECTED"
        );


        publishStatus();


        return true;

    }


    Serial.print(
        "MQTT CONNECTION FAILED: "
    );


    Serial.println(
        mqttClient.state()
    );


    return false;

}


// ======================================================
// MQTT MAINTENANCE
// ======================================================

void maintainMQTT() {

    if (
        WiFi.status() !=
        WL_CONNECTED
    ) {

        return;

    }


    if (
        mqttClient.connected()
    ) {

        mqttClient.loop();

        return;

    }


    if (
        millis() -
        lastMQTTAttempt <
        3000
    ) {

        return;

    }


    lastMQTTAttempt =
        millis();


    connectMQTT();

}


// ======================================================
// SETUP
// ======================================================

void setup() {

    Serial.begin(
        115200
    );


    delay(
        1000
    );


    Serial.println();

    Serial.println(
        "=========================================="
    );

    Serial.println(
        "          AGV PHASE 3 START"
    );

    Serial.println(
        "=========================================="
    );


    // ==================================================
    // IR
    // ==================================================

    for (
        int i = 0;
        i < 8;
        i++
    ) {

        pinMode(
            IR_PINS[i],
            INPUT
        );

    }


    // ==================================================
    // MOTOR PWM
    // ==================================================

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


    // ==================================================
    // WIFI
    // ==================================================

    connectWiFi();


    // ==================================================
    // MQTT
    // ==================================================

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


    Serial.println();

    Serial.println(
        "AGV READY"
    );

}


// ======================================================
// LOOP
// ======================================================

void loop() {

    // ==================================================
    // WIFI
    // ==================================================

    if (
        WiFi.status() !=
        WL_CONNECTED
    ) {

        lineFollowing =
            false;


        stopMotors();


        connectWiFi();


        return;

    }


    // ==================================================
    // MQTT
    // ==================================================

    maintainMQTT();


    // ==================================================
    // MQTT SAFETY
    // ==================================================

    if (
        !mqttClient.connected()
    ) {

        lineFollowing =
            false;


        stopMotors();

    }

    else {

        // ==================================================
        // LINE FOLLOWING
        // ==================================================

        if (
            lineFollowing
        ) {

            lineFollow();

        }

    }


    // ==================================================
    // STATUS
    // ==================================================

    if (
        millis() -
        lastHeartbeat >=
        1000
    ) {

        lastHeartbeat =
            millis();


        readIRSensors();


        publishStatus();

    }


    // ==================================================
    // TELEMETRY
    // ==================================================

    if (
        millis() -
        lastTelemetry >=
        200
    ) {

        lastTelemetry =
            millis();


        readIRSensors();


        publishTelemetry();

    }


    delay(2);

}