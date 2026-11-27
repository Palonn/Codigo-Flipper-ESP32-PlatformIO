# Cómo está programado el Flipper-ESP32 (explicado fácil)

Este documento explica, con palabras sencillas, cómo funciona el código de cada
parte del proyecto. No hace falta saber palabras complicadas: aquí se explica todo
como si se lo contaras a un compañero.

---

## 1. La idea general

El programa está dividido en **archivos** (también llamados "módulos"). Cada archivo
se encarga de **una sola cosa**:

- Un archivo maneja la **pantalla**.
- Otro maneja las **tarjetas NFC**.
- Otro la **tarjeta de memoria**.
- Y así con cada parte.

Así es más fácil de entender y de arreglar: si algo falla en las tarjetas, sabes que
está en el archivo de las tarjetas.

El archivo **`main.cpp`** es el que **manda**: usa a los demás archivos como ayudantes.

---

## 2. Cómo arranca y cómo trabaja (el ciclo)

Todo programa de este tipo tiene **dos partes**:

1. **La preparación inicial (se hace una sola vez al encender).**
   Se configura la pantalla, se encienden los módulos, se lee la configuración de la
   tarjeta de memoria, se muestra la introducción y la prueba automática.

2. **El bucle principal (se repite para siempre).**
   Se muestra el menú, se revisa qué eligió la persona, y se hace esa acción. Cuando
   termina, vuelve a mostrar el menú. Una y otra vez.

En el código, la preparación inicial se llama **`setup()`** y el bucle principal se
llama **`loop()`**. Esos dos nombres son obligatorios en este tipo de placas.

---

## 3. El archivo principal: `main.cpp`

Es el cerebro. Hace estas cosas:

### a) Leer el joystick
La función **`waitButton()`** revisa el joystick y **espera** a que la persona lo mueva
o lo presione. Cuando lo hace, devuelve un número:

- `1` = arriba
- `2` = abajo
- `3` = presionó (aceptar)
- `4` = izquierda (volver)

Además, espera a que el joystick **regrese al centro** antes de contar otra vez, para
que no se mueva de golpe varias opciones.

### b) Mostrar menús
La función **`runMenu()`** dibuja una lista de opciones con una flecha `>` y espera a
que elijas una. Devuelve el número de la opción elegida.

La función **`showText()`** muestra un texto largo en la pantalla, repartiéndolo en
varias líneas.

### c) Leer y guardar la configuración
- **`cfgRead("intro")`** lee del archivo `/config.txt` qué introducción usar.
- **`cfgWrite("intro", "algo")`** guarda una elección en ese mismo archivo.

### d) Medir la batería
**`readBatteryPct()`** mide el voltaje de la batería y lo convierte en un porcentaje
(de 0 a 100).

### e) Dormir
**`doSleep()`** apaga la pantalla y pone el aparato en modo de bajo consumo. Despierta
cuando presionas el joystick.

### f) Las acciones (lo que hace cada opción del menú)
Cada opción llama a una función que hace el trabajo:

- **`nfcRead()`** → lee una tarjeta NFC.
- **`nfcCloneAction()`** → copia una tarjeta.
- **`nfcEmulate()`** → se hace pasar por una tarjeta.
- **`irReceiveAction()`** / **`irReplayAction()`** → recibe o repite infrarrojo.
- **`rfScan()`** / **`rfLoopback()`** → radio de 2.4 GHz.
- **`wifiScanAction()`** / **`wifiConnectAction()`** / **`wifiAPAction()`** → WiFi.
- **`btScanAction()`** → busca aparatos Bluetooth.
- **`usbDemo()`** → prueba el teclado Bluetooth.

### g) Los programas de la tarjeta de memoria
- **`runProgram(ruta)`** lee un archivo de la tarjeta y lo ejecuta. Si el archivo
  termina en `.lua`, lo entrega al intérprete de Lua; si no, lo lee como una lista de
  órdenes simples.
- **`programasMenu()`** muestra la lista de programas que hay en `/programas/`.
- **`configuracionMenu()`** muestra las introducciones que hay en `/intro/`.
- **`infoAction()`** muestra la información del aparato.

### h) El arranque y el bucle
- **`setup()`** prepara todo y muestra la introducción y la prueba automática.
- **`loop()`** muestra el menú principal (Configuración / Info / Programas / Dormir) y
  hace lo que elijas.

---

## 4. El archivo de la pantalla: `display.cpp`

Maneja la pantalla. Usa una librería llamada **U8g2**, que sabe dibujar en este tipo de
pantallas.

- **`displayInit()`** enciende la pantalla.
- **`displayClear()`** borra todo (deja la pantalla en negro).
- **`displayTitle(texto)`** escribe un título arriba y una raya debajo.
- **`displayLine(número, texto)`** escribe una línea de texto. El número dice en qué
  renglón va (del 0 al 4).
- **`displayMenu(título, opciones, cantidad, elegida)`** dibuja el menú y marca con una
  flecha la opción elegida.
- **`displaySelfTest(...)`** dibuja la prueba automática del arranque (revisa la
  pantalla, NFC, tarjeta de memoria, radio y joystick).
- **`displayBootAnimation()`** dibuja una carita animada (la introducción de respaldo).
- **`displayPlayIntroSD(ruta)`** reproduce una introducción guardada en la tarjeta de
  memoria. Lee la imagen guardada (un trozo de 1024 valores por cada cuadro) y la
  dibuja uno tras otro, como una película.
- **`displayJoySkip()`** dice si la persona movió el joystick, para poder **saltar** la
  introducción.

---

## 5. El archivo de las tarjetas NFC: `nfc.cpp`

Maneja el lector de tarjetas. Usa una librería llamada **Adafruit PN532**.

- **`nfcInit()`** enciende el lector y comprueba que responde.
- **`nfcReadCard(tarjeta)`** detecta una tarjeta cercana y guarda su **número de
  identificación** (el UID, que es como su "número de serie").
- **`nfcUidToString(tarjeta)`** convierte ese número a texto para mostrarlo.
- **`nfcReadNdefText(tarjeta, texto)`** lee el **texto** que trae la tarjeta. Para eso
  prueba abrir la zona donde se guarda el texto usando distintas llaves.
- **`nfcCloneDump(tarjeta)`** copia **todos los datos** de la tarjeta a un texto, para
  guardarlo en la tarjeta de memoria.
- **`nfcEmulateStart()`** hace que el lector se comporte como si fuera una tarjeta.
- **`nfcDiagnose(tarjeta)`** y **`nfcDiagLog(tarjeta)`** prueban llaves y apuntan los
  resultados, para averiguar por qué una tarjeta no se deja leer.

Cosas importantes que hace por dentro:

- **`readType2(texto)`** lee las tarjetas del tipo "NTAG".
- **`readMifareClassic(texto)`** lee las tarjetas del tipo "Mifare Classic": prueba
  distintas llaves hasta encontrar la que abre la zona con el texto.
- **`parseNdefTlv(...)`** ordena los datos leídos y saca el texto que hay dentro.

---

## 6. El archivo de la radio de 2.4 GHz: `radio.cpp`

Maneja los dos módulos de radio. Usa una librería llamada **RF24**.

- **`radioInit()`** enciende los dos radios y comprueba que responden.
- **`radioScanChannels(lista)`** revisa uno por uno los 126 canales de la banda de
  2.4 GHz y anota en cuáles hay señal.
- **`radioLoopbackTest()`** hace una prueba: un módulo **envía** un mensaje y el otro
  **lo recibe**. Si llega bien, la prueba pasa.

---

## 7. El archivo de la tarjeta de memoria: `storage.cpp`

Maneja la tarjeta microSD. Usa una librería llamada **SD**.

- **`storageInit()`** enciende la tarjeta de memoria.
- **`storageAppend(ruta, línea)`** agrega una línea al final de un archivo (no borra lo
  que ya había).
- **`storageRead(ruta)`** lee todo el contenido de un archivo y lo devuelve como texto.
- **`storageList(carpeta)`** hace una lista con los nombres de los archivos que hay en
  una carpeta.
- **`storageWrite(ruta, contenido)`** escribe un archivo desde cero (reemplaza lo que
  hubiera).

---

## 8. El archivo del teclado Bluetooth: `badusb.cpp`

Hace que el aparato se comporte como un **teclado** conectado por Bluetooth. Usa una
librería de teclado Bluetooth.

- **`badusbInit()`** enciende el teclado y lo deja listo para emparejarse.
- **`badusbType(texto)`** escribe un texto (como si tecleara).
- **`badusbPressRelease(tecla)`** presiona y suelta una tecla especial (Enter, Esc…).
- **`badusbCombo(modificador, tecla)`** hace una combinación (por ejemplo, Ctrl + C).
- **`badusbRunScript(ruta)`** lee un archivo de la tarjeta de memoria y ejecuta sus
  órdenes una por una (escribir, esperar, presionar teclas…).

---

## 9. El archivo del infrarrojo: `ir.cpp`

Maneja el receptor y el emisor de infrarrojo. Usa una librería llamada **IRremote**.

- **`irInit()`** enciende el receptor y el emisor.
- **`irReceive()`** lee un código que llegue de un control remoto y lo devuelve como
  texto.
- **`irReplay()`** vuelve a enviar el último código que se recibió.

---

## 10. El archivo del WiFi: `wifi_mod.cpp`

Maneja la conexión inalámbrica. Usa la librería **WiFi**.

- **`wifiScan()`** busca las redes que hay cerca y devuelve una lista con sus nombres y
  su intensidad.
- **`wifiConnect(nombre, clave)`** se conecta a una red usando el nombre y la clave.
- **`wifiAP(nombre, clave)`** crea una red propia (el aparato se vuelve un punto de
  acceso).
- **`wifiOff()`** apaga el WiFi.
- **`wifiIP()`** devuelve la dirección que le tocó al conectarse.

Un detalle: antes de buscar redes, **apaga el Bluetooth** un momento, porque el WiFi y
el Bluetooth usan la misma antena y se estorban.

---

## 11. El archivo del Bluetooth: `bt_mod.cpp`

- **`btScan()`** busca aparatos Bluetooth que estén cerca y devuelve una lista con sus
  nombres y su intensidad de señal.

---

## 12. Cómo funcionan los "programas" de la tarjeta de memoria

Los programas **no son archivos compilados**: son **archivos de texto** que el aparato
lee y ejecuta. Hay dos formas:

### a) Programas de órdenes simples (`.txt`)
Cada línea es una orden. Por ejemplo:

```
TITLE Hola
PRINT Bienvenido
DELAY 2000
```

El aparato lee cada línea y hace lo que dice: poner un título, escribir un texto,
esperar dos segundos…

### b) Programas en Lua (`.lua`)
Lua es un lenguaje de programación sencillo. El aparato tiene un **intérprete de Lua**
dentro (en su memoria interna), que lee el archivo y lo ejecuta. Los programas pueden
usar funciones como `print(...)` para escribir o `os.nfc_read()` para leer una tarjeta.

### ¿Cómo se cambia un programa?
1. Sacas la tarjeta de memoria.
2. Editas o reemplazas el archivo en la carpeta `/programas/`.
3. La vuelves a poner y reinicias.
4. El aparato ejecuta el programa nuevo. **No hay que volver a grabar el aparato.**

---

## 13. Palabras que conviene saber

- **Función:** un bloque de código con un nombre que hace una tarea. Se "llama" por su
  nombre cuando se necesita.
- **Variable:** una cajita donde se guarda un valor (un número, un texto…).
- **Bucle:** repetir algo muchas veces.
- **Condición (si / si no):** decidir entre dos caminos según algo.
- **Texto:** una cadena de letras y números.
- **Lista:** varios valores guardados juntos, en orden.
- **Pin (patita):** cada conexión del chip donde se enchufa un cable.
- **Bus:** un camino compartido por donde viajan los datos entre varios módulos.
- **Memoria temporal:** un espacio pequeño donde se guardan datos mientras se usan.
- **Intérprete:** un programa que lee otro programa escrito en un lenguaje y lo ejecuta.

---

_Fin de la explicación._
