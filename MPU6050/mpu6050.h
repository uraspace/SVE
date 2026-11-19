#pragma once


// Uso no main: Wire.begin(); mpu6050_inicializar(); e, a cada ciclo,
// mpu6050_atualizar() seguido dos mpu6050_obter_*().

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

#include "../estado_voo.h"

/*
  TODO:
   - [] Guardar os dados em SD
   - [] Guardar o estado de voo no SD a cada mudança
   - [] Ler o estado de voo do SD após ligar o SVE
   - [] Determinar o angulo para soltar o paraquedas e implementar
   - [] teste
*/

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------
constexpr uint8_t MPU6050_ENDERECO_I2C          = 0x68;   // 0x69 se AD0 estiver em 3,3 V
constexpr float   MPU6050_LIMIAR_DECOLAGEM_MS2  = 19.6f;  // 2 g (a faixa do sensor é de +-16 g)
constexpr float   MPU6050_TEMPERATURA_CRITICA_C = 50.0f;

// ---------------------------------------------------------------------------
// Estado interno (uso interno deste arquivo)
// ---------------------------------------------------------------------------
struct Mpu6050Dados {
  Adafruit_MPU6050 sensor;
  bool             inicializado    = false;
  EstadoVoo        estado_atual    = EstadoVoo::RAMPA;
  float            aceleracao_x    = 0.0f;
  float            aceleracao_y    = 0.0f;
  float            aceleracao_z    = 0.0f;
  float            rotacao_x       = 0.0f;
  float            rotacao_y       = 0.0f;
  float            rotacao_z       = 0.0f;
  float            temperatura_c   = 0.0f;
  unsigned long    inicio_subida   = 0;   // instante da decolagem (ms)
  unsigned long    tempo_subida_ms = 0;   // duração da queima (ms)
};

// Instância única, criada na primeira chamada (não chamar fora deste arquivo).
inline Mpu6050Dados& mpu6050_dados() {
  static Mpu6050Dados instancia;
  return instancia;
}

// ---------------------------------------------------------------------------
// Interface pública
// ---------------------------------------------------------------------------

// Configura o sensor (faixa de +-16 g). Devolve false se ele não responder.
// Chamar uma vez, no setup(), depois de Wire.begin().
inline bool mpu6050_inicializar() {
  Mpu6050Dados& d = mpu6050_dados();

  if (!d.sensor.begin(MPU6050_ENDERECO_I2C, &Wire)) {
    d.inicializado = false;
    return false;
  }
// Definições do IMU
  d.sensor.setAccelerometerRange(MPU6050_RANGE_16_G);
  d.sensor.setGyroRange(MPU6050_RANGE_1000_DEG);
  d.sensor.setFilterBandwidth(MPU6050_BAND_44_HZ);

  d.estado_atual = EstadoVoo::RAMPA;
  d.inicializado = true;
  return true;
}

// Faz UMA leitura, guarda os valores e atualiza o estado de voo. Não bloqueia.
// Devolve false se o sensor não foi inicializado ou se a leitura falhou.
inline bool mpu6050_atualizar() {
  Mpu6050Dados& d = mpu6050_dados();
  if (!d.inicializado) {
    return false;
  }

  sensors_event_t evento_acel, evento_giro, evento_temp;
  if (!d.sensor.getEvent(&evento_acel, &evento_giro, &evento_temp)) {
    return false;
  }
  d.aceleracao_x  = evento_acel.acceleration.x;
  d.aceleracao_y  = evento_acel.acceleration.y;
  d.aceleracao_z  = evento_acel.acceleration.z;
  d.rotacao_x     = evento_giro.gyro.x
  d.rotacao_y     = evento_giro.gyro.y
  d.rotacao_z     = evento_giro.gyro.z
  d.temperatura_c = evento_temp.temperature;


  switch (d.estado_atual) {
    case EstadoVoo::RAMPA:
      if (d.aceleracao_z >= MPU6050_LIMIAR_DECOLAGEM_MS2) {
        d.estado_atual  = EstadoVoo::QUEIMA;
        d.inicio_subida = millis();
      }
      break;

    case EstadoVoo::QUEIMA:
      // Detectar fim da queima (não detecta o apogeu, deve ser detectado com o barômetro)
 /*     if (d.aceleracao_z < 0) {
       d.tempo_subida_ms = millis() - d.inicio_subida;
      d.estado_atual    = EstadoVoo::COASTING;
      }*/
      break;

    default:
      break;  
  }
  return true;
}

// Estado de voo atual.
inline EstadoVoo mpu6050_obter_estado() {
  return mpu6050_dados().estado_atual;
}

// Obtem as acelerações em x, y, z
inline float mpu6050_obter_aceleracao_x() {
  return mpu6050_dados().aceleracao_x;
}
inline float mpu6050_obter_aceleracao_y() {
  return mpu6050_dados().aceleracao_y;
}
inline float mpu6050_obter_aceleracao_z() {
  return mpu6050_dados().aceleracao_z;
}

// Obtem as rotacoes em x, y, z (rad/s)
inline float mpu6050_obter_rotacao_x(){
  return mpu6050_dados().rotacao_x;
}
inline float mpu6050_obter_rotacao_y(){
  return mpu6050_dados().rotacao_y;
}
inline float mpu6050_obter_rotacao_z(){
  return mpu6050_dados().rotacao_z;
}

inline float mpu6050_obter_pitch(){
  ax = mpu6050_obter_aceleracao_x();
  ay = mpu6050_obter_aceleracao_y();
  az = mpu6050_obter_aceleracao_z();
  float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / PI;
  return pitch;
}

inline float mpu6050_obter_roll(){
  ax = mpu6050_obter_aceleracao_x();
  ay = mpu6050_obter_aceleracao_y();
  az = mpu6050_obter_aceleracao_z();
  float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / PI;
  return roll;
}


// Última temperatura lida (do próprio chip). Unidade: graus Celsius.
inline float mpu6050_obter_temperatura() {
  return mpu6050_dados().temperatura_c;
}

// Duração da queima, em ms. Válido a partir do estado COASTING.
inline unsigned long mpu6050_obter_tempo_subida_ms() {
  return mpu6050_dados().tempo_subida_ms;
}

// True se a última temperatura lida passou do limite crítico.
inline bool mpu6050_temperatura_critica() {
  return mpu6050_dados().temperatura_c > MPU6050_TEMPERATURA_CRITICA_C;
}
