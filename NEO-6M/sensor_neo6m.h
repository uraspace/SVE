#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <cstring>

/*
  Modulo GPS NEO-6M para o computador de bordo URASPACE.

  Este modulo e header-only: a implementacao e as declaracoes ficam neste
  unico arquivo. Nao e necessario adicionar sensor_neo6m.cpp ao projeto.

  O barramento I2C deve ser inicializado uma unica vez pelo programa principal:
    Wire.begin();
    Wire.setClock(100000);

  Uso no main.ino:
    #include "NEO-6M/sensor_neo6m.h"

    neo6m::inicializar();
    neo6m::atualizar(estado_atual == EstadoVoo::RAMPA);
*/

namespace neo6m {

// ---------------------------------------------------------------------------
// Tipos publicos
// ---------------------------------------------------------------------------
enum class VariacaoVertical {
  SUBINDO,
  DESCENDO,
  ESTAVEL
};

struct DadosGps {
  bool fix_valido;
  uint8_t satelites;
  double latitude_graus;
  double longitude_graus;
  double altitude_absoluta_m;
  double altitude_inicial_m;
  double altitude_relativa_m;
  double altitude_maxima_m;
  double velocidade_vertical_ms;
  double ultimo_marco_atingido_m;
  VariacaoVertical variacao;
  bool apogeu_detectado;
  bool data_hora_valida;
  uint16_t ano;
  uint8_t mes;
  uint8_t dia;
  uint8_t hora;
  uint8_t minuto;
  uint8_t segundo;
};

namespace detalhes {

// ---------------------------------------------------------------------------
// Configuracao do hardware e da comunicacao
// ---------------------------------------------------------------------------
constexpr uint8_t ENDERECO_GPS_I2C = 0x42;
constexpr uint8_t REGISTRADOR_BYTES_DISPONIVEIS = 0xFD;
constexpr uint8_t REGISTRADOR_FLUXO_DADOS = 0xFF;
constexpr uint8_t TAMANHO_BLOCO_I2C_BYTES = 32;
constexpr uint8_t TAMANHO_MAXIMO_ENVIO_I2C_BYTES = 31;

// Configuracao UBX reaplicada a cada inicializacao do ESP32.
constexpr bool CONFIGURAR_MODULO_NO_INICIO = true;
constexpr uint8_t TAXA_ATUALIZACAO_HZ = 5;
constexpr uint8_t MODELO_DINAMICO_AEREO_4G = 8;
constexpr uint8_t TAMANHO_MAXIMO_CARGA_UBX_BYTES = 36;

// ---------------------------------------------------------------------------
// Parametros de processamento
// ---------------------------------------------------------------------------
constexpr double LIMIAR_VELOCIDADE_VERTICAL_MS = 5.0;
constexpr double PASSO_MARCO_M = 100.0;
constexpr double FATOR_FILTRO_REFERENCIA = 0.2;
constexpr double QUEDA_APOGEU_M = 3.0;
constexpr uint8_t LEITURAS_CONFIRMA_APOGEU = 3;
constexpr double ALTITUDE_MINIMA_APOGEU_M = 15.0;
constexpr unsigned long TEMPO_MAXIMO_DADOS_MS = 3000;

// ---------------------------------------------------------------------------
// Estado interno: uma unica instancia compartilhada pelo modulo
// ---------------------------------------------------------------------------
struct EstadoInterno {
  TinyGPSPlus receptor_gps;

  bool referencia_altitude_definida = false;
  double altitude_inicial_m = 0.0;
  double altitude_absoluta_m = 0.0;
  double altitude_relativa_m = 0.0;
  double altitude_anterior_m = 0.0;
  double altitude_maxima_m = 0.0;
  double velocidade_vertical_ms = 0.0;
  double proximo_marco_m = PASSO_MARCO_M;
  double ultimo_marco_atingido_m = 0.0;

  VariacaoVertical variacao_vertical = VariacaoVertical::ESTAVEL;

  bool primeira_leitura = true;
  bool apogeu_ja_detectado = false;
  bool recebeu_leitura_de_altitude = false;
  uint8_t contagem_queda_apogeu = 0;
  unsigned long instante_anterior_ms = 0;
  unsigned long ultima_leitura_ms = 0;
};

inline EstadoInterno& estado() {
  static EstadoInterno instancia;
  return instancia;
}

// ---------------------------------------------------------------------------
// Comunicacao I2C: consulta e leitura do fluxo NMEA
// ---------------------------------------------------------------------------
inline uint16_t ler_bytes_disponiveis() {
  Wire.beginTransmission(ENDERECO_GPS_I2C);
  Wire.write(REGISTRADOR_BYTES_DISPONIVEIS);

  if (Wire.endTransmission(false) != 0) {
    return 0;
  }

  if (Wire.requestFrom(ENDERECO_GPS_I2C, static_cast<uint8_t>(2)) != 2) {
    return 0;
  }

  const uint16_t alto = static_cast<uint8_t>(Wire.read());
  const uint16_t baixo = static_cast<uint8_t>(Wire.read());
  return static_cast<uint16_t>((alto << 8) | baixo);
}

inline uint8_t ler_bloco_do_gps(uint8_t tamanho) {
  EstadoInterno& dados = estado();

  Wire.beginTransmission(ENDERECO_GPS_I2C);
  Wire.write(REGISTRADOR_FLUXO_DADOS);

  if (Wire.endTransmission(false) != 0) {
    return 0;
  }

  const uint8_t recebidos = Wire.requestFrom(ENDERECO_GPS_I2C, tamanho);

  for (uint8_t indice = 0; indice < recebidos; indice++) {
    if (!Wire.available()) {
      break;
    }

    const int caractere = Wire.read();
    if (caractere >= 0) {
      dados.receptor_gps.encode(static_cast<char>(caractere));
    }
  }

  return recebidos;
}

// Lê no maximo um bloco por chamada para nao prender o loop principal.
inline void processar_buffer_do_gps() {
  const uint16_t disponiveis = ler_bytes_disponiveis();
  if (disponiveis == 0) {
    return;
  }

  const uint8_t tamanho = (disponiveis > TAMANHO_BLOCO_I2C_BYTES)
                              ? TAMANHO_BLOCO_I2C_BYTES
                              : static_cast<uint8_t>(disponiveis);

  ler_bloco_do_gps(tamanho);
}

// ---------------------------------------------------------------------------
// Comunicacao UBX: configuracao do receptor u-blox
// ---------------------------------------------------------------------------
inline bool enviar_comando_ubx(uint8_t classe, uint8_t identificador,
                               const uint8_t* carga, uint8_t tamanho_carga) {
  if (tamanho_carga > TAMANHO_MAXIMO_CARGA_UBX_BYTES) {
    return false;
  }

  uint8_t pacote[TAMANHO_MAXIMO_CARGA_UBX_BYTES + 8] = {0};
  pacote[0] = 0xB5;
  pacote[1] = 0x62;
  pacote[2] = classe;
  pacote[3] = identificador;
  pacote[4] = tamanho_carga;
  pacote[5] = 0;

  if (tamanho_carga > 0) {
    std::memcpy(&pacote[6], carga, tamanho_carga);
  }

  uint8_t soma_a = 0;
  uint8_t soma_b = 0;

  for (uint8_t indice = 2; indice < 6 + tamanho_carga; indice++) {
    soma_a = static_cast<uint8_t>(soma_a + pacote[indice]);
    soma_b = static_cast<uint8_t>(soma_b + soma_a);
  }

  pacote[6 + tamanho_carga] = soma_a;
  pacote[7 + tamanho_carga] = soma_b;

  const uint8_t tamanho_total = static_cast<uint8_t>(tamanho_carga + 8);
  uint8_t deslocamento = 0;

  while (deslocamento < tamanho_total) {
    const uint8_t restantes = static_cast<uint8_t>(tamanho_total - deslocamento);
    const uint8_t tamanho_bloco = (restantes > TAMANHO_MAXIMO_ENVIO_I2C_BYTES)
                                      ? TAMANHO_MAXIMO_ENVIO_I2C_BYTES
                                      : restantes;

    Wire.beginTransmission(ENDERECO_GPS_I2C);

    if (Wire.write(REGISTRADOR_FLUXO_DADOS) != 1) {
      return false;
    }

    if (Wire.write(&pacote[deslocamento], tamanho_bloco) != tamanho_bloco) {
      return false;
    }

    if (Wire.endTransmission() != 0) {
      return false;
    }

    deslocamento = static_cast<uint8_t>(deslocamento + tamanho_bloco);
  }

  return true;
}

inline bool configurar_taxa_de_atualizacao(uint8_t taxa_hz) {
  if (taxa_hz == 0) {
    return false;
  }

  const uint16_t periodo_ms = static_cast<uint16_t>(1000 / taxa_hz);
  const uint8_t carga[6] = {
    static_cast<uint8_t>(periodo_ms & 0xFF),
    static_cast<uint8_t>(periodo_ms >> 8),
    1, 0,
    1, 0
  };

  return enviar_comando_ubx(0x06, 0x08, carga, sizeof(carga));
}

inline bool configurar_modelo_dinamico(uint8_t modelo) {
  uint8_t carga[36] = {0};
  carga[0] = 0x01;
  carga[2] = modelo;

  return enviar_comando_ubx(0x06, 0x24, carga, sizeof(carga));
}

// ---------------------------------------------------------------------------
// Processamento da altitude e do movimento vertical
// ---------------------------------------------------------------------------
inline bool existe_altitude_nova() {
  EstadoInterno& dados = estado();

  return dados.receptor_gps.altitude.isValid() &&
         dados.receptor_gps.altitude.isUpdated() &&
         dados.receptor_gps.location.isValid() &&
         dados.receptor_gps.location.age() < TEMPO_MAXIMO_DADOS_MS;
}

inline void atualizar_referencia(double altitude_gps_m, bool em_rampa) {
  EstadoInterno& dados = estado();

  if (!dados.referencia_altitude_definida) {
    if (em_rampa) {
      dados.altitude_inicial_m = altitude_gps_m;
      dados.referencia_altitude_definida = true;
    }
    return;
  }

  if (em_rampa) {
    dados.altitude_inicial_m += FATOR_FILTRO_REFERENCIA *
                                (altitude_gps_m - dados.altitude_inicial_m);
  }
}

inline double calcular_altitude_relativa(double altitude_gps_m) {
  EstadoInterno& dados = estado();
  const double diferenca_m = altitude_gps_m - dados.altitude_inicial_m;
  return (diferenca_m > 0.0) ? diferenca_m : 0.0;
}

inline void atualizar_velocidade_vertical(unsigned long agora_ms, bool em_rampa) {
  EstadoInterno& dados = estado();

  if (em_rampa) {
    dados.velocidade_vertical_ms = 0.0;
    dados.variacao_vertical = VariacaoVertical::ESTAVEL;
    dados.altitude_anterior_m = dados.altitude_absoluta_m;
    dados.instante_anterior_ms = agora_ms;
    dados.primeira_leitura = false;
    return;
  }

  if (dados.primeira_leitura) {
    dados.primeira_leitura = false;
    dados.velocidade_vertical_ms = 0.0;
    dados.variacao_vertical = VariacaoVertical::ESTAVEL;
  } else {
    const double intervalo_s = (agora_ms - dados.instante_anterior_ms) / 1000.0;
    if (intervalo_s <= 0.0) {
      return;
    }

    dados.velocidade_vertical_ms =
        (dados.altitude_absoluta_m - dados.altitude_anterior_m) / intervalo_s;

    if (dados.velocidade_vertical_ms >= LIMIAR_VELOCIDADE_VERTICAL_MS) {
      dados.variacao_vertical = VariacaoVertical::SUBINDO;
    } else if (dados.velocidade_vertical_ms <= -LIMIAR_VELOCIDADE_VERTICAL_MS) {
      dados.variacao_vertical = VariacaoVertical::DESCENDO;
    } else {
      dados.variacao_vertical = VariacaoVertical::ESTAVEL;
    }
  }

  dados.altitude_anterior_m = dados.altitude_absoluta_m;
  dados.instante_anterior_ms = agora_ms;
}

inline void atualizar_altitude_maxima() {
  EstadoInterno& dados = estado();

  if (dados.altitude_relativa_m > dados.altitude_maxima_m) {
    dados.altitude_maxima_m = dados.altitude_relativa_m;
  }
}

inline void atualizar_marcos() {
  EstadoInterno& dados = estado();

  if (dados.altitude_relativa_m < dados.proximo_marco_m) {
    return;
  }

  const uint32_t quantidade_marcos =
      static_cast<uint32_t>((dados.altitude_relativa_m - dados.proximo_marco_m) /
                            PASSO_MARCO_M) + 1;

  dados.ultimo_marco_atingido_m = dados.proximo_marco_m +
                                  (quantidade_marcos - 1) * PASSO_MARCO_M;
  dados.proximo_marco_m += quantidade_marcos * PASSO_MARCO_M;
}

// Criterio preliminar: queda em relacao a maxima, confirmada em leituras
// consecutivas e acompanhada de classificacao de movimento descendente.
inline void atualizar_apogeu(bool em_rampa) {
  EstadoInterno& dados = estado();

  if (dados.apogeu_ja_detectado || em_rampa ||
      !dados.referencia_altitude_definida) {
    return;
  }

  if (dados.altitude_maxima_m < ALTITUDE_MINIMA_APOGEU_M) {
    dados.contagem_queda_apogeu = 0;
    return;
  }

  const bool caiu_da_maxima =
      (dados.altitude_maxima_m - dados.altitude_relativa_m) >= QUEDA_APOGEU_M;
  const bool movimento_descendente =
      dados.variacao_vertical == VariacaoVertical::DESCENDO;

  if (caiu_da_maxima && movimento_descendente) {
    if (dados.contagem_queda_apogeu < LEITURAS_CONFIRMA_APOGEU) {
      dados.contagem_queda_apogeu++;
    }
  } else {
    dados.contagem_queda_apogeu = 0;
  }

  if (dados.contagem_queda_apogeu >= LEITURAS_CONFIRMA_APOGEU) {
    dados.apogeu_ja_detectado = true;
  }
}

// ---------------------------------------------------------------------------
// Montagem do pacote publico de dados
// ---------------------------------------------------------------------------
inline void preencher_posicao(DadosGps& saida) {
  EstadoInterno& dados = estado();

  saida.fix_valido = dados.receptor_gps.location.isValid() &&
                     dados.receptor_gps.location.age() < TEMPO_MAXIMO_DADOS_MS;

  saida.latitude_graus = saida.fix_valido ? dados.receptor_gps.location.lat() : 0.0;
  saida.longitude_graus = saida.fix_valido ? dados.receptor_gps.location.lng() : 0.0;

  const bool satelites_recentes = dados.receptor_gps.satellites.isValid() &&
                                  dados.receptor_gps.satellites.age() <
                                      TEMPO_MAXIMO_DADOS_MS;
  saida.satelites = satelites_recentes
                        ? static_cast<uint8_t>(dados.receptor_gps.satellites.value())
                        : 0;
}

inline void preencher_data_hora(DadosGps& saida) {
  EstadoInterno& dados = estado();

  saida.data_hora_valida = dados.receptor_gps.date.isValid() &&
                           dados.receptor_gps.time.isValid() &&
                           dados.receptor_gps.date.age() < TEMPO_MAXIMO_DADOS_MS &&
                           dados.receptor_gps.time.age() < TEMPO_MAXIMO_DADOS_MS;

  saida.ano = saida.data_hora_valida ? dados.receptor_gps.date.year() : 0;
  saida.mes = saida.data_hora_valida ? dados.receptor_gps.date.month() : 0;
  saida.dia = saida.data_hora_valida ? dados.receptor_gps.date.day() : 0;
  saida.hora = saida.data_hora_valida ? dados.receptor_gps.time.hour() : 0;
  saida.minuto = saida.data_hora_valida ? dados.receptor_gps.time.minute() : 0;
  saida.segundo = saida.data_hora_valida ? dados.receptor_gps.time.second() : 0;
}

inline void preencher_altitudes(DadosGps& saida) {
  EstadoInterno& dados = estado();

  saida.altitude_absoluta_m = dados.altitude_absoluta_m;
  saida.altitude_inicial_m = dados.altitude_inicial_m;
  saida.altitude_relativa_m = dados.altitude_relativa_m;
  saida.altitude_maxima_m = dados.altitude_maxima_m;
  saida.velocidade_vertical_ms = dados.velocidade_vertical_ms;
  saida.ultimo_marco_atingido_m = dados.ultimo_marco_atingido_m;
  saida.variacao = dados.variacao_vertical;
  saida.apogeu_detectado = dados.apogeu_ja_detectado;
}

}  // namespace detalhes

// ---------------------------------------------------------------------------
// API publica
// ---------------------------------------------------------------------------

// Verifica se o receptor responde no endereco I2C configurado.
inline bool esta_conectado() {
  Wire.beginTransmission(detalhes::ENDERECO_GPS_I2C);
  return Wire.endTransmission() == 0;
}

// Zera os calculos do voo sem reinicializar o barramento I2C.
inline void resetar_voo() {
  detalhes::EstadoInterno& dados = detalhes::estado();

  dados.referencia_altitude_definida = false;
  dados.altitude_inicial_m = 0.0;
  dados.altitude_absoluta_m = 0.0;
  dados.altitude_relativa_m = 0.0;
  dados.altitude_anterior_m = 0.0;
  dados.altitude_maxima_m = 0.0;
  dados.velocidade_vertical_ms = 0.0;
  dados.proximo_marco_m = detalhes::PASSO_MARCO_M;
  dados.ultimo_marco_atingido_m = 0.0;
  dados.variacao_vertical = VariacaoVertical::ESTAVEL;
  dados.primeira_leitura = true;
  dados.apogeu_ja_detectado = false;
  dados.recebeu_leitura_de_altitude = false;
  dados.contagem_queda_apogeu = 0;
  dados.instante_anterior_ms = 0;
  dados.ultima_leitura_ms = 0;
}

// Inicializa a logica do modulo. Wire.begin() deve ter sido chamado no main.
inline bool inicializar() {
  resetar_voo();

  if (!esta_conectado()) {
    return false;
  }

  if (detalhes::CONFIGURAR_MODULO_NO_INICIO) {
    detalhes::configurar_taxa_de_atualizacao(detalhes::TAXA_ATUALIZACAO_HZ);
    detalhes::configurar_modelo_dinamico(detalhes::MODELO_DINAMICO_AEREO_4G);
  }

  return true;
}

// Processa um bloco de dados por chamada. em_rampa permite calibrar a referencia.
// Retorna true quando uma nova altitude foi processada.
inline bool atualizar(bool em_rampa) {
  detalhes::EstadoInterno& dados = detalhes::estado();
  detalhes::processar_buffer_do_gps();

  if (!detalhes::existe_altitude_nova()) {
    return false;
  }

  // meters() consome a marcacao de altitude atualizada da TinyGPSPlus.
  dados.altitude_absoluta_m = dados.receptor_gps.altitude.meters();
  const unsigned long agora_ms = millis();
  dados.ultima_leitura_ms = agora_ms;
  dados.recebeu_leitura_de_altitude = true;

  detalhes::atualizar_referencia(dados.altitude_absoluta_m, em_rampa);

  if (!dados.referencia_altitude_definida) {
    dados.velocidade_vertical_ms = 0.0;
    dados.variacao_vertical = VariacaoVertical::ESTAVEL;
    dados.altitude_anterior_m = dados.altitude_absoluta_m;
    dados.instante_anterior_ms = agora_ms;
    dados.primeira_leitura = false;
    return true;
  }

  if (em_rampa) {
    dados.altitude_relativa_m = 0.0;
    dados.altitude_maxima_m = 0.0;
    dados.velocidade_vertical_ms = 0.0;
    dados.variacao_vertical = VariacaoVertical::ESTAVEL;
    detalhes::atualizar_velocidade_vertical(agora_ms, true);
    return true;
  }

  dados.altitude_relativa_m =
      detalhes::calcular_altitude_relativa(dados.altitude_absoluta_m);
  detalhes::atualizar_velocidade_vertical(agora_ms, false);
  detalhes::atualizar_altitude_maxima();
  detalhes::atualizar_marcos();
  detalhes::atualizar_apogeu(false);

  return true;
}

// Retorna um pacote com a ultima posicao e os ultimos calculos armazenados.
inline DadosGps obter_dados() {
  DadosGps saida{};
  detalhes::preencher_posicao(saida);
  detalhes::preencher_data_hora(saida);
  detalhes::preencher_altitudes(saida);
  return saida;
}

inline double obter_altitude_relativa_m() {
  return detalhes::estado().altitude_relativa_m;
}

inline double obter_altitude_maxima_m() {
  return detalhes::estado().altitude_maxima_m;
}

inline double obter_velocidade_vertical_ms() {
  return detalhes::estado().velocidade_vertical_ms;
}

inline double obter_ultimo_marco_atingido_m() {
  return detalhes::estado().ultimo_marco_atingido_m;
}

inline VariacaoVertical obter_variacao_vertical() {
  return detalhes::estado().variacao_vertical;
}

inline bool apogeu_detectado() {
  return detalhes::estado().apogeu_ja_detectado;
}

inline bool tem_fix() {
  detalhes::EstadoInterno& dados = detalhes::estado();
  return dados.receptor_gps.location.isValid() &&
         dados.receptor_gps.location.age() < detalhes::TEMPO_MAXIMO_DADOS_MS;
}

inline bool referencia_definida() {
  return detalhes::estado().referencia_altitude_definida;
}

inline bool dados_recentes() {
  detalhes::EstadoInterno& dados = detalhes::estado();
  return dados.recebeu_leitura_de_altitude &&
         (millis() - dados.ultima_leitura_ms) < detalhes::TEMPO_MAXIMO_DADOS_MS;
}

// Define manualmente a altitude da rampa, em metros.
inline void definir_altitude_inicial(double altitude_m) {
  detalhes::EstadoInterno& dados = detalhes::estado();

  dados.altitude_inicial_m = altitude_m;
  dados.referencia_altitude_definida = true;
  dados.altitude_relativa_m =
      detalhes::calcular_altitude_relativa(dados.altitude_absoluta_m);
  dados.primeira_leitura = true;
  dados.contagem_queda_apogeu = 0;
  dados.apogeu_ja_detectado = false;
}

}  // namespace neo6m
