#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP3XX.h>

/*
  TODO:
   - [] Medir quanto tempo performReading() leva e confirmar que o loop continua rápido
*/

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------
constexpr uint8_t       BMP390_ENDERECO_I2C                = 0x77;  // 0x76 com o jumper ADDR fechado ou SDO em GND
constexpr unsigned long BMP390_INTERVALO_LEITURA_MS        = 20;    // 50 Hz, igual à taxa de saída
constexpr uint8_t       BMP390_SOBREAMOSTRAGEM_TEMPERATURA = BMP3_OVERSAMPLING_8X;
constexpr uint8_t       BMP390_SOBREAMOSTRAGEM_PRESSAO     = BMP3_OVERSAMPLING_4X;
constexpr uint8_t       BMP390_COEFICIENTE_FILTRO_IIR      = BMP3_IIR_FILTER_COEFF_3;
constexpr uint8_t       BMP390_TAXA_SAIDA                  = BMP3_ODR_50_HZ;

// ---------------------------------------------------------------------------
// Estado interno (uso interno deste arquivo)
// ---------------------------------------------------------------------------
struct Bmp390Dados {
  Adafruit_BMP3XX sensor;
  bool            sensor_pronto             = false;
  bool            leitura_valida            = false;
  unsigned long   instante_ultima_leitura_ms = 0;
  float           pressao_atual_pa          = 0.0f;
  float           temperatura_atual_c       = 0.0f;
};

// Instância única, criada na primeira chamada (não chamar fora deste arquivo).
inline Bmp390Dados& bmp390_dados() {
  static Bmp390Dados instancia;
  return instancia;
}

// ---------------------------------------------------------------------------
// Interface pública
// ---------------------------------------------------------------------------

// Configura o sensor por I2C. Devolve false se ele não respondeu.
// Chamar uma vez, no setup(), depois de Wire.begin().
inline bool bmp390_inicializar() {
  Bmp390Dados& d = bmp390_dados();
  d.sensor_pronto  = false;
  d.leitura_valida = false;

  if (!d.sensor.begin_I2C(BMP390_ENDERECO_I2C)) {
    return false;
  }
  d.sensor.setTemperatureOversampling(BMP390_SOBREAMOSTRAGEM_TEMPERATURA);
  d.sensor.setPressureOversampling(BMP390_SOBREAMOSTRAGEM_PRESSAO);
  d.sensor.setIIRFilterCoeff(BMP390_COEFICIENTE_FILTRO_IIR);
  d.sensor.setOutputDataRate(BMP390_TAXA_SAIDA);
  d.sensor.performReading();  // descarta a primeira leitura, que pode ser imprecisa

  d.sensor_pronto = true;
  return true;
}

// Faz UMA leitura de pressão e temperatura e guarda o resultado, no máximo a
// cada BMP390_INTERVALO_LEITURA_MS. Entre uma leitura e outra, devolve true e
// mantém o último valor. Devolve false se o sensor não foi inicializado ou se
// a leitura falhou.
inline bool bmp390_atualizar() {
  Bmp390Dados& d = bmp390_dados();
  if (!d.sensor_pronto) {
    return false;
  }

  // Ainda não é hora de medir de novo: o último valor continua valendo.
  const unsigned long agora_ms = millis();
  if (d.leitura_valida &&
      (agora_ms - d.instante_ultima_leitura_ms) < BMP390_INTERVALO_LEITURA_MS) {
    return true;
  }

  if (!d.sensor.performReading()) {
    d.leitura_valida = false;
    return false;
  }
  d.pressao_atual_pa           = static_cast<float>(d.sensor.pressure);
  d.temperatura_atual_c        = static_cast<float>(d.sensor.temperature);
  d.instante_ultima_leitura_ms = agora_ms;
  d.leitura_valida             = true;
  return true;
}

// Última pressão lida por bmp390_atualizar(). Unidade: pascal (Pa).
inline float bmp390_obter_pressao_pa() {
  return bmp390_dados().pressao_atual_pa;
}

// Última temperatura lida por bmp390_atualizar(). Unidade: grau Celsius.
inline float bmp390_obter_temperatura_c() {
  return bmp390_dados().temperatura_atual_c;
}
