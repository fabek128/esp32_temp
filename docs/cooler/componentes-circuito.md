# Lista de componentes: ESP32-S3 + SST-FHS80X

Lista para implementar el circuito de control PWM documentado en [`control-esp32-s3.md`](control-esp32-s3.md) y representado en [`diagrama_completo2.png`](../diagrama_completo2.png). El diagrama es conceptual: **no conectar hasta verificar físicamente el pinout del ventilador y los pads de la placa**.

## Ya disponibles según el proyecto

| Cant. | Elemento | Uso/nota |
|---:|---|---|
| 1 | ESP32-S3, alimentado por USB | GPIO4 PWM, GPIO5 TACH; confirmar que estén expuestos en la revisión real. |
| 1 | Módulo MAX6675 + termocupla | Ya conectado a 3V3/GND, SO→GPIO13, CS→GPIO10, SCK→GPIO12. |
| 1 | Cooler SilverStone SST-FHS80X | 12 V, 0,85 A nominal, conector PWM 4 pines. |
| 1 | Fuente ATX del gabinete | Usar exclusivamente su salida DC de +12 V/GND; confirmar conector y límite de corriente en la etiqueta. |
| 1 | Cable USB de datos | Mantiene alimentada la ESP32 por USB; no inyectar los 12 V a la placa. |

Los MOSFET IRF530N e IRF9540N mencionados no se usan en este circuito. El ventilador incorpora su etapa de potencia; Q1 solo adapta la señal lógica PWM.

## Comprar: componentes eléctricos mínimos

| Cant. | Componente | Especificación | Función |
|---:|---|---|---|
| 1 | Transistor NPN **2N3904** | Through-hole TO-92 de fabricante identificable; respetar el pinout de su datasheet | Q1: colector abierto para controlar la entrada PWM sin exponer GPIO4 al pull-up interno del fan. |
| 1 | Resistor **1 kΩ** | ¼ W, 5 % o mejor | En serie entre GPIO4 y base de Q1. |
| 1 | Resistor **10 kΩ** | ¼ W, 5 % o mejor | Entre base y emisor/GND; deja Q1 apagado durante reset o ESP32 sin alimentación. |
| 1 | Resistor **10 kΩ** | ¼ W, 5 % o mejor | Pull-up de TACH a 3V3 para GPIO5; conectar solo después de confirmar salida TACH colector abierto. |

El 2N3904 tiene capacidad eléctrica ampliamente mayor que la corriente lógica prevista por la interfaz PWM estándar; no se debe usar para conmutar los 0,85 A del motor. [Datasheet onsemi 2N3904](https://www.onsemi.com/download/data-sheet/pdf/2n3904-d.pdf)

## Cableado y montaje

Estos elementos dependen de lo que Fabian ya tenga; adquirir solo lo que falte:

| Cant. | Material | Requisito |
|---:|---|---|
| 1 | Adaptador/breakout o extensión para ventilador PWM de 4 pines | Elegir por el tipo real del conector (macho/hembra) y conservar acceso a los cuatro contactos. Preferir adaptador sin cortar el cable original. Verificar pinout, no confiar en colores. |
| 1 | Placa perforada pequeña o protoboard soldable | Para montar Q1 y las resistencias; evitar un montaje suelto dentro del gabinete. |
| Según necesidad | Cable de señal | Corto, aislado; para GPIO4, GPIO5 y 3V3/GND de señal. |
| Según necesidad | Cable para alimentación del ventilador | Cobre flexible y aislamiento adecuados a corriente nominal y pico de arranque medido; usar un arnés/conector compatible con el modelo exacto de PSU. **Los pinouts de cables modulares de PSU no son universales.** |
| Según necesidad | Tubo termocontraíble/aislación | Aislar terminales y patas expuestas; evitar cortocircuitos con el gabinete. |
| 1 opcional, recomendado | Portafusible en línea para +12 V del fan | Ubicar cerca de la toma de +12 V. Seleccionar el valor del fusible después de medir/obtener la corriente de arranque y verificar el límite del cable; no dimensionarlo solo con los 0,85 A nominales. |

## No comprar para esta versión

- MOSFET IRF530N/IRF9540N, módulo IRF520 o controlador PWM externo.
- Relé, diodo flyback o DC-DC para la ESP32: el 4-wire fan recibe 12 V directamente y la ESP32 sigue por USB.
- Pull-up de 5 V o 12 V para PWM/TACH; la señal de tacómetro se eleva a 3V3 y el pin PWM se aísla mediante Q1.
- Circuito para cortar la alimentación del ventilador: el apagado por PWM está deshabilitado; el firmware limita el duty mínimo a 40 %.

## Herramientas de verificación

- Multímetro para verificar continuidad, pinout, polaridad y ausencia de 12 V en los pines de la ESP32.
- Osciloscopio o analizador lógico recomendado para comprobar los 25 kHz en GPIO4 y el nivel de la señal PWM ya conectada al NPN. No conectar una sonda/analizador de 5 V al GPIO.
- Medidor de corriente o instrumento adecuado para medir el pico de arranque antes de definir el fusible.

## Condiciones antes de energizar

1. Confirmar GPIO4/GPIO5 en la serigrafía/revisión física de la placa; MAX6675 usa GPIO13/10/12.
2. Confirmar el pin 1 y orientación del conector SST-FHS80X con información fiable o medición segura. La ficha consultada no especifica el pinout desde el lado del cable ni la tensión real de TACH.
3. Verificar el pinout de patas del transistor 2N3904 según el fabricante concreto; TO-92 no garantiza el mismo orden de terminales entre marcas.
4. Comprobar que +12 V llegue solamente al pin 2 del ventilador y que fuente, ventilador, transistor y ESP32 compartan GND.
5. Mantener la placa alimentada por USB. No conectar +12 V a VIN, 5V, VBUS o 3V3 de la ESP32.
6. Probar primero el control con PWM liberado (orden 100 %) y validar RPM antes de usar la curva automática o instalarlo para enfriar la Tesla P100.

