# FLIPPER-ESP32 — Documentación Completa

Proyecto de la materia de programación: un dispositivo multiuso estilo **Flipper Zero**
construido con un **ESP32 clásico** y módulos económicos, con una arquitectura de
**"SO en la flash + programas en la microSD"** (scripts Lua y de comandos).

> Autor del proyecto: _(tu nombre)_ · Materia: Programación · Fecha: 2026

---

## Índice

1. [Resumen](#1-resumen)
2. [Hardware (componentes)](#2-hardware-componentes)
3. [Cableado completo](#3-cableado-completo)
4. [Arquitectura del sistema](#4-arquitectura-del-sistema)
5. [Estructura de la microSD](#5-estructura-de-la-microsd)
6. [El menú del OS](#6-el-menú-del-os)
7. [Módulos en detalle](#7-módulos-en-detalle)
8. [Intérprete de comandos (.txt)](#8-intérprete-de-comandos-txt)
9. [API de Lua](#9-api-de-lua)
10. [Compilar y flashear](#10-compilar-y-flashear)
11. [Problemas encontrados y soluciones](#11-problemas-encontrados-y-soluciones)
12. [Historial del desarrollo](#12-historial-del-desarrollo)
13. [Uso responsable y legal](#13-uso-responsable-y-legal)
14. [Créditos y referencias](#14-créditos-y-referencias)

---

## 1. Resumen

**FLIPPER-ESP32** es un "multiherramienta" que integra:

- 🪪 **NFC/RFID** (PN532): leer UID, leer el texto NDEF, clonar (dump) y emular (genérico).
- 📡 **Infrarrojo**: recibir y reenviar códigos de controles remotos.
- 📶 **Radio 2.4 GHz** (2× NRF24L01): escanear canales y prueba de envío/recepción.
- 📶 **WiFi**: escanear redes, conectar (credenciales desde la SD) y crear un punto de acceso.
- 🔵 **Bluetooth BLE**: escanear dispositivos cercanos.
- 💾 **microSD**: stores de datos, intro, programas y dumps.
- ⌨️ **BadUSB** (teclado BLE): ejecutar scripts de teclado.
- 🖥️ **Pantalla OLED/TFT** + joystick: menú navegable.
- 🔋 **Batería** y **modo dormir** (bajo consumo).
- 🎬 **Intro de arranque** configurable desde la SD.

La idea central: el **"SO" vive en la flash** (menú, drivers, intérpretes) y los
**"programas" viven en la SD** (`/programas/*.lua` o `*.txt`). Cambias el programa en la
SD, reinicias y se ejecuta **sin reflashear**.

---

## 2. Hardware (componentes)

| Cant. | Componente | Rol |
|---|---|---|
| 1 | **ESP32 clásico** (ESP32-D0WD-V3, WiFi+BT, dual core) | Cerebro / "SO" |
| 1 | **Pantalla SH1106** 128×64 (I2C) | Interfaz / menú |
| 1 | **PN532** (Elechouse NFC Module V3) | NFC/RFID 13.56 MHz |
| 2 | **NRF24L01** | Radio 2.4 GHz |
| 1 | **HW-125** (adaptador microSD, SPI) | Almacenamiento |
| 1 | **HW-373 v1.2.1** (TP4056, USB-C) | Cargador de batería Li-Ion |
| 1 | **Joystick KY-023** (2 ejes + botón) | Navegación |
| 1 | **KY-022** (receptor IR 38 kHz) | Recibir infrarrojo |
| 1 | **LED IR** + resistencia 100–220 Ω | Emitir infrarrojo |
| 1 | Batería Li-Ion 3.7 V + divisor 10k/10k | Alimentación / medición |
| 1 | Resistencia 220 Ω | LED de estado |

> **Nota:** el ESP32-S2 **no** era el chip correcto (no tiene Bluetooth). Este proyecto usa
> el ESP32 **clásico** (el que dice "ESP32 WiFi+BT SoC" en el chip).

---

## 3. Cableado completo

> ⚠️ **Todo el módulo NRF24L01, PN532, SD y pantalla a 3.3 V de lógica.**
> La **alimentación de la microSD (HW-125) va a 5 V** (tiene regulador propio).
> El NRF24L01 **se quema con 5 V**.

### Bus I2C (pantalla + PN532, compartido)

| Señal | ESP32 |
|---|---|
| SDA | GPIO **21** |
| SCL | GPIO **22** |

| Módulo | VCC | GND | SDA | SCL | Notas |
|---|---|---|---|---|---|
| Pantalla SH1106 | 3V3 | GND | 21 | 22 | — |
| PN532 (NFC) | 3V3 | GND | 21 | 22 | DIP: **SW1=ON, SW2=OFF** |

> El I2C es un **bus compartido**: pantalla (0x3C) y PN532 (0x24) van en los mismos dos pines.

### Bus SPI (microSD + 2× NRF24, compartido)

| Señal | ESP32 |
|---|---|
| SCK | GPIO **18** |
| MISO | GPIO **19** |
| MOSI | GPIO **23** |

| Módulo | Pin | ESP32 |
|---|---|---|
| **microSD (HW-125)** | CS | GPIO **5** |
| | VCC | **5 V** ⚠️ |
| | GND | GND |
| **NRF24 #1** | CSN | GPIO **16** |
| | CE | GPIO **17** |
| | VCC | 3V3 |
| **NRF24 #2** | CSN | GPIO **27** |
| | CE | GPIO **26** |
| | VCC | 3V3 |

> Los NRF24 usan el mismo bus SPI (SCK/MISO/MOSI) con CS distintos.
> Recomendado: capacitor **10 µF** entre VCC y GND de cada NRF24 (picos de TX).

### Joystick, IR, batería, LED

| Componente | Pin | ESP32 |
|---|---|---|
| Joystick | VRx | GPIO **34** |
| | VRy | GPIO **35** |
| | SW | GPIO **25** |
| | VCC | 3V3 |
| **KY-022** (receptor IR) | S (OUT) | GPIO **4** |
| | + / VCC | 3V3 |
| | – / GND | GND |
| **LED IR** (emisor) | ánodo (largo) → R 100–220 Ω | GPIO **13** |
| | cátodo (corto) | GND |
| **Batería** (divisor 10k/10k) | punto medio | GPIO **33** |
| **LED de estado** | ánodo → R 220 Ω | GPIO **2** |

### Alimentación (HW-373 / TP4056)

```
Batería Li-Ion 3.7 V ──┬── BAT+ / BAT-  (carga)
                       │
                       └── OUT+ (3.7 V) ──▶ [Boost MT3608 3.7→5 V] ──▶ VIN del ESP32
```

- El TP4056 **carga** la batería; su `OUT+` da **3.7 V** (no 5 V).
- El ESP32 (placa dev) espera **5 V en VIN/USB** (tiene su regulador a 3.3 V).
- Para portátil: usa un **boost MT3608** (3.7→5 V) entre `OUT+` y `VIN`.
- Para el prototipo: alimentación por **USB** (lo más simple).

---

## 4. Arquitectura del sistema

```
┌──────────────────────────────────────────┐
│  FLASH del ESP32  ("SO" / firmware)      │
│  ─────────────────────────────────────   │
│  • Menú launcher                          │
│  • Drivers: pantalla, NFC, IR, RF,       │
│    WiFi, BT, SD, BadUSB, joystick, bat.  │
│  • Intérprete de comandos (.txt)          │
│  • Intérprete LUA (Lua 5.4.7)            │
│  • API de hardware (os.*)                │
└───────────────┬──────────────────────────┘
                │ lee y ejecuta
┌───────────────▼──────────────────────────┐
│  microSD  ("programas" y datos)          │
│  /config.txt    → intro elegida          │
│  /intro/*.bin   → animaciones            │
│  /programas/*.lua, *.txt → programas     │
│  /nfc/ /ir/ /badusb/ → datos             │
└──────────────────────────────────────────┘
```

**Por qué así:** el ESP32 **no** puede ejecutar binarios compilados desde la SD (no tiene
cargador dinámico). Pero **sí** puede correr **scripts** (Lua/comandos) que lee de la SD,
usando un intérprete que está compilado en la flash. Eso es lo que hace el "SO".

### Estructura del firmware (PlatformIO)

```
flipper-s2/
├── platformio.ini
├── include/config.h        ← pines y opciones
├── src/
│   ├── main.cpp            ← launcher + intérpretes + acciones
│   ├── display.*           ← pantalla U8g2, auto-test, intros, menú
│   ├── nfc.*               ← PN532: UID, texto NDEF, clonar, emular, diagnóstico
│   ├── radio.*             ← NRF24: escaneo, loopback
│   ├── storage.*           ← microSD: leer/escribir/listar
│   ├── badusb.*            ← teclado BLE (BLE Keyboard)
│   ├── ir.*                ← IR: recibir/reenviar (IRremote)
│   ├── wifi_mod.*          ← WiFi: scan / connect / AP
│   └── bt_mod.*            ← BLE scan
└── DOCUMENTACION.md
```

### `platformio.ini` (resumen)

```ini
[env:esp32dev]
platform = espressif32@6.6.0
board = esp32dev
framework = arduino
board_build.partitions = huge_app.csv   ; app de ~3 MB
lib_deps =
    olikraus/U8g2
    adafruit/Adafruit PN532
    adafruit/Adafruit BusIO
    nrf24/RF24
    t-vk/ESP32 BLE Keyboard
    Arduino-IRremote/IRremote
    inanimate/Esp32Lua                  ; intérprete Lua
```

---

## 5. Estructura de la microSD

```
/config.txt              ← configuración (ej: intro=intro.bin)
/intro/
    intro.bin            ← animación de arranque (frames 128x64, 1 bpp)
/programas/
    nfc.lua  ir.lua  rf.lua  wifi.lua  bt.lua  badusb.lua
    hello.lua  nfc_lua.lua
    (también puedes poner .txt con el intérprete de comandos)
/nfc/                    ← dumps de tarjetas (/nfc/clone_XXXX.txt)
/ir/                     ← códigos IR guardados
/badusb/                 ← scripts de teclado (demo.txt)
```

**Formato de la intro (`/intro/*.bin`):** 1024 bytes por frame (128×64, 1 bit por píxel,
8 páginas × 128 columnas, LSB = fila superior). Se reproduce a ~12 fps.

**`/config.txt`:**
```
intro=intro.bin
```

---

## 6. El menú del OS

```
┌─────────────────────┐
│   FLIPPER-ESP32     │
│─────────────────────│
│ > Configuracion     │  elige la intro de /intro/ (se guarda en /config.txt)
│   Info              │  versión, batería, número de programas
│   Programas         │  lista /programas/ y ejecuta el elegido
│   Dormir            │  bajo consumo (despierta con el joystick)
└─────────────────────┘
```

**Flujo de arranque:**
1. Lee `/config.txt` → obtiene la intro.
2. Reproduce `/intro/<intro>` (se puede **saltar** moviendo/presionando el joystick).
3. Si no hay intro, muestra una **animación chibi** de respaldo.
4. Muestra el **auto-test** (pantalla, NFC, SD, RF, joystick).
5. Muestra el **menú del OS**.

**Controles:** Arriba/Abajo = navegar · Pulsar (SW) = OK · Izquierda = atrás.

---

## 7. Módulos en detalle

### 7.1 Pantalla (SH1106, I2C)
- Driver **U8g2** (`U8G2_SH1106_128X64_NONAME_F_HW_I2C`).
- Funciones: `displayTitle`, `displayLine`, `displayMenu` (con scroll), `displaySelfTest`,
  `displayBootAnimation` (chibi), `displayPlayIntroSD(path)` (intro desde SD).
- La intro se puede **saltar** con el joystick (`displayJoySkip`).

### 7.2 NFC (PN532, I2C)
- Lee el **UID** de tarjetas ISO14443A.
- Lee el **texto NDEF** de **Mifare Classic** (con la llave NDEF `D3F7D3F7D3F7`) y de
  **NTAG/Ultralight** (Type 2). Soporta registros de texto (`T`), URL (`U`) y MIME `text/plain`.
- **Clonar:** genera un dump completo (UID + 16 sectores) y lo guarda en `/nfc/clone_<UID>.txt`.
- **Emular:** modo target del PN532 (tarjeta NDEF genérica; no emula la tarjeta exacta).
- **Diagnóstico:** `/nfc_diag.txt` con sectores/llaves probadas.

### 7.3 Infrarrojo (KY-022 + LED IR)
- **Recibir:** decodifica el código (NEC, etc.) con IRremote.
- **Reenviar:** reproduce el último código recibido.
- Pines: RX=4, TX=13.

### 7.4 Radio 2.4 GHz (2× NRF24L01)
- **Escanear:** recorre los 126 canales y detecta actividad (portadora).
- **Loopback:** el módulo 1 transmite y el 2 recibe (prueba).

### 7.5 WiFi
- **Escanear:** lista redes (SSID + RSSI) y guarda en `/wifi_scan.txt`.
- **Conectar (SD):** lee `/wifi.txt` (línea 1 = SSID, línea 2 = clave) y se conecta.
- **Punto AP:** crea el AP `FLIPPER-ESP32` (IP 192.168.4.1, abierto).
- ⚠️ Sin honeypot ni captura de credenciales.

### 7.6 Bluetooth (BLE)
- **Scan BLE:** lista dispositivos cercanos (nombre + RSSI) y guarda en `/bt_scan.txt`.
- ⚠️ Sin "spam" (ataque de saturación).

### 7.7 microSD (HW-125, SPI)
- Guarda/lee archivos, dumps, logs, intro y programas.
- **VCC a 5 V** (regulador del módulo).

### 7.8 BadUSB (teclado BLE)
- El ESP32 se presenta como **teclado Bluetooth**.
- Ejecuta scripts de `/badusb/*.txt` (`STRING`, `ENTER`, `GUI r`, `DELAY`, etc.).
- Solo en tu propia computadora.

### 7.9 Joystick (KY-023)
- Ejes X/Y (analógico) + botón SW.
- Navegación con **"regreso al centro"** (una pulsación = un movimiento).
- Saltar intro/auto-test.

### 7.10 Batería
- Lee el ADC (GPIO33) con divisor 10k/10k; muestra el % en Info.

### 7.11 Dormir (light sleep)
- Entra en bajo consumo y **despierta con el joystick**.

### 7.12 LED de estado
- LED en GPIO2 (indicador).

---

## 8. Intérprete de comandos (.txt)

Los archivos `.txt` en `/programas/` se ejecutan con un intérprete de comandos simple:

| Comando | Efecto |
|---|---|
| `TITLE <texto>` | Título en pantalla |
| `PRINT <texto>` | Escribe una línea |
| `CLEAR` | Limpia la pantalla |
| `DELAY <ms>` | Espera |
| `WAIT` | Espera OK |
| `NFC_READ` | Lee tarjeta (UID + texto) |
| `NFC_CLONE` | Clona (dump a SD) |
| `IR_RECV` | Recibe IR |
| `IR_REPLAY` | Reenvía IR |
| `WIFI_SCAN` | Escanea WiFi |
| `WIFI_AP` | Punto de acceso |
| `BT_SCAN` | Escanea BLE |
| `RF_SCAN` | Escanea 2.4 GHz |
| `BADUSB <archivo>` | Ejecuta script de teclado |

Ejemplo (`/programas/hola.txt`):
```
TITLE Hola desde la SD
PRINT Bienvenido al OS
DELAY 2000
```

---

## 9. API de Lua

Los archivos `.lua` en `/programas/` se ejecutan con **Lua 5.4.7** (embebido en la flash).
La API se expone en el módulo global **`os`**, y `print()` escribe directo en la pantalla.

### Pantalla y tiempo
| Función | Descripción |
|---|---|
| `print("texto")` | Imprime en la pantalla (una línea) |
| `os.title("Titulo")` | Título + limpia |
| `os.clear()` | Limpia la pantalla |
| `os.delay(ms)` | Espera N milisegundos |
| `os.wait()` | Espera a pulsar OK |
| `os.joy_wait()` | Espera un botón y devuelve 1=arriba, 2=abajo, 3=OK, 4=atrás |

### Hardware
| Función | Descripción |
|---|---|
| `os.nfc_read()` | Escanea y devuelve el **UID** (o `nil`) |
| `os.nfc_reader()` | Flujo completo: UID + texto NDEF |
| `os.nfc_clone()` | Clona (dump a `/nfc/`) |
| `os.nfc_emulate()` | Emulación genérica |
| `os.ir_recv()` | Recibe IR |
| `os.ir_replay()` | Reenvía el último IR |
| `os.rf_scan()` | Escanea 2.4 GHz |
| `os.rf_loopback()` | Prueba loopback NRF24 |
| `os.wifi_scan()` | Devuelve la lista de redes (string) |
| `os.wifi_connect()` | Conecta con `/wifi.txt` de la SD |
| `os.wifi_ap()` | Crea el AP |
| `os.bt_scan()` | Devuelve la lista BLE (string) |
| `os.badusb("ruta")` | Ejecuta un script de teclado |

### Ejemplo (`/programas/nfc.lua`)
```lua
local function menu(title, opts)
  local sel = 1
  while true do
    os.title(title)
    for i=1,#opts do print((i==sel and "> " or "  ") .. opts[i]) end
    local b = os.joy_wait()
    if b == 1 then sel = sel-1; if sel<1 then sel=#opts end
    elseif b == 2 then sel = sel+1; if sel>#opts then sel=1 end
    elseif b == 3 then return sel
    elseif b == 4 then return nil end
  end
end
while true do
  local s = menu("NFC", {"Leer tarjeta","Clonar","Emular","Salir"})
  if not s or s == 4 then break end
  if s == 1 then os.nfc_reader()
  elseif s == 2 then os.nfc_clone()
  elseif s == 3 then os.nfc_emulate() end
end
```

---

## 10. Compilar y flashear

### Requisitos
- **VS Code** + extensión **PlatformIO**.
- Driver USB-serial (CH340).

### Pasos
1. Abre la carpeta `flipper-s2/` en PlatformIO.
2. Revisa `include/config.h` (pines, modelo de pantalla, etc.).
3. Conecta el ESP32 por USB (aparece como `/dev/ttyUSB0` en Linux).
4. **Compilar:** `pio run`
5. **Flashear:** `pio run -t upload`

### ⚠️ Flashear con BOOT (en ESTA placa)
El circuito de **auto-reset** de esta placa está dañado: esptool no entra solo en modo
grabación. Por eso:

1. **Mantén presionado BOOT** (GPIO0 a GND).
2. Ejecuta el upload (`pio run -t upload`).
3. Suelta BOOT y presiona **EN** (reset) al terminar.

> En una placa sana, este paso es automático (no haría falta BOOT).
> **Para USAR el dispositivo nunca hace falta BOOT** — solo para re-grabar el SO.
> Para cambiar un **programa**, basta con editar la SD y reiniciar.

### Tamaño
Con `huge_app.csv` (3 MB de app) el firmware ocupa ~1.76 MB (≈56 %), dejando margen.

---

## 11. Problemas encontrados y soluciones

| Problema | Causa | Solución |
|---|---|---|
| El chip no era ESP32-S2 | El módulo dice "WiFi+BT" (el S2 no tiene BT) | Usar ESP32 **clásico**; ajustar pines y `platformio.ini` |
| El PN532 "no aparece" en I2C | Venía en modo **HSU/UART** por defecto | DIP switch a **I2C**: SW1=ON, SW2=OFF |
| La pantalla se apagaba al conectar el NFC | El NFC en modo UART "pisaba" el bus I2C | Poner el NFC en I2C (los switches) |
| La microSD no se detectaba | El HW-125 necesita **5 V** (regulador) | VCC del HW-125 a **5 V** |
| Falla al flashear (`Invalid head of packet` / `No serial data`) | Circuito de **auto-reset** dañado | **Mantener BOOT** durante el flasheo |
| El NFC leía el UID pero no el texto | Era **Mifare Classic** (sector con llave) | Autenticar con la llave correcta |
| `NINGUNA LLAVE AUTENTICO` | La tarjeta usa una llave no común | Sacar la llave con **MIFARE Classic Tool** (dump `.mct`) |
| Texto "no legible" aunque la llave servía | El registro era **MIME `text/plain`** (no `T`) | Añadir el parseo MIME |
| El joystick "iba muy rápido" | Repetía al mantenerlo | "Regresar al centro" antes de cada movimiento |
| La intro no cabía en la flash | Los frames ocupan mucho | Leer la intro desde la **SD** (`/intro/*.bin`) |
| No se puede ejecutar un `.bin` de la SD | El ESP32 no tiene cargador dinámico | Usar **scripts** (Lua/comandos) desde la SD |
| `luaopen_os` daba error de compilación | Choca con la librería `os` estándar de Lua | Renombrar el loader a `luaopen_flipper` |
| El escaneo WiFi se colgaba o decía "Sin redes" | El escaneo async se adelantaba (0) + el BLE ocupaba la antena | Escaneo **síncrono** + liberar el BLE antes (`BLEDevice::deinit`) |

---

## 12. Historial del desarrollo

1. **Identificación de componentes** (¡se creía ESP32-S2, era ESP32 clásico!).
2. **Pantalla SH1106**: prueba de dibujo (I2C, dirección 0x3C).
3. **PN532**: detección I2C + lectura de UID (tras configurar los DIP switches).
4. **microSD**: escritura/lectura de prueba (tras poner VCC a 5 V).
5. **NRF24**: escaneo y loopback (con capacitores de desacople).
6. **Demo "Bad Apple"**: video 128×64 transmitido por serial + audio en la PC.
7. **Integración**: menú con NFC, IR, RF, BadUSB, SD.
8. **Joystick**: navegación con "regreso al centro".
9. **IR**: receptor KY-022 + LED emisor.
10. **Intro de arranque**: primero chibi, luego videos (Nichijou, Zero Two) desde la SD.
11. **Auto-test** al arrancar.
12. **NFC — texto**: depuración hasta leer NDEF de Mifare Classic (llave `D3F7…`, registro MIME).
13. **WiFi + BLE scan + Sleep + Batería**.
14. **Arquitectura OS**: launcher (Configuración/Info/Programas/Dormir) + intérprete de comandos.
15. **Intérprete Lua** (Lua 5.4.7) + API `os.*` + programas `.lua` en la SD.

---

## 13. Uso responsable y legal

Este proyecto es **educativo**. Varias funciones son de **doble uso** y tienen límites legales:

- **NFC/RFID:** clona/emula **solo tus propias tarjetas**. Clonar tarjetas de acceso o
  transporte ajenas es **ilegal**.
- **Infrarrojo:** reenvía **solo** códigos de tus propios aparatos.
- **WiFi:** escanear/conectar/crear AP **solo en tus redes** o con autorización. **No** se
  incluye captura de credenciales (honeypot) ni ataque a terceros.
- **Bluetooth:** el **spam BLE** (saturación) **no** se implementó: es un ataque DoS.
- **BadUSB:** úsalo **solo** en tu propia computadora.
- Cumple las leyes locales y las normas de tu institución.

---

## 14. Créditos y referencias

- **U8g2** (pantallas OLED): https://github.com/olikraus/u8g2
- **Adafruit PN532**: https://github.com/adafruit/Adafruit-PN532
- **RF24** (NRF24L01): https://github.com/nRF24/RF24
- **IRremote** (Armin Joachimsmeyer): https://github.com/Arduino-IRremote/IRremote
- **ESP32 BLE Keyboard** (T-vK): https://github.com/T-vK/ESP32-BLE-Keyboard
- **Esp32Lua** (Lua 5.4.7): https://github.com/inanimate-tech/Esp32Lua
- **Lua**: https://www.lua.org/
- **MIFARE Classic Tool** (para llaves/dumps): app Android.
- Proyectos de referencia estilo Flipper: Flipper Zero, ESP32 Marauder.

---

_Fin de la documentación._
