[![C](https://img.shields.io/badge/C-blue)](https://en.wikipedia.org/wiki/C_(programming_language))

# TETORIS: The Game

This Game is a Tetris-like game i made in C for a school project. it based on the TETORIS song by 柊マグネタイト. i love it so i made my game based on this. Enjoy !

[Click Here](https://www.youtube.com/watch?v=3LIri5EZoY4) to see a video of the game. (video is in french, but it's just globally saying the controls)

# Build it (Linux or [MSYS2 on Windows](https://objects.githubusercontent.com/github-production-release-asset-2e65be/80988227/a3239d91-fc08-46f0-805e-db6d76379532?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=releaseassetproduction%2F20250512%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20250512T135721Z&X-Amz-Expires=300&X-Amz-Signature=8359191500695e980212b07cc949cfa66b1c1e2dd43a6bb678855a35176a6a69&X-Amz-SignedHeaders=host&response-content-disposition=attachment%3B%20filename%3Dmsys2-x86_64-20250221.exe&response-content-type=application%2Foctet-stream))

To build it yourself just run ``` ./compil.sh ``` it will autocompile itself into 'tetoris' (On linux) and then just do ``` ./tetoris ```.

NOTE: For Windows, i suggest you just launch the pre-existing executable as it's a mess to build.

# Run

## Windows 
just uncompress and launch TETORIS.exe and the game will launch.

## Linux

1. Uncompress

2. go in a terminal and type 
```sh
sudo apt install gcc libsdl2-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-image-dev
```

3. Launch using terminal ```./TETORIS``` or just click on the file if you can.

# Controls

UP: Turn Blocks

LEFT: Go left

RIGHT: Go right

Down: blocks fall faster

M: disable music =(

ESC: Menu


COMMANDS INSIDE MENU :

R: Reset Game

Q: Quit

# Known bugs
Sometimes animation can be laggy (as i'm starting into c animations) so if blocks is not orientated as it should be just close the game and restart the game.

# FAQ

Q: Will it be updated ?

A: Yes, but i don't really see what can i add, if you have ideas i'm good to go for it.

Q: Can i edit/fork/republish this code ?

A: As it's MIT License, Yes you can. but it would be great if you credit me. so just add in your README or somewhere : 'Copyright (c) 2025 HekZer036' or 'Original Author : HekZer036'

# Credits

Developer : Me

Music : [Sudo Goji VGM - TETORIS 8-Bit Chiptuned](https://www.youtube.com/watch?v=t5DS7ha_evs)

Original idea : [柊マグネタイト - SV TETORIS Project](https://www.youtube.com/watch?v=Soy4jGPHr3g)
