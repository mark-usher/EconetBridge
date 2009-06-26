@ECHO OFF
"C:\Program Files\Atmel\AVR Tools\AvrAssembler2\avrasm2.exe" -S "C:\Bbc\EconetBridge\MemoryTest\labels.tmp" -fI -W+ie -o "C:\Bbc\EconetBridge\MemoryTest\MemoryTest.hex" -d "C:\Bbc\EconetBridge\MemoryTest\MemoryTest.obj" -e "C:\Bbc\EconetBridge\MemoryTest\MemoryTest.eep" -m "C:\Bbc\EconetBridge\MemoryTest\MemoryTest.map" "C:\Bbc\EconetBridge\MemoryTest\main.asm"
