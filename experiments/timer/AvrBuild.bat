@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\timer\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\timer\timer.hex" -d "C:\Bbc\EconetBridge\timer\timer.obj" -e "C:\Bbc\EconetBridge\timer\timer.eep" -m "C:\Bbc\EconetBridge\timer\timer.map" "C:\Bbc\EconetBridge\timer\timer.asm"
