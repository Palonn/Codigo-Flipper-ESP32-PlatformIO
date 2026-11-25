#pragma once
#include <Arduino.h>

// Inicializa la microSD. Devuelve true si la detecta.
bool storageInit();

// Agrega una línea de texto al final de un archivo.
bool storageAppend(const char* path, const String& line);

// Lee el contenido completo de un archivo como texto.
String storageRead(const char* path);

// Lista los nombres de archivos/carpetas de un directorio.
String storageList(const char* dir);

// Escribe (sobrescribe) un archivo con el contenido dado.
bool storageWrite(const char* path, const String& content);
