#include <Wire.h>
#include "estado_voo.h"
#include "MPU6050/mpu6050.h"


constexpr unsigned long TAXA_BAUD                     = 115200; // velocidade da porta serial (bits/s)
constexpr unsigned long INTERVALO_ESPERA_DECOLAGEM_MS = 100;
constexpr unsigned long INTERVALO_LOOP_MS             = 10;


bool          mpu6050_pronto            = false;
unsigned long instante_ultimo_ciclo_ms  = 0;
unsigned long instante_ultima_espera_ms = 0;



// True a cada vez que passou pelo menos intervalo_ms desde a última vez.
bool intervalo_passou(unsigned long &instante_ultimo_ms, unsigned long intervalo_ms) {
  const unsigned long agora_ms = millis();
  if (agora_ms - instante_ultimo_ms < intervalo_ms) {
    return false;
  }
  instante_ultimo_ms = agora_ms;
  return true;
}

// ---------------------------------------------------------------------------
// MPU6050
// ---------------------------------------------------------------------------
void registrar_fim_da_subida(float aceleracao_z) { //Por enquanto só registra o fim da queima
  Serial.print("Detectada aceleracao vertical negativa: ");
  Serial.println(aceleracao_z, 1);
  Serial.print("Tempo de subida: ");
  Serial.print(mpu6050_obter_tempo_subida_ms() / 1000);
  Serial.println(" segundos");
}

void tratar_mpu6050() {
  const EstadoVoo estado_anterior = mpu6050_obter_estado();

  if (!mpu6050_atualizar()) {
    Serial.println("ERRO: falha na leitura do MPU6050");
    return;
  }

  const EstadoVoo estado_atual = mpu6050_obter_estado();
  const float     aceleracao_z = mpu6050_obter_aceleracao_z();

  switch (estado_anterior) {
    case EstadoVoo::RAMPA:
      if (estado_atual == EstadoVoo::QUEIMA) {
        Serial.println("Aceleração vertical (z) acima do limiar de decolagem: Inicio de voo detectado.");
      } else if (intervalo_passou(instante_ultima_espera_ms, INTERVALO_ESPERA_DECOLAGEM_MS)) {
        Serial.println("check");
        Serial.println(aceleracao_z, 1);
      }
      break;

    case EstadoVoo::QUEIMA:
      Serial.println(aceleracao_z);

      if (estado_atual == EstadoVoo::COASTING) {
        registrar_fim_da_subida(aceleracao_z);
      }

      if (mpu6050_temperatura_critica()) {
        Serial.print("TEMPERATURA CRITICA EXCEDIDA!: ");
        Serial.println(mpu6050_obter_temperatura(), 1);
      }
      break;

    default:
      break;  // demais estados ainda não são tratados
  }
}

// ---------------------------------------------------------------------------
// ESP32
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(TAXA_BAUD);
  Serial.println("ESP32 Inicializado");

  Wire.begin();  // pinos padrão: GPIO21 (SDA) e GPIO22 (SCL)

  mpu6050_pronto = mpu6050_inicializar();
  if (mpu6050_pronto) {
    Serial.println("MPU6050 Initialized");
  } else {
    Serial.println("ERROR INITIALIZING MPU6050");
  }
}

void loop() {
  if (mpu6050_pronto && intervalo_passou(instante_ultimo_ciclo_ms, INTERVALO_LOOP_MS)) {
    tratar_mpu6050();
  }
}
