{ nixpkgs ? import <nixpkgs> { } }:

let
  pkgs = nixpkgs;
  #pkgs = import (fetchTarball("https://github.com/NixOS/nixpkgs/archive/a58a0b5098f0c2a389ee70eb69422a052982d990.tar.gz")) {};
  #pkgs = import  (fetchTarball("channel:nixpkgs-unstable")) {};
in
  nixpkgs.stdenv.mkDerivation {
    name = "env";
    buildInputs = with pkgs; [
      cmake
      clang
      ninja
    ];

    RUST_SRC_PATH = "${pkgs.rust.packages.stable.rustPlatform.rustLibSrc}";
  }
