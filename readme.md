8080 emulator for Taito space invaders 1978

The folder rbin contains a release build. Use build.bat to create a debug build (uncomment "goto release" for a release build).

The emulator expects a space invaders rom called "invaders.rom". If you find the rom split in multiple files (.h, .g, .f, .e) place them alongside the exe and execute the merge_roms.bat to create a .rom file.

If you open the emulator normally, it will execute the rom. If you use "tests" as argument, it will run the CPU tests instead.

invaders.sound is a file containing the .wav sound files from http://www.brentradio.com/SpaceInvaders.htm

CONTROLS:

1 - P1 start

6 - Insert coin

A - Move left

D - Move right

,(comma) - Shoot

Resources used :

CPU TESTS:
https://altairclone.com/downloads/cpu_tests/

Compared results for tests:
https://www.reddit.com/r/C_Programming/comments/8loz4p/heres_a_cycleaccurate_intel_8080_emulator_i_wrote/

Audio files:
http://www.brentradio.com/SpaceInvaders.htm

8080 Programmers manual (download link)
https://altairclone.com/downloads/manuals/8080%20Programmers%20Manual.pdf

8080 - 8085 programmers manual (download link)
https://altairclone.com/downloads/manuals/8080-8085%20Programmers%20Manual.pdf

Emulator 101, followed to a certain point.
http://www.emulator101.com/

Space invaders Hardware, code, RAM use:
https://computerarcheology.com/Arcade/SpaceInvaders/

TODO: Add information
