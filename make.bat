SET PATH=D:\devkitadv\bin

gcc  -o gbaRaycaster_ver1.elf gbaRaycaster_ver1.c -lm

rem gcc -c -mthumb -mthumb-interwork gbaRaycaster_ver1.c
rem gcc -mthumb -mthumb-interwork -o gbaRaycaster_ver1.elf gbaRaycaster_ver1.o

objcopy -O binary gbaRaycaster_ver1.elf raycaster-gba_ver1.gba

gbafix raycaster-gba_ver1.gba -p -traycaster-gba_ver1 -c7777 -mJW -v1

pause


