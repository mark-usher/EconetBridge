@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\SerialA\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\SerialA\SerialA.hex" -d "C:\Bbc\EconetBridge\SerialA\SerialA.obj" -e "C:\Bbc\EconetBridge\SerialA\SerialA.eep" -m "C:\Bbc\EconetBridge\SerialA\SerialA.map" -l "C:\Bbc\EconetBridge\SerialA\SerialA.lst" "C:\Bbc\EconetBridge\SerialA\SerialA.asm"
