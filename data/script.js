// ==================== WEBSOCKET ====================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

const DEVICE_CONFIG = [
    { id: 1, key: "led1", name: "LED 1 (Tích hợp)", gpio: 48, state: false, pending: false },
    { id: 2, key: "led2", name: "LED 2 (NeoPixel/Ngoài)", gpio: 45, state: false, pending: false }
];

const CONTROL_MODE = {
    AUTO: "AUTO",
    MANUAL: "MANUAL"
};

let currentControlMode = CONTROL_MODE.AUTO;

let gaugeTemp;
let gaugeHumi;

window.addEventListener("load", onLoad);

function onLoad() {
    initGauges();
    initWebSocket();
    renderRelays();
    bindModeControls();
    renderModeControls();
}

function onOpen() {
    console.log("WebSocket đã kết nối");
}

function onClose() {
    console.log("WebSocket đã ngắt, thử kết nối lại...");
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    console.log("Đang mở kết nối WebSocket...");
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function sendData(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("Gửi:", data);
    } else {
        console.warn("WebSocket chưa sẵn sàng");
        alert("WebSocket chưa kết nối với thiết bị");
    }
}

function onMessage(event) {
    console.log("Nhận:", event.data);

    let payload;
    try {
        payload = JSON.parse(event.data);
    } catch (err) {
        applyTextStatus(event.data);
        return;
    }

    updateGauges(payload);
    applyControlPayload(payload);
    applyFlexibleRelayStatus(payload);
    updateSystemInfo(payload);
}

function applyControlPayload(data) {
    if (typeof data !== "object" || data === null) return;

    const root = (typeof data.control === "object" && data.control !== null) ? data.control : data;
    let changed = false;

    const mode = pickFirstString(root, ["mode", "control_mode"]).toUpperCase();
    if (mode === CONTROL_MODE.AUTO || mode === CONTROL_MODE.MANUAL) {
        if (currentControlMode !== mode) {
            currentControlMode = mode;
            changed = true;
        }
    }

    const led1FromObject = normalizeStatus(root.led1);
    const led2FromObject = normalizeStatus(root.led2);
    const led1FromValue = normalizeStatus({ status: root.led1 });
    const led2FromValue = normalizeStatus({ status: root.led2 });

    const led1Status = led1FromObject !== null ? led1FromObject : led1FromValue;
    const led2Status = led2FromObject !== null ? led2FromObject : led2FromValue;

    const led1 = DEVICE_CONFIG.find((d) => d.id === 1);
    const led2 = DEVICE_CONFIG.find((d) => d.id === 2);

    if (led1 && led1Status !== null) {
        led1.state = led1Status;
        led1.pending = false;
        changed = true;
    }
    if (led2 && led2Status !== null) {
        led2.state = led2Status;
        led2.pending = false;
        changed = true;
    }

    if (changed) {
        renderModeControls();
        renderRelays();
    }
}

function updateGauges(data) {
    const temp = pickFirstNumber(data, ["temp", "temperature", "nhiet_do"]);
    const hum = pickFirstNumber(data, ["hum", "humidity", "do_am"]);

    if (temp !== null && gaugeTemp) {
        gaugeTemp.refresh(temp);
    }
    if (hum !== null && gaugeHumi) {
        gaugeHumi.refresh(hum);
    }

    // --- BỔ SUNG LOGIC HIỂN THỊ VÀ ĐỔI MÀU AI CHỈ VỚI VÀI DÒNG ---
    if (data.ai !== undefined && data.ai !== null) {
        let aiScore = parseFloat(data.ai);
        let elValue = document.getElementById("ai_value");
        let elStatus = document.getElementById("ai_status");

        if (elValue && elStatus) {
            elValue.innerText = aiScore.toFixed(2);

            // Ngưỡng cảnh báo: Lớn hơn 0.6 là Bất thường
            if (aiScore > 0.6) {
                elStatus.innerText = "⚠️ BẤT THƯỜNG";
                elStatus.style.color = "#ff879f";
                elStatus.style.background = "rgba(171, 46, 82, 0.24)";
                elValue.style.color = "#ff879f";
            } else {
                elStatus.innerText = "✅ BÌNH THƯỜNG";
                elStatus.style.color = "#6ffdf0";
                elStatus.style.background = "rgba(32, 144, 146, 0.24)";
                elValue.style.color = "#2de2d2";
            }
        }
    }
}

function applyTextStatus(rawText) {
    const msg = String(rawText || "").toUpperCase();
    if (!msg) return;

    const isOn = msg.includes(" ON") || msg.endsWith("ON");
    const isOff = msg.includes(" OFF") || msg.endsWith("OFF");
    if (!isOn && !isOff) return;

    const target = DEVICE_CONFIG.find((d) => d.pending) || DEVICE_CONFIG[0];
    if (!target) return;

    target.state = isOn;
    target.pending = false;
    renderRelays();
}

function applyFlexibleRelayStatus(data) {
    const candidates = collectCandidates(data);
    if (candidates.length === 0) return;

    let hasChanges = false;
    candidates.forEach((entry) => {
        const match = findRelay(entry);
        const status = normalizeStatus(entry);
        if (!match || status === null) return;

        match.state = status;
        match.pending = false;
        hasChanges = true;
    });

    if (hasChanges) {
        renderRelays();
    }
}

function collectCandidates(data) {
    const list = [];
    if (typeof data !== "object" || data === null) return list;

    list.push(data);

    if (typeof data.value === "object" && data.value !== null) list.push(data.value);
    if (Array.isArray(data.devices)) list.push(...data.devices);
    if (Array.isArray(data.relays)) list.push(...data.relays);
    if (Array.isArray(data.data)) list.push(...data.data);
    if (typeof data.control === "object" && data.control !== null) {
        list.push(data.control);
        if (typeof data.control.led1 === "object" && data.control.led1 !== null) list.push(data.control.led1);
        if (typeof data.control.led2 === "object" && data.control.led2 !== null) list.push(data.control.led2);
    }
    return list;
}

function findRelay(entry) {
    const gpio = pickFirstNumber(entry, ["gpio", "pin"]);
    if (gpio !== null) return DEVICE_CONFIG.find((d) => Number(d.gpio) === Number(gpio)) || null;

    const id = pickFirstNumber(entry, ["id", "relay_id", "device_id"]);
    if (id !== null) return DEVICE_CONFIG.find((d) => Number(d.id) === Number(id)) || null;

    const name = pickFirstString(entry, ["name", "device", "label", "key"]);
    if (name) {
        const normalized = normalizeName(name);
        return DEVICE_CONFIG.find((d) => normalizeName(d.name) === normalized || normalizeName(d.key) === normalized) || null;
    }
    return DEVICE_CONFIG.find((d) => d.pending) || null;
}

function normalizeStatus(entry) {
    const raw = pickFirstValue(entry, ["status", "state", "value", "relay", "led"]);

    if (typeof raw === "boolean") return raw;
    if (typeof raw === "number") return raw > 0;
    if (typeof raw === "string") {
        const s = raw.trim().toUpperCase();
        if (["ON", "1", "TRUE", "BAT", "BẬT", "HIGH"].includes(s)) return true;
        if (["OFF", "0", "FALSE", "TAT", "TẮT", "LOW"].includes(s)) return false;
    }
    return null;
}

function pickFirstNumber(obj, keys) {
    const value = pickFirstValue(obj, keys);
    if (value === undefined || value === null || value === "") return null;
    const num = Number(value);
    return Number.isFinite(num) ? num : null;
}

function pickFirstString(obj, keys) {
    const value = pickFirstValue(obj, keys);
    if (value === undefined || value === null) return "";
    return String(value).trim();
}

function pickFirstValue(obj, keys) {
    if (typeof obj !== "object" || obj === null) return undefined;
    for (const key of keys) {
        if (Object.prototype.hasOwnProperty.call(obj, key)) return obj[key];
    }
    return undefined;
}

function normalizeName(value) {
    return String(value || "").toLowerCase().replace(/[^a-z0-9]/g, "");
}

// ==================== UI NAVIGATION ====================
function showSection(id, event) {
    document.querySelectorAll(".section").forEach((sec) => sec.style.display = "none");
    const target = document.getElementById(id);
    if (target) target.style.display = "block";

    document.querySelectorAll(".nav-item").forEach((item) => item.classList.remove("active"));
    event.currentTarget.classList.add("active");
}

// ==================== HOME GAUGES ====================
function initGauges() {
    gaugeTemp = new JustGage({
        id: "gauge_temp",
        value: 0,
        min: -10,
        max: 50,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#00BCD4", "#4CAF50", "#FFC107", "#F44336"]
    });

    gaugeHumi = new JustGage({
        id: "gauge_humi",
        value: 0,
        min: 0,
        max: 100,
        donut: true,
        pointer: false,
        gaugeWidthScale: 0.25,
        gaugeColor: "transparent",
        levelColorsGradient: true,
        levelColors: ["#42A5F5", "#00BCD4", "#0288D1"]
    });
}

// ==================== DEVICE CONTROL ====================
function renderRelays() {
    const container = document.getElementById("relayContainer");
    container.innerHTML = "";
    const isManual = currentControlMode === CONTROL_MODE.MANUAL;

    DEVICE_CONFIG.forEach((relay) => {
        const card = document.createElement("div");
        card.className = `device-card ${relay.state ? "state-on" : "state-off"}`;

        const statusText = relay.state ? "ĐANG BẬT" : "ĐANG TẮT";
        const actionText = relay.state ? "TẮT" : "BẬT";
        const pendingText = isManual
            ? (relay.pending ? "Đang chờ thiết bị phản hồi..." : "Đồng bộ với phản hồi ESP32")
            : "Đang ở chế độ AUTO - khóa điều khiển tay";

        card.innerHTML = `
      <i class="fa-solid fa-lightbulb device-icon"></i>
      <div class="status-badge ${relay.state ? "on" : "off"}">${statusText}</div>
      <h3>${relay.name}</h3>
      <p>GPIO: ${relay.gpio}</p>
            <button class="toggle-btn ${relay.state ? "on" : "off"}" onclick="toggleRelay(${relay.id})" ${isManual ? "" : "disabled"}>
        ${actionText}
      </button>
      <small class="sync-note">${pendingText}</small>
    `;

        container.appendChild(card);
    });
}

function toggleRelay(id) {
    if (currentControlMode !== CONTROL_MODE.MANUAL) {
        alert("Đang ở chế độ AUTO. Hãy chuyển sang MANUAL để điều khiển bằng tay.");
        return;
    }

    const relay = DEVICE_CONFIG.find((item) => item.id === id);
    if (!relay) return;

    const nextState = !relay.state;
    relay.pending = true;
    renderRelays();

    const relayJSON = JSON.stringify({
        page: "device",
        value: {
            id: relay.id,
            key: relay.key,
            name: relay.name,
            gpio: relay.gpio,
            status: nextState ? "ON" : "OFF"
        }
    });

    sendData(relayJSON);
}

function bindModeControls() {
    const btnAuto = document.getElementById("btnAutoMode");
    const btnManual = document.getElementById("btnManualMode");

    if (btnAuto) btnAuto.addEventListener("click", () => setControlMode(CONTROL_MODE.AUTO));
    if (btnManual) btnManual.addEventListener("click", () => setControlMode(CONTROL_MODE.MANUAL));
}

function setControlMode(mode) {
    if (mode !== CONTROL_MODE.AUTO && mode !== CONTROL_MODE.MANUAL) return;

    currentControlMode = mode;
    renderModeControls();
    renderRelays();

    const payload = JSON.stringify({ page: "control", value: { mode: mode } });
    sendData(payload);
}

function renderModeControls() {
    const modeValue = document.getElementById("modeValue");
    const modeHint = document.getElementById("modeHint");
    const btnAuto = document.getElementById("btnAutoMode");
    const btnManual = document.getElementById("btnManualMode");

    if (modeValue) modeValue.textContent = currentControlMode === CONTROL_MODE.MANUAL ? "MANUAL" : "AUTO";
    if (modeHint) modeHint.textContent = currentControlMode === CONTROL_MODE.MANUAL
        ? "Cho phép bật/tắt đèn từ giao diện web."
        : "Tự động theo cảm biến, khóa nút điều khiển tay.";

    if (btnAuto) btnAuto.classList.toggle("active", currentControlMode === CONTROL_MODE.AUTO);
    if (btnManual) btnManual.classList.toggle("active", currentControlMode === CONTROL_MODE.MANUAL);
}

// ==================== SYSTEM INFO (dynamic) ====================
function updateSystemInfo(payload) {
    const sys = (typeof payload === "object" && payload !== null) ? payload.system : null;
    if (!sys) return;

    const container = document.getElementById("systemInfo");
    if (!container) return;

    if (container.querySelector(".info-loading")) container.innerHTML = "";

    Object.entries(sys).forEach(([key, rawValue]) => {
        const displayKey = key.replace(/([A-Z])/g, " $1").replace(/^./, (s) => s.toUpperCase());
        let formattedValue = rawValue;

        const numVal = Number(rawValue);
        if (Number.isFinite(numVal)) {
            if (key.toLowerCase().includes("heap") && !key.toLowerCase().includes("size")) {
                formattedValue = rawValue.toString(); 
            } else if (key.toLowerCase() === "rssi") {
                formattedValue = rawValue + " dBm";
            }
        }

        let item = container.querySelector(`[data-key="${key}"]`);
        if (!item) {
            item = document.createElement("div");
            item.className = "info-item";
            item.dataset.key = key;
            item.innerHTML = `
                <span class="info-label">${displayKey}</span>
                <span class="info-value">${formattedValue}</span>
            `;
            container.appendChild(item);
        } else {
            item.querySelector(".info-value").textContent = formattedValue;
        }
    });
}

// ==================== CÀI ĐẶT HỆ THỐNG ====================
document.addEventListener("DOMContentLoaded", () => {
    const settingsForm = document.getElementById("settingsForm");
    if (settingsForm) {
        settingsForm.addEventListener("submit", (e) => {
            e.preventDefault();
            
            const ssid = document.getElementById("wifi_ssid").value;
            const pass = document.getElementById("wifi_pass").value;
            const server = document.getElementById("mqtt_server").value;
            const token = document.getElementById("mqtt_token").value;
            const port = document.getElementById("mqtt_port").value;
            
            // Đóng gói dữ liệu thành đường dẫn HTTP GET
            const url = `/save_wifi?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}&server=${encodeURIComponent(server)}&token=${encodeURIComponent(token)}&port=${encodeURIComponent(port)}`;

            // Dùng fetch (HTTP GET) vượt tường lửa thay vì WebSocket
            fetch(url)
                .then(() => {
                    alert("✅ Đã nạp cấu hình! Mạch ESP32 đang khởi động lại...");
                })
                .catch(() => {
                    alert("✅ Đã nạp cấu hình! Điện thoại/PC sẽ mất kết nối do mạch đã khởi động lại.");
                });
        });
    }
});