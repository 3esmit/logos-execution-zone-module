{
  description = "Logos Execution Zone Core Module";

  inputs = {
    logos-module-builder.url = "github:3esmit/logos-module-builder?rev=6ef42ea8661121831ece79e6b702e27ac1cf46e7";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    logos-execution-zone.url = "github:3esmit/logos-execution-zone?rev=685d07dee11a9d934fa9a205de73f356dfbeb6d7";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;
      externalLibInputs = {
        wallet_ffi = {
          input = inputs.logos-execution-zone;
          packages.default = "wallet";
        };
      };
      tests = {
        dir = ./tests;
        mockCLibs = [ "wallet_ffi" ];
      };
    };
}
