#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// =========================================================================
// PROTÓTIPOS DAS FUNÇÕES
// =========================================================================
void handleRoot();
void handleData();
void handleJSON();
void handleCSV();
void readSoilMoisture();
void storeMeasurement();

// Configurações WiFi
const char* ssid = "A casa de feioso";
const char* password = "123456789";

// Pino do sensor de umidade do solo (Calibrado com os seus valores!)
const int sensorPin = 34; 
const int dryValue = 4095;   // Totalmente Seco
const int wetValue = 600;   // 100% Úmido

WebServer server(80);

// Variáveis para armazenamento de dados
int soilMoisture = 0;
int soilMoisturePercent = 0;
unsigned long lastMeasurement = 0;
const unsigned long measurementInterval = 600000; // 10 minutos

// Estrutura para histórico de medições com Timestamp Real
struct Measurement {
  unsigned long timestamp;
  int moisture;
  int moisturePercent;
};

const int HISTORY_SIZE = 150;
Measurement measurements[HISTORY_SIZE];
int currentIndex = 0;

void setup() {
  Serial.begin(115200);
  
  // Conectar ao WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  // Configura o fuso horário do Brasil (Brasília: GMT -3) e puxa a hora da internet
  configTime(-3 * 3600, 0, "pool.ntp.br", "time.nist.gov");
  Serial.println("Horário sincronizado via NTP!");

  // Configurar rotas do servidor web
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/json", handleJSON);
  server.on("/csv", handleCSV);

  server.begin();
  Serial.println("Servidor HTTP iniciado");

  // Inicializar histórico vazio
  for (int i = 0; i < HISTORY_SIZE; i++) {
    measurements[i] = {0, 0, 0};
  }
}

void loop() {
  server.handleClient();
  
  // Fazer medições a cada intervalo
  if (millis() - lastMeasurement >= measurementInterval) {
    readSoilMoisture();
    storeMeasurement();
    lastMeasurement = millis();
  }
}

void readSoilMoisture() {
  soilMoisture = analogRead(sensorPin);
  
  // Converte usando a sua calibração de 4095 a 1300
  soilMoisturePercent = map(soilMoisture, dryValue, wetValue, 0, 100);
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

  Serial.printf("Bruto: %d | Porcentagem: %d%%\n", soilMoisture, soilMoisturePercent);
}

void storeMeasurement() {
  time_t now;
  time(&now); // Pega o horário real em segundos (Unix Epoch)

  measurements[currentIndex] = {
    (unsigned long)now,
    soilMoisture,
    soilMoisturePercent
  };
  
  currentIndex = (currentIndex + 1) % HISTORY_SIZE;
}

// =========================================================================
// PÁGINA HTML PRINCIPAL (Com o JavaScript corrigido para ler a nova estrutura)
// =========================================================================
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Monitor de Umidade do Solo</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .status-card { background: #4CAF50; color: white; padding: 20px; border-radius: 8px; margin-bottom: 20px; text-align: center; }
        .download-section { margin: 20px 0; padding: 15px; background: #e3f2fd; border-radius: 8px; }
        button { background: #2196F3; color: white; border: none; padding: 10px 20px; margin: 5px; border-radius: 5px; cursor: pointer; }
        button:hover { background: #1976D2; }
        .chart-container { position: relative; height: 400px; width: 100%; margin: 20px 0; }
        th, td { padding: 8px; text-align: center; border: 1px solid #ddd; }
        tr:nth-child(even) { background-color: #f9f9f9; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 Monitor de Umidade do Solo</h1>
        
        <div class="status-card">
            <h2>Leitura Atual</h2>
            <div>
                <p>Valor Bruto: <span id="rawValue">--</span></p>
                <p>Porcentagem: <span id="percentValue">--</span>%</p>
                <p>Status: <span id="statusText">--</span></p>
            </div>
        </div>

        <div class="chart-container">
            <canvas id="moistureChart"></canvas>
        </div>

        <div class="download-section">
            <h3>Exportar Dados para Power BI</h3>
            <p>Baixe os dados nos formatos abaixo para análise no Power BI:</p>
            <button onclick="downloadData('json')">📊 Baixar JSON</button>
            <button onclick="downloadData('csv')">📈 Baixar CSV</button>
            <button onclick="location.reload()">🔄 Atualizar Página</button>
        </div>

        <div style="margin-top: 20px;">
            <h3>Últimas Medições</h3>
            <table style="width: 100%; border-collapse: collapse;">
                <thead>
                    <tr style="background-color: #f2f2f2;">
                        <th>Horário da Medição</th>
                        <th>Valor Bruto</th>
                        <th>Porcentagem</th>
                    </tr>
                </thead>
                <tbody id="dataBody">
                </tbody>
            </table>
        </div>
    </div>

    <script>
        let moistureChart;

        // Atualiza a cada 3 segundos
        setInterval(updateData, 3000);
        updateData();

        async function updateData() {
            try {
                const response = await fetch('/data');
                const data = await response.json();
                
                document.getElementById('rawValue').textContent = data.rawValue;
                document.getElementById('percentValue').textContent = data.percentValue;
                document.getElementById('statusText').textContent = getStatusText(data.percentValue);
                
                updateChart(data);
                updateTable(data.history);
                
            } catch (error) {
                console.error('Erro ao atualizar dados:', error);
            }
        }

        function getStatusText(percent) {
            if (percent >= 70) return '🌊 Muito Úmido';
            if (percent >= 40) return '✅ Ideal';
            if (percent >= 20) return '⚠️ Atenção';
            return '🔥 Muito Seco';
        }

        function updateChart(data) {
            if (!moistureChart) {
                const ctx = document.getElementById('moistureChart').getContext('2d');
                moistureChart = new Chart(ctx, {
                    type: 'line',
                    data: {
                        labels: [],
                        datasets: [{
                            label: 'Umidade do Solo (%)',
                            data: [],
                            borderColor: '#4CAF50',
                            backgroundColor: 'rgba(76, 175, 80, 0.1)',
                            borderWidth: 2,
                            fill: true,
                            tension: 0.4
                        }]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        scales: {
                            y: { beginAtZero: true, max: 100 }
                        }
                    }
                });
            }

            // Usa a hora vinda do ESP32 para o eixo X do gráfico
            const timeLabel = data.currentTime;
            
            // Só adiciona no gráfico se for um horário válido e diferente do último
            if(timeLabel && moistureChart.data.labels[moistureChart.data.labels.length - 1] !== timeLabel) {
                moistureChart.data.labels.push(timeLabel);
                moistureChart.data.datasets[0].data.push(data.percentValue);
                
                if (moistureChart.data.labels.length > 20) {
                    moistureChart.data.labels.shift();
                    moistureChart.data.datasets[0].data.shift();
                }
                moistureChart.update();
            }
        }

        function updateTable(history) {
            const tbody = document.getElementById('dataBody');
            tbody.innerHTML = '';
            
            if(!history) return;

            history.forEach(measurement => {
                const row = document.createElement('tr');
                row.innerHTML = `
                    <td>${measurement.timeText}</td>
                    <td>${measurement.rawValue}</td>
                    <td>${measurement.percentValue}%</td>
                `;
                tbody.appendChild(row);
            });
        }

        function downloadData(format) {
            window.open(`/${format}`, '_blank');
        }
    </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

// Rota interna de dados em tempo real
void handleData() {
  StaticJsonDocument<2048> doc;
  doc["rawValue"] = soilMoisture;
  doc["percentValue"] = soilMoisturePercent;
  
  // Pega o horário atual do ESP32
  time_t now;
  time(&now);
  struct tm * timeinfo = localtime(&now);
  char currentBuf[10];
  strftime(currentBuf, sizeof(currentBuf), "%H:%M:%S", timeinfo);
  doc["currentTime"] = String(currentBuf);
  
  JsonArray history = doc.createNestedArray("history");
  
  // Lê do mais recente para o mais antigo para preencher a tabela
  int count = 0;
  for (int i = 0; i < HISTORY_SIZE; i++) {
    int index = (currentIndex - 1 - i + HISTORY_SIZE) % HISTORY_SIZE;
    if (measurements[index].timestamp != 0 && count < 150) { // Mostra as últimas 15 na tabela
      JsonObject measurement = history.createNestedObject();
      
      time_t rawtime = measurements[index].timestamp;
      struct tm * mTime = localtime(&rawtime);
      char mBuf[10];
      strftime(mBuf, sizeof(mBuf), "%H:%M:%S", mTime);
      
      measurement["timeText"] = String(mBuf);
      measurement["rawValue"] = measurements[index].moisture;
      measurement["percentValue"] = measurements[index].moisturePercent;
      count++;
    }
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleJSON() {
  StaticJsonDocument<8192> doc;
  JsonArray data = doc.to<JsonArray>();

  for (int i = 0; i < HISTORY_SIZE; i++) {
    int index = (currentIndex + i) % HISTORY_SIZE;
    if (measurements[index].timestamp != 0) {
      JsonObject measurement = data.createNestedObject();
      
      time_t rawtime = measurements[index].timestamp;
      struct tm * timeinfo = localtime(&rawtime);
      char buf[30];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);

      measurement["timestamp"] = measurements[index].timestamp;
      measurement["raw_value"] = measurements[index].moisture;
      measurement["percentage"] = measurements[index].moisturePercent;
      measurement["datetime"] = String(buf); 
    }
  }

  String response;
  serializeJsonPretty(doc, response);
  server.sendHeader("Content-Type", "application/json");
  server.sendHeader("Content-Disposition", "attachment; filename=soil_moisture_data.json");
  server.send(200, "application/json", response);
}

void handleCSV() {
  String csv = "timestamp,raw_value,percentage,datetime\n";
  
  for (int i = 0; i < HISTORY_SIZE; i++) {
    int index = (currentIndex + i) % HISTORY_SIZE;
    if (measurements[index].timestamp != 0) {
      time_t rawtime = measurements[index].timestamp;
      struct tm * timeinfo = localtime(&rawtime);
      char buf[30];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);

      csv += String(measurements[index].timestamp) + ",";
      csv += String(measurements[index].moisture) + ",";
      csv += String(measurements[index].moisturePercent) + ",";
      csv += String(buf) + "\n"; 
    }
  }
  
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=soil_moisture_data.csv");
  server.send(200, "text/csv", csv);
}