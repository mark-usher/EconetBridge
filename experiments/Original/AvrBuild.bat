@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\Original\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\Original\Original.hex" -d "C:\Bbc\EconetBridge\Original\Original.obj" -e "C:\Bbc\EconetBridge\Original\Original.eep" -m "C:\Bbc\EconetBridge\Original\Original.map" "C:\Bbc\EconetBridge\Original\bridge.asm"
