#include <LiquidCrystal.h>

// ─── PINOS ───────────────────────────────────────────────────────
const int PIN_POT_TEMP    = A0;
const int PIN_POT_CARGA   = A1;
const int PIN_LED_VERDE   = 2;
const int PIN_LED_AMARELO = 3;
const int PIN_LED_VERM    = 4;
const int PIN_LED_AZUL    = 5;
const int PIN_BUZZER      = 8;
const int PIN_MOTOR       = 9;

// LCD: RS=12, E=11, D4=7, D5=6, D6=10, D7=13
LiquidCrystal lcd(12, 11, 7, 6, 10, 13);

// ─── ESTADOS (0=NORMAL, 1=ALERTA, 2=CRITICO) ─────────────────────
int estadoAtual    = 0;
int estadoAnterior = 0;

// ─── LIMIARES ────────────────────────────────────────────────────
float TEMP_ALERTA  = 35.0;
float TEMP_CRITICO = 45.0;

// ─── VELOCIDADES DO MOTOR ────────────────────────────────────────
const int MOTOR_DESLIGADO = 0;
const int MOTOR_DEVAGAR   = 120;
const int MOTOR_RAPIDO    = 255;

// ─── CONTROLE DE BUZZER ──────────────────────────────────────────
unsigned long tempoUltimoBuzz = 0;
bool buzzerLigado = false;

// ─── CONTROLE DE LCD E LOG ───────────────────────────────────────
unsigned long tempoUltimoLCD = 0;
unsigned long tempoUltimoLog = 0;

// ─── LEITURAS ────────────────────────────────────────────────────
float tempAtual  = 25.0;
int   cargaAtual = 0;

// ─── CARACTERE CUSTOMIZADO ───────────────────────────────────────
byte charTermometro[8] = {
  0b00100, 0b01010, 0b01010,
  0b01110, 0b01110, 0b11111,
  0b11111, 0b01110
};

// ═════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_VERDE,   OUTPUT);
  pinMode(PIN_LED_AMARELO, OUTPUT);
  pinMode(PIN_LED_VERM,    OUTPUT);
  pinMode(PIN_LED_AZUL,    OUTPUT);
  pinMode(PIN_BUZZER,      OUTPUT);
  pinMode(PIN_MOTOR,       OUTPUT);

  analogWrite(PIN_MOTOR, MOTOR_DESLIGADO);

  lcd.begin(16, 2);
  lcd.createChar(0, charTermometro);

  // Boot
  lcd.setCursor(0, 0);
  lcd.print("DATASHIELD ORBIT");
  lcd.setCursor(0, 1);
  lcd.print("  INICIALIZANDO ");
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOOT: ");
  for (int i = 0; i < 10; i++) {
    lcd.print("|");
    delay(100);
  }

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("SISTEMA OK!");
  lcd.setCursor(0, 1);
  lcd.print("AUTONOMO ATIVO");

  int leds[4] = {PIN_LED_VERDE, PIN_LED_AMARELO, PIN_LED_VERM, PIN_LED_AZUL};
  for (int k = 0; k < 3; k++) {
    for (int i = 0; i < 4; i++) {
      digitalWrite(leds[i], HIGH);
      delay(120);
      digitalWrite(leds[i], LOW);
    }
  }

  tone(PIN_BUZZER, 1000, 150);
  delay(200);
  tone(PIN_BUZZER, 1500, 150);
  delay(800);
  lcd.clear();

  Serial.println("=== DATASHIELD ORBITAL INICIADO ===");
}

// ═════════════════════════════════════════════════════════════════
void loop() {
  lerSensores();
  atualizarEstado();
  controlarAtuadores();
  atualizarLCD();
  logSerial();
  delay(100);
}

// ─────────────────────────────────────────────────────────────────
void lerSensores() {
  int rawTemp  = analogRead(PIN_POT_TEMP);
  int rawCarga = analogRead(PIN_POT_CARGA);
  tempAtual  = map(rawTemp,  0, 1023, 200, 600) / 10.0;
  cargaAtual = map(rawCarga, 0, 1023, 0, 100);
}

// ─────────────────────────────────────────────────────────────────
void atualizarEstado() {
  estadoAnterior = estadoAtual;

  if (tempAtual >= TEMP_CRITICO) {
    estadoAtual = 2;
  } else if (tempAtual >= TEMP_ALERTA) {
    estadoAtual = 1;
  } else {
    estadoAtual = 0;
  }

  if (estadoAtual != estadoAnterior) {
    Serial.print("[TRANSICAO] -> ");
    if (estadoAtual == 0) Serial.println("NORMAL");
    else if (estadoAtual == 1) Serial.println("ALERTA");
    else Serial.println("CRITICO");
  }
}

// ─────────────────────────────────────────────────────────────────
void controlarAtuadores() {
  digitalWrite(PIN_LED_VERDE,   LOW);
  digitalWrite(PIN_LED_AMARELO, LOW);
  digitalWrite(PIN_LED_VERM,    LOW);
  digitalWrite(PIN_LED_AZUL,    LOW);

  if (estadoAtual == 0) {
    digitalWrite(PIN_LED_VERDE, HIGH);
    if (cargaAtual > 0) digitalWrite(PIN_LED_AZUL, HIGH);
    analogWrite(PIN_MOTOR, MOTOR_DESLIGADO);
    noTone(PIN_BUZZER);
    buzzerLigado = false;

  } else if (estadoAtual == 1) {
    digitalWrite(PIN_LED_AMARELO, HIGH);
    if (cargaAtual > 0) digitalWrite(PIN_LED_AZUL, HIGH);
    analogWrite(PIN_MOTOR, MOTOR_DEVAGAR);
    buzzerIntermitente();

  } else if (estadoAtual == 2) {
    digitalWrite(PIN_LED_VERM, HIGH);
    analogWrite(PIN_MOTOR, MOTOR_RAPIDO);
    tone(PIN_BUZZER, 2000);
    buzzerLigado = true;
  }
}

// ─────────────────────────────────────────────────────────────────
void buzzerIntermitente() {
  unsigned long agora = millis();
  if (!buzzerLigado) {
    if (agora - tempoUltimoBuzz >= 1000) {
      tone(PIN_BUZZER, 1500, 100);
      tempoUltimoBuzz = agora;
      buzzerLigado = true;
    }
  } else {
    if (agora - tempoUltimoBuzz >= 100) {
      noTone(PIN_BUZZER);
      buzzerLigado = false;
      tempoUltimoBuzz = agora;
    }
  }
}

// ─────────────────────────────────────────────────────────────────
void atualizarLCD() {
  if (millis() - tempoUltimoLCD < 500) return;
  tempoUltimoLCD = millis();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("DATASHIELD ORBIT");

  lcd.setCursor(0, 1);
  lcd.print("T:");
  if ((int)tempAtual < 10) lcd.print(" ");
  lcd.print((int)tempAtual);
  lcd.print("  ");
  lcd.print("C:");
  if (cargaAtual < 10)       lcd.print("  ");
  else if (cargaAtual < 100) lcd.print(" ");
  lcd.print(cargaAtual);
  lcd.print("%");
}

// ─────────────────────────────────────────────────────────────────
void logSerial() {
  if (millis() - tempoUltimoLog < 2000) return;
  tempoUltimoLog = millis();

  Serial.print("ESTADO=");
  if (estadoAtual == 0)      Serial.print("NORMAL");
  else if (estadoAtual == 1) Serial.print("ALERTA");
  else                       Serial.print("CRITICO");

  Serial.print(" | TEMP=");
  Serial.print(tempAtual, 1);
  Serial.print("C | CARGA=");
  Serial.print(cargaAtual);
  Serial.print("% | MOTOR=");
  if (estadoAtual == 0)      Serial.println("DESLIGADO");
  else if (estadoAtual == 1) Serial.println("DEVAGAR");
  else                       Serial.println("RAPIDO");
}