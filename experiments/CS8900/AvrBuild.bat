@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\CS8900\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\CS8900\CS8900.hex" -d "C:\Bbc\EconetBridge\CS8900\CS8900.obj" -e "C:\Bbc\EconetBridge\CS8900\CS8900.eep" -m "C:\Bbc\EconetBridge\CS8900\CS8900.map" "C:\Bbc\EconetBridge\CS8900\bridge.asm"
