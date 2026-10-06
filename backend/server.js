const express = require("express");
const mqtt = require("mqtt");
const path = require("path");

const app = express();

const PORT = 3000;

// ======================================================
// HIVEMQ CLOUD
// ======================================================

const MQTT_BROKER =
    "mqtts://e61e359e1edb4ae3ab97dce7981729b8.s1.eu.hivemq.cloud:8883";

const MQTT_USERNAME =
    "Major_AGV";

const MQTT_PASSWORD =
    "Major_AGV";

const AGV_ID =
    "AGV_01";

// ======================================================
// MQTT TOPICS
// ======================================================

const COMMAND_TOPIC =
    `agv/${AGV_ID}/command`;

const STATUS_TOPIC =
    `agv/${AGV_ID}/status`;

const TELEMETRY_TOPIC =
    `agv/${AGV_ID}/telemetry`;

const EVENT_TOPIC =
    `agv/${AGV_ID}/event`;

// ======================================================
// EXPRESS
// ======================================================

app.use(
    express.json()
);

app.use(
    express.static(
        path.join(
            __dirname,
            "public"
        )
    )
);

// ======================================================
// AGV STATUS
// ======================================================

let agvStatus = {

    mqttConnected: false,

    espConnected: false,

    state: "OFFLINE",

    lineFollowing: false,

    cycleActive: false,

    weight: 0,

    weightKg: 0,

    ultrasonicFront: 999,

    ultrasonicRear: 999,

    backtracking: false,

    backtrackingStarted: false,

    ir: {

        S1: 0,
        S2: 0,
        S3: 0,
        S4: 0,
        S5: 0,
        S6: 0,
        S7: 0,
        S8: 0

    },

    lastCommand: "NONE",

    lastEvent:
        "Waiting for ESP32...",

    lastUpdate: null

};

// ======================================================
// MQTT CLIENT
// ======================================================

console.log("");

console.log(
    "=========================================="
);

console.log(
    "        AGV PHASE 3 MQTT SERVER"
);

console.log(
    "=========================================="
);

console.log(
    "Connecting to HiveMQ..."
);

console.log(
    "Broker:",
    MQTT_BROKER
);

console.log(
    "Username:",
    MQTT_USERNAME
);

const mqttClient =
    mqtt.connect(
        MQTT_BROKER,
        {

            username:
                MQTT_USERNAME,

            password:
                MQTT_PASSWORD,

            clientId:
                `AGV_SERVER_${Date.now()}`,

            clean:
                true,

            connectTimeout:
                15000,

            reconnectPeriod:
                3000,

            keepalive:
                30,

            rejectUnauthorized:
                true

        }
    );

// ======================================================
// MQTT CONNECTED
// ======================================================

mqttClient.on(
    "connect",
    () => {

        console.log("");

        console.log(
            "✓ HIVEMQ CONNECTED"
        );

        agvStatus.mqttConnected =
            true;

        mqttClient.subscribe(
            [
                STATUS_TOPIC,
                TELEMETRY_TOPIC,
                EVENT_TOPIC
            ],
            {
                qos: 1
            },
            (error) => {

                if (
                    error
                ) {

                    console.error(
                        "Subscription error:",
                        error.message
                    );

                    return;

                }

                console.log(
                    "✓ MQTT subscriptions active"
                );

            }
        );

    }
);

// ======================================================
// MQTT RECONNECT
// ======================================================

mqttClient.on(
    "reconnect",
    () => {

        console.log(
            "↻ Reconnecting to HiveMQ..."
        );

        agvStatus.mqttConnected =
            false;

        agvStatus.espConnected =
            false;

    }
);

// ======================================================
// MQTT CLOSE
// ======================================================

mqttClient.on(
    "close",
    () => {

        console.log(
            "✕ MQTT connection closed"
        );

        agvStatus.mqttConnected =
            false;

        agvStatus.espConnected =
            false;

    }
);

// ======================================================
// MQTT ERROR
// ======================================================

mqttClient.on(
    "error",
    (error) => {

        console.error(
            "MQTT ERROR:",
            error.message
        );

        agvStatus.mqttConnected =
            false;

    }
);

// ======================================================
// UPDATE SENSOR DATA
// ======================================================

function updateSensorData(data) {

    if (
        data.ir
    ) {

        agvStatus.ir = {

            S1:
                data.ir.S1 ?? 0,

            S2:
                data.ir.S2 ?? 0,

            S3:
                data.ir.S3 ?? 0,

            S4:
                data.ir.S4 ?? 0,

            S5:
                data.ir.S5 ?? 0,

            S6:
                data.ir.S6 ?? 0,

            S7:
                data.ir.S7 ?? 0,

            S8:
                data.ir.S8 ?? 0

        };

    }

    if (
        data.weight !== undefined
    ) {

        agvStatus.weight =
            Number(
                data.weight
            );

    }

    if (
        data.weightKg !== undefined
    ) {

        agvStatus.weightKg =
            Number(
                data.weightKg
            );

    } else {

        agvStatus.weightKg =
            agvStatus.weight /
            1000;

    }

    if (
        data.ultrasonicFront !==
        undefined
    ) {

        agvStatus.ultrasonicFront =
            Number(
                data.ultrasonicFront
            );

    }

    if (
        data.ultrasonicRear !==
        undefined
    ) {

        agvStatus.ultrasonicRear =
            Number(
                data.ultrasonicRear
            );

    }

    if (
        data.cycleActive !==
        undefined
    ) {

        agvStatus.cycleActive =
            data.cycleActive;

    }

    if (
        data.lineFollowing !==
        undefined
    ) {

        agvStatus.lineFollowing =
            data.lineFollowing;

    }

    if (
        data.state
    ) {

        agvStatus.state =
            data.state;

    }

    // ==================================================
    // BACKTRACKING STATUS
    // ==================================================

    agvStatus.backtracking =
        data.backtracking === true ||
        data.state ===
            "MOVING_BACKWARD" ||
        data.state ===
            "BACKTRACKING";

    agvStatus.backtrackingStarted =
        data.backtrackingStarted === true ||
        data.state ===
            "WAITING_BEFORE_REVERSE";

}

// ======================================================
// MQTT MESSAGE
// ======================================================

mqttClient.on(
    "message",
    (topic, messageBuffer) => {

        const message =
            messageBuffer.toString();

        console.log(
            `[MQTT] ${topic} → ${message}`
        );

        agvStatus.lastUpdate =
            new Date().toISOString();

        // ==================================================
        // STATUS
        // ==================================================

        if (
            topic === STATUS_TOPIC
        ) {

            try {

                const data =
                    JSON.parse(
                        message
                    );

                agvStatus.espConnected =
                    true;

                updateSensorData(
                    data
                );

            }

            catch (
                error
            ) {

                console.error(
                    "Invalid STATUS JSON:",
                    error.message
                );

            }

            return;

        }

        // ==================================================
        // TELEMETRY
        // ==================================================

        if (
            topic === TELEMETRY_TOPIC
        ) {

            try {

                const data =
                    JSON.parse(
                        message
                    );

                agvStatus.espConnected =
                    true;

                updateSensorData(
                    data
                );

            }

            catch (
                error
            ) {

                console.error(
                    "Invalid TELEMETRY JSON:",
                    error.message
                );

            }

            return;

        }

        // ==================================================
        // EVENT
        // ==================================================

        if (
            topic === EVENT_TOPIC
        ) {

            try {

                const data =
                    JSON.parse(
                        message
                    );

                agvStatus.espConnected =
                    true;

                if (
                    data.event
                ) {

                    agvStatus.lastEvent =
                        data.event;

                    // ======================================
                    // EVENT → STATE
                    // ======================================

                    const event =
                        String(
                            data.event
                        ).toUpperCase();

                    if (
                        event.includes(
                            "BACKTRACKING STARTED"
                        )
                    ) {

                        agvStatus.backtracking =
                            true;

                        agvStatus.backtrackingStarted =
                            true;

                    }

                    if (
                        event.includes(
                            "SOURCE REACHED"
                        )
                    ) {

                        agvStatus.backtracking =
                            false;

                        agvStatus.backtrackingStarted =
                            false;

                    }

                }

            }

            catch {

                agvStatus.lastEvent =
                    message;

            }

        }

    }
);

// ======================================================
// SEND COMMAND
// ======================================================

function sendCommand(
    command
) {

    if (
        !mqttClient.connected
    ) {

        console.error(
            "Cannot send command."
        );

        console.error(
            "MQTT is disconnected."
        );

        return false;

    }

    const payload =
        JSON.stringify({

            command:
                command,

            timestamp:
                Date.now()

        });

    mqttClient.publish(
        COMMAND_TOPIC,
        payload,
        {

            qos:
                1,

            retain:
                false

        },
        (error) => {

            if (
                error
            ) {

                console.error(
                    "Publish error:",
                    error.message
                );

            }

            else {

                console.log(
                    `✓ COMMAND SENT: ${command}`
                );

            }

        }
    );

    agvStatus.lastCommand =
        command;

    return true;

}

// ======================================================
// START
// ======================================================

app.post(
    "/api/start",
    (req, res) => {

        const success =
            sendCommand(
                "START"
            );

        if (
            !success
        ) {

            return res.status(
                503
            ).json({

                success:
                    false,

                message:
                    "MQTT is not connected"

            });

        }

        res.json({

            success:
                true,

            command:
                "START"

        });

    }
);

// ======================================================
// RESUME
// ======================================================

app.post(
    "/api/resume",
    (req, res) => {

        const success =
            sendCommand(
                "RESUME"
            );

        if (
            !success
        ) {

            return res.status(
                503
            ).json({

                success:
                    false,

                message:
                    "MQTT is not connected"

            });

        }

        res.json({

            success:
                true,

            command:
                "RESUME"

        });

    }
);

// ======================================================
// STOP
// ======================================================

app.post(
    "/api/stop",
    (req, res) => {

        const success =
            sendCommand(
                "STOP"
            );

        if (
            !success
        ) {

            return res.status(
                503
            ).json({

                success:
                    false,

                message:
                    "MQTT is not connected"

            });

        }

        res.json({

            success:
                true,

            command:
                "STOP"

        });

    }
);

// ======================================================
// RESET
// ======================================================

app.post(
    "/api/reset",
    (req, res) => {

        const success =
            sendCommand(
                "RESET"
            );

        if (
            !success
        ) {

            return res.status(
                503
            ).json({

                success:
                    false,

                message:
                    "MQTT is not connected"

            });

        }

        res.json({

            success:
                true,

            command:
                "RESET"

        });

    }
);

// ======================================================
// MANUAL
// ======================================================

app.post(
    "/api/manual",
    (req, res) => {

        const command =
            req.body.command;

        const allowedCommands = [

            "MANUAL_FORWARD",

            "MANUAL_BACKWARD",

            "MANUAL_LEFT",

            "MANUAL_RIGHT",

            "MANUAL_STOP"

        ];

        if (
            !allowedCommands.includes(
                command
            )
        ) {

            return res.status(
                400
            ).json({

                success:
                    false,

                message:
                    "Invalid manual command"

            });

        }

        const success =
            sendCommand(
                command
            );

        if (
            !success
        ) {

            return res.status(
                503
            ).json({

                success:
                    false,

                message:
                    "MQTT is not connected"

            });

        }

        res.json({

            success:
                true,

            command:
                command

        });

    }
);

// ======================================================
// STATUS
// ======================================================

app.get(
    "/api/status",
    (req, res) => {

        if (
            agvStatus.lastUpdate
        ) {

            const age =
                Date.now() -
                new Date(
                    agvStatus.lastUpdate
                ).getTime();

            if (
                age > 5000
            ) {

                agvStatus.espConnected =
                    false;

            }

        }

        res.json(
            agvStatus
        );

    }
);

// ======================================================
// MQTT STATUS
// ======================================================

app.get(
    "/api/mqtt",
    (req, res) => {

        res.json({

            connected:
                mqttClient.connected,

            broker:
                MQTT_BROKER,

            commandTopic:
                COMMAND_TOPIC

        });

    }
);

// ======================================================
// HOME
// ======================================================

app.get(
    "/",
    (req, res) => {

        res.sendFile(
            path.join(
                __dirname,
                "public",
                "index.html"
            )
        );

    }
);

// ======================================================
// SERVER
// ======================================================

app.listen(
    PORT,
    () => {

        console.log("");

        console.log(
            "=========================================="
        );

        console.log(
            "       AGV PHASE 3 SERVER RUNNING"
        );

        console.log(
            "=========================================="
        );

        console.log(
            `Website: http://localhost:${PORT}`
        );

        console.log(
            `AGV ID: ${AGV_ID}`
        );

        console.log(
            "=========================================="
        );

    }
);