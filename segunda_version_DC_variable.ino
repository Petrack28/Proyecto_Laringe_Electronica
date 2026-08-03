// ============================================================================
// Laringe Electrónica - WiFi AP + Servidor Web + Memoria Flash
// Ahora con control de Ciclo de Trabajo (Duty Cycle) ajustable
// ============================================================================

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// ---- Pines ----
const int SOLENOIDE_PIN = 8;
const int BTN_BASE  = 5;
const int BTN_AGUDO = 6;
const int BTN_GRAVE = 7;

const int PWM_RESOLUCION = 10;      // 10 bits -> valores de 0 a 1023
const int PWM_MAX = 1023;           // 2^10 - 1

// ---- WiFi AP ----
const char* AP_SSID = "Laringe_ESP32";
const char* AP_PASS = "laringe123";

// ---- Servidor y Preferencias ----
WebServer server(80);
Preferences prefs;

// ---- Frecuencias (se cargan desde flash al inicio) ----
int FREC_BASE  = 75;
int FREC_AGUDO = 90;
int FREC_GRAVE = 60;

// ---- Ciclo de trabajo en % (0-100) POR CADA FRECUENCIA, se cargan desde flash ----
int DUTY_BASE  = 25;   // ej. 40 Hz -> periodo largo, duty bajo alcanza
int DUTY_AGUDO = 50;   // ej. 100 Hz -> periodo corto, necesita mas duty
int DUTY_GRAVE = 20;

int frecuenciaAnterior = -1;

// ============================================================
// Convierte porcentaje (0-100) a valor PWM (0-1023)
// ============================================================
int porcentajeADuty(int porcentaje) {
  porcentaje = constrain(porcentaje, 0, 100);
  return (porcentaje * PWM_MAX) / 100;
}

// ============================================================
// HTML de la app web
// ============================================================
const char* HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Laringe Electrónica</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: #eee;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 30px 20px;
      min-height: 100vh;
    }
    h1 {
      font-size: 1.5rem;
      margin-bottom: 8px;
      color: #00d4ff;
    }
    p.sub {
      font-size: 0.85rem;
      color: #aaa;
      margin-bottom: 30px;
      text-align: center;
    }
    .card {
      background: #16213e;
      border-radius: 16px;
      padding: 24px;
      width: 100%;
      max-width: 400px;
      margin-bottom: 16px;
      box-shadow: 0 4px 15px rgba(0,0,0,0.3);
    }
    .card h2 {
      font-size: 1rem;
      margin-bottom: 16px;
      color: #00d4ff;
    }
    .slider-row {
      display: flex;
      align-items: center;
      gap: 12px;
      margin-bottom: 12px;
    }
    .slider-row label {
      width: 60px;
      font-size: 0.85rem;
      color: #aaa;
    }
    input[type=range] {
      flex: 1;
      accent-color: #00d4ff;
    }
    input[type=range].duty {
      accent-color: #ff9900;
    }
    .val {
      width: 55px;
      text-align: right;
      font-weight: bold;
      color: #fff;
      font-size: 0.95rem;
    }
    .divider {
      border-top: 1px solid #2a3a5c;
      margin: 16px 0;
    }
    button {
      width: 100%;
      padding: 14px;
      border: none;
      border-radius: 12px;
      font-size: 1rem;
      font-weight: bold;
      cursor: pointer;
      margin-top: 8px;
      transition: opacity 0.2s;
    }
    button:active { opacity: 0.7; }
    .btn-save {
      background: #00d4ff;
      color: #1a1a2e;
    }
    .btn-test {
      background: #0f3460;
      color: #00d4ff;
      border: 1px solid #00d4ff;
    }
    .status {
      margin-top: 16px;
      padding: 12px;
      border-radius: 10px;
      text-align: center;
      font-size: 0.9rem;
      display: none;
    }
    .status.ok  { background: #0d3b2e; color: #00ff99; display: block; }
    .status.err { background: #3b0d0d; color: #ff6060; display: block; }
  </style>
</head>
<body>
  <h1>🎙️ Laringe Electrónica</h1>
  <p class="sub">Ajusta y guarda las frecuencias y el ciclo de trabajo en memoria flash</p>

  <div class="card">
    <h2>Frecuencias (Hz)</h2>

    <div class="slider-row">
      <label>BASE</label>
      <input type="range" id="fBase" min="20" max="300" value="75"
             oninput="document.getElementById('vBase').textContent=this.value+' Hz'">
      <span class="val" id="vBase">75 Hz</span>
    </div>
    <div class="slider-row">
      <label>Duty</label>
      <input type="range" class="duty" id="dBase" min="5" max="100" value="25"
             oninput="document.getElementById('vdBase').textContent=this.value+' %'">
      <span class="val" id="vdBase">25 %</span>
    </div>

    <div class="divider"></div>

    <div class="slider-row">
      <label>AGUDO</label>
      <input type="range" id="fAgudo" min="20" max="300" value="90"
             oninput="document.getElementById('vAgudo').textContent=this.value+' Hz'">
      <span class="val" id="vAgudo">90 Hz</span>
    </div>
    <div class="slider-row">
      <label>Duty</label>
      <input type="range" class="duty" id="dAgudo" min="5" max="100" value="50"
             oninput="document.getElementById('vdAgudo').textContent=this.value+' %'">
      <span class="val" id="vdAgudo">50 %</span>
    </div>

    <div class="divider"></div>

    <div class="slider-row">
      <label>GRAVE</label>
      <input type="range" id="fGrave" min="20" max="300" value="60"
             oninput="document.getElementById('vGrave').textContent=this.value+' Hz'">
      <span class="val" id="vGrave">60 Hz</span>
    </div>
    <div class="slider-row">
      <label>Duty</label>
      <input type="range" class="duty" id="dGrave" min="5" max="100" value="20"
             oninput="document.getElementById('vdGrave').textContent=this.value+' %'">
      <span class="val" id="vdGrave">20 %</span>
    </div>

    <button class="btn-test" onclick="testFrecuencias()">🔊 Probar frecuencias</button>
    <button class="btn-save" onclick="guardarFrecuencias()">💾 Guardar en memoria flash</button>

    <div class="status" id="status"></div>
  </div>

  <script>
    window.onload = () => {
      fetch('/get')
        .then(r => r.json())
        .then(d => {
          document.getElementById('fBase').value  = d.base;
          document.getElementById('fAgudo').value = d.agudo;
          document.getElementById('fGrave').value = d.grave;
          document.getElementById('dBase').value  = d.dutyBase;
          document.getElementById('dAgudo').value = d.dutyAgudo;
          document.getElementById('dGrave').value = d.dutyGrave;
          document.getElementById('vBase').textContent  = d.base  + ' Hz';
          document.getElementById('vAgudo').textContent = d.agudo + ' Hz';
          document.getElementById('vGrave').textContent = d.grave + ' Hz';
          document.getElementById('vdBase').textContent  = d.dutyBase  + ' %';
          document.getElementById('vdAgudo').textContent = d.dutyAgudo + ' %';
          document.getElementById('vdGrave').textContent = d.dutyGrave + ' %';
        });
    };

    function guardarFrecuencias() {
      const base  = document.getElementById('fBase').value;
      const agudo = document.getElementById('fAgudo').value;
      const grave = document.getElementById('fGrave').value;
      const dBase  = document.getElementById('dBase').value;
      const dAgudo = document.getElementById('dAgudo').value;
      const dGrave = document.getElementById('dGrave').value;

      fetch(`/set?base=${base}&agudo=${agudo}&grave=${grave}&dutyBase=${dBase}&dutyAgudo=${dAgudo}&dutyGrave=${dGrave}`)
        .then(r => r.json())
        .then(d => mostrarStatus(d.ok ? '✅ Guardado en flash correctamente' : '❌ Error al guardar', d.ok))
        .catch(() => mostrarStatus('❌ Error de conexión', false));
    }

    function testFrecuencias() {
      const base  = document.getElementById('fBase').value;
      const agudo = document.getElementById('fAgudo').value;
      const grave = document.getElementById('fGrave').value;
      const dBase  = document.getElementById('dBase').value;
      const dAgudo = document.getElementById('dAgudo').value;
      const dGrave = document.getElementById('dGrave').value;

      fetch(`/test?base=${base}&agudo=${agudo}&grave=${grave}&dutyBase=${dBase}&dutyAgudo=${dAgudo}&dutyGrave=${dGrave}`)
        .then(() => mostrarStatus('🔊 Probando frecuencias...', true));
    }

    function mostrarStatus(msg, ok) {
      const s = document.getElementById('status');
      s.textContent = msg;
      s.className = 'status ' + (ok ? 'ok' : 'err');
      setTimeout(() => s.className = 'status', 3000);
    }
  </script>
</body>
</html>
)rawliteral";

// ============================================================
// Funciones del servidor
// ============================================================

void handleRoot() {
  server.send(200, "text/html", HTML);
}

void handleGet() {
  String json = "{\"base\":" + String(FREC_BASE) +
                ",\"agudo\":" + String(FREC_AGUDO) +
                ",\"grave\":" + String(FREC_GRAVE) +
                ",\"dutyBase\":" + String(DUTY_BASE) +
                ",\"dutyAgudo\":" + String(DUTY_AGUDO) +
                ",\"dutyGrave\":" + String(DUTY_GRAVE) + "}";
  server.send(200, "application/json", json);
}

void handleSet() {
  if (server.hasArg("base") && server.hasArg("agudo") && server.hasArg("grave") &&
      server.hasArg("dutyBase") && server.hasArg("dutyAgudo") && server.hasArg("dutyGrave")) {
    FREC_BASE  = server.arg("base").toInt();
    FREC_AGUDO = server.arg("agudo").toInt();
    FREC_GRAVE = server.arg("grave").toInt();
    DUTY_BASE  = constrain(server.arg("dutyBase").toInt(),  5, 100);
    DUTY_AGUDO = constrain(server.arg("dutyAgudo").toInt(), 5, 100);
    DUTY_GRAVE = constrain(server.arg("dutyGrave").toInt(), 5, 100);

    prefs.begin("laringe", false);
    prefs.putInt("fBase",  FREC_BASE);
    prefs.putInt("fAgudo", FREC_AGUDO);
    prefs.putInt("fGrave", FREC_GRAVE);
    prefs.putInt("dBase",  DUTY_BASE);
    prefs.putInt("dAgudo", DUTY_AGUDO);
    prefs.putInt("dGrave", DUTY_GRAVE);
    prefs.end();

    server.send(200, "application/json", "{\"ok\":true}");
    Serial.printf("Guardado -> Base:%d(%d%%) Agudo:%d(%d%%) Grave:%d(%d%%)\n",
                  FREC_BASE, DUTY_BASE, FREC_AGUDO, DUTY_AGUDO, FREC_GRAVE, DUTY_GRAVE);
  } else {
    server.send(400, "application/json", "{\"ok\":false}");
  }
}

void handleTest() {
  int base  = server.arg("base").toInt();
  int agudo = server.arg("agudo").toInt();
  int grave = server.arg("grave").toInt();
  int dBase  = server.hasArg("dutyBase")  ? constrain(server.arg("dutyBase").toInt(),  5, 100) : DUTY_BASE;
  int dAgudo = server.hasArg("dutyAgudo") ? constrain(server.arg("dutyAgudo").toInt(), 5, 100) : DUTY_AGUDO;
  int dGrave = server.hasArg("dutyGrave") ? constrain(server.arg("dutyGrave").toInt(), 5, 100) : DUTY_GRAVE;

  auto tocar = [&](int frec, int dutyPorc) {
    ledcDetach(SOLENOIDE_PIN);
    ledcAttach(SOLENOIDE_PIN, frec, PWM_RESOLUCION);
    ledcWrite(SOLENOIDE_PIN, porcentajeADuty(dutyPorc));
    delay(1000);
    ledcWrite(SOLENOIDE_PIN, 0);
    delay(200);
  };

  tocar(base, dBase);
  tocar(agudo, dAgudo);
  tocar(grave, dGrave);

  server.send(200, "application/json", "{\"ok\":true}");
}

// ============================================================
// Actualizar frecuencia PWM (usa el duty cycle correspondiente a esa frecuencia)
// ============================================================
void actualizarFrecuencia(int frecNueva, int dutyPorc) {
  if (frecNueva == frecuenciaAnterior) return;
  ledcDetach(SOLENOIDE_PIN);
  ledcAttach(SOLENOIDE_PIN, frecNueva, PWM_RESOLUCION);
  ledcWrite(SOLENOIDE_PIN, porcentajeADuty(dutyPorc));
  frecuenciaAnterior = frecNueva;
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Cargar frecuencias y duty cycles desde flash
  prefs.begin("laringe", true);
  FREC_BASE  = prefs.getInt("fBase",  75);
  FREC_AGUDO = prefs.getInt("fAgudo", 90);
  FREC_GRAVE = prefs.getInt("fGrave", 60);
  DUTY_BASE  = prefs.getInt("dBase",  25);
  DUTY_AGUDO = prefs.getInt("dAgudo", 50);
  DUTY_GRAVE = prefs.getInt("dGrave", 20);
  prefs.end();
  Serial.printf("Cargado -> Base:%d(%d%%) Agudo:%d(%d%%) Grave:%d(%d%%)\n",
                FREC_BASE, DUTY_BASE, FREC_AGUDO, DUTY_AGUDO, FREC_GRAVE, DUTY_GRAVE);

  // Botones
  pinMode(BTN_BASE,  INPUT_PULLUP);
  pinMode(BTN_AGUDO, INPUT_PULLUP);
  pinMode(BTN_GRAVE, INPUT_PULLUP);

  // PWM
  ledcAttach(SOLENOIDE_PIN, FREC_BASE, PWM_RESOLUCION);
  ledcWrite(SOLENOIDE_PIN, 0);

  // WiFi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP iniciado -> IP: %s\n", WiFi.softAPIP().toString().c_str());

  // Rutas
  server.on("/",     handleRoot);
  server.on("/get",  handleGet);
  server.on("/set",  handleSet);
  server.on("/test", handleTest);
  server.begin();

  Serial.println("Servidor web iniciado");
}

// ============================================================
// Loop
// ============================================================
void loop() {
  server.handleClient();

  bool presion_base  = (digitalRead(BTN_BASE)  == HIGH);
  bool presion_agudo = (digitalRead(BTN_AGUDO) == HIGH);
  bool presion_grave = (digitalRead(BTN_GRAVE) == HIGH);

  if (presion_agudo) {
    actualizarFrecuencia(FREC_AGUDO, DUTY_AGUDO);
  }
  else if (presion_grave) {
    actualizarFrecuencia(FREC_GRAVE, DUTY_GRAVE);
  }
  else if (presion_base) {
    actualizarFrecuencia(FREC_BASE, DUTY_BASE);
  }
  else {
    ledcWrite(SOLENOIDE_PIN, 0);
    frecuenciaAnterior = -1;
  }
}
