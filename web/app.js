// =====================================================================
//  app.js  —  Logika website Smarthome (MQTT.js over WSS)
// =====================================================================
(function () {
  "use strict";
  const CFG = window.SMARTHOME_CONFIG;

  // ---------- Elemen ----------
  const brokerStatus = document.getElementById("brokerStatus");
  const deviceSelect = document.getElementById("deviceSelect");
  const deviceOnline = document.getElementById("deviceOnline");
  const cardsEl = document.getElementById("cards");
  const tpl = document.getElementById("cardTemplate");

  // ---------- State ----------
  let client = null;
  let currentDevice = pad(1);            // "01"
  const cards = [];                      // [{root, relayIdx, ...}] index 0,1

  function pad(n) { return String(n).padStart(2, "0"); }
  function base(dev) { return `${CFG.namespace}/alat-${dev}`; }

  // ---------- Build UI ----------
  function buildDeviceOptions() {
    for (let i = 1; i <= CFG.deviceCount; i++) {
      const opt = document.createElement("option");
      opt.value = pad(i);
      opt.textContent = `Alat ${pad(i)}`;
      deviceSelect.appendChild(opt);
    }
  }

  function buildCards() {
    for (let i = 0; i < 2; i++) {
      const node = tpl.content.firstElementChild.cloneNode(true);
      node.dataset.relay = String(i + 1);
      node.querySelector(".dev-name").textContent =
        CFG.deviceNames[i] || `Perangkat ${i + 1}`;

      const c = {
        root: node,
        badge: node.querySelector(".state-badge"),
        toggle: node.querySelector(".toggle"),
        state: false,
      };

      c.toggle.addEventListener("click", () => {
        const next = c.state ? "OFF" : "ON";
        publish(`${base(currentDevice)}/relay/${i + 1}/set`, next, false);
      });

      cards.push(c);
      cardsEl.appendChild(node);
    }
  }

  // ---------- MQTT ----------
  function connect() {
    const proto = CFG.protocol || "wss";
    const url = `${proto}://${CFG.host}:${CFG.port}${CFG.path || ""}`;
    setBroker("connecting", "Menyambung…");
    const opts = {
      clientId: "web-" + Math.random().toString(16).slice(2, 10),
      reconnectPeriod: 3000,
      clean: true,
    };
    if (CFG.username) opts.username = CFG.username;   // broker publik: anonim
    if (CFG.password) opts.password = CFG.password;
    client = mqtt.connect(url, opts);

    client.on("connect", () => {
      setBroker("connected", "Tersambung");
      subscribeDevice(currentDevice);
    });
    client.on("reconnect", () => setBroker("connecting", "Menyambung ulang…"));
    client.on("error", (e) => setBroker("error", "Error koneksi"));
    client.on("close", () => setBroker("error", "Terputus"));
    client.on("message", onMessage);
  }

  function subscribeDevice(dev) {
    client.subscribe(`${base(dev)}/#`, { qos: 1 });
  }
  function unsubscribeDevice(dev) {
    client.unsubscribe(`${base(dev)}/#`);
  }

  function publish(topic, payload, retain) {
    if (!client || !client.connected) return;
    client.publish(topic, payload, { qos: 1, retain: !!retain });
  }

  function onMessage(topic, payloadBuf) {
    const prefix = base(currentDevice) + "/";
    if (!topic.startsWith(prefix)) return;
    const sub = topic.slice(prefix.length);   // mis. "relay/1/state"
    const payload = payloadBuf.toString();

    if (sub === "status") {
      setDeviceOnline(payload === "online");
      return;
    }
    const m = sub.match(/^relay\/([12])\/state$/);
    if (!m) return;
    updateRelay(Number(m[1]) - 1, payload);
  }

  // ---------- UI update ----------
  function updateRelay(idx, payload) {
    const c = cards[idx];
    c.state = payload === "ON";
    c.root.classList.toggle("is-on", c.state);
    c.badge.textContent = c.state ? "ON" : "OFF";
  }

  function resetCards() {
    cards.forEach((c) => {
      c.state = false;
      c.root.classList.remove("is-on");
      c.badge.textContent = "OFF";
    });
    setDeviceOnline(false);
  }

  function setBroker(cls, text) {
    brokerStatus.className = "broker-status " + cls;
    brokerStatus.querySelector(".label").textContent = text;
    cardsEl.classList.toggle("disconnected", cls !== "connected");
  }

  function setDeviceOnline(online) {
    deviceOnline.className = "device-online " + (online ? "online" : "offline");
    deviceOnline.querySelector(".label").textContent = online ? "Online" : "Offline";
  }

  // ---------- Ganti alat ----------
  deviceSelect.addEventListener("change", () => {
    if (client && client.connected) unsubscribeDevice(currentDevice);
    currentDevice = deviceSelect.value;
    resetCards();
    if (client && client.connected) subscribeDevice(currentDevice);
  });

  // ---------- Init ----------
  buildDeviceOptions();
  buildCards();
  connect();
})();
