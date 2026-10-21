Bearing Type: Dual Ball Bearings
Connector: 4-pin PWM fan connector
Rated Voltage: 12V
Starting voltage: 6V
Rated Current: 0.85A
Rated Power: 10.2 W
Rotational Speed: 2,000 ~ 10,000 RPM
Flow Rate: 83.66 CFM
Static Pressure: 2 inches (50.77 mm) H2O
Noise Level: 61.4 dBA
Dimension: 80mm (W) x 38mm (H) x 80mm (D)
Weight: 7.1 oz (203 g).


Features & Specs
Power Connector Type	4-Pin
Voltage	12 volts
Wattage	10.2 watts
Cooling Method	Air
Compatible Devices	Desktop
Noise Level	61.4 db
Maximum Rotational Speed	10000 rpm

Item details
Brand Name	SilverStone
Manufacturer	SilverStone
Number of Items	1
Unit Count	1 Count
Included Components	Main unit, etc
Item Weight	9.6 ounces
Model	SST-FHS80X
Part Number	SST-FHS80X
Best Sellers Rank	
#840 in Computer CPU Cooling Fans
ASIN	B0BWJ3YLQY

## Integración prevista en el gabinete

El ESP32-S3 estará instalado dentro del gabinete y se indicó que tendrá acceso a los conectores externos de una fuente ATX de 850 W. La potencia total de 850 W no confirma por sí sola cuánta corriente está disponible en un conector específico ni el pinout de los cables modulares. Antes del montaje hay que identificar marca/modelo de la fuente, su etiqueta de salida de 12 V y el tipo de conector que se usará. Los cables modulares deben ser los compatibles con ese modelo de fuente; sus pinouts del lado de la fuente no son universales.

El cooler SST-FHS80X está especificado para **12 V DC, 0,85 A y 10,2 W**, con conector PWM de 4 pines y rango publicado de 2.000–10.000 RPM. El fabricante/distribuidor consultado no publica en la ficha disponible los umbrales eléctricos detallados, el pinout desde el lado del cable ni el comportamiento específico a PWM 0 %. Por tanto, se toma el estándar de ventiladores Intel de 4 hilos como referencia de interfaz, pero hay que verificar el conector y el pin 1 físicamente antes de cablear.

Para generar la forma de onda se usa LEDC del ESP32-S3 a 25 kHz en GPIO4. No se conecta directamente el GPIO a PWM: aunque el modo open-drain evita conducir un nivel alto, el GPIO del ESP32-S3 **no es tolerante a 5 V** y no se ha validado que el periférico LEDC del core instalado conserve open-drain al enrutarlo al GPIO. La conexión prevista usa un NPN pequeño como colector abierto; así la línea PWM del ventilador puede subir a su nivel interno sin exponer el GPIO. Los MOSFET IRF530N e IRF9540N disponibles no son necesarios ni se recomiendan en esta señal.

El tacómetro del estándar es una salida colector/drenador abierto, normalmente 2 pulsos por revolución. Se prevé un pull-up externo de 10 kΩ a 3,3 V y lectura por GPIO5. No conectarlo al ESP32 hasta confirmar que el tacómetro del ejemplar tiene salida abierta y no una salida push-pull a tensión mayor.

El firmware toma la curva automática de temperatura configurada en `src/main.cpp`: 40 % hasta 50 °C, interpolación lineal hasta 100 % a 80 °C. PWM al 0 % no se considera seguro: el comportamiento de apagado no está confirmado para este modelo y el firmware lo limita al mínimo configurado. El 40 % es un valor inicial conservador de control, **no una especificación de RPM mínima**; hay que probar el arranque estable del ventilador y ajustar antes de instalarlo para enfriar la Tesla P100. Si falla el sensor, la orden es 100 %.

Ver conexiones propuestas y pruebas por etapas en [`control-esp32-s3.md`](control-esp32-s3.md), y la lista completa de componentes en [`componentes-circuito.md`](componentes-circuito.md). No cablear hasta comprobar revisión/serigrafía exacta de la placa, conector/pin 1 del cooler y tensión de tacómetro.

### Alimentación de la placa

- Mantener la alimentación USB actual mientras sea posible.
- Si se decide alimentar el ESP32 desde la fuente ATX, usar un convertidor DC-DC regulado adecuado para obtener la tensión de entrada admitida por la placa (por ejemplo, 5 V por su entrada USB/5V, conforme al manual de la placa).
- **No conectar 12 V directamente a 3V3, 5V o VBUS del ESP32.** Confirmar polaridad, tensión y capacidad del convertidor con multímetro antes de conectar la placa.
- No abrir la fuente ATX ni trabajar en su lado de tensión de red. Desconectar la alimentación de red antes de montar o modificar conexiones de baja tensión.

### Datos pendientes antes de cablear o habilitar el control

1. Marca/modelo de la fuente ATX y límite de corriente del riel de 12 V en su etiqueta.
2. Conector de salida concreto y adaptador/cable compatible con el modelo de fuente.
3. Confirmar por lectura de la serigrafía que GPIO4 y GPIO5 están expuestos y disponibles en la revisión de la placa conectada.
4. Confirmar con multímetro el pinout del cable SST-FHS80X y el nivel del tacómetro; no confiar solo en el color o la forma del conector.
5. Confirmar que la termocupla mide el punto térmico que se desea enfriar y validar la curva de velocidades mínimas/umbrales.

Ante fallo del sensor, el firmware ordena PWM de máxima velocidad. En reset o sin alimentación de la ESP32, el pull-down de base deja el NPN apagado y la entrada PWM liberada (máxima velocidad según interfaz estándar). Esto no protege ante fallo físico del ventilador, falta de 12 V o cable suelto; hace falta supervisión térmica independiente para una instalación crítica.

**Estado actual:** firmware actualizado pero no cargado ni probado con el ventilador. GPIO4/5 son asignaciones para ESP32-S3 aún pendientes de confirmar físicamente en la placa. El cableado y la curva deben validarse antes del uso térmico.
