# FocusMachine
FocusMachine é um projeto desenvolvido para a disciplina de Sistemas Computacionais Embebidos (Embedded Systems) da licenciatura em Engenharia Eletrotécnica e de Computadores do Instituto Politécnico de Leiria.

# O projeto
O projeto consiste em um temporizador inteligente que implementa o método Pomodoro de gerenciamento de tempo. Durante os momentos de foco, o dispositivo monitora as condições ambientes (temperatura, umidade relativa, luminosidade e movimentação) e apresenta um índice de conforto para realização de tarefas de foco prolongado. Além disso, utilizando um pipeline com duas redes neurais leves (Lightweight Neural Networks), o sistema classifica imagens periodicamente capturadas do usuário para determinar se este encontra-se em estado de foco (a olhar para a frente) ou não (a olhar para os lados ou ausente da imagem). Estes dados coletados são sintetizados, ao final de cada ciclo de trabalho em um índice percentual de concentração.

# Hardware
- ESP32-S3
- Câmera OV3660
- DHT22
- PIR HC-SR501
- LDR
- Display TFT com driver ILI9341
- Buzzer passivo

# Firmware
O Firmware do projeto é baseado em FreeRTOS. A arquitetura é baseada nas tasks:
- PomodoroFSMTask
- TelemetryTask
- DisplayTask
- DHTSensorTask
- LDRSensorTask
- PIRSensorTask
- CameraInferenceTask
- BuzzerTask

Diversos recursos inerentes ao Sistema Operativo em Tempo Real são explorados, como Idle Task Hook, Queues, Semaphores, Counting Semaphores, Mutexes, dentre outros.
