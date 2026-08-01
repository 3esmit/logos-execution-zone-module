{
  description = "Logos Execution Zone Core Module";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    logos-execution-zone.url = "github:3esmit/logos-execution-zone?rev=d9d152ffebabd0ae397294e240973c6a48cfa514";
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
