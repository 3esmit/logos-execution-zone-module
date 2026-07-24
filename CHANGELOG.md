# Changelog

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
