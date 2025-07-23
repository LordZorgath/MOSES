MOSES is an experimental multi-system emulator, focused mainly, but not exclusively, on cycle-accurate emulation of vintage computers.

Compile instructions:

```
git clone https://github.com/LordZorgath/MOSES
mkdir build && cd build
cmake --build ./
```
Run instructions:

```
./MOSES --core <core> -f </path/to/game/>
```
Optional commands: `-sc <integer scaling factor> --vol <volume as a %>`

Currently, the only two cores are `chip8` and `xochip`. `xochip-fast` runs the core at 200,000 instructions per frame instead of 1,000, this is needed for some games.
