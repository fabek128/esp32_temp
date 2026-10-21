# Monitor de temperatura ESP32-S3 + MAX6675

## Hardware y seguridad

Firmware validado para ESP32-S3-DevKitC-1 compatible, ESP32-S3 detectado con 16 MB de flash y PSRAM integrada de 8 MB. MAX6675 conectado en 3,3 V, GND común, SO GPIO13, CS GPIO10 y SCK GPIO12. No conecte el sensor a 5 V: mantenga las señales compatibles con el ESP32-S3.

La carga normal de PlatformIO solo actualiza las regiones del firmware; no ejecutar borrado completo (`erase-flash`) ni modificar eFuses/particiones para este proyecto.

## Compilar, cargar y monitorizar

1. Instale PlatformIO Core (`python3 -m pip install platformio`) o use una instalación existente.
2. En la raíz del proyecto, ejecute `pio run`.
3. Identifique el puerto de la placa con `ls /dev/cu.*`; no suponga que el nombre no cambia.
4. Cargue con `pio run -e esp32-s3-devkitc-1 -t upload --upload-port /dev/cu.usbmodemXXXX`.
5. Monitor serie alternativo: `pio device monitor -p /dev/cu.usbmodemXXXX -b 115200`.
6. Monitor estadístico: `python3 -m venv .venv && .venv/bin/pip install -r requirements.txt && .venv/bin/python scripts/monitor.py`. Para seleccionar puerto manualmente, agregue `--port /dev/cu.usbmodemXXXX`.

El monitor Python solo acepta autodetección cuando hay un candidato USB único; si hay ambigüedad, indique `--port`. El monitor conserva estadísticas durante la sesión; al reconectar, continúa acumulándolas.

## Protocolo y diagnóstico

El MAX6675 devuelve 16 bits: bit D2 indica termocupla abierta y D[14:3] representan temperatura en incrementos de 0,25 °C. El firmware espera al menos 250 ms con CS alto entre conversiones y publica una muestra por segundo. No emite temperatura para una termocupla abierta o dato fuera de rango. La salida legible es `[TEMP] n.nn C`; errores se publican como `[ERROR]`.

Para JSON, cambie `-D MAX6675_JSON_OUTPUT=0` por `1` en `platformio.ini` y vuelva a compilar/cargar. Las muestras correctas incluyen temperatura y uptime; errores usan `temperature_c: null`.

Si la carga falla, vuelva a comprobar el puerto y el cable USB de datos. Para entrar manualmente al bootloader: mantenga BOOT presionado, pulse y suelte RESET, suelte BOOT y reintente únicamente en el puerto identificado. No cambie cableado o pines por conjetura.

## Estado de validación de hardware

En la prueba inicial se observó salida de arranque por USB y mensajes de termocupla abierta cada segundo durante más de 30 segundos. Esto confirma que el firmware corre y el puerto serie funciona, pero el módulo reporta D2=1: no hay una temperatura válida, por lo que no se informa ninguna medida. Con la alimentación desconectada, revise continuidad y asiento de la termocupla en los bornes del HW-550 y las conexiones de SO/CS/SCK/VCC/GND según la tabla acordada; no cambie GPIO ni puentee señales. Luego vuelva a conectar y reinicie para repetir la validación.

El registro fotográfico de la instalación y el mapa de señales se encuentra en [`docs/install/README.md`](install/README.md). La documentación visual es referencia; confirma las etiquetas serigrafiadas antes de intervenir el cableado.

La integración eléctrica, lista de componentes, curva inicial y pruebas progresivas del SST-FHS80X con ESP32-S3 se documentan en [`docs/cooler/control-esp32-s3.md`](cooler/control-esp32-s3.md), [`docs/cooler/componentes-circuito.md`](cooler/componentes-circuito.md) y [`docs/cooler/cooler1.md`](cooler/cooler1.md). El firmware ya genera PWM LEDC y mide tacómetro en GPIO asignados tentativamente. No se cargó ni probó con ventilador conectado; confirmar antes la placa/pinout y el cableado indicado en la guía.

## Wi-Fi

El firmware puede conectarse como estación Wi-Fi. `scripts/pio_wifi_env.py` busca primero las variables de entorno `WIFI_SSID` y `WIFI_PASSWORD`; si faltan, acepta en `.env` `WIFI_SSID`/`WIFI_PASSWORD` o las claves locales existentes `wifi_net`/`wifi_pass`. No imprime ni registra los valores. Si solo está definida una de las dos, el build falla.

Para compilar con Wi-Fi, ejecuta `pio run -e esp32-s3-devkitc-1`; para cargar, añade el puerto identificado al comando de carga de arriba. El firmware informa estado y código de estado Wi-Fi, pero no SSID, contraseña ni dirección IP; reintenta la conexión sin bloquear el muestreo del MAX6675. Sin ambas variables, Wi-Fi queda desactivado. Con Arduino ESP32, `estado=3` indica conectado, `1` que no encuentra el SSID, `4` fallo de conexión, `5` conexión perdida y `6` desconectado/intentando.

**Riesgo de credenciales:** para esta implementación las credenciales se incorporan como constantes en el firmware compilado. Cualquier persona con acceso de lectura al binario o acceso físico suficiente al dispositivo puede extraerlas; `.pio` está ignorado por Git, pero eso no protege contra extracción del chip o copias del binario. Usa una red IoT/de pruebas aislada, no una red corporativa o productiva. No compartas el firmware/binario. Como mínimo, limita `.env` al usuario (`chmod 600 .env`); el archivo local actual debe revisarse antes de usarlo. Para mayor protección futura, usa aprovisionamiento y almacenamiento seguro de credenciales en lugar de compilarlas en el firmware.

### Resultado de la prueba Wi-Fi

La carga se verificó y el muestreo MAX6675 siguió funcionando. La prueba serial reportó `estado=1` (`WL_NO_SSID_AVAIL`), por lo que **no se confirmó conexión Wi-Fi**. Esto suele indicar que el SSID configurado no se ve en la banda compatible de 2,4 GHz, está oculto/no disponible o no coincide con `wifi_net`; no se imprimieron el SSID ni la contraseña. Confirma localmente el nombre exacto de la red de 2,4 GHz y disponibilidad/visibilidad del punto de acceso, luego vuelve a compilar y cargar. No envíes las credenciales por chat.
