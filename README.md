### Compiling from Source
```bash
git clone https://github.com/Alloy-Linux/nixos-pkgs-db-generator.git
cd nixos-pkgs-db-generator.
nix develop -i
gcc -o SQLGenerator main.c -lsqlite3 -lcjson
./SQLGenerator
```

### Running the executable
Download the flakes
```bash
cd path/to/flakes
nix develop -i
/path/to/executable
```


&copy; Copyright Alloy Linux 2025
