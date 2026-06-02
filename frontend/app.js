const DEFAULT_API_BASE = window.location.port === "5173" ? "http://127.0.0.1:8000" : window.location.origin;
const STORAGE_KEY = "rack_api_base";
const GRAFANA_URL = "http://localhost:3000/d/monitoreo-rack-servidores";

const state = {
  apiBase: localStorage.getItem(STORAGE_KEY) || DEFAULT_API_BASE,
  status: null,
  modes: null,
  busy: false,
  refreshing: false,
};

const el = {
  apiBaseInput: document.getElementById("apiBaseInput"),
  saveApiButton: document.getElementById("saveApiButton"),
  refreshButton: document.getElementById("refreshButton"),
  statusValue: document.getElementById("statusValue"),
  reasonValue: document.getElementById("reasonValue"),
  modeValue: document.getElementById("modeValue"),
  modeHint: document.getElementById("modeHint"),
  networkValue: document.getElementById("networkValue"),
  lastWillValue: document.getElementById("lastWillValue"),
  doorValue: document.getElementById("doorValue"),
  doorSecurityValue: document.getElementById("doorSecurityValue"),
  lastUpdateValue: document.getElementById("lastUpdateValue"),
  temperatureValue: document.getElementById("temperatureValue"),
  humidityValue: document.getElementById("humidityValue"),
  positionValue: document.getElementById("positionValue"),
  accessValue: document.getElementById("accessValue"),
  modeButtons: document.getElementById("modeButtons"),
  modeCommandStatus: document.getElementById("modeCommandStatus"),
  modeDetails: document.getElementById("modeDetails"),
  doorLockValue: document.getElementById("doorLockValue"),
  unlockUsername: document.getElementById("unlockUsername"),
  unlockPassword: document.getElementById("unlockPassword"),
  unlockButton: document.getElementById("unlockButton"),
  lockButton: document.getElementById("lockButton"),
  fanModeValue: document.getElementById("fanModeValue"),
  fanPowerValue: document.getElementById("fanPowerValue"),
  fanPowerInput: document.getElementById("fanPowerInput"),
  fanPowerOutput: document.getElementById("fanPowerOutput"),
  fanManualButton: document.getElementById("fanManualButton"),
  fanAutoButton: document.getElementById("fanAutoButton"),
  grafanaLink: document.getElementById("grafanaLink"),
  apiDocsLink: document.getElementById("apiDocsLink"),
  eventLog: document.getElementById("eventLog"),
  toast: document.getElementById("toast"),
};

function apiUrl(path) {
  return `${state.apiBase.replace(/\/$/, "")}${path}`;
}

function label(value, fallback = "--") {
  if (value === null || value === undefined || value === "") return fallback;
  return String(value)
    .replaceAll("_", " ")
    .replace(/\b\w/g, (char) => char.toUpperCase());
}

function formatNumber(value, suffix, decimals = 1) {
  const number = Number(value);
  if (!Number.isFinite(number)) return "--";
  return `${number.toFixed(decimals)} ${suffix}`;
}

function statusLevel(status) {
  if (status === "optimal" || status === "online" || status === "closed" || status === "door_secure") return "ok";
  if (
    status === "warning" ||
    status === "unlocking" ||
    status === "unlocked" ||
    status === "door_unlocked_closed" ||
    status === "door_open_authorized"
  ) return "warning";
  if (
    status === "critical" ||
    status === "sensor_error" ||
    status === "offline" ||
    status === "open" ||
    status === "door_open_forced" ||
    status === "door_lock_error"
  ) return "critical";
  return "neutral";
}

function reasonLabel(reason) {
  const labels = {
    all_conditions_optimal: "Todo en rango",
    network_disconnected: "Red desconectada",
    door_open: "Puerta abierta",
    door_unlocked_closed: "Puerta desbloqueada",
    door_secure: "Puerta cerrada segura",
    door_open_authorized: "Puerta abierta autorizada",
    door_open_forced: "Apertura no autorizada",
    door_lock_error: "Error de bloqueo",
    temperature_warning_high: "Temperatura alta",
    temperature_critical_high: "Temperatura critica",
    warning_low: "Humedad baja",
    warning_high: "Humedad alta",
    critical_low: "Humedad critica baja",
    critical_high: "Humedad critica alta",
    tilt_warning: "Inclinacion",
    tilt_critical: "Inclinacion critica",
    vibration_warning: "Vibracion",
    vibration_critical: "Vibracion critica",
    dht_sensor_error: "Error DHT",
    mpu_sensor_error: "Error MPU",
  };
  return labels[reason] || label(reason);
}

async function request(path, options = {}) {
  const response = await fetch(apiUrl(path), {
    headers: { "Content-Type": "application/json" },
    ...options,
  });

  const text = await response.text();
  let data = {};
  if (text) {
    try {
      data = JSON.parse(text);
    } catch {
      data = { detail: text };
    }
  }
  if (!response.ok) {
    throw new Error(data.detail || `HTTP ${response.status}`);
  }
  return data;
}

function setBusy(isBusy) {
  state.busy = isBusy;
  [
    el.refreshButton,
    ...el.modeButtons.querySelectorAll("button"),
    el.unlockButton,
    el.lockButton,
    el.fanManualButton,
    el.fanAutoButton,
  ].forEach((button) => {
    button.disabled = isBusy;
  });
}

function showToast(message, type = "ok") {
  el.toast.textContent = message;
  el.toast.className = `toast show ${type}`;
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => {
    el.toast.className = "toast";
  }, 3200);
}

function logEvent(message) {
  const item = document.createElement("div");
  const now = new Date().toLocaleTimeString();
  item.textContent = `${now} - ${message}`;
  el.eventLog.prepend(item);

  while (el.eventLog.children.length > 8) {
    el.eventLog.removeChild(el.eventLog.lastChild);
  }
}

function updateStatStates(status) {
  el.statusValue.closest(".stat").dataset.state = statusLevel(status.status);
  el.modeValue.closest(".stat").dataset.state = "info";
  el.networkValue.closest(".stat").dataset.state = statusLevel(status.network);
  el.doorValue.closest(".stat").dataset.state = statusLevel(status.door_security_state || status.door_state);
}

function renderModeDetails(activeMode) {
  const mode = state.modes?.modes?.[activeMode];
  const cells = el.modeDetails.querySelectorAll("strong");
  if (!mode) {
    cells[0].textContent = "--";
    cells[1].textContent = "--";
    cells[2].textContent = "--";
    return;
  }

  cells[0].textContent = `${mode.temperature.optimal_max_c} C`;
  cells[1].textContent = `${mode.temperature.critical_c} C`;
  cells[2].textContent = `${mode.fan.base_pct}%`;
}

function render() {
  const status = state.status || {};
  const activeMode = status.mode || state.modes?.active_mode || "standard";

  el.statusValue.textContent = label(status.status);
  el.reasonValue.textContent = reasonLabel(status.reason);
  el.modeValue.textContent = label(activeMode);
  el.modeHint.textContent = "Perfil activo";
  el.networkValue.textContent = label(status.network);
  el.lastWillValue.textContent = label(status.last_will);
  el.doorValue.textContent = label(status.door_state);
  el.doorSecurityValue.textContent = reasonLabel(status.door_security_state);
  el.temperatureValue.textContent = formatNumber(status.temperature_c, "C");
  el.humidityValue.textContent = formatNumber(status.humidity_pct, "%");
  el.positionValue.textContent = label(status.mpu_position);
  el.accessValue.textContent = label(status.access_event);
  el.doorLockValue.textContent = label(status.door_lock_state);
  el.fanModeValue.textContent = label(status.fan_mode);
  el.fanPowerValue.textContent = formatNumber(status.fan_power_pct, "%", 0);
  el.lastUpdateValue.textContent = new Date().toLocaleTimeString();

  if (Number.isFinite(Number(status.fan_power_pct))) {
    el.fanPowerInput.value = Number(status.fan_power_pct);
    el.fanPowerOutput.value = `${Number(status.fan_power_pct).toFixed(0)}%`;
  }

  el.modeButtons.querySelectorAll("button").forEach((button) => {
    button.classList.toggle("active", button.dataset.mode === activeMode);
  });

  updateStatStates(status);
  renderModeDetails(activeMode);
}

async function refresh(options = {}) {
  const silent = Boolean(options.silent);
  if (state.refreshing) return;

  try {
    state.refreshing = true;
    if (!silent) setBusy(true);
    const [status, modes] = await Promise.all([
      request("/api/rack/status"),
      request("/api/rack/modes"),
    ]);
    state.status = status;
    state.modes = modes;
    render();
  } catch (error) {
    if (!silent) showToast(`API: ${error.message}`, "error");
    logEvent(`Error API: ${error.message}`);
  } finally {
    state.refreshing = false;
    if (!silent) setBusy(false);
  }
}

async function runCommand(labelText, action) {
  try {
    setBusy(true);
    await action();
    showToast(labelText, "ok");
    logEvent(labelText);
    await refresh();
  } catch (error) {
    showToast(error.message, "error");
    logEvent(`Error: ${error.message}`);
  } finally {
    setBusy(false);
  }
}

el.apiBaseInput.value = state.apiBase;
el.grafanaLink.href = GRAFANA_URL;
el.apiDocsLink.href = apiUrl("/docs");

el.saveApiButton.addEventListener("click", () => {
  state.apiBase = el.apiBaseInput.value.trim() || DEFAULT_API_BASE;
  localStorage.setItem(STORAGE_KEY, state.apiBase);
  el.apiDocsLink.href = apiUrl("/docs");
  showToast("API guardada", "ok");
  refresh();
});

el.refreshButton.addEventListener("click", refresh);

el.modeButtons.addEventListener("click", (event) => {
  const button = event.target.closest("button[data-mode]");
  if (!button) return;
  const mode = button.dataset.mode;
  runCommand(`Modo ${label(mode)}`, () =>
    request("/api/rack/mode", {
      method: "PUT",
      body: JSON.stringify({ mode }),
    }),
  );
});

el.unlockButton.addEventListener("click", () => {
  const username = el.unlockUsername.value.trim();
  const password = el.unlockPassword.value;
  runCommand("Desbloqueo solicitado", () =>
    request("/api/door/unlock", {
      method: "POST",
      body: JSON.stringify({ username, password }),
    }),
  );
});

el.lockButton.addEventListener("click", () => {
  runCommand("Cierre solicitado", () =>
    request("/api/door/lock", {
      method: "POST",
      body: JSON.stringify({}),
    }),
  );
});

el.fanPowerInput.addEventListener("input", () => {
  el.fanPowerOutput.value = `${el.fanPowerInput.value}%`;
});

el.fanManualButton.addEventListener("click", () => {
  const power_pct = Number(el.fanPowerInput.value);
  runCommand(`Fan manual ${power_pct}%`, () =>
    request("/api/fan/manual", {
      method: "POST",
      body: JSON.stringify({ power_pct }),
    }),
  );
});

el.fanAutoButton.addEventListener("click", () => {
  runCommand("Fan automatico", () =>
    request("/api/fan/auto", {
      method: "POST",
      body: JSON.stringify({}),
    }),
  );
});

refresh();
window.setInterval(() => {
  if (!state.busy) refresh({ silent: true });
}, 5000);
