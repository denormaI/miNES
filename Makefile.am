AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = mines
mines_SOURCES = \
    src/main.cpp \
    src/gl_shader.cpp \
    src/display_window.cpp \
    src/mines_sys.cpp \
    src/nes_cpu.cpp \
    src/nes_ppu.cpp \
    src/_nes_apu.cpp \
    src/nes_cart.cpp \
    src/nes_controllers.cpp \
    src/ines_mapper.cpp \
    src/ines_mapper0.cpp \
    src/ines_mapper1.cpp \
    src/ines_mapper2.cpp \
    src/ines_mapper3.cpp \
    src/ines_mapper7.cpp \
    src/Nes_Snd_Emu-0.1.7/Sound_Queue.cpp \
    src/Nes_Snd_Emu-0.1.7/Wave_Writer.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/apu_snapshot.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Blip_Buffer.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Multi_Buffer.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Nes_Apu.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Nes_Namco.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Nes_Oscs.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Nes_Vrc6.cpp \
    src/Nes_Snd_Emu-0.1.7/nes_apu/Nonlinear_Buffer.cpp

mines_CXXFLAGS = -Wall -std=c++14 -Isrc/Nes_Snd_Emu-0.1.7

if COMPILER_IS_CLANG
mines_CXXFLAGS += -Wc++11-narrowing
endif

if OSX
mines_CXXFLAGS += -F/Library/Frameworks -DGL_SILENCE_DEPRECATION
mines_LDFLAGS = -framework OpenGL -framework SDL2
endif

LIBS =`pkg-config --libs glew sdl2`
$(info pkg-config gives [${LIBS}])

if LINUX
mines_CXXFLAGS += `pkg-config --cflags glew sdl2`
mines_LDFLAGS = `pkg-config --libs glew sdl2`
endif

