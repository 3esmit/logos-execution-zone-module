#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

workflow=".github/workflows/release.yml"
action_workflow="3esmit/logos-modules-release-action/.github/workflows/release.yml"
action_ref="${action_workflow}@049097bc9956c681905cde8a397991cf2b51c20b"

assert_workflow_line() {
  local expected="$1"
  if ! grep -Fqx "$expected" "$workflow"; then
    printf 'missing release workflow contract: %s\n' "$expected" >&2
    exit 1
  fi
}

test -f "$workflow"
test "$(jq -r '.name' metadata.json)" = "lez_core"

version="$(jq -er '.version | strings | select(length > 0)' metadata.json)"
grep -Fq "## [${version}]" CHANGELOG.md
grep -Fq "return \"${version}\";" src/lez_core_module.cpp

execution_zone_rev="$(
  jq -er '
    .nodes[.nodes.root.inputs["logos-execution-zone"]].locked
    | select(
        .type == "github"
        and .owner == "3esmit"
        and .repo == "logos-execution-zone"
        and (.rev | test("^[0-9a-f]{40}$"))
      )
    | .rev
  ' flake.lock
)"
jq -e --arg rev "$execution_zone_rev" '
  .nodes[.nodes.root.inputs["logos-execution-zone"]]
  | .original.type == "github"
    and .original.owner == "3esmit"
    and .original.repo == "logos-execution-zone"
    and .original.rev == $rev
' flake.lock >/dev/null
grep -Fqx \
  "    logos-execution-zone.url = \"github:3esmit/logos-execution-zone?rev=${execution_zone_rev}\";" \
  flake.nix

assert_workflow_line "    uses: ${action_ref}"
assert_workflow_line "      module_path: ."
assert_workflow_line "      metadata_path: metadata.json"
assert_workflow_line "      build_attr: lgx-portable"
assert_workflow_line "      variants: linux-amd64,darwin-arm64"
assert_workflow_line "      require_all_variants: true"
assert_workflow_line "      dispatch_rebuild_index: false"
assert_workflow_line "      prerelease: true"
assert_workflow_line "      install_macos_metal_toolchain: true"
assert_workflow_line "      reclaim_linux_disk_space: true"
assert_workflow_line "      signing_mode: none"

test "$(grep -Fc "uses: ${action_workflow}@" "$workflow")" -eq 1

printf 'source release workflow contract valid for lez_core v%s\n' "$version"
