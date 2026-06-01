# Conexiones Del Prototipo

Todas las tierras deben estar unidas: `GND` del ESP32, sensores y LED RGB.

## Tabla General

| Componente | Pin del componente | Conexion ESP32 |
|---|---|---|
| DHT11/DHT22 | VCC | 3V3 |
| DHT11/DHT22 | GND | GND |
| DHT11/DHT22 | DATA | GPIO 4 |
| HC-SR04 | VCC | 5V / VIN |
| HC-SR04 | GND | GND |
| HC-SR04 | TRIG | GPIO 32 |
| HC-SR04 | ECHO | Divisor de voltaje -> GPIO 34 |
| MPU6050 | VCC | 3V3 |
| MPU6050 | GND | GND |
| MPU6050 | SDA | GPIO 21 |
| MPU6050 | SCL | GPIO 22 |
| LED RGB | Comun | 3V3 |
| LED RGB | Rojo | Resistencia -> GPIO 25 |
| LED RGB | Verde | Resistencia -> GPIO 26 |
| LED RGB | Azul | Resistencia -> GPIO 27 |
| Servo bloqueo puerta | Signal | GPIO 14 propuesto |
| Servo bloqueo puerta | VCC | 5V externo recomendado |
| Servo bloqueo puerta | GND | GND comun |
| Ventilador | Control PWM | GPIO 33 propuesto -> transistor/MOSFET |
| Ventilador | VCC | Fuente externa segun ventilador |
| Ventilador | GND | GND comun |

## Diagrama General

```text
ESP32
├── 3V3  -> DHT VCC, MPU6050 VCC
├── 5V   -> HC-SR04 VCC
├── GND  -> DHT GND, MPU6050 GND, HC-SR04 GND
├── 3V3  -> LED RGB comun
├── GPIO 4  -> DHT DATA
├── GPIO 21 -> MPU6050 SDA
├── GPIO 22 -> MPU6050 SCL
├── GPIO 25 -> Resistencia -> LED rojo
├── GPIO 26 -> Resistencia -> LED verde
├── GPIO 27 -> Resistencia -> LED azul
├── GPIO 32 -> HC-SR04 TRIG
├── GPIO 34 <- divisor de voltaje <- HC-SR04 ECHO
├── GPIO 14 -> servo puerta signal (fase futura)
└── GPIO 33 -> control PWM ventilador via transistor/MOSFET (fase futura)
```

## Sensor Ultrasonico HC-SR04

El pin `ECHO` del HC-SR04 entrega una senal de 5V. Esa senal no debe conectarse directamente al ESP32. Se usa un divisor de voltaje para bajar la senal a aproximadamente 3.3V.

Conexion recomendada:

```text
HC-SR04 VCC  -> ESP32 5V / VIN
HC-SR04 GND  -> ESP32 GND
HC-SR04 TRIG -> ESP32 GPIO 32
HC-SR04 ECHO -> divisor de voltaje -> ESP32 GPIO 34
```

Divisor de voltaje para `ECHO`:

```text
HC-SR04 ECHO (5V)
        |
       R1 = 1k ohm
        |
        +-----> ESP32 GPIO 34
        |
       R2 = 2k ohm
        |
       GND
```

Con `R1 = 1k` y `R2 = 2k`, la salida hacia el ESP32 queda cerca de 3.3V.

En el firmware el HC-SR04 se interpreta como sensor de puerta:

```text
distancia <= DOOR_CLOSED_MAX_CM -> closed
distancia > DOOR_CLOSED_MAX_CM o sin eco -> open
```

El valor inicial de `DOOR_CLOSED_MAX_CM` es `8.0 cm` y debe ajustarse segun la ubicacion real del sensor dentro de la maqueta.

## LED RGB

Por el comportamiento observado en la prueba, el LED esta trabajando con logica invertida: el comun va a `3V3` y cada color se enciende cuando su pin queda en `LOW`.

```text
Comun        -> 3V3
Rojo         -> resistencia 220/330 ohm -> GPIO 25
Verde        -> resistencia 220/330 ohm -> GPIO 26
Azul         -> resistencia 220/330 ohm -> GPIO 27
```

En el firmware esta configurado asi:

```cpp
constexpr bool LED_COMMON_ANODE = true;
```

Con esta configuracion, cada color se enciende cuando el pin correspondiente esta en `LOW`.

## Actuadores Futuros

El servo de bloqueo y el ventilador todavia no estan instalados. En la fase de API REST y frontend se simularan por Monitor Serial antes de conectarlos fisicamente.

### Servo Para Bloqueo De Puerta

Conexion propuesta:

```text
Servo signal -> ESP32 GPIO 14
Servo VCC    -> fuente externa 5V
Servo GND    -> GND comun con ESP32
```

Notas:

- No alimentar el servo directamente desde un GPIO.
- Si el servo consume demasiado desde el pin `VIN`, usar una fuente externa de 5V.
- El GND de la fuente externa debe unirse al GND del ESP32.
- El firmware debe iniciar en estado seguro: puerta trabada o simulacion de puerta trabada.

### Ventilador

El ventilador no debe conectarse directo a un GPIO. El ESP32 debe controlar un transistor o MOSFET con PWM.

Conexion conceptual:

```text
ESP32 GPIO 33 -> resistencia 220/330 ohm -> gate/base transistor
Fuente +      -> ventilador +
Ventilador -  -> transistor/MOSFET -> GND
GND fuente    -> GND ESP32
```

Notas:

- Si el ventilador es DC con motor, agregar diodo de proteccion en paralelo con el ventilador.
- Usar una fuente externa acorde al voltaje del ventilador, por ejemplo 5V o 12V.
- El GPIO solo entrega la senal PWM, no la potencia del ventilador.
- Mientras no exista hardware, la potencia se simula por Serial como `ACTUADOR FAN: potencia=60%`.
