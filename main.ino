#include <Wire.h>
#include "estado_voo.h"
#include "MPU6050/mpu6050.h"
#include "NEO-6M/sensor_neo6m.h"


constexpr unsigned long TAXA_BAUD                     = 115200; // velocidade da porta serial (bits/s)
constexpr unsigned long INTERVALO_ESPERA_DECOLAGEM_MS = 100;
constexpr unsigned long INTERVALO_LOOP_MS             = 10;


bool          mpu6050_pronto            = false;
unsigned long instante_ultimo_ciclo_ms  = 0;
unsigned long instante_ultima_espera_ms = 0;

// ---------------------------------------------------------------------------
// GPS NEO-6M: configuracoes adicionais
// ---------------------------------------------------------------------------
constexpr uint32_t CLOCK_I2C_GPS_HZ = 100000; // DDC do NEO-6: I2C Standard Mode
constexpr unsigned long INTERVALO_ATUALIZACAO_GPS_MS = 10;
constexpr unsigned long INTERVALO_STATUS_GPS_MS = 1000;

bool neo6m_pronto = false;
bool apogeu_gps_reportado = false;
double ultimo_marco_gps_impresso_m = 0.0;
unsigned long instante_ultima_atualizacao_gps_ms = 0;
unsigned long instante_ultimo_status_gps_ms = 0;



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
// GPS NEO-6M: funcoes adicionadas ao programa principal
// ---------------------------------------------------------------------------
void tratar_neo6m() {
  if (!neo6m_pronto) {
    return;
  }

  // A referencia de altitude e atualizada enquanto o MPU6050 indicar RAMPA.
  // Sem o MPU6050, nao ha uma transicao de voo confiavel para liberar essa referencia.
  const bool em_rampa = mpu6050_pronto &&
                        (mpu6050_obter_estado() == EstadoVoo::RAMPA);

  if (!neo6m::atualizar(em_rampa)) {
    return;
  }

  const neo6m::DadosGps dados = neo6m::obter_dados();

  if (dados.ultimo_marco_atingido_m > ultimo_marco_gps_impresso_m) {
    ultimo_marco_gps_impresso_m = dados.ultimo_marco_atingido_m;
    Serial.print("GPS: marco de altitude atingido: ");
    Serial.print(ultimo_marco_gps_impresso_m, 0);
    Serial.println(" m acima do lancamento");
  }

  if (neo6m::apogeu_detectado() && !apogeu_gps_reportado) {
    Serial.println("GPS: criterio preliminar de apogeu detectado.");
    apogeu_gps_reportado = true;

    // TODO: integrar o evento a maquina de estados quando houver uma funcao
    // publica definida para registrar a transicao para DESCIDA.
    // A estimativa GPS nao deve comandar sozinha o sistema de recuperacao.
  }
}

void imprimir_status_gps() {
  if (!neo6m_pronto) {
    Serial.println("GPS NEO-6M indisponivel");
    return;
  }

  const neo6m::DadosGps dados = neo6m::obter_dados();

  Serial.print("GPS | fix: ");
  Serial.print(dados.fix_valido ? "valido" : "indisponivel");
  Serial.print(" | satelites: ");
  Serial.print(dados.satelites);
  Serial.print(" | altitude absoluta: ");
  Serial.print(dados.altitude_absoluta_m, 1);
  Serial.print(" m | ");

  if (neo6m::referencia_definida()) {
    Serial.print("altitude relativa: ");
    Serial.print(dados.altitude_relativa_m, 1);
    Serial.print(" m | maxima: ");
    Serial.print(dados.altitude_maxima_m, 1);
    Serial.print(" m | velocidade vertical: ");
    Serial.print(dados.velocidade_vertical_ms, 1);
    Serial.print(" m/s");
  } else {
    Serial.print("aguardando referencia de altitude na rampa");
  }

  if (!neo6m::dados_recentes()) {
    Serial.print(" | AVISO: dados de altitude antigos");
  }

  Serial.println();
}

// ---------------------------------------------------------------------------
// ESP32
// ---------------------------------------------------------------------------
void setup(){
  Serial.begin(TAXA_BAUD);
  Serial.println("ESP32 Inicializado");

  Wire.begin();  // pinos padrão: GPIO21 (SDA) e GPIO22 (SCL)
  Wire.setClock(CLOCK_I2C_GPS_HZ);

  mpu6050_pronto = mpu6050_inicializar();
  if (mpu6050_pronto) {
    Serial.println("MPU6050 Initialized");
  } else {
    Serial.println("ERROR INITIALIZING MPU6050");
  }

  // Inicializacao adicionada: o GPS utiliza o mesmo barramento I2C.
  neo6m_pronto = neo6m::inicializar();
  if (neo6m_pronto) {
    Serial.println("GPS NEO-6M inicializado");
  } else {
    Serial.println("ERRO: GPS NEO-6M nao respondeu no I2C");
  }
}

void loop(){
  if (mpu6050_pronto && intervalo_passou(instante_ultimo_ciclo_ms, INTERVALO_LOOP_MS)) {
    tratar_mpu6050();
  }

  // Atualizacao adicionada: consulta o GPS sem bloquear a execucao do MPU6050.
  if (neo6m_pronto && intervalo_passou(instante_ultima_atualizacao_gps_ms,
                                       INTERVALO_ATUALIZACAO_GPS_MS)) {
    tratar_neo6m();
  }

  if (intervalo_passou(instante_ultimo_status_gps_ms, INTERVALO_STATUS_GPS_MS)) {
    imprimir_status_gps();
  }
}
