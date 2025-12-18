# FocusMachine

Nome ALUNO A - Lucas Ferreira

Nome ALUNO B - Michal Mikulka

IPLEIRIA - Instituto Politécnico de Leiria

ESTG - Escola Superior de Tecnologia e Gestão

LEEC - Licenciatura em Engenharia Eletrotécnica e de Computadores

Sistemas Computacionais Embebidos


Vìdeo: https://youtu.be/I6PjmHkpRpc  

Código (gist): https://gist.github.com/LucasMacielFer/4735fd26c567fd56f3efb55dae64ac1a


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
