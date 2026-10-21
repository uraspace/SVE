#include <Arduino.h>          // necessário ao integrar no main.cpp
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

/*
  Convenção de nomes:
   - funções e variáveis: snake_case em português, sem acento
   - constantes: MAIUSCULO_SNAKE em português, com sufixo da unidade (_MS, _MS2, _C)
   - tipos: PascalCase; valores de enum: MAIUSCULO

  TODO:
   - [] Refazer o sistema de display de altitude
   - [] Adicionar funções que faltam
   - [] Guardar os dados em SD
   - [x] Implementar os estados de voo
   - [] Otimizar o código para I2C
   - [] QUANDO IMPLEMENTADO remover setup() e loop() e remover as travas geradas pelas funções nos outros sensores
*/

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------
constexpr unsigned long TAXA_BAUD                     = 115200; // velocidade da porta serial (bits/s)
constexpr float         LIMIAR_DECOLAGEM_MS2          = 19.6f;  // 2 g (a faixa do sensor é de +-16 g)
constexpr float         TEMPERATURA_CRITICA_C         = 35.0f;
constexpr unsigned long INTERVALO_ESPERA_DECOLAGEM_MS = 100;
constexpr unsigned long INTERVALO_LOOP_MS             = 10;

// ---------------------------------------------------------------------------
// Estados de voo
// ---------------------------------------------------------------------------
enum class EstadoVoo {
  RAMPA,
  QUEIMA,
  COASTING,
  DESCIDA,
  POUSO,
  REPOUSO
};

// ---------------------------------------------------------------------------
// Variáveis globais
// ---------------------------------------------------------------------------
Adafruit_MPU6050 mpu;

EstadoVoo     estado_atual  = EstadoVoo::RAMPA;  // ainda não usado pela lógica de voo
//bool          voo_iniciado  = false;
unsigned long inicio_subida = 0;

// ---------------------------------------------------------------------------
// Sensor
// ---------------------------------------------------------------------------
void inicializar_sensor() {
  while (!mpu.begin()) {
    Serial.println("ERROR INITIALIZING MPU6050");
  }
  Serial.println("MPU6050 Initialized");
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
}

float ler_aceleracao_z() {
  sensors_event_t evento_acel, evento_giro, evento_temp;
  mpu.getEvent(&evento_acel, &evento_giro, &evento_temp);
  return evento_acel.acceleration.z;
}

// ---------------------------------------------------------------------------
// Fases do voo
// ---------------------------------------------------------------------------
void aguardar_decolagem() {
  float aceleracao_z = 0;
  while (aceleracao_z < LIMIAR_DECOLAGEM_MS2) {
    aceleracao_z = ler_aceleracao_z();
    millis(INTERVALO_ESPERA_DECOLAGEM_MS);
    Serial.println("check");
    Serial.println(aceleracao_z, 1);
  }
  Serial.println("Aceleração vertical (z) acima do limiar de decolagem: Inicio de voo detectado.");
  // voo_iniciado  = true; // PODE SER ATUALIZADA PARA EstadoVoo::QUEIMA
  estado_atual = EstadoVoo::QUEIMA;
  inicio_subida = millis();
}

void registrar_fim_da_subida(float aceleracao_z) {
  const unsigned long tempo_subida = millis() - inicio_subida;

  Serial.print("Detectada aceleracao vertical negativa: ");
  Serial.println(aceleracao_z, 1);
  Serial.print("Tempo de subida: ");
  Serial.print(tempo_subida / 1000);
  Serial.println(" segundos");
}

void processar_voo() {
  sensors_event_t evento_acel, evento_giro, evento_temp;
  mpu.getEvent(&evento_acel, &evento_giro, &evento_temp);

  Serial.println(evento_acel.acceleration.z);

  // Detectar fim da queima (não detecta o apogeu, deve ser detectado com o barometro)
  if (evento_acel.acceleration.z < 0) {
    registrar_fim_da_subida(evento_acel.acceleration.z);
    //voo_iniciado = false;
    estado_atual = EstadoVoo::COASTING;

  }

  if (evento_temp.temperature > TEMPERATURA_CRITICA_C) {
    Serial.print("TEMPERATURA CRITICA EXCEDIDA!: ");
    Serial.println(evento_temp.temperature, 1);
  }
}

// ---------------------------------------------------------------------------
// RUN (deve ser integrado ao main.cpp, versão de testes apenas)
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(TAXA_BAUD);
  Serial.println("ESP32 Inicializado");

  inicializar_sensor();
  aguardar_decolagem();
}

void loop() {
 // if (voo_iniciado) {
   if(estado_atual == EstadoVoo::QUEIMA){ 
    processar_voo();
  }
 millis(INTERVALO_LOOP_MS);
}
