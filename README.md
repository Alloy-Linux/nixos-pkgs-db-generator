### Compiling from Source (Recomended)
```bash
git clone https://github.com/Alloy-Linux/nixos-pkgs-db-generator.git
cd nixos-pkgs-db-generator
nix develop -i
gcc -o SQLGenerator main.c -lsqlite3 -lcjson
chmod +x SQLGenerator
./SQLGenerator
```
### Running the executable (Currently not working)
Download the flakes
```bash
cd path/to/flakes
nix develop -i
/path/to/executable
```

&copy; Copyright Alloy Linux 2025
