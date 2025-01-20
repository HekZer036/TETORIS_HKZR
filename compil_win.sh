pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc 
pacman -S mingw-w64-ucrt-x86_64-SDL2 
pacman -S mingw-w64-ucrt-x86_64-SDL2_image 
pacman -S mingw-w64-ucrt-x86_64-SDL2_mixer 
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
gcc -o Tetoris.exe Prod.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_mixer -lSDL2_image