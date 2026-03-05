# FocusMachine

Este repositório contém o desenvolvimento do sistema FocusMachine, uma solução inteligente de 
gestão de tempo que utiliza monitorização ambiental e biométrica para otimizar a produtividade. 
O projeto foi desenvolvido por Lucas Ferreira e Michal Mikulka no âmbito da unidade curricular 
de Sistemas Computacionais Embebidos (2025/2026).

A solução diferencia-se dos temporizadores convencionais ao integrar uma câmara com TinyML para
detecção facial, permitindo calcular um "índice de foco" baseado na atenção visual do utilizador
durante as sessões de trabalho.

### Vìdeo: https://youtu.be/I6PjmHkpRpc  

## 🛠️ Arquitetura e Tecnologias

- Core: Microcontrolador ESP32-S3 operando sobre o Sistema Operativo de Tempo Real FreeRTOS.

- Monitorização Biométrica: Câmara OV3660 com processamento local de imagem para detecção facial via rede neural embebida.

- Monitorização Ambiental: Integração de sensores DHT22 (temperatura/humidade), LDR (luminosidade) e PIR HC-SR01 (movimento).

- Interface: Display TFT ILI9341 (320×240) para visualização de métricas e estado do sistema.

- Interação: Sensores de toque capacitivo para configuração de tempos e botão físico para controlo de estados (FSM).


## ✨ Funcionalidades Principais

- Gestão Pomodoro: Implementação de uma máquina de estados finita com ciclos configuráveis de "Foco", "Pausa Curta" e "Pausa Longa".

- Cálculo de Performance: Geração automática de um índice de foco (0-100%) e um índice de conforto ambiente baseado na fusão de dados sensoriais.

- Privacidade: Processamento de imagem 100% local (Edge AI), garantindo que nenhum dado visual sai do dispositivo.

- Alertas Inteligentes: Notificações sonoras via Buzzer PWM para transições de estado, exigindo reconhecimento do utilizador.

- Gestão de Recursos: Uso de Message Queues, Semáforos (binários e de contagem) e Mutexes para garantir a integridade e sincronização das tarefas no RTOS.


## 📊 Gestão de Firmware (RTOS Tasks)

- PomodoroFSMTask (Prioridade 4): Atua como o "cérebro" do sistema (Brain Task). Gere a Máquina de Estados Finita que controla os ciclos de "Foco", "Pausa Curta" e "Pausa Longa", coordenando as restantes tarefas e a lógica do temporizador decrescente.

- TelemetryTask (Prioridade 3): Responsável pela fusão de dados provenientes de todos os sensores e da câmara. Calcula o índice de foco (baseado na atenção visual) e o índice de conforto ambiental (baseado em temperatura, humidade e luz), formatando-os para serem desenhados no display.

- CameraInferenceTask (Prioridade 1): Executada exclusivamente no Core 1 para evitar latência no temporizador. Captura periodicamente imagens através da câmara OV3660 e corre o modelo de TinyML para detetar se o rosto do utilizador está presente e focado.

- DisplayTask (Prioridade 2): Gere a comunicação via SPI com o ecrã TFT ILI9341. Atualiza a interface gráfica, incluindo o cronómetro e as métricas de desempenho, com uma latência máxima de 1 segundo.

- BuzzerTask (Prioridade 3): Uma tarefa dinâmica do tipo fire and forget. É criada pela PomodoroFSMTask apenas quando é necessário emitir alertas sonoros nas transições de estado e eliminada imediatamente após a conclusão do aviso.

- DHTSensorTask (Prioridade 1): Realiza a leitura do sensor DHT22 via protocolo OneWire para monitorizar a temperatura e a humidade relativa do ambiente.

- LDRSensorTask (Prioridade 1): Faz a aquisição analógica dos níveis de luminosidade através de um fotoresistor (LDR).

- PIRSensorTask (Prioridade 1): Monitoriza a presença e o movimento do utilizador utilizando o sensor HC-SR01. Gere o acúmulo de disparos detetados para alimentar o cálculo de conforto.
