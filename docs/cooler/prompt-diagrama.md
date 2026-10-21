# Prompt para generar el diagrama de conexionado

Usar este prompt para generar una ilustración técnica. El diagrama es conceptual; no sustituye la comprobación de pinout y continuidad con la placa y el ventilador reales.

```text
Crea un diagrama eléctrico técnico, claro y sobrio, en español, formato apaisado 16:9, fondo blanco, líneas limpias y etiquetas legibles. No inventes componentes, colores de cables, conectores ni orientación física que no estén especificados. Representa conexiones lógicas, no una fotografía de una placa.

Título: “Control seguro del ventilador SilverStone SST-FHS80X con ESP32-S3”.

Bloques a mostrar:
1) Una placa ESP32-S3 alimentada exclusivamente por USB desde una computadora. Etiquetar pines GPIO4 (PWM), GPIO5 (TACH), 3V3 y GND. Nota: confirmar que GPIO4 y GPIO5 estén expuestos en la revisión de placa real. MAX6675 existente conectado a 3V3, GND común, SO→GPIO13, CS→GPIO10 y SCK→GPIO12.
2) Una fuente externa de 12 V DC que alimenta solo al ventilador: +12V→pin 2 del ventilador; GND→pin 1. Unir el GND de 12V, ESP32-S3, MAX6675 y transistor en un único nodo común. Destacar en rojo: “NO conectar +12 V a la ESP32-S3”. No representar la fuente abierta ni su lado de red AC.
3) Un ventilador PWM de 4 pines rotulado “SST-FHS80X, 12 V, 0,85 A”. Usar SOLO numeración lógica de pines estándar, sin codificar colores ni dibujar una vista frontal/trasera ambigua del conector: pin 1 GND; pin 2 +12 V; pin 3 TACH, salida de tacómetro colector abierto (por confirmar en el ejemplar); pin 4 PWM, entrada de control del ventilador (no etiquetar como colector abierto). Agregar nota: “Verificar pin 1 y orientación en el conector real; no asumir por color”.
4) Etapa de control mínima con Q1 = transistor NPN 2N3904 como salida de colector abierto del controlador: GPIO4→resistor 1 kΩ→base Q1; resistor 10 kΩ desde base a emisor/GND (pulldown); emisor→GND común; colector→entrada pin 4 PWM del ventilador. La línea PWM tiene pull-up interno en el ventilador según interfaz estándar; NO conectar esa línea directamente al GPIO. Dibujar la inversión lógica: Q1 encendido lleva PWM a nivel bajo; Q1 apagado libera la línea y solicita máxima velocidad. Aclarar que “colector abierto” describe la salida del circuito Q1, no el pin de entrada PWM del ventilador.
5) Tacómetro: pin 3 TACH→GPIO5, con resistor de 10 kΩ desde TACH a 3V3 (pull-up). Nunca dibujar un pull-up a 5 V o 12 V. Nota: solo conectar después de confirmar que el TACH es salida colector/drenador abierto y compatible con pull-up externo a 3,3 V.

Notas de seguridad obligatorias dentro de la ilustración:
- “ESP32-S3 trabaja con GPIO de 3,3 V; no es tolerante a 5 V”.
- “GPIO4 genera PWM por LEDC a 25 kHz; el NPN implementa colector abierto y aísla el GPIO del pull-up interno del ventilador”.
- “GPIO5 mide TACH; la lectura de RPM requiere verificar pulsos/vuelta del ventilador”.
- “El firmware limita duty mínimo a 40 %; 0 % no apaga el fan y no se usa”.
- “En reset/fallo del MAX6675 se libera PWM o se ordena 100 % para priorizar máxima velocidad”.
- “GPIO4/GPIO5 y el pinout del fan deben verificarse físicamente antes de cablear”.

Usa estilos distintos para +12V, 3V3, GND, señal PWM y TACH; coloca leyenda de colores de líneas indicando que son colores gráficos, no colores reales de cables. Mantén legibilidad alta, evita cruces de líneas o resuélvelos con puentes/junction dots correctos. Revisa que cada conexión del texto esté dibujada exactamente, que no haya conexión de +12V al ESP32, y que el colector del NPN vaya a PWM, no a +12V ni a TACH. No agregues MOSFET, relé, diodo, controlador externo ni componentes que no estén en la lista.
```

## Verificación manual antes de publicar

- Confirmar que GPIO4/5 estén físicamente disponibles en la placa exacta.
- Verificar pin 1 del conector del SST-FHS80X con documentación del fabricante o medición segura; no asumir orientación por una ilustración genérica.
- Confirmar salida de tacómetro y niveles eléctricos antes de conectar GPIO5.
- Comparar el dibujo con `control-esp32-s3.md`; cualquier discrepancia invalida el gráfico.
