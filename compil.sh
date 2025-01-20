# /bin/bash

apt install gcc libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev libsdl2-image-dev
gcc -o Tetoris Prod.c -lSDL2 -lSDL2_ttf -lSDL2_mixer -lSDL2_image
./Tetoris