# FLIPPER-ESP32

Dispositivo multiuso estilo **Flipper Zero** hecho con un **ESP32 clásico**, con
arquitectura de **"OS en la flash + programas en la microSD"**.

> 📘 **Documentación completa:** ver [`DOCUMENTACION.md`](DOCUMENTACION.md)
> (hardware, cableado, arquitectura, API, troubleshooting, historial).

---

## Qué incluye

- 🪪 **NFC/RFID** (PN532): leer UID, leer texto NDEF, clonar (dump) y emular.
- 📡 **Infrarrojo**: recibir y reenviar códigos.
- 📶 **Radio 2.4 GHz** (2× NRF24L01): escanear y loopback.
- 📶 **WiFi**: escanear / conectar (desde SD) / punto de acceso.
- 🔵 **Bluetooth BLE**: escanear dispositivos.
- 💾 **microSD**: intro, programas, dumps y logs.
- ⌨️ **BadUSB** (teclado BLE).
- 🖥️ **Pantalla SH1106** + **joystick** (menú).
- 🔋 **Batería** y **modo dormir**.
- 🎬 **Intro** configurable desde la SD.
- 🐍 **Intérprete Lua** + API `os.*` para programas en la SD.

## Arquitectura

```
FLASH (SO):  menú + drivers + intérprete Lua/comandos
SD:          /config.txt  /intro/*.bin  /programas/*.lua  /nfc/  /ir/  /badusb/
```

Cambias un programa en la SD, reinicias y corre **sin reflashear**.

## Compilar / Flashear

```bash
pio run            # compilar
pio run -t upload  # flashear
```

> ⚠️ En esta placa, **mantén BOOT presionado durante el flasheo** (el auto-reset está
> dañado). Para **usar** el dispositivo no hace falta BOOT.

## Estructura del proyecto

```
flipper-s2/
├── platformio.ini
├── include/config.h
├── src/
│   ├── main.cpp        (launcher + intérpretes + acciones)
│   ├── display.*       (U8g2, intro, auto-test, menú)
│   ├── nfc.*           (PN532: UID, NDEF, clonar, emular)
│   ├── radio.*         (NRF24)
│   ├── storage.*       (microSD)
│   ├── badusb.*        (teclado BLE)
│   ├── ir.*            (infrarrojo)
│   ├── wifi_mod.*      (WiFi)
│   └── bt_mod.*        (BLE scan)
└── DOCUMENTACION.md
```

## ⚠️ Uso responsable

Solo en **tus propios** dispositivos, tarjetas y redes (o con autorización escrita).
No se incluye captura de credenciales WiFi ni spam BLE. Cumple las leyes locales.
