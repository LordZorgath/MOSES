# MOSES 

MOSES is an experimental multi-system emulator, focused mainly, but not exclusively, on cycle-accurate emulation of vintage computers.

# Compile instructions:

```
git clone https://github.com/LordZorgath/MOSES
mkdir build && cd build
cmake ../
cmake --build ./
```
# Run instructions:

```
./MOSES --core <core> -f </path/to/game/>
```
Optional commands: `-sc <integer scaling factor> --vol <volume as a %>`

Supported cores:
`chip8`
`xochip`
`xochip-fast` - same as `xochip`, but runs the core much faster, some games require this.
`schip` - Four variants supported: `-variant=SCHIP1`, `-variant=SCHIP11`, `-variant=SCHIPC`, and `-variant=SCHIPM`. `SCHIPM` will be used by default.
