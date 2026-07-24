# Logos Execution Zone Core Module

### Setup

#### IDE

If you're using an IDE with CMake integration make sure it points to the same cmake directory as the `justfile`, which defaults to `build`.

This will reduce friction when working on the project.

#### Nix

* Use `nix flake update` to bring all nix context and packages
* Use `nix build` to build the package
* Use `nix run` to launch the module-viewer and check your module loads properly
* Use `nix develop` to setup your IDE

### Releases

This repository owns LEZ Core package releases. Run **Publish LEZ Core** from
the `main` branch to build the portable `lez_core` package for Linux x86_64 and
Apple-silicon macOS. The workflow publishes the `.lgx` asset and `sidecar.json`
under the `lez_core-v<version>` tag, using the version in `metadata.json`.

The initial releases are GitHub prereleases while the package is in alpha.
Package catalogs may index these source-owned assets, but do not rebuild or
re-host them.
