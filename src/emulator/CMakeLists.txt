add_library(smolboy ../common/Log.cpp device/VideoDevice.cpp core/ppu/PPU.cpp core/Gameboy.cpp core/cpu/CPU.cpp
core/cpu/Instructions.cpp core/Memory.cpp core/Cartridge.cpp core/Timer.cpp core/Mapper.cpp core/Scheduler.cpp core/apu/APU.cpp
core/apu/PulseChannel.cpp core/apu/WaveChannel.cpp core/apu/NoiseChannel.cpp)