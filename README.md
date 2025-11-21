# Servico-02---Praticas-IOT
projeto de iot para monitoramento, com comunicação via ESP-NOW e envia dados para um banco de dados


Sensor (sender_servico02.ino)

    ESP32
    Sensor DHT11 (temperatura e umidade)
    Sensor Ultrassônico HC-SR04 (distância)
    Sensor PIR (movimento)
    Fotoresistor (LDR)

Display (receiver_servico02_Led.ino)

    ESP32
    Display LED Matriz MAX7219 (8x8)
    LED Vermelho
    Led Verde

 O Sensor é responsável pela coleta de dados e transmissão via ESP-NOW e InfluxDB.

 O Display recebe os dados via ESP-NOW e exibe as informações em um display LED matriz 8x8.
