# Pruebas De Hardware

Este documento registra la fase de diagnostico local del prototipo. En esta etapa no se usa WiFi, HiveMQ, InfluxDB ni Grafana: solamente ESP32, sensores, LED RGB y Monitor Serial.

## Objetivo

Validar que cada componente conectado al ESP32 funciona antes de integrar la comunicacion MQTT y el almacenamiento en la nube.

## Pines Usados

| Componente | Senal | Pin ESP32 |
|---|---:|---:|
| DHT11/DHT22 | Data | GPIO 4 |
| HC-SR04 | TRIG | GPIO 32 |
| HC-SR04 | ECHO con divisor de voltaje | GPIO 34 |
| MPU6050 | SDA | GPIO 21 |
| MPU6050 | SCL | GPIO 22 |
| LED RGB | Rojo | GPIO 25 |
| LED RGB | Verde | GPIO 26 |
| LED RGB | Azul | GPIO 27 |
| LED RGB | Comun | 3V3 |

## Conexion Del LED RGB

El LED RGB esta funcionando con logica invertida. La pata comun debe ir a `3V3` y cada color debe conectarse al ESP32 usando una resistencia de `220 ohm` o `330 ohm`.

| LED RGB | Conexion |
|---|---|
| Comun | 3V3 |
| Rojo | Resistencia -> GPIO 25 |
| Verde | Resistencia -> GPIO 26 |
| Azul | Resistencia -> GPIO 27 |

En `include/config.h` debe permanecer:

```cpp
constexpr bool LED_COMMON_ANODE = true;
```

## Conexion Del Sensor Ultrasonico

El sensor ultrasonico tipo HC-SR04 se alimenta con 5V, pero su salida `ECHO` tambien puede entregar 5V. Para proteger el ESP32, `ECHO` debe entrar al `GPIO 34` mediante divisor de voltaje.

```text
HC-SR04 VCC  -> ESP32 5V / VIN
HC-SR04 GND  -> ESP32 GND
HC-SR04 TRIG -> ESP32 GPIO 32
HC-SR04 ECHO -> R1 1k -> GPIO 34 -> R2 2k -> GND
```

## Procedimiento

1. Conectar el ESP32 al computador por USB.
2. Cargar el firmware de diagnostico.
3. Abrir el Monitor Serial a `115200` baudios.
4. Verificar la secuencia inicial del LED RGB: rojo, verde, azul, amarillo, blanco y apagado.
5. Revisar cada 2 segundos las lecturas impresas por Serial.
6. Registrar el resultado de cada componente en la tabla de resultados.

## Pruebas Esperadas

| Componente | Prueba | Resultado Esperado |
|---|---|---|
| LED RGB | Secuencia de colores al iniciar | Cambian rojo, verde, azul, amarillo y blanco |
| DHT11/DHT22 | Lectura ambiente | Temperatura y humedad aparecen en Serial |
| HC-SR04 | Acercar y alejar un objeto | La distancia en cm cambia claramente |
| MPU6050 | Mover o inclinar maqueta | Cambian X, Y, Z e inclinacion |

## Tabla De Resultados

| Componente | Estado | Observaciones |
|---|---|---|
| LED RGB | OK | Gradiente y colores primarios verificados |
| DHT11/DHT22 | OK | Temperatura y humedad impresas por Serial |
| HC-SR04 | OK | Distancia en cm impresa por Serial con divisor en ECHO |
| MPU6050 | OK | Aceleracion, inclinacion, vibracion y posicion verificadas |

## Notas De Calibracion

- El LED RGB quedo configurado con logica invertida, por eso `LED_COMMON_ANODE` debe estar en `true`.
- Si el sensor instalado es DHT11, cambiar `DHT_TYPE` de `22` a `11`.
- Para el HC-SR04, verificar que `ECHO` pase por divisor de voltaje antes de llegar al ESP32.
- El MPU6050 debe quedar fijo a la maqueta para que la inclinacion sea estable. En este montaje, `Y positivo` se considera `vertical normal`.
