**miNES** is an approximately cycle-accurate NES emulator.

## Build on Windows (Visual Studio 2015)
1. Open `mines.sln`.
2. Copy `glew32.dll` from `lib\glew-2.1.0\bin\Release\Win32` and `SDL2.dll` from `lib\SDL2-2.0.9\lib\x86` to the binary's location (vc_mswu).

## Build on Linux
1. Install dependencies.
```sudo apt-get install autoconf automake autotools-dev libglew-dev libsdl2-dev```
2. ```autoreconf --install && ./configure && make```

## Run
If audio is choppy on Linux run `export SDL_AUDIODRIVER=alsa` before miNES as a workaround.
```
	usage: mines <rom path> <optional arguments>
		optional: --stretch	don't preserve video aspect ratio
```

## Supported Mappers

Extremely WIP.

* NROM (0)
* MMC1 (1) **Incomplete** but works for some games, e.g. The Legend of Zelda.
* UxROM (2)
* CNROM (3)
* AOROM (7)

## Images
![Super Mario Bros](https://i.imgur.com/t1c1wHy.jpg)
![Mega Man](https://i.imgur.com/4VtUXjI.jpg)
![The Legend of Zelda](https://i.imgur.com/rTTSR5T.jpg)
