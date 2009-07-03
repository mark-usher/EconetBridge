@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\cTest\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\cTest\cTest.hex" -d "C:\Bbc\EconetBridge\cTest\cTest.obj" -e "C:\Bbc\EconetBridge\cTest\cTest.eep" -m "C:\Bbc\EconetBridge\cTest\cTest.map" "C:\Bbc\EconetBridge\cTest\cTest.asm"
