{
  description = "Logos Execution Zone Core Module";

  inputs = {
    logos-module-builder.url = "github:3esmit/logos-module-builder?rev=1afad1253b57a8c1848ae6dc955dcab477b3b4c3";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";
    logos-execution-zone.url = "github:3esmit/logos-execution-zone?rev=f17401245510d27605054a1e766de4ee44972e12";
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
