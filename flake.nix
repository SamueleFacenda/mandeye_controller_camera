# nix comment
{
  description = "Flake for mandeye building tests";
  inputs = {
    nixpkgs.url = "nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks.url = "github:cachix/pre-commit-hooks.nix";
    pre-commit-hooks.inputs.nixpkgs.follows = "nixpkgs";
  };
  outputs = { self, nixpkgs, flake-utils, pre-commit-hooks }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = (nixpkgs.legacyPackages.${system}.extend (final: prev: {})); in
      {

        packages = rec {
          default = mandeye;
          mandeye = pkgs.stdenv.mkDerivation {
            pname = "mandeye";
            version = "0.4";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
            ];
            buildInputs = with pkgs; [
              libserial
              rapidjson
              opencv
              tbb
            ];
            cmakeFlags = [
              "-DTBB_ROOT=${pkgs.tbb}"
              "-DTBB_INCLUDE_DIR=${pkgs.tbb.dev}/include"
            ];
          };
          libserial = pkgs.stdenv.mkDerivation {
            pname = "libserial";
            version = "1.0.0";
            src = pkgs.fetchFromGitHub {
              repo = "libserial";
              owner = "crayzeewulf";
              rev = "ee1992dc3c5dc41a0e18e2d1dad18c103eb405ec";
              sha256 = "Qs1Kq7TI+JEV4hsQRL2yHW8x9/C0s0gN6aDwql3AG6c=";
            };
            nativeBuildInputs = with pkgs; [
              cmake
            ];
            buildInputs = with pkgs; [
              autoconf
              libtool
            ];
            cmakeFlags = [
              "-DLIBSERIAL_BUILD_DOCS=OFF"
              "-DLIBSERIAL_ENABLE_TESTING=OFF"
              "-DLIBSERIAL_PYTHON_ENABLE=OFF"
            ];
          };
        };

        # apps = {
        #   default = {
        #     type = "app";
        #     program = "${self.defaultPackage.${system}}/bin/a.out";
        #   };
        # };
        devShells = {
          default = pkgs.mkShell {
            nativeBuildInputs = with pkgs; [
              cmake
            ];
            buildInputs = with pkgs; [
              self.packages.${system}.libserial
              rapidjson
              opencv
              tbb
            ];

            # inherit (self.checks.${system}.pre-commit-check) shellHook;

          };

        };

        checks = {
          pre-commit-check = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks = {
              nixpkgs-fmt.enable = true;
              shellcheck.enable = true;
              clang-format = {
                enable = true;
                types_or = [ "c" "c++" ];
              };
              clang-tidy.enable = true;
            };
          };
        };

      });
}
