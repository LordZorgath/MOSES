# MOSES 

MOSES is an experimental multi-system emulator, focused mainly, but not exclusively, on cycle-accurate emulation of vintage computers.

# Compile instructions:

```
git clone https://github.com/LordZorgath/MOSES
cd MOSES
mkdir build && cd build
cmake ../
cmake --build ./
```
# Run instructions:

```
./MOSES -c, --core <core> -f, --file </path/to/game/>
```
Optional commands: 

`-s, --scale <integer scaling factor>`

`-v, --volume <integer volume, 0-100>`

`-u, --unlimited-framerate`

Supported cores:

`chip8`

`schip`

`xochip`

Core specific options should be specified by adding comma-seperated keys after the core. Values should be seperated by an equals sign. For example, `chip8,nodisplaywait,sp=1000`

Valid core specific options:

`chip8`, `schip` and `xochip`: `speed=<integer>`, the number of instructions to execute in a frame.

`chip8` and `schip`: `nodisplaywait`, disables display wait emulation. This is needed to run the cores as fast as possible.

`schip`: `legacy`, enables emulation of legacy Schip behaviour. Otherwise, the Schip core will act as it does in Octo.
