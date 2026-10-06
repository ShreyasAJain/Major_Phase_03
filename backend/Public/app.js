// ======================================================
// AGV PHASE 3 FRONTEND
// ======================================================

// ======================================================
// ELEMENTS
// ======================================================

const mqttStatus =
    document.getElementById(
        "mqttStatus"
    );

const mqttDot =
    document.getElementById(
        "mqttDot"
    );

const espStatus =
    document.getElementById(
        "espStatus"
    );

const espDot =
    document.getElementById(
        "espDot"
    );

const stateBadge =
    document.getElementById(
        "stateBadge"
    );

const lineFollowing =
    document.getElementById(
        "lineFollowing"
    );

const lastCommand =
    document.getElementById(
        "lastCommand"
    );

const manualContent =
    document.getElementById(
        "manualContent"
    );

const manualToggle =
    document.getElementById(
        "manualToggle"
    );

// ======================================================
// TOAST
// ======================================================

function showToast(message) {

    const toast =
        document.createElement(
            "div"
        );

    toast.className = "toast";

    toast.textContent = message;

    document.body.appendChild(
        toast
    );

    setTimeout(() => {

        toast.classList.add(
            "show"
        );

    }, 10);

    setTimeout(() => {

        toast.classList.remove(
            "show"
        );

        setTimeout(() => {

            toast.remove();

        }, 300);

    }, 2500);
}

// ======================================================
// API HELPER
// ======================================================

async function sendRequest(
    url,
    options = {}
) {

    try {

        const response =
            await fetch(
                url,
                options
            );

        const data =
            await response.json();

        return data;

    } catch (error) {

        console.error(
            "Request failed:",
            error
        );

        showToast(
            "Connection error"
        );

        return null;
    }
}

// ======================================================
// START
// ======================================================

async function startAGV() {

    const data =
        await sendRequest(
            "/api/start",
            {
                method: "POST"
            }
        );

    if (data) {

        showToast(
            "START command sent"
        );

        updateLastCommand(
            "START"
        );
    }
}

// ======================================================
// STOP
// ======================================================

async function stopAGV() {

    const data =
        await sendRequest(
            "/api/stop",
            {
                method: "POST"
            }
        );

    if (data) {

        showToast(
            "STOP command sent"
        );

        updateLastCommand(
            "STOP"
        );
    }
}

// ======================================================
// RESUME
// ======================================================

async function resumeAGV() {

    const data =
        await sendRequest(
            "/api/resume",
            {
                method: "POST"
            }
        );

    if (data) {

        showToast(
            "RESUME command sent"
        );

        updateLastCommand(
            "RESUME"
        );
    }
}

// ======================================================
// RESET
// ======================================================

async function resetAGV() {

    const data =
        await sendRequest(
            "/api/reset",
            {
                method: "POST"
            }
        );

    if (data) {

        showToast(
            "RESET command sent"
        );

        updateLastCommand(
            "RESET"
        );
    }
}

// ======================================================
// MANUAL MODE
// ======================================================

function toggleManualMode() {

    if (
        manualContent.style.display ===
        "block"
    ) {

        manualContent.style.display =
            "none";

        manualToggle.textContent =
            "SHOW MANUAL";

    } else {

        manualContent.style.display =
            "block";

        manualToggle.textContent =
            "HIDE MANUAL";
    }
}

// ======================================================
// MANUAL COMMAND
// ======================================================

async function manualCommand(
    command
) {

    const data =
        await sendRequest(
            "/api/manual",
            {
                method: "POST",

                headers: {
                    "Content-Type":
                        "application/json"
                },

                body:
                    JSON.stringify({
                        command:
                            command
                    })
            }
        );

    if (data) {

        showToast(
            `${command} sent`
        );

        updateLastCommand(
            command
        );
    }
}

// ======================================================
// LAST COMMAND
// ======================================================

function updateLastCommand(
    command
) {

    if (lastCommand) {

        lastCommand.textContent =
            command;
    }
}

// ======================================================
// CONNECTION STATUS
// ======================================================

function updateConnectionStatus(
    data
) {

    if (!data) {
        return;
    }

    // ==================================================
    // MQTT
    // ==================================================

    if (
        data.mqttConnected === true ||
        data.mqtt === true
    ) {

        mqttStatus.textContent =
            "CONNECTED";

        mqttDot.classList.add(
            "connected"
        );

    } else {

        mqttStatus.textContent =
            "DISCONNECTED";

        mqttDot.classList.remove(
            "connected"
        );
    }

    // ==================================================
    // ESP32
    // ==================================================

    if (
        data.connected === true ||
        data.espConnected === true
    ) {

        espStatus.textContent =
            "CONNECTED";

        espDot.classList.add(
            "connected"
        );

    } else {

        espStatus.textContent =
            "DISCONNECTED";

        espDot.classList.remove(
            "connected"
        );
    }
}

// ======================================================
// STATE
// ======================================================

function updateState(
    data
) {

    if (!data) {
        return;
    }

    const state =
        data.state ||
        "OFFLINE";

    stateBadge.textContent =
        state;

    if (
        state === "RUNNING" ||
        state === "MOVING" ||
        state === "MOVING_FORWARD"
    ) {

        stateBadge.className =
            "state running";

    } else if (
        state === "STOPPED" ||
        state === "PAUSED" ||
        state === "WAITING_FOR_LOAD" ||
        state === "WAITING_FOR_REMOVAL" ||
        state === "WAITING_BEFORE_REVERSE"
    ) {

        stateBadge.className =
            "state stopped";

    } else if (
        state === "BACKTRACKING" ||
        state === "MOVING_BACKWARD"
    ) {

        stateBadge.className =
            "state backtracking";

    } else {

        stateBadge.className =
            "state";
    }

    if (lineFollowing) {

        if (
            data.lineFollowing === true ||
            data.lineFollowing === "ON"
        ) {

            lineFollowing.textContent =
                "ON";

        } else {

            lineFollowing.textContent =
                "OFF";
        }
    }
}

// ======================================================
// IR SENSOR UPDATE
// ======================================================

function updateIRSensors(
    data
) {

    if (!data) {
        return;
    }

    for (
        let i = 1;
        i <= 8;
        i++
    ) {

        const element =
            document.getElementById(
                `S${i}`
            );

        if (!element) {
            continue;
        }

        let value = 0;

        if (
            data[`S${i}`] !==
            undefined
        ) {

            value =
                data[`S${i}`];

        } else if (
            data.ir &&
            data.ir[`S${i}`] !==
            undefined
        ) {

            value =
                data.ir[`S${i}`];
        }

        element.textContent =
            value;
    }
}

// ======================================================
// WEIGHT
// ======================================================

function updateWeight(
    data
) {

    if (!data) {
        return;
    }

    const weightElement =
        document.getElementById(
            "weight"
        );

    const weightStatus =
        document.getElementById(
            "weightStatus"
        );

    if (weightElement) {

        let weight =
            data.weightKg;

        if (
            weight === undefined &&
            data.weight !== undefined
        ) {

            /*
             * ESP32 sends weight in grams
             * through `weight`.
             */
            weight =
                Number(data.weight) /
                1000;
        }

        if (
            weight !== undefined
        ) {

            weightElement.textContent =
                Number(weight).toFixed(
                    2
                ) + " kg";
        }
    }

    if (weightStatus) {

        if (
            data.weightReady ===
                true ||
            data.loadReady ===
                true ||
            (
                data.weightKg !==
                    undefined &&
                Number(
                    data.weightKg
                ) >= 2
            )
        ) {

            weightStatus.textContent =
                "READY";

        } else if (
            data.state ===
            "WAITING_FOR_REMOVAL"
        ) {

            weightStatus.textContent =
                "REMOVE LOAD";

        } else if (
            data.state ===
            "WAITING_BEFORE_REVERSE"
        ) {

            weightStatus.textContent =
                "REMOVED — 2S WAIT";

        } else {

            weightStatus.textContent =
                "WAITING";
        }
    }
}

// ======================================================
// ULTRASONIC
// ======================================================

function updateUltrasonic(
    data
) {

    if (!data) {
        return;
    }

    const front =
        document.getElementById(
            "ultrasonicFront"
        );

    const left =
        document.getElementById(
            "ultrasonicLeft"
        );

    const right =
        document.getElementById(
            "ultrasonicRight"
        );

    const ultrasonic =
        data.ultrasonic ||
        {};

    if (front) {

        const value =
            data.ultrasonicFront ??
            ultrasonic.front;

        if (
            value !== undefined
        ) {

            front.textContent =
                value + " cm";
        }
    }

    if (left) {

        const value =
            data.ultrasonicLeft ??
            ultrasonic.left;

        if (
            value !== undefined
        ) {

            left.textContent =
                value + " cm";
        }
    }

    if (right) {

        const value =
            data.ultrasonicRight ??
            ultrasonic.right;

        if (
            value !== undefined
        ) {

            right.textContent =
                value + " cm";
        }
    }

    // Rear ultrasonic support
    const rear =
        document.getElementById(
            "ultrasonicRear"
        );

    if (rear) {

        const value =
            data.ultrasonicRear ??
            ultrasonic.rear;

        if (
            value !== undefined
        ) {

            rear.textContent =
                value + " cm";
        }
    }
}

// ======================================================
// BACKTRACKING
// ======================================================

function updateBacktracking(
    data
) {

    if (!data) {
        return;
    }

    const element =
        document.getElementById(
            "backtrackingStatus"
        );

    if (!element) {
        return;
    }

    if (
        data.backtracking === true ||
        data.state ===
            "BACKTRACKING" ||
        data.state ===
            "MOVING_BACKWARD"
    ) {

        element.textContent =
            "ACTIVE";

    } else if (
        data.backtrackingStarted ===
            true ||
        data.state ===
            "WAITING_BEFORE_REVERSE"
    ) {

        element.textContent =
            "STARTING";

    } else if (
        data.state ===
            "WAITING_FOR_REMOVAL"
    ) {

        element.textContent =
            "WAITING FOR LOAD";

    } else {

        element.textContent =
            "INACTIVE";
    }
}

// ======================================================
// STATUS
// ======================================================

async function updateStatus() {

    const data =
        await sendRequest(
            "/api/status"
        );

    if (!data) {
        return;
    }

    updateConnectionStatus(
        data
    );

    updateState(
        data
    );

    updateIRSensors(
        data
    );

    updateWeight(
        data
    );

    updateUltrasonic(
        data
    );

    updateBacktracking(
        data
    );

    if (
        data.lastCommand
    ) {

        updateLastCommand(
            data.lastCommand
        );
    }

    const time =
        new Date()
            .toLocaleTimeString();

    const updateElement =
        document.getElementById(
            "lastUpdate"
        );

    if (updateElement) {

        updateElement.textContent =
            `Last update: ${time}`;
    }
}

// ======================================================
// INITIAL STATUS
// ======================================================

updateStatus();

// ======================================================
// LIVE STATUS POLLING
// ======================================================

setInterval(
    updateStatus,
    1000
);