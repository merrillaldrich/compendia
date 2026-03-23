# Versioning

## Overview

The version has a single source of truth: the `project(compendia VERSION ...)` line in `CMakeLists.txt`. CMake expands it into `version.h` at build time (via `version.h.in`), and CPack uses it for the ZIP filename. There is no automatic git-tag-to-version wiring — you manage it manually.

## Release workflow

### 1. Finish the work and commit everything

Make sure `main` is clean and the build is good. All commits for this release should already be on `main`.

### 2. Tag the release commit

```bash
git tag -a v0.1.0 -m "Release 0.1.0"
git push origin v0.1.0
```

The tag name should match the version in `CMakeLists.txt`. Use annotated tags (`-a`) rather than lightweight ones — they carry a timestamp and message.

### 3. Build and package from the tagged commit

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cd build && cpack -G ZIP -C Release
```

The output will be `Compendia-0.1.0-Windows-x64.zip`. See [PACKAGING.md](PACKAGING.md) for full packaging details.

### 4. Advance the version in CMakeLists.txt

Immediately after tagging, bump the version on `main` so subsequent builds are clearly not the tagged release:

```cmake
project(compendia VERSION 0.2.0 LANGUAGES CXX)
```

Commit the change:

```bash
git add CMakeLists.txt
git commit -m "Bump version to 0.2.0"
```

## Publishing a GitHub Release

After tagging and building the ZIP, publish it as a GitHub Release using the `gh` CLI. The tag must already be pushed to the remote before running this.

```bash
gh release create v0.1.0 build/Compendia-0.1.0-Windows-x64.zip \
  --title "Compendia 0.1.0" \
  --notes "Brief description of what's in this release."
```

This creates the release, attaches the ZIP as a downloadable asset, and publishes it immediately. To review on GitHub before publishing, create a draft first:

```bash
gh release create v0.1.0 build/Compendia-0.1.0-Windows-x64.zip \
  --title "Compendia 0.1.0" \
  --draft \
  --generate-notes
```

`--generate-notes` auto-generates release notes from commits since the last tag. Visit the release page on GitHub to review and publish the draft.

Useful flags:

| Flag | Purpose |
|---|---|
| `--draft` | Creates the release as a draft for review before publishing |
| `--prerelease` | Marks the release as a pre-release on GitHub |
| `--notes-file NOTES.md` | Pulls release notes from a file instead of inline |
| `--generate-notes` | Auto-generates notes from commits since the last tag |

To upload an additional asset to an existing release:

```bash
gh release upload v0.1.0 some-other-file.zip
```

## Choosing version numbers

This project uses [Semantic Versioning](https://semver.org/): `MAJOR.MINOR.PATCH`.

| Change type | Example bump |
|---|---|
| Breaking change or major milestone | `0.1.0` → `1.0.0` |
| New feature, backwards-compatible | `0.1.0` → `0.2.0` |
| Bug fix only | `0.1.0` → `0.1.1` |

While the major version is `0`, the convention is relaxed — `0.x` signals pre-release/unstable, so minor bumps can include breaking changes.
