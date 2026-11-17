#pragma once

// Estados de voo compartilhados por todos os módulos e pelo main.
// Cada módulo só deve mudar os estados pelos quais é responsável.

enum class EstadoVoo {
  RAMPA,
  QUEIMA,
  COASTING,
  DESCIDA,
  POUSO,
  REPOUSO
};
