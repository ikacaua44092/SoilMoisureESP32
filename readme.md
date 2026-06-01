# 🌱 Plataforma Edge-to-BI para Telemetria de Umidade de Solo

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Anlytics-orange?style=flat-square&logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino_Core-blue?style=flat-square&logo=arduino)](https://www.arduino.cc/)
[![Target](https://img.shields.io/badge/Hardware-ESP32-red?style=flat-square)](https://www.espressif.com/)

Um sistema completo de internet das coisas (IoT) voltado para o monitoramento preditivo e automação de dados agrícolas. Este projeto realiza a coleta de dados de umidade de solo na camada de *Edge Computing*, processa os dados localmente utilizando estruturas de memória otimizadas e expõe endpoints para consumo assíncrono via Web e extração direta em ferramentas de Business Intelligence (Power BI).

---

## 📌 Cenário e Problema de Negócio

Na agricultura de precisão ou no manejo de áreas verdes urbanas, a falta de dados estruturados em tempo real sobre a umidade do solo gera desperdício de recursos hídricos (superirrigação) ou estresse hídrico na vegetação (subirrigação). 

**Solução proposta:** Um dispositivo de baixo custo e alta eficiência energética capaz de monitorar, armazenar localmente séries temporais e servir como um *pipeline* de dados limpos, eliminando a dependência imediata de infraestruturas pesadas de nuvem (Cloud) no momento da coleta.

---

## 🏗️ Arquitetura do Sistema e Fluxo de Dados

O projeto foi desenhado seguindo as melhores práticas de engenharia de software, dividindo o fluxo de dados em três camadas principais (Ingestão, Persistência e Consumo):

![Arquitetura de Dados](https://raw.githubusercontent.com/SEU_USUARIO_DO_GITHUB/SEU_REPOSITORIO/main/caminho_para_sua_imagem_b9eb5d.png)

> 💡 *Nota para o Portfólio:* Insira aqui a imagem do seu fluxograma de blocos colorido (`image_b9eb5d.png`) após subir o arquivo no repositório.

1. **Camada de Ingestão (Verde):** O microcontrolador executa rotinas não-bloqueantes controladas por tempo (`millis()`). A função de leitura captura o sinal analógico do sensor galvânico e normaliza o dado utilizando filtros matemáticos de saturação (`constrain`).
2. **Camada de Persistência Local (Amarelo):** Armazenamento em memória RAM através de um **Buffer Circular (Fila)** com tamanho fixo (`HISTORY_SIZE = 150`), garantindo que o sistema recicle o espaço de forma perpétua sem estourar os recursos de hardware.
3. **Camada de Consumo e Entrega (Azul):** O ESP32 atua como um servidor HTTP na porta 80, respondendo em paralelo com layouts interativos em HTML/JS e pacotes purificados para ferramentas analíticas.

---

## ⚙️ Stack Tecnológica

* **Hardware:** Módulo ESP32 (NodeMCU) + Sensor de Umidade do Solo Analógico (Pino G34).
* **Firmware:** Linguagem C++ sob o ecossistema PlatformIO / Arduino Core.
* **Protocolos e Redes:** Wi-Fi local (IEEE 802.11 b/g/n) + Sincronização de relógio mundial via protocolo **NTP** (`pool.ntp.br`).
* **Formatos de Dados:** JSON Esturutrado (`ArduinoJson.h`) e Séries Temporais em CSV.

---

## 🔌 Endpoints da API Local

Ao se conectar ao endereço IP gerado pelo dispositivo (ex: `http://10.0.0.108`), o servidor disponibiliza as seguintes rotas:

| Rota | Formato | Função Principal |
| :--- | :---: | :--- |
| `/` | `HTML/CSS/JS` | Interface Web com gráficos de linha interativos gerados via `Chart.js`. |
| `/data` | `JSON` | Retorna o status atual e a lista de histórico recente para atualização assíncrona da tela. |
| `/json` | `JSON (File)` | Fornece o histórico completo estruturado em um arquivo de download para integração com scripts em Python ou bancos NoSQL. |
| `/csv` | `CSV (File)` | **Endpoint de Integração com Power BI**: Disponibiliza a tabela purificada com cabeçalhos estruturados para ingestão direta em dashboards. |

---

## 📊 Estrutura de Dados (Modelo de Tabela)

O layout dos dados exportados segue rigorosamente o esquema definido pela `struct Measurement`:

* `timestamp`: Carimbo de data/hora no padrão Unix Epoch (segundos corridos).
* `raw_value`: Leitura analógica bruta gerada pelo conversor ADC do chip (escala $600$ a $4095$).
* `percentage`: Valor tratado, calibrado e convertido para regras de negócio ($0\%$ a $100\%$).
* `datetime`: Conversão amigável do timestamp para o formato de texto padrão (`AAAA-MM-DD HH:MM:SS`).

---

## 🗂️ Como Organizar e Executar este Projeto

### Estrutura de Arquivos Recomendada
```text
├── include/
│   └── credentials.h      # Arquivo local com as senhas de rede (protegido no .gitignore)
├── src/
│   └── main.cpp           # Código-fonte principal do Firmware do ESP32
├── platformio.ini         # Configurações de ambiente e dependências de bibliotecas
└── README.md              # Documentação do projeto
