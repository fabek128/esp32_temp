# Control del SST-FHS80X con ESP32-S3

## Alcance y verificaciones

- La ficha disponible del SST-FHS80X confirma 12 V, 0,85 A, conector PWM de 4 pines y 2.000–10.000 RPM con PWM. No publica frecuencia, niveles lógicos, pinout por color ni respuesta a 0 % específicamente.
- Se usa como referencia el estándar Intel de ventiladores PWM de 4 hilos: pines 1 GND, 2 +12 V, 3 tacómetro (salida colector abierto) y 4 PWM (entrada colector abierto, 25 kHz nominal y 21–28 kHz permitido). El tacómetro suele generar dos pulsos por vuelta. El estándar admite distintos comportamientos al duty mínimo/0 %; no se habilita apagado en el firmware.
- La interfaz estándar permite un colector abierto como driver. Se coloca un NPN entre el GPIO y PWM para no exponer un GPIO de 3,3 V a una posible elevación interna de hasta 5,25 V en la línea PWM. Open-drain no vuelve tolerante a 5 V al GPIO del ESP32-S3.
- El chip conectado por `/dev/cu.usbmodem1101` se identificó con esptool como ESP32-S3, con 16 MB de flash y 8 MB de PSRAM. La configuración actual de PlatformIO usa el perfil genérico DevKitC-1-N8 de 8 MB sin PSRAM; no usar PSRAM desde el firmware por ahora. El perfil del repo no identifica con certeza la revisión física de la placa.
- Corrección importante: GPIO25 **no existe** en ESP32-S3 y GPIO27 puede estar reservado para memoria externa. Se reasignó a GPIO4 (PWM) y GPIO5 (tacómetro), que son GPIO regulares del ESP32-S3 y no se solapan con el MAX6675 (13/10/12), USB (19/20) ni el bus de memoria (26–37). Aún hay que confirmar que esos pads están expuestos en la placa física.
- El diagrama anterior que mostraba ESP-WROOM-32/GPIO25 fue retirado porque no correspondía al ESP32-S3 detectado. El prompt para producir una nueva ilustración controlada está en [`prompt-diagrama.md`](prompt-diagrama.md); el diagrama nuevo seguirá siendo conceptual hasta confirmar el pinout físico del ventilador y la serigrafía de la placa.

## Revisión de [`docs/diagrama_completo2.png`](../diagrama_completo2.png)

La versión nueva corrige la etiqueta anterior: pin 4 aparece como entrada PWM y Q1 como salida de colector abierto del controlador. El esquema concuerda visualmente con las conexiones lógicas de esta guía (GPIO4/5, NPN, resistencias, MAX6675 y GND compartido). Sigue siendo conceptual: confirmar pads de la placa, pin 1/orientación del conector y salida eléctrica TACH del ventilador antes de cablear.

## Componentes mínimos

1. 1 × transistor NPN de señal **2N3904** (alternativa: BC547, respetando el pinout de su fabricante).
2. 1 × resistor de **1 kΩ** entre GPIO4 y base.
3. 1 × resistor de **10 kΩ** entre base y emisor/GND, para mantenerlo apagado durante reset.
4. 1 × resistor de **10 kΩ** desde la señal de tacómetro a 3V3 como pull-up.

Los IRF530N/IRF9540N disponibles no se usan: no hacen falta para la entrada lógica PWM y sus parámetros de compuerta no son la opción adecuada para esta interfaz de 3,3 V. La potencia del motor la conmuta el driver interno del ventilador; no se conecta el GPIO al motor.

## Diagrama eléctrico propuesto

```text
Fuente ATX (solo lado DC aislado)              ESP32-S3 alimentado por USB
  +12 V  ------------------------------------> Fan pin 2 (+12 V)
  GND    -----------+------------------------> Fan pin 1 (GND)
                    +------------------------> ESP32 GND
                    +------------------------> Q1 emisor (2N3904)

Fan pin 4 (PWM) -----------------------------> Q1 colector
                                                  Q1 = NPN, colector abierto
ESP32 GPIO4 --[1 kΩ]--> Q1 base
                            |
                          [10 kΩ]
                            |
                           GND

Fan pin 3 (TACH) --------+-------------------> ESP32 GPIO5
                          |
                        [10 kΩ]
                          |
                         3V3 del ESP32
```

**No conectar +12 V a la ESP32.** GND de fuente y ESP32 se comparte porque la señal necesita referencia común. La ESP32 mantiene su alimentación USB. No abrir la fuente ATX ni tocar su lado de red.

Las posiciones 1–4 son numeración lógica del estándar, no el orden visual inferido de una foto. Identificar pin 1 por el moldeado/guía del conector o continuidad con el cable y comprobar antes con multímetro. No confiar solo en colores. Confirmar la orientación/pinout del transistor concreto con su datasheet: los TO-92 no tienen un orden universal de patas.

## Firmware y comportamiento

- LEDC a 25 kHz, 10 bits, GPIO4, con inversión lógica para la etapa NPN: cuando el transistor conduce, la línea PWM va a nivel bajo.
- GPIO5 mide flancos descendentes del tacómetro. Con 2 pulsos/vuelta, `RPM = pulsos_en_1s × 30`.
- Curva inicial en `src/main.cpp`: 40 % hasta 50 °C; rampa lineal hasta 100 % a 80 °C. Ejemplos: 60 °C ≈ 60 %, 70 °C ≈ 80 %. El valor bajo es un punto de partida para probar, no RPM garantizada por SilverStone.
- Ningún pedido puede parar el ventilador: 0 % se limita a 40 %. Al inicio/reset, GPIO en alta impedancia y pulldown de base apagan Q1; PWM queda liberado y el fan solicita máxima velocidad. Ante termocupla abierta/lectura inválida el firmware ordena 100 %.
- Si no se detectan pulsos TACH durante 3 s, el firmware enclava el comando al 100 % hasta reiniciar y reporta error. Esto también ocurre si TACH quedó desconectado; el GPIO5/pull-up debe cablearse y probarse antes de usar la curva automática.
- RPM es telemetría, no la variable regulada en lazo cerrado. La curva controla duty PWM por temperatura. Validar qué RPM produce cada duty y elevar el mínimo si no arranca o se detiene.
- Si falta 12 V, el motor/ventilador falla o hay un cable abierto, el firmware no puede garantizar refrigeración. Para una GPU crítica se requiere un mecanismo de protección térmica independiente.

## Pruebas progresivas (no saltar pasos)

1. **Identificación:** sin alimentación, leer el modelo/revisión serigrafiados de la placa y localizar pads GPIO4, GPIO5, GND y 3V3. Verificar que GPIO13, GPIO10 y GPIO12 siguen reservados al MAX6675 y no haya conflicto.
2. **Inspección del ventilador:** confirmar modelo SST-FHS80X, pin 1/orientación de conector y continuidad del cableado. Obtener del fabricante o medir de forma segura que TACH sea open-collector y no supere 3,3 V con pull-up externo. No conectar TACH si no se confirma.
3. **Revisión sin alimentación:** verificar orientación de Q1, valores de resistencias, aislamiento de patas y ausencia de conexión de 12 V a la placa. Comprobar continuidad entre tierras.
4. **Prueba de PWM sin fan:** con fuente de 12 V todavía desconectada, cargar firmware y medir GPIO4/onda de base (osciloscopio/analizador lógico): frecuencia 25 kHz, duty inicial equivalente a máxima velocidad (NPN apagado). Verificar que reset deje el GPIO bajo/alta impedancia y el pulldown mantenga Q1 apagado.
5. **Prueba con fan:** conectar primero GND compartido y fan 12 V; aplicar 12 V al fan solamente tras confirmar pinout/polaridad. Comenzar con comando 100 %, comprobar giro y RPM. No alimentar el motor desde ESP32 ni USB.
6. **Prueba de velocidad:** cambiar gradualmente temperatura simulada o valores de curva en un entorno de prueba, observar duty y RPM. Probar extremos 40/100 %. Confirmar arranque repetible en el duty mínimo; subir el mínimo si hay vibración, falta de arranque o RPM errática.
7. **Fallo de sensor/reset:** desconectar la termocupla con seguridad y confirmar orden 100 %. Reiniciar ESP32 y confirmar que Q1 permanece apagado durante boot y el ventilador queda a máxima velocidad con PWM liberado.
8. **Integración térmica:** solo después de pruebas anteriores, comprobar ubicación de termocupla y curva bajo carga real, con supervisión de temperaturas de la GPU. No usar una prueba de banco para dar por validada la refrigeración de la P100.

## Fuentes

- [SilverStone/Tekwind SST-FHS80X: ficha y valores nominales](https://www.tekwind.co.jp/SilverStone/products/entry_17022.php)
- [Intel 4-Wire PWM Fan Specification, copia del documento técnico](https://www.konilabs.net/docs/standards/fan/intel_4wire_pwm_fans_specs_rev1_2.pdf)
- [Espressif ESP32-S3 GPIO y modo open-drain](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- [Espressif ESP32-S3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

