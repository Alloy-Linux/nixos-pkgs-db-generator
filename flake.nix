
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in {
        devShells = {
          default = pkgs.mkShell {
            packages = [
              pkgs.gcc
              pkgs.sqlite
              pkgs.cjson
              pkgs.pkg-config
            ];

            shellHook = ''
              echo "Development environment loaded"
            '';
          };
        };
        defaultPackage = pkgs.stdenv.mkDerivation {
          pname = "nixos-pkgs-db-generator";
          version = "0.0.1";
          src = ./.;

          buildInputs = [ 
            pkgs.gcc
            pkgs.sqlite
            pkgs.cjson
            pkgs.pkg-config
          ];

          buildPhase = ''
            gcc -o SQLGenerator main.c -lsqlite3 -lcjson
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp SQLGenerator $out/bin/nixos-pkgs-db-generator
          '';
        };
      }
    );
}
