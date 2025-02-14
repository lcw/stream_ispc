{
  description = "python / ispc environment";

  inputs =
    {
      nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    };

  outputs = { self, nixpkgs, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.x86_64-linux.default =
        pkgs.mkShell
        {
            packages = with pkgs; [
              (python3.withPackages (ps: [ ]))
              gcc
              gnumake
              ispc
            ];
            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [ pkgs.stdenv.cc.cc pkgs.libz ];
            NIX_ENFORCE_NO_NATIVE = 0;
        };
    };
}
