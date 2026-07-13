const int SOLENOIDE_PIN = 8;
const int BTN_BASE = 5;
const int BTN_AGUDO = 6;
const int BTN_GRAVE = 7;

const int PWM_RESOLUCION = 10;

const int FREC_BASE = 75;
const int FREC_AGUDA = 90;
const int FREC_GRAVE = 60;

const int DUTY_OPTIMO = 800;

int frecuenciaAnterior = -1;

void actualizarFrecuencia(int frecNueva) {
  if (frecNueva == frecuenciaAnterior) return; // no reconfigura si es la misma

  ledcDetach(SOLENOIDE_PIN);
  ledcAttach(SOLENOIDE_PIN, frecNueva, PWM_RESOLUCION);
  ledcWrite(SOLENOIDE_PIN, DUTY_OPTIMO);

  frecuenciaAnterior = frecNueva;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(BTN_BASE, INPUT_PULLUP);
  pinMode(BTN_AGUDO, INPUT_PULLUP);
  pinMode(BTN_GRAVE, INPUT_PULLUP);

  ledcAttach(SOLENOIDE_PIN, FREC_BASE, PWM_RESOLUCION);
  ledcWrite(SOLENOIDE_PIN, 0);

  Serial.println("Sistema iniciado");
}

void loop() {
  bool presion_base  = (digitalRead(BTN_BASE) == HIGH);
  bool presion_agudo = (digitalRead(BTN_AGUDO) == HIGH);
  bool presion_grave = (digitalRead(BTN_GRAVE) == HIGH);

  // Sin delay — respuesta inmediata al cambio de botón
  if (presion_agudo) {
    actualizarFrecuencia(FREC_AGUDA);
    Serial.println("-> AGUDO");
  } 
  else if (presion_grave) {
    actualizarFrecuencia(FREC_GRAVE);
    Serial.println("-> GRAVE");
  } 
  else if (presion_base) {
    actualizarFrecuencia(FREC_BASE);
    Serial.println("-> BASE");
  } 
  else {
    ledcWrite(SOLENOIDE_PIN, 0);
    frecuenciaAnterior = -1;
  }
}
