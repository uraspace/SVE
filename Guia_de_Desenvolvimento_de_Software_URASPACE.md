# Guia de Desenvolvimento de Software URASPACE

**Padrões de código, estrutura do repositório e fluxo de contribuição**

| Item | Detalhe |
|---|---|
| Versão | 1.0 |
| Data | 21/10/2026 |
| Repositório | https://github.com/uraspace/SVE |
| Público-alvo | Integrantes da equipe que desenvolvem o software embarcado |
| Plataforma | ESP32, framework Arduino, linguagem C++ |

## Índice

- [1. Introdução](#1-introdução)
  - [1.1 Objetivo](#11-objetivo)
  - [1.2 Público-alvo e pré-requisitos](#12-público-alvo-e-pré-requisitos)
  - [1.3 Escopo](#13-escopo)
  - [1.4 Como usar este guia](#14-como-usar-este-guia)
- [2. Visão geral do projeto](#2-visão-geral-do-projeto)
  - [2.1 Arquitetura de software](#21-arquitetura-de-software)
  - [2.2 Princípios de projeto](#22-princípios-de-projeto)
  - [2.3 Hardware de referência](#23-hardware-de-referência)
- [3. Estrutura do repositório](#3-estrutura-do-repositório)
  - [3.1 Uma pasta por sensor](#31-uma-pasta-por-sensor)
  - [3.2 Conteúdo de cada pasta](#32-conteúdo-de-cada-pasta)
- [4. Fluxo de contribuição (Git e GitHub)](#4-fluxo-de-contribuição-git-e-github)
  - [4.1 Conceitos básicos](#41-conceitos-básicos)
  - [4.2 Visão geral do fluxo](#42-visão-geral-do-fluxo)
  - [4.3 Passo a passo](#43-passo-a-passo)
  - [4.4 Regras de conduta](#44-regras-de-conduta)
  - [4.5 Mensagens de commit e descrição do PR](#45-mensagens-de-commit-e-descrição-do-pr)
- [5. Arduino em poucos minutos](#5-arduino-em-poucos-minutos)
  - [5.1 Estrutura básica de um programa](#51-estrutura-básica-de-um-programa)
  - [5.2 Diferenças em relação a Python](#52-diferenças-em-relação-a-python)
  - [5.3 Ferramentas essenciais](#53-ferramentas-essenciais)
- [6. Diretrizes de código](#6-diretrizes-de-código)
  - [6.1 Convenção de nomes](#61-convenção-de-nomes)
  - [6.2 Estilo e formatação](#62-estilo-e-formatação)
  - [6.3 Organização de um arquivo](#63-organização-de-um-arquivo)
  - [6.4 Funções](#64-funções)
  - [6.5 Código não bloqueante](#65-código-não-bloqueante)
  - [6.6 Tratamento de erros e mensagens](#66-tratamento-de-erros-e-mensagens)
- [7. Comunicação I2C](#7-comunicação-i2c)
  - [7.1 Configuração padrão](#71-configuração-padrão)
  - [7.2 Regras de software](#72-regras-de-software)
  - [7.3 Registro de endereços](#73-registro-de-endereços)
- [8. Ciclo de desenvolvimento de um módulo](#8-ciclo-de-desenvolvimento-de-um-módulo)
  - [8.1 Visão geral](#81-visão-geral)
  - [8.2 Fase 1: sketch de teste](#82-fase-1-sketch-de-teste)
  - [8.3 Fase 2: converter o sketch em módulo](#83-fase-2-converter-o-sketch-em-módulo)
  - [8.4 Fase 3: integração ao main.cpp](#84-fase-3-integração-ao-maincpp)
- [9. Testes e validação](#9-testes-e-validação)
- [10. Documentação](#10-documentação)
- [11. Solução de problemas comuns](#11-solução-de-problemas-comuns)
- [12. Histórico de revisões](#12-histórico-de-revisões)
- [Apêndice A - Modelo de módulo (.h e .cpp)](#apêndice-a---modelo-de-módulo-h-e-cpp)
- [Apêndice B - Modelo de README do sensor](#apêndice-b---modelo-de-readme-do-sensor)
- [Apêndice C - Checklist de pull request](#apêndice-c---checklist-de-pull-request)
- [Apêndice D - Glossário](#apêndice-d---glossário)

## 1. Introdução

### 1.1 Objetivo

Este guia define como o software embarcado da URASPACE é organizado, escrito e integrado. Cada integrante trabalha em um sensor diferente, em paralelo. Para que todos os códigos se encaixem sem retrabalho, todos precisam seguir as mesmas regras de estrutura, nomes, estilo e integração.

### 1.2 Público-alvo e pré-requisitos

O guia foi escrito para integrantes que conhecem um pouco de Python e C, mas que não precisam ter experiência prévia com Arduino, Git ou GitHub. Esses assuntos são apresentados do zero nas seções 4 e 5.

### 1.3 Escopo

Este guia cobre:

- a estrutura do repositório e das pastas de cada sensor;
- o fluxo de contribuição pelo GitHub (fork, branch, pull request e sincronização);
- a estrutura de um programa Arduino (`setup()`, `loop()` e ferramentas básicas);
- as convenções de nomes, estilo e organização do código;
- as regras de comunicação I2C compartilhadas por todos os sensores;
- o ciclo de desenvolvimento de um módulo: teste, conversão em biblioteca e integração ao `main.cpp`.

Este guia **não** cobre detalhes específicos de cada componente. Esses detalhes (configuração, limites, calibração) ficam documentados no `README.md` da pasta de cada sensor.

### 1.4 Como usar este guia

Leia o documento inteiro uma vez antes de começar e depois use-o como consulta. Os apêndices trazem modelos prontos para copiar. Neste documento:

- **deve** e **não deve** indicam regras obrigatórias;
- **recomenda-se** indica uma boa prática que pode ser flexibilizada com justificativa;
- trechos em `fonte monoespaçada` indicam código, nomes de arquivos e nomes de funções.

## 2. Visão geral do projeto

### 2.1 Arquitetura de software

Cada sensor é desenvolvido por uma pessoa como um **módulo independente**. No final, todos os módulos são integrados em um único programa principal, o `main.cpp`, que roda no ESP32 e coordena os sensores.

```text
   Sensor A         Sensor B         Sensor C
  (1 pessoa)       (1 pessoa)       (1 pessoa)
      |                |                |
      v                v                v
 +-----------+    +-----------+    +-----------+
 |  modulo   |    |  modulo   |    |  modulo   |
 | .h / .cpp |    | .h / .cpp |    | .h / .cpp |
 +-----+-----+    +-----+-----+    +-----+-----+
       |                |                |
       +----------------+----------------+
                        |
                        v
                 +-------------+
                 |   main.cpp  |
                 | setup()     |
                 | loop()      |
                 +-------------+
```

### 2.2 Princípios de projeto

- **Módulos independentes:** cada sensor tem sua pasta, seus arquivos e uma interface padronizada.
- **Código não bloqueante:** nenhuma parte do código pode prender o programa esperando algo acontecer (seção 6.5).
- **Um único `setup()` e um único `loop()`:** no programa final, quem os possui é o `main.cpp`. Os módulos não têm essas funções.
- **Falhas isoladas:** se um sensor falhar, os demais continuam funcionando.
- **Legibilidade primeiro:** nomes claros, funções curtas e nenhum número solto no código.

### 2.3 Hardware de referência

| Item | Padrão do projeto |
|---|---|
| Microcontrolador | ESP32 (framework Arduino) |
| Comunicação com sensores | I2C nos pinos padrão: GPIO21 (SDA) e GPIO22 (SCL) |
| Nível lógico | 3,3 V |
| Velocidade do I2C | 100 kHz (padrão) |
| Velocidade da porta serial | 115200 baud |

> **Atenção:** os pinos 21 e 22 são os padrão do ESP32 clássico. Placas de outros modelos da família (S2, S3, C3) usam pinos diferentes. Confirme o modelo da sua placa antes de ligar o sensor.

## 3. Estrutura do repositório

### 3.1 Uma pasta por sensor

O repositório https://github.com/uraspace/SVE é organizado com **uma pasta para cada sensor**. Cada integrante é responsável pela pasta do seu sensor e trabalha somente nela, com uma única exceção: a pequena integração no `main.cpp`, descrita na seção 8.4.

A pasta deve ter o **nome do sensor (o modelo da peça)**, e não o nome da função que ele exerce. Por exemplo, `MPU6050` e não `acelerometro`. O motivo é que o modelo identifica a peça sem ambiguidade, enquanto o nome da função não: um mesmo sensor pode medir várias grandezas, e dois sensores diferentes podem ter a mesma função.

Regras para o nome da pasta:

- usar o modelo exatamente como aparece no datasheet (por exemplo `MPU6050`, `BMP280`);
- sem espaços, acentos ou caracteres especiais;
- pastas criadas antes deste guia com nome de função devem ser renomeadas.

### 3.2 Conteúdo de cada pasta

Estrutura de referência do repositório (usando dois sensores como exemplo):

```text
SVE/
+-- main.cpp                  programa principal (integração)
+-- MPU6050/
|   +-- MPU6050.ino           sketch de teste, com setup() e loop()
|   +-- sensor_mpu6050.h      cabeçalho do módulo
|   +-- sensor_mpu6050.cpp    implementação do módulo
|   +-- README.md             documentação do sensor
+-- BMP280/
    +-- BMP280.ino
    +-- sensor_bmp280.h
    +-- sensor_bmp280.cpp
    +-- README.md
```

Alterações na organização da raiz do repositório devem ser combinadas com o responsável pelo repositório e registradas neste guia.

| Arquivo | Função | Quando existe |
|---|---|---|
| `<SENSOR>.ino` | Sketch de teste com `setup()` e `loop()` | Desde a Fase 1. Depois da conversão, vira um exemplo de uso do módulo |
| `sensor_<modelo>.h` | Cabeçalho: declara a interface pública do módulo | A partir da Fase 2 |
| `sensor_<modelo>.cpp` | Implementação do módulo | A partir da Fase 2 |
| `README.md` | Documentação do sensor (modelo no Apêndice B) | Desde o início |

Duas observações:

- Na Arduino IDE, o arquivo `.ino` precisa ter o **mesmo nome da pasta**, e todos os arquivos da pasta são compilados juntos.
- Os arquivos `.h` e `.cpp` levam o prefixo `sensor_` para nunca coincidirem com o nome de bibliotecas de terceiros (por exemplo, existe uma biblioteca externa com um arquivo chamado `MPU6050.h`).

## 4. Fluxo de contribuição (Git e GitHub)

Ninguém precisa saber Git para contribuir. Tudo pode ser feito pelo site do GitHub ou pelo **GitHub Desktop**, um programa com interface gráfica. Para código Arduino, o GitHub Desktop costuma ser mais prático, porque permite trabalhar com os arquivos no próprio computador e enviá-los depois.

### 4.1 Conceitos básicos

| Termo | O que é | Analogia |
|---|---|---|
| Repositório | Pasta do projeto com todo o histórico de alterações | Pasta compartilhada com histórico de versões |
| Repositório principal | A versão oficial, em github.com/uraspace/SVE | O documento original |
| Fork | Cópia pessoal do repositório principal, na sua conta | Uma cópia do arquivo para editar sem mexer no original |
| Branch | Linha de trabalho separada dentro do seu fork | Um rascunho para uma tarefa específica |
| Commit | Salvar um conjunto de alterações, com uma mensagem | Salvar uma versão com um comentário |
| Pull request (PR) | Pedido para incluir suas alterações no principal | Enviar o rascunho para revisão |
| Sync | Atualizar o seu fork com as novidades do principal | Baixar a versão mais recente antes de editar |
| Merge | Incluir de fato as alterações no principal | Aprovar e publicar o rascunho |

### 4.2 Visão geral do fluxo

Você **nunca altera diretamente** o repositório principal. O caminho é sempre este:

```text
  uraspace/SVE  (repositório principal)
     |   ^
     |   |  4. pull request (depois de pronto e testado)
     |   |
     |   |  1. fork e sync
     v   |
  seu-usuario/SVE  (seu fork)
     |
     |  2. branch da tarefa
     |  3. commits (seu trabalho)
     v
  suas alterações
```

### 4.3 Passo a passo

1. **Criar o seu fork (uma única vez).** Na página do repositório principal, clique em **Fork**. O GitHub cria uma cópia dele na sua conta. É nessa cópia que você trabalha.
2. **Sincronizar o fork (sempre antes de começar uma tarefa).** Na página do seu fork, use o botão **Sync fork** e depois **Update branch**. Isso traz para a sua cópia tudo o que os outros já incluíram no principal. Quem esquece essa etapa fica para trás e gera conflitos.
3. **Criar uma branch para a tarefa.** No seu fork, abra o seletor de branches, digite um nome curto para a tarefa (por exemplo `mpu6050-leitura-aceleracao`) e crie a branch. Use a `main` do seu fork somente para sincronizar, nunca para trabalhar.
4. **Fazer as alterações e salvar (commit).** Com a sua branch selecionada, edite ou envie os arquivos pelo site ou pelo GitHub Desktop. Ao salvar, escreva uma mensagem curta dizendo o que mudou (seção 4.5). Prefira salvamentos pequenos e frequentes.
5. **Sincronizar de novo e abrir o pull request.** Quando a tarefa estiver pronta e testada, sincronize o fork mais uma vez e abra o PR a partir da sua branch, com destino ao repositório principal. O GitHub costuma mostrar o botão **Compare & pull request**. Preencha título e descrição conforme a seção 4.5.
6. **Revisão.** O responsável pelo repositório revisa o PR e pode pedir ajustes por comentário. Faça os ajustes na mesma branch: os novos commits entram automaticamente no PR. Depois da aprovação, o PR é incluído no principal (merge).
7. **Depois do merge.** Sincronize o fork novamente. A próxima tarefa começa com uma nova branch.

Se a sua tarefa demorar vários dias e o repositório principal mudar nesse intervalo, peça ajuda ao responsável para trazer as novidades para a sua branch.

### 4.4 Regras de conduta

- **Trabalhe apenas na pasta do seu sensor.** Não altere pastas de outros sensores sem combinar com o responsável por elas.
- **Mantenha as alterações no `main.cpp` pequenas** e restritas à integração do seu módulo (seção 8.4). Como várias pessoas editam esse arquivo, sincronize logo antes de mexer nele e abra o PR em seguida. Isso reduz a chance de conflito.
- **Um PR por tarefa**, o menor possível. PRs pequenos são revisados mais rápido.
- **Não envie** arquivos compilados, pastas de build, configurações pessoais do editor, senhas ou bibliotecas externas copiadas para dentro do repositório.
- **Teste antes de abrir o PR** (seção 9).
- Se o GitHub avisar que o PR tem **conflito**, não tente resolver sozinho. Peça ajuda ao responsável.

### 4.5 Mensagens de commit e descrição do PR

**Commits:** escreva em português, de forma curta e específica, dizendo o que foi feito.

| Bom | Ruim |
|---|---|
| Adiciona leitura de aceleração no módulo MPU6050 | mudanças |
| Corrige limiar de decolagem | ajustes |
| Remove delay bloqueante da inicialização | final2 |

**Pull requests:** o título começa com o nome da pasta do sensor entre colchetes, por exemplo `[MPU6050] Converte sketch em módulo`. A descrição deve responder a quatro perguntas:

1. O que mudou?
2. Como foi testado (hardware usado e resultado)?
3. O que mudou no `main.cpp`, se algo mudou?
4. Há alguma pendência ou limitação?

O checklist completo está no Apêndice C.

## 5. Arduino em poucos minutos

### 5.1 Estrutura básica de um programa

Programas Arduino para o ESP32 são escritos em C++. Você não escreve a função `main()`: o framework já a contém e chama duas funções que **você** define.

| Conceito | Python | C | Arduino |
|---|---|---|---|
| Preparação inicial, uma vez | código antes do laço | código antes do laço em `main()` | `setup()` |
| Repetição para sempre | `while True:` | `while (1)` em `main()` | `loop()` |

Exemplo mínimo, já seguindo as convenções do projeto:

```cpp
constexpr unsigned long TAXA_BAUD            = 115200;
constexpr unsigned long INTERVALO_LEITURA_MS = 100;

unsigned long instante_ultima_leitura_ms = 0;

void setup() {
  Serial.begin(TAXA_BAUD);          // abre a porta serial
  Serial.println("Sistema iniciado");
}

void loop() {
  const unsigned long agora_ms = millis();

  if (agora_ms - instante_ultima_leitura_ms >= INTERVALO_LEITURA_MS) {
    instante_ultima_leitura_ms = agora_ms;
    Serial.println("Leitura periodica");
  }
}
```

O que acontece ao ligar a placa:

1. O framework executa `setup()` **uma vez**.
2. Em seguida, executa `loop()` **repetidamente, para sempre**, milhares de vezes por segundo.
3. Cada passagem pelo `loop()` deve ser rápida, para que todos os sensores sejam atendidos com frequência.

### 5.2 Diferenças em relação a Python

- **Tipos são declarados:** `float aceleracao_z = 0;`, e não apenas `aceleracao_z = 0`.
- **Toda instrução termina com `;`** e blocos são delimitados por chaves `{ }`, não por indentação.
- **Comentários** usam `//` (uma linha) ou `/* ... */` (várias linhas).
- **Funções declaram o tipo de retorno:** `void` quando não retornam nada, `bool`, `float` etc. nos demais casos.
- **Em arquivos `.cpp`, uma função precisa ser declarada antes de ser usada.** Nos arquivos `.ino`, a Arduino IDE resolve isso automaticamente, mas não confie nisso nos módulos.

Tipos mais usados:

| Tipo | Uso |
|---|---|
| `bool` | verdadeiro ou falso |
| `int` | número inteiro com sinal |
| `uint8_t` | inteiro de 0 a 255 (bytes, endereços I2C) |
| `float` | número com casas decimais; em valores literais use o sufixo `f`, como em `19.6f` |
| `unsigned long` | tempos em milissegundos (retorno de `millis()`) |
| `constexpr` | palavra que transforma o valor em constante conhecida na compilação |

### 5.3 Ferramentas essenciais

- **`Serial.begin(TAXA_BAUD)`, `Serial.print()`, `Serial.println()`:** enviam texto para o computador. Para ver o texto, abra o Monitor Serial da Arduino IDE com a **mesma velocidade** (115200 baud).
- **`millis()`:** devolve quantos milissegundos se passaram desde que a placa foi ligada. Serve para medir tempo.
- **`delay(ms)`:** pausa o programa por `ms` milissegundos. **Evite:** durante a pausa, o ESP32 não faz mais nada, ou seja, nenhum outro sensor é lido. O padrão correto para "fazer algo a cada X ms" é o do exemplo da seção 5.1, com `millis()`.

## 6. Diretrizes de código

### 6.1 Convenção de nomes

| Elemento | Padrão | Exemplo |
|---|---|---|
| Funções | `snake_case`, português, sem acento, começando com um verbo no infinitivo | `inicializar()`, `ler_aceleracao_z()`, `registrar_fim_da_subida()` |
| Variáveis | `snake_case`, português, sem acento | `aceleracao_z`, `inicio_subida`, `voo_iniciado` |
| Constantes | `MAIUSCULO_SNAKE`, declaradas com `constexpr`, com sufixo da unidade | `LIMIAR_DECOLAGEM_MS2`, `TEMPERATURA_CRITICA_C` |
| Tipos (enum, struct, class) | `PascalCase` | `EstadoVoo` |
| Valores de enum | `MAIUSCULO` | `RAMPA`, `QUEIMA`, `COASTING` |
| Namespace do módulo | minúsculas, igual ao modelo do sensor | `mpu6050` |
| Arquivos do módulo | `sensor_` + modelo em minúsculas | `sensor_mpu6050.h` |
| Pasta do sensor | modelo como no datasheet | `MPU6050` |

Regras complementares:

- **Sem acentos, espaços ou caracteres especiais** em nomes de variáveis, funções, arquivos e pastas.
- **Nomes completos e descritivos:** `aceleracao_z`, e não `az` ou `a`. Abreviações só quando forem universais (`i2c`, `mpu`).
- **Idioma:** português. As exceções são os nomes que vêm de bibliotecas externas (`getEvent`, `begin`), que devem ser mantidos como são.
- **Booleanos** devem ler-se como afirmação: `voo_iniciado`, `sensor_pronto`.
- **Funções começam com verbo:** `inicializar`, `ler`, `atualizar`, `obter`, `aguardar`, `registrar`, `processar`.

Sufixos de unidade para constantes (em variáveis, use o mesmo sufixo em minúsculas quando a unidade não for óbvia, como em `intervalo_ms`):

| Sufixo | Unidade |
|---|---|
| `_MS` | milissegundos |
| `_MS2` | metros por segundo ao quadrado (m/s²) |
| `_M_S` | metros por segundo (m/s) |
| `_M` | metros |
| `_C` | graus Celsius |
| `_PA` | pascal |
| `_HZ` | hertz |

Use sempre unidades do Sistema Internacional, exceto quando o sensor ou o datasheet exigir outra, e nesse caso deixe a unidade explícita no nome.

### 6.2 Estilo e formatação

- Indentação de **2 espaços** (sem tabulação).
- A chave de abertura fica na **mesma linha** (`void funcao() {`).
- **Sempre use chaves** em `if`, `for` e `while`, mesmo quando houver uma única instrução.
- Uma instrução por linha, com linhas de até 100 caracteres.
- Espaço ao redor de operadores e depois de vírgulas.
- **Comentários em português**, explicando o **porquê** e não o óbvio. Use banners para separar as seções do arquivo (veja a seção 6.3).
- **Nenhum número solto:** valores como limiares, tempos e endereços viram constantes nomeadas.
- Não envie código comentado no PR. Pendências ficam em um bloco `TODO` no topo do arquivo.

```cpp
// RUIM: o que significam 19.6 e 100?
if (aceleracao_z > 19.6) {
  delay(100);
}

// BOM: constantes nomeadas, com unidade e significado
constexpr float LIMIAR_DECOLAGEM_MS2 = 19.6f;  // 2 g

if (aceleracao_z > LIMIAR_DECOLAGEM_MS2) {
  // ...
}
```

### 6.3 Organização de um arquivo

Siga esta ordem, separando as seções com banners:

1. `#include`
2. Bloco de comentário com `TODO`, se houver
3. Configuração (constantes `constexpr`)
4. Tipos (`enum`, `struct`)
5. Variáveis globais (somente as estritamente necessárias)
6. Funções, agrupadas por assunto
7. `setup()` e `loop()` por último (apenas no sketch de teste)

```cpp
#include <BibliotecaDoSensor.h>

/*
  TODO:
   - [] Descrição da pendência
*/

// ---------------------------------------------------------------------------
// Configuração
// ---------------------------------------------------------------------------
constexpr unsigned long TAXA_BAUD = 115200;

// ---------------------------------------------------------------------------
// Variáveis globais
// ---------------------------------------------------------------------------
bool voo_iniciado = false;

// ---------------------------------------------------------------------------
// Sensor
// ---------------------------------------------------------------------------
void inicializar_sensor() {
  // ...
}
```

### 6.4 Funções

- **Uma responsabilidade por função.** Se uma função faz duas coisas, separe em duas.
- Funções **curtas**: se não cabe em uma tela, provavelmente deve ser dividida.
- Comente acima de cada função o que ela faz, o que retorna e em que unidade.
- Funções que podem falhar devolvem `bool` (`true` em caso de sucesso).
- Evite variáveis globais. No módulo final, o estado interno fica escondido no `.cpp` (veja o Apêndice A).
- Não deixe variáveis, parâmetros ou funções sem uso.

### 6.5 Código não bloqueante

Esta é a regra mais importante do projeto. Como todos os sensores rodam no mesmo `loop()`, uma função que prende o programa impede **todos os outros sensores** de serem lidos.

> **Regra de ouro:** nenhuma função de módulo pode esperar. Cada chamada faz o seu trabalho, devolve o resultado e retorna imediatamente.

1. **Não use `delay()`** nos módulos. Evite-o desde a Fase 1, para facilitar a conversão.
2. **Não use laços `while` que esperam um evento externo** (o sensor responder, um valor atingir um limiar). Verifique a condição uma vez por chamada e retorne.
3. **Falhas não travam o programa.** A função devolve `false`, e quem decide o que fazer é o `main.cpp`.
4. **Para medir tempo, use `millis()`.**

```cpp
// RUIM: prende o programa até o valor passar do limiar
while (ler_valor() < LIMIAR) {
}

// BOM: verifica uma vez por chamada e retorna
bool limiar_atingido() {
  return ler_valor() >= LIMIAR;
}

// RUIM: trava para sempre se o sensor falhar
while (!sensor.begin()) {
}

// BOM: informa a falha e deixa o main decidir
bool inicializar() {
  return sensor.begin();
}
```

### 6.6 Tratamento de erros e mensagens

- `inicializar()` e `atualizar()` devolvem `bool`. O `main.cpp` **deve** checar o retorno.
- Nunca assuma que uma leitura é válida sem checar o retorno de `atualizar()`.
- **Módulos não escrevem na porta serial.** Dados e erros sobem para o `main.cpp`, que decide o que imprimir. Mensagens de diagnóstico ficam no sketch de teste.
- Mensagens na serial em português, de preferência sem acento (alguns monitores seriais exibem acentos de forma incorreta), com o prefixo `INFO:`, `AVISO:` ou `ERRO:`.

## 7. Comunicação I2C

### 7.1 Configuração padrão

- **Pinos:** os pinos I2C padrão do ESP32, GPIO21 (SDA) e GPIO22 (SCL). Não remapeie os pinos sem combinar com toda a equipe.
- **Nível lógico:** 3,3 V. Os pinos do ESP32 não toleram 5 V. Confirme no datasheet do módulo que ele opera em 3,3 V.
- **Velocidade:** 100 kHz (padrão). Só aumente se todos os sensores do barramento suportarem e se a equipe concordar.
- **GND comum** a todos os módulos e ao ESP32.
- **Resistores de pull-up:** o barramento precisa de um par de resistores de pull-up. Muitos módulos já os trazem na placa, e vários módulos em paralelo reduzem a resistência equivalente. Consulte o responsável pelo hardware antes de adicionar ou remover resistores.

### 7.2 Regras de software

- **`Wire.begin()` é chamado uma única vez, no `main.cpp`,** antes de inicializar os módulos. O seu código não deve chamá-lo. Algumas bibliotecas de sensores o chamam internamente, o que é tolerado.
- **Endereço I2C:** definido como constante no `.cpp` do módulo (`ENDERECO_I2C`), em hexadecimal, e passado explicitamente na inicialização quando a biblioteca permitir.
- **Endereços únicos:** dois dispositivos no mesmo barramento não podem ter o mesmo endereço. Se houver conflito, consulte o datasheet (muitos módulos têm um pino que seleciona um segundo endereço) e avise a equipe.
- **Para descobrir o endereço** de um módulo, use um sketch de varredura I2C (*I2C scanner*), fácil de encontrar em tutoriais.
- **Sem tarefas concorrentes no barramento:** o `Wire` não é seguro para uso simultâneo. Não use tarefas do FreeRTOS que acessem o I2C sem combinar com a equipe.

### 7.3 Registro de endereços

Cada PR que adiciona um sensor deve incluir uma linha nesta tabela.

| Sensor (pasta) | Endereço I2C | Observações |
|---|---|---|
| MPU6050 | 0x68 | 0x69 se o pino AD0 estiver ligado a 3,3 V |

## 8. Ciclo de desenvolvimento de um módulo

### 8.1 Visão geral

Todo sensor passa por três fases, sempre nesta ordem.

| Fase | Entrega | Resultado |
|---|---|---|
| 1. Sketch de teste | `<SENSOR>.ino` com `setup()` e `loop()` | Sensor funcionando sozinho, validado em bancada |
| 2. Módulo | `sensor_<modelo>.h` e `.cpp`, sem `setup()` e `loop()` | Código reutilizável, com interface padronizada |
| 3. Integração | Alteração no `main.cpp` | Sensor funcionando junto com os demais |

### 8.2 Fase 1: sketch de teste

1. Crie a pasta do sensor e o arquivo `<SENSOR>.ino` com `setup()` e `loop()`.
2. Faça o sensor funcionar sozinho, imprimindo as leituras no Monitor Serial.
3. **Já siga todas as convenções deste guia:** nomes, constantes nomeadas, funções curtas e com uma responsabilidade, sem `delay()` desnecessário. Quanto mais organizado o sketch, mais simples a Fase 2.
4. Valide as leituras em bancada (seção 9).
5. Escreva o `README.md` (Apêndice B).

### 8.3 Fase 2: converter o sketch em módulo

Nesta fase, o código deixa de ser um programa e vira uma biblioteca: **o módulo não possui `setup()` nem `loop()`**.

1. Crie `sensor_<modelo>.h` e `sensor_<modelo>.cpp` na pasta do sensor.
2. Coloque todo o código do módulo dentro de um `namespace` com o nome do modelo em minúsculas (por exemplo `namespace mpu6050`). Isso evita que as funções de sensores diferentes colidam, já que todos terão funções como `inicializar()` e `atualizar()`.
3. Mantenha o estado interno (objeto da biblioteca, variáveis, constantes) em um `namespace` anônimo dentro do `.cpp`, invisível para quem usa o módulo.
4. Transforme o conteúdo de `setup()` e de `loop()` nas funções da interface padrão:

| No sketch de teste | No módulo |
|---|---|
| Conteúdo do `setup()` | `bool inicializar()` |
| Conteúdo do `loop()` | `bool atualizar()` |
| Valores que o `loop()` imprimia | funções `obter_<grandeza>()` |

5. Elimine `delay()` e laços `while` bloqueantes (seção 6.5).
6. Remova os `Serial.print()` do módulo (seção 6.6).
7. No `.h`, use `#pragma once`, inclua `<Arduino.h>` e declare **apenas** a interface pública, com um comentário acima de cada função.
8. Transforme o `.ino` em um exemplo mínimo de uso do módulo.
9. Repita os testes de bancada: o comportamento deve ser equivalente ao do sketch original.

**Interface padrão de todo módulo:**

| Função | Papel |
|---|---|
| `bool inicializar()` | Configura o sensor. Devolve `false` se ele não responder. Chamada uma vez, no `setup()` |
| `bool atualizar()` | Faz **uma** leitura e guarda o resultado. Não bloqueia. Chamada a cada passagem do `loop()` |
| `<tipo> obter_<grandeza>()` | Devolve o último valor lido, em unidade SI documentada. Não acessa o sensor |

O Apêndice A traz um modelo completo de `.h` e `.cpp`.

### 8.4 Fase 3: integração ao main.cpp

1. Sincronize o fork e crie uma branch (seção 4.3).
2. No `main.cpp`, adicione o `#include` do seu módulo.
3. Chame `inicializar()` no `setup()`, **checando o retorno**.
4. Chame `atualizar()` no `loop()`, checando o retorno, e use os valores com `obter_<grandeza>()`.
5. **Não coloque lógica do sensor no `main.cpp`.** A lógica fica no módulo, e o `main.cpp` apenas o chama.
6. Teste o programa completo e abra o PR (seção 4.3).

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "sensor_exemplo.h"        // um include por módulo

constexpr unsigned long TAXA_BAUD = 115200;

void setup() {
  Serial.begin(TAXA_BAUD);
  Wire.begin();                    // pinos padrão: GPIO21 (SDA) e GPIO22 (SCL)

  if (!exemplo::inicializar()) {
    Serial.println("ERRO: sensor exemplo nao inicializou");
  }
}

void loop() {
  if (exemplo::atualizar()) {
    Serial.println(exemplo::obter_valor(), 1);
  }
}
```

## 9. Testes e validação

Antes de abrir um PR, execute e registre no PR os testes abaixo.

- **Teste de bancada (Fase 1):** com o Monitor Serial a 115200 baud, confirme que as leituras são coerentes com um valor de referência conhecido e que as unidades estão corretas.
- **Teste de falha:** desconecte o sensor e confirme que o programa **não trava** e que `inicializar()` devolve `false`. Reconecte e confirme que o sistema se recupera, quando aplicável.
- **Teste do módulo (Fase 2):** repita os testes de bancada com o código convertido. O resultado deve ser equivalente ao do sketch original.
- **Teste de integração (Fase 3):** compile o `main.cpp` completo, com a sua alteração sobre a versão mais recente do repositório (por isso o sync), e confirme que todos os módulos inicializam e que o `loop()` continua rápido.
- **Registro:** descreva no PR qual hardware foi usado e qual foi o resultado de cada teste.

## 10. Documentação

- **`README.md` em cada pasta de sensor** é obrigatório. Use o modelo do Apêndice B e mantenha-o atualizado a cada mudança de configuração ou de interface.
- **Comentários no código:** explique o motivo das decisões, a unidade das grandezas e os limites conhecidos. Cada função pública do `.h` tem um comentário acima dela.
- **Este guia:** mudanças nas convenções devem ser discutidas com a equipe e registradas na seção 12.

## 11. Solução de problemas comuns

| Sintoma | Causas prováveis |
|---|---|
| `inicializar()` devolve `false` | SDA e SCL trocados; sensor sem alimentação de 3,3 V; GND não compartilhado; endereço I2C incorreto (use um I2C scanner); placa que não usa GPIO21 e GPIO22 como padrão |
| Leituras sempre iguais ou zeradas | `atualizar()` não está sendo chamada no `loop()`; sensor em modo de repouso; falha na comunicação não tratada |
| Erro de compilação "was not declared in this scope" | Função usada antes de ser declarada em um `.cpp`; falta de `#include`; namespace incorreto |
| Erro de compilação "multiple definition" | Função definida no `.h` (a definição vai no `.cpp`); funções com o mesmo nome fora de um namespace em módulos diferentes |
| Texto ilegível no Monitor Serial | Velocidade do monitor diferente de 115200 |
| Programa "trava" na inicialização | `while` bloqueante ou `delay()` longo no código |
| PR com aviso de conflito | Fork desatualizado; peça ajuda ao responsável pelo repositório |

## 12. Histórico de revisões

| Versão | Data | Descrição |
|---|---|---|
| 1.0 | 21/10/2026 | Versão inicial |

## Apêndice A - Modelo de módulo (.h e .cpp)

Substitua `exemplo` pelo modelo do seu sensor e preencha os trechos indicados. Arquivo `sensor_exemplo.h`:

```cpp
#pragma once

#include <Arduino.h>

namespace exemplo {

// Configura o sensor. Devolve true se ele respondeu.
// Chamar uma vez, no setup() do main.cpp, depois de Wire.begin().
bool inicializar();

// Faz UMA leitura e guarda o resultado internamente. Nao bloqueia.
// Devolve true se a leitura foi bem-sucedida.
bool atualizar();

// Ultimo valor lido por atualizar(). Unidade: (documente aqui, em SI).
float obter_valor();

}  // namespace exemplo
```

Arquivo `sensor_exemplo.cpp`:

```cpp
#include "sensor_exemplo.h"

#include <BibliotecaDoSensor.h>     // substitua pela biblioteca real

namespace exemplo {

// Tudo dentro deste namespace anonimo e interno ao arquivo.
namespace {

constexpr uint8_t ENDERECO_I2C = 0x00;   // preencha com o endereco do sensor

BibliotecaDoSensor sensor;               // objeto da biblioteca do sensor
float valor_atual = 0.0f;

}  // namespace

bool inicializar() {
  return sensor.begin(ENDERECO_I2C);
}

bool atualizar() {
  // Leia o sensor, valide e guarde em valor_atual.
  // Em caso de falha, devolva false (sem esperar nem travar).
  return true;
}

float obter_valor() {
  return valor_atual;
}

}  // namespace exemplo
```

## Apêndice B - Modelo de README do sensor

Copie este modelo para o `README.md` da pasta do seu sensor.

```text
# <MODELO DO SENSOR>

Responsável: <nome>
Descrição: o que o sensor mede, em uma frase.

## Hardware
- Endereço I2C: 0x.. (e como alterá-lo, se for o caso)
- Alimentação: 3,3 V
- Ligação: SDA -> GPIO21, SCL -> GPIO22

## Dependências
- Biblioteca externa: nome e versão usada

## Configuração usada
- Faixas, resolução, taxa de amostragem e outros parâmetros

## Interface do módulo
- inicializar(): o que faz
- atualizar(): o que faz
- obter_<grandeza>(): grandeza e unidade

## Como testar
- Passos para reproduzir o teste de bancada

## Limitações conhecidas
- O que o módulo ainda não faz ou não trata
```

## Apêndice C - Checklist de pull request

Antes de abrir o PR, confira:

- [ ] O fork foi sincronizado com o repositório principal.
- [ ] As alterações estão em uma branch própria, e não na `main` do fork.
- [ ] As alterações estão restritas à pasta do meu sensor e à integração mínima no `main.cpp`.
- [ ] A pasta do sensor tem o nome do modelo, sem acentos ou espaços.
- [ ] Nomes de variáveis, funções, constantes e arquivos seguem a seção 6.1.
- [ ] Não há números soltos, código comentado ou variáveis sem uso.
- [ ] O módulo não possui `setup()` nem `loop()`, e não usa `delay()` nem `while` bloqueante.
- [ ] `inicializar()` e `atualizar()` devolvem `bool`, e o `main.cpp` checa o retorno.
- [ ] O módulo não escreve na porta serial.
- [ ] O endereço I2C foi registrado na seção 7.3.
- [ ] O `README.md` do sensor está atualizado.
- [ ] Os testes da seção 9 foram feitos e descritos no PR.
- [ ] Nenhum arquivo compilado, configuração pessoal ou senha foi enviado.

## Apêndice D - Glossário

| Termo | Significado |
|---|---|
| ESP32 | Microcontrolador usado no projeto |
| Sketch | Programa Arduino (arquivo `.ino`) |
| Módulo | Conjunto de arquivos `.h` e `.cpp` que controla um sensor |
| Cabeçalho (`.h`) | Arquivo que declara o que um módulo oferece a quem o usa |
| Implementação (`.cpp`) | Arquivo com o código que realiza o que o cabeçalho declara |
| Namespace | Espaço de nomes que evita colisão entre funções de módulos diferentes |
| I2C | Protocolo de comunicação de dois fios (SDA e SCL) entre o ESP32 e os sensores |
| SDA / SCL | Linha de dados / linha de relógio do I2C |
| Baud | Velocidade da porta serial, em bits por segundo |
| Código bloqueante | Código que prende o programa esperando algo, impedindo o resto de funcionar |
| Fork | Cópia pessoal do repositório na sua conta do GitHub |
| Branch | Linha de trabalho separada dentro de um repositório |
| Commit | Conjunto de alterações salvo com uma mensagem |
| Pull request (PR) | Pedido para incluir suas alterações no repositório principal |
| Sync | Atualizar o seu fork com as novidades do repositório principal |
| Merge | Inclusão das alterações de um PR no repositório principal |
