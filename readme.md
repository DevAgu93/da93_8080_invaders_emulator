8080 emulator for Taito space invaders 1978

The folder rbin contains a release build. Use build.bat to create a debug build (uncomment "goto release" for a release build).

The emulator expects a space invaders rom called "invaders.rom". If you find the rom split in multiple files (.h, .g, .f, .e) place them alongside the exe and execute the merge_roms.bat to create a .rom file.

If you open the emulator normally, it will execute the normal game. If you use "tests" as argument, it will run the CPU tests instead.

CONTROLS:

1 - P1 start
6 - Insert coin
A - Move left
D - Move right
,(comma) - Shoot
