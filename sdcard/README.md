# Contenido de la microSD

Copia esta carpeta a la raíz de una microSD **FAT32**.

```
/config.txt          -> intro=intro.bin
/intro/intro.bin     -> animación de arranque (ponla tú; 128x64, 1 bpp, 12 fps)
/programas/*.lua     -> programas Lua
/programas/*.txt     -> programas del intérprete de comandos
/badusb/demo.txt     -> script de teclado
/nfc/  /ir/          -> dumps y códigos guardados
```

La intro (`intro.bin`) no se incluye (es un video). Para cambiar de programa:
edita/reemplaza el `.lua` y reinicia.
