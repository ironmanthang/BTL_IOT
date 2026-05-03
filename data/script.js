var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

const DEVICE_CONFIG = [
    { id: 1, key: "led1", name: "LED Cảnh Báo", gpio: 48, state: false },
    { id: 2, key: "led2", name: "Đèn NeoPixel", gpio: 45, state: false }
];

const CONTROL_MODE = { AUTO: "AUTO", MANUAL: "MANUAL" };
let currentControlMode = CONTROL_MODE.AUTO;
let gaugeTemp, gaugeHumi;

window.addEventListener("load", () => {
    initGauges();
    initRelaysDOM(); // TẠO GIAO DIỆN ĐÚNG 1 LẦN DUY NHẤT (CHỐNG LAG)
    initWebSocket();
    bindModeControls();
});

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = () => console.log("WebSocket connected");
    websocket.onclose = () => setTimeout(initWebSocket, 2000);
    websocket.onmessage = onMessage;
}

function sendData(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
    }
}

function onMessage(event) {
    let payload;
    try { payload = JSON.parse(event.data); } catch (err) { return; }
    
    updateGauges(payload);
    applyControlPayload(payload);
    updateSystemInfo(payload);
}

function updateGauges(data) {
    if (data.temp !== undefined && gaugeTemp) gaugeTemp.refresh(data.temp);
    if (data.hum !== undefined && gaugeHumi) gaugeHumi.refresh(data.hum);

    if (data.ai !== undefined && data.ai !== null) {
        let aiScore = parseFloat(data.ai);
        let elValue = document.getElementById("ai_value");
        let elStatus = document.getElementById("ai_status");
        let aiCard = document.querySelector(".ai-card");

        if (elValue && elStatus && aiCard) {
            elValue.innerText = aiScore.toFixed(2);
            if (aiScore > 0.6) {
                elStatus.innerHTML = '<i class="fa-solid fa-triangle-exclamation"></i> BẤT THƯỜNG';
                aiCard.className = "gauge-card ai-card ai-danger";
            } else {
                elStatus.innerHTML = '<i class="fa-solid fa-check"></i> BÌNH THƯỜNG';
                aiCard.className = "gauge-card ai-card ai-normal";
            }
        }
    }
}

function applyControlPayload(data) {
    if (!data) return;
    const root = data.control || data;
    
    const mode = (root.mode || root.control_mode || "").toUpperCase();
    if (mode === CONTROL_MODE.AUTO || mode === CONTROL_MODE.MANUAL) {
        currentControlMode = mode;
        renderModeControls();
    }

    const led1Status = normalizeStatus(root.led1);
    const led2Status = normalizeStatus(root.led2);

    if (led1Status !== null) DEVICE_CONFIG[0].state = led1Status;
    if (led2Status !== null) DEVICE_CONFIG[1].state = led2Status;

    updateRelaysDOM(); // CHỈ THAY ĐỔI MÀU SẮC, KHÔNG LOAD LẠI HTML
}

function normalizeStatus(entry) {
    if (!entry) return null;
    const raw = entry.status !== undefined ? entry.status : entry.state !== undefined ? entry.state : entry.value !== undefined ? entry.value : entry;
    if (typeof raw === "boolean") return raw;
    if (typeof raw === "string") return ["ON", "1", "TRUE", "BAT", "BẬT"].includes(raw.trim().toUpperCase());
    return null;
}

// KHỞI TẠO KHUNG HTML CHO NÚT BẤM (CHẠY 1 LẦN LÚC MỚI VÀO WEB)
function initRelaysDOM() {
    const container = document.getElementById("relayContainer");
    container.innerHTML = "";
    DEVICE_CONFIG.forEach((relay) => {
        container.innerHTML += `
            <div id="device-card-${relay.id}" class="device-card state-off">
                <i class="fa-solid fa-lightbulb device-icon"></i>
                <div><span id="badge-${relay.id}" class="status-badge off">ĐANG TẮT</span></div>
                <h3>${relay.name}</h3>
                <p>GPIO: ${relay.gpio}</p>
                <button id="btn-${relay.id}" class="toggle-btn off" onclick="toggleRelay(${relay.id})">BẬT</button>
                <small id="note-${relay.id}" class="sync-note">Trạng thái thực tế từ mạch</small>
            </div>
        `;
    });
}

// CẬP NHẬT GIAO DIỆN SIÊU TỐC
function updateRelaysDOM() {
    const isManual = currentControlMode === CONTROL_MODE.MANUAL;

    DEVICE_CONFIG.forEach((relay) => {
        const card = document.getElementById(`device-card-${relay.id}`);
        const badge = document.getElementById(`badge-${relay.id}`);
        const btn = document.getElementById(`btn-${relay.id}`);
        const note = document.getElementById(`note-${relay.id}`);
        
        if (!card || !badge || !btn || !note) return;

        card.className = `device-card ${relay.state ? "state-on" : "state-off"}`;
        
        badge.className = `status-badge ${relay.state ? "on" : "off"}`;
        badge.innerText = relay.state ? "ĐANG BẬT" : "ĐANG TẮT";
        
        btn.className = `toggle-btn ${relay.state ? "on" : "off"}`;
        btn.innerText = relay.state ? "TẮT" : "BẬT";
        btn.disabled = !isManual;

        note.innerText = isManual ? "Trạng thái thực tế từ mạch" : "Đang khóa (Chế độ AUTO)";
    });
}

function toggleRelay(id) {
    if (currentControlMode !== CONTROL_MODE.MANUAL) return;
    const relay = DEVICE_CONFIG.find((item) => item.id === id);
    if (!relay) return;

    // HIỆU ỨNG SIÊU MƯỢT: Đổi màu ngay lập tức trên web trước khi mạch xác nhận
    relay.state = !relay.state;
    updateRelaysDOM(); 

    sendData(JSON.stringify({ page: "device", value: { id: relay.id, gpio: relay.gpio, status: relay.state ? "ON" : "OFF" } }));
}

function bindModeControls() {
    document.getElementById("btnAutoMode").onclick = () => { currentControlMode = CONTROL_MODE.AUTO; renderModeControls(); updateRelaysDOM(); sendData(JSON.stringify({ page: "control", value: { mode: "AUTO" } })); };
    document.getElementById("btnManualMode").onclick = () => { currentControlMode = CONTROL_MODE.MANUAL; renderModeControls(); updateRelaysDOM(); sendData(JSON.stringify({ page: "control", value: { mode: "MANUAL" } })); };
}

function renderModeControls() {
    let val = document.getElementById("modeValue");
    let autoBtn = document.getElementById("btnAutoMode");
    let manBtn = document.getElementById("btnManualMode");
    if(val) val.textContent = currentControlMode;
    if(autoBtn) autoBtn.classList.toggle("active", currentControlMode === CONTROL_MODE.AUTO);
    if(manBtn) manBtn.classList.toggle("active", currentControlMode === CONTROL_MODE.MANUAL);
}

function updateSystemInfo(payload) {
    const sys = payload?.system; if (!sys) return;
    const container = document.getElementById("systemInfo");
    if (container.querySelector(".info-loading")) container.innerHTML = "";

    Object.entries(sys).forEach(([key, val]) => {
        let displayKey = key.replace(/([A-Z])/g, " $1").trim();
        let item = container.querySelector(`[data-key="${key}"]`);
        if (!item) {
            container.innerHTML += `<div class="info-item" data-key="${key}"><span class="info-label">${displayKey}</span><span class="info-value">${val}</span></div>`;
        } else {
            item.querySelector(".info-value").textContent = val;
        }
    });
}

function initGauges() {
    gaugeTemp = new JustGage({ id: "gauge_temp", value: 0, min: -10, max: 50, donut: true, pointer: false, gaugeWidthScale: 0.3, gaugeColor: "rgba(255,255,255,0.05)", levelColors: ["#00b4d8", "#2de2d2", "#ffb703", "#ff4d6d"], valueFontColor: "#fff" });
    gaugeHumi = new JustGage({ id: "gauge_humi", value: 0, min: 0, max: 100, donut: true, pointer: false, gaugeWidthScale: 0.3, gaugeColor: "rgba(255,255,255,0.05)", levelColors: ["#0077b6", "#00b4d8", "#90e0ef"], valueFontColor: "#fff" });
}

document.getElementById("settingsForm").addEventListener("submit", (e) => {
    e.preventDefault();
    const url = `/save_wifi?ssid=${encodeURIComponent(document.getElementById("wifi_ssid").value)}&pass=${encodeURIComponent(document.getElementById("wifi_pass").value)}&server=${encodeURIComponent(document.getElementById("mqtt_server").value)}&token=${encodeURIComponent(document.getElementById("mqtt_token").value)}&port=${encodeURIComponent(document.getElementById("mqtt_port").value)}`;
    fetch(url).then(() => alert("✅ ESP32 đã nhận cấu hình và đang khởi động lại!")).catch(() => alert("✅ Điện thoại/PC sẽ mất kết nối do mạch Reboot."));
});

function showSection(id, event) {
    document.querySelectorAll(".section").forEach(sec => sec.style.display = "none");
    document.getElementById(id).style.display = "block";
    document.querySelectorAll(".nav-item").forEach(item => item.classList.remove("active"));
    event.currentTarget.classList.add("active");
}