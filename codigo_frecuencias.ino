// ============================================================================
// Proyecto: Laringe Electrónica con ESP32 (Código Final con Botones)
// ============================================================================

// Definición de Pines según tu configuración
const int SOLENOIDE_PIN = 8;  // Salida de pulsos hacia la Base del TIP41C
const int BTN_BASE = 5;       // Tono normal (Frecuencia base)
const int BTN_AGUDO = 6;      // Tono alto (Para preguntas o énfasis)
const int BTN_GRAVE = 7;      // Tono bajo (Para finales de frase)

// Configuración del periférico PWM (LEDC del ESP32)
const int PWM_RESOLUCION = 10; // Resolución de 10 bits (Valores de 0 a 1023)

// Frecuencias optimizadas para el rango de voz humana (Masculina base)
const int FREC_BASE = 60;   // 110 Hz - Tono conversacional limpio
const int FREC_AGUDA = 60;  // 135 Hz - Tono agudo
const int FREC_GRAVE = 80;   // 90 Hz  - Tono grave

// Ciclo de trabajo (Duty Cycle) optimizado para transductores inductivos
// Un valor de 115 sobre 1023 equivale a ~11% de Duty Cycle.
const int DUTY_OPTIMO = 800; 

void setup() {
  Serial.begin(115200);
  delay(1000); // da tiempo a que el puerto USB CDC se estabilice

  // Configurar los pines de los botones con resistencia de Pull-Up interna
  // IMPORTANTE: Los botones deben ir conectados entre el GPIO y GND
  pinMode(BTN_BASE, INPUT_PULLUP);
  pinMode(BTN_AGUDO, INPUT_PULLUP);
  pinMode(BTN_GRAVE, INPUT_PULLUP);

  // Inicializa el pin con la frecuencia base y resolución
  ledcAttach(SOLENOIDE_PIN, FREC_BASE, PWM_RESOLUCION);
  
  // Asegurar que el solenoide comience completamente apagado
  ledcWrite(SOLENOIDE_PIN, 0); 

  Serial.println("Sistema iniciado correctamente");
}

void loop() {
  // Leer el estado físico de los botones (LOW significa presionado debido al Pull-Up)
  bool presion_base  = (digitalRead(BTN_BASE) == HIGH);
  bool presion_agudo = (digitalRead(BTN_AGUDO) == HIGH);
  bool presion_grave = (digitalRead(BTN_GRAVE) == HIGH);

  Serial.printf("Base:%d Agudo:%d Grave:%d\n", presion_base, presion_agudo, presion_grave);
  delay(200);

  // Control dinámico de frecuencia y activación por demanda (Prosodia)
  if (presion_agudo) {
    // Cambia la frecuencia a tono agudo y activa el solenoide con el duty óptimo
    ledcChangeFrequency(SOLENOIDE_PIN, FREC_AGUDA, PWM_RESOLUCION);
    ledcWrite(SOLENOIDE_PIN, DUTY_OPTIMO); 
    Serial.println("-> AGUDO activado");
  } 
  else if (presion_grave) {
    // Cambia la frecuencia a tono grave
    ledcChangeFrequency(SOLENOIDE_PIN, FREC_GRAVE, PWM_RESOLUCION);
    ledcWrite(SOLENOIDE_PIN, DUTY_OPTIMO);
    Serial.println("-> GRAVE activado");
  } 
  else if (presion_base) {
    // Mantiene la frecuencia base estándar
    ledcChangeFrequency(SOLENOIDE_PIN, FREC_BASE, PWM_RESOLUCION);
    ledcWrite(SOLENOIDE_PIN, DUTY_OPTIMO);
    Serial.println("-> BASE activado");
  } 
  else {
    // Si la persona suelta todos los botones, el solenoide se apaga inmediatamente
    ledcWrite(SOLENOIDE_PIN, 0); 
  }
}