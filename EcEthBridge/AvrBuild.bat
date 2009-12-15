@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\EcEthBridge\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\EcEthBridge\EcEthBridge.hex" -d "C:\Bbc\EconetBridge\EcEthBridge\EcEthBridge.obj" -e "C:\Bbc\EconetBridge\EcEthBridge\EcEthBridge.eep" -m "C:\Bbc\EconetBridge\EcEthBridge\EcEthBridge.map" "C:\Bbc\EconetBridge\EcEthBridge\bridge.asm"
