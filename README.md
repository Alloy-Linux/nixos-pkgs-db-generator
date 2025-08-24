### Compiling from Source (Recomended)
```bash
git clone https://github.com/Alloy-Linux/nixos-pkgs-db-generator.git
cd nixos-pkgs-db-generator
nix develop -i
gcc -o SQLGenerator main.c -lsqlite3 -lcjson
chmod +x SQLGenerator
./SQLGenerator
```
Note that `nix develop -i` requires root!
### Running the executable
Download the flakes
```bash
cd path/to/flakes
nix develop -i
/path/to/executable
```


&copy; Copyright Alloy Linux 2025
