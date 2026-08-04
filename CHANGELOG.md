# Changelog

## [0.4.0-alpha.3] - Alpha

### Added

- Expose bounded local public-block history and preserve the maintained
  registration and transaction-envelope behavior needed by Logos Palace.

## [0.4.0-alpha.2] - Alpha

### Changed

- Update the maintained execution-zone runtime pin to include the bounded
  wallet cold-start calibration fix.

## [0.4.0-alpha.1] - Alpha

### Changed

- Build LEZ Core against the maintained execution-zone fork at an immutable
  revision.
- Keep wallet create/open compatible with the fork's statistics sidecar while
  preserving the existing module API.

## [0.3.1] - Alpha

### Added

- Source-owned alpha release workflow for the portable LEZ Core package.

### Fixed

- Reclaim unrelated hosted-runner payloads before the Linux portable build to avoid disk exhaustion.
