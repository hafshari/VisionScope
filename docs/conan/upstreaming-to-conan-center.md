# Upstreaming a Conan package to Conan Center

Investigation / process notes for packaging third-party C/C++ libraries so they can be consumed from the official Conan remote (`https://center2.conan.io`).

- **Status:** Living documentation (not a product decision ADR)
- **Date:** 2026-07-19
- **Audience:** VisionScope contributors who need a library that is missing from Conan Center (e.g. a dedicated camera-capture dependency)
- **Upstream repo:** [conan-io/conan-center-index](https://github.com/conan-io/conan-center-index) (CCI)

## What “upstream Conan” means

| Term | Meaning |
|------|---------|
| **Recipe** | A `conanfile.py` (+ metadata) that knows how to download, build, and package a library |
| **Conan Center Index (CCI)** | GitHub repo of all official recipes; contributions are PRs here |
| **Conan Center / C3I** | CI + binary build farm that builds recipe PRs and, after merge, publishes packages |
| **Remote** | Client-facing package store. Current default: `https://center2.conan.io` (Conan ≥ 2.9.2). Older `https://center.conan.io` is frozen |

You do **not** upload binaries to Conan Center yourself for official packages. You contribute the **recipe**; C3I builds and publishes binaries.

Update a stale local remote with:

```bash
conan remote update conancenter --url="https://center2.conan.io"
```

## End-to-end flow

```text
Fork CCI → copy package template → fill config/conandata/conanfile/test_package
        → conan create locally → open PR → CLA + review + C3I builds → merge
        → package available as name/version from conancenter
```

Official guide: [Adding Packages to ConanCenter](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/README.md).

## Prerequisites

1. Recent **Conan 2** client (VisionScope pins `conan==2.30.0` in [`requirements.txt`](../../requirements.txt)).
2. Fork + clone [conan-center-index](https://github.com/conan-io/conan-center-index).
3. First-time PR: sign the [CLA](https://cla-assistant.io/conan-io/conan-center-index).
4. Prefer building an **existing** recipe once first ([developing recipes locally](https://github.com/conan-io/conan-center-index/blob/master/docs/developing_recipes_locally.md)) so tooling and layouts are familiar.

## Recipe layout (required by CCI)

One library = one folder under `recipes/`. A PR may change **only that one** recipe folder.

Canonical layout (preferred: single recipe in `all/`):

```text
recipes/
└── library_name/
    ├── config.yml
    └── all/
        ├── conanfile.py
        ├── conandata.yml
        ├── patches/                    # optional
        │   └── 1.0.0-0001-fix-cmake.patch
        └── test_package/
            ├── conanfile.py
            ├── CMakeLists.txt
            └── test_package.cpp
```

Source of truth: [Folders and files](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/folders_and_files.md).

### `config.yml`

Maps publishable versions → recipe folder:

```yaml
versions:
  "1.7.0":
    folder: all
```

For a **new** recipe, start with **only the latest** upstream version.

### `conandata.yml`

Declares downloadable sources (and optional patches) with URLs + checksums. The recipe reads this via `self.conan_data`. Prefer official release tarballs/tags over floating git branches.

### `conanfile.py`

Implements the Conan 2 recipe lifecycle. Typical CMake library shape:

| Method / attr | Role |
|---------------|------|
| `name`, `package_type`, `license`, `url`, `homepage`, `description`, `topics` | Package identity |
| `settings` | Usually `os`, `arch`, `compiler`, `build_type` |
| `options` / `default_options` | e.g. `shared`, `fPIC` — match upstream defaults |
| `export_sources()` | Export patches / helper CMake if needed |
| `source()` | `get()` / unpack from `conandata.yml` |
| `generate()` | `CMakeToolchain`, `CMakeDeps`, virtualenv as needed |
| `build()` | Configure + build upstream |
| `package()` | Install into package folder; copy license |
| `package_info()` | `cpp_info` libs/includes/components for consumers |

CCI expectations (high level):

- Recipes should match **upstream intent** (options, components, default flags).
- Prefer fixing build issues **upstream**; patches in CCI are scrutinized ([patch policy](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/sources_and_patches.md)).
- Do **not** run upstream unit-test suites by default in recipes (C3I is a binary farm, not a test lab).
- Dependencies: only other Conan Center packages; version ranges are generally disallowed; no `python_requires` in CCI recipes. See [dependencies](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/dependencies.md).

Start from CCI [`package_templates/`](https://github.com/conan-io/conan-center-index/tree/master/docs/package_templates) rather than inventing layout from scratch.

### `test_package/`

**Required.** Minimal consumer that `#include`s headers and links the library so packaging/`package_info` is verified. Keep it tiny: no GUIs, network, servers, or large assets.

## Local development loop

From the recipe folder (example):

```bash
cd recipes/library_name

# Create + run test_package for one version
conan create all/conanfile.py --version=1.7.0

# Extra configs worth smoke-testing before PR
conan create all/conanfile.py --version=1.7.0 -s build_type=Debug
conan create all/conanfile.py --version=1.7.0 -o "library_name/*:shared=True"
```

Tips:

- Work inside `recipes/<name>/` so you do not accidentally touch multiple recipes in one PR.
- If upstream CMake is consumer-hostile, fix with minimal patches and prefer contributing the fix upstream too.
- Cross-build and unusual compilers will be exercised by C3I; still try Debug/shared/local host first.

## Submitting upstream

1. Push the recipe branch to **your fork** of `conan-center-index`.
2. Open a PR against `conan-io/conan-center-index` (one recipe per PR).
3. Sign CLA if prompted.
4. Watch **Checks** on the PR: linters, hooks, and C3I multi-config builds (often 30+ binaries for a C++ library).
5. Address review comments from Conan Center maintainers.
6. After merge, consume from the official remote, e.g.:

```text
[requires]
library_name/1.7.0
```

Review process overview: [review_process.md](https://github.com/conan-io/conan-center-index/blob/master/docs/review_process.md).

Supported build matrix: [supported platforms and configurations](https://github.com/conan-io/conan-center-index/blob/master/docs/supported_platforms_and_configurations.md).

## Local / private packages vs Conan Center

Not every dependency needs to go upstream immediately. Useful staging paths for VisionScope:

| Approach | When to use | Downstream cost |
|----------|-------------|-----------------|
| **CCI PR (official)** | Library is general-purpose and worth community maintenance | Review latency; highest reuse |
| **Local `conan create` + user remote / Artifactory** | Spike a recipe; internal forks; not ready for CCI policy | You host binaries/recipes |
| **`conanfile.py` / `conanfile.txt` + `requirements()` on a git/URL package** | Temporary; experimental | Fragile for CI reproducibility |
| **CMake `FetchContent` / submodule** | Avoid Conan for one dep temporarily | Split dep story vs rest of stack |

Recommended path for a missing camera library: **prototype the recipe locally → use it from a private/user remote in VisionScope CI → upstream to CCI when stable**.

Sketch of a local prototype (outside CCI):

```bash
# In a scratch recipe dir matching Conan layout
conan create . --name=examplelib --version=1.0.0
conan list examplelib/1.0.0
# Point VisionScope at a custom remote or the local cache during the spike
```

For CCI contribution, always follow the **CCI folder layout** above so the same recipe can be copy-pasted into a PR.

## Checklist before opening a CCI PR

- [ ] Latest upstream version only (for brand-new recipes)
- [ ] `config.yml` + `conandata.yml` with verified checksums
- [ ] Attributes filled (`license`, `homepage`, `description`, `topics`, `package_type`)
- [ ] Options/defaults match upstream
- [ ] `package_info()` correct (libs, includes, components / CMake target names)
- [ ] License file packaged
- [ ] Minimal `test_package` passes via `conan create`
- [ ] Smoke-tested Debug and `shared=True` when applicable
- [ ] Patches documented and justified; prefer upstream fixes
- [ ] Single-recipe PR; CLA signed

## Relevance to VisionScope

From [ADR-0001](../adr/0001-cross-platform-camera-capture.md):

- Prefer deps already on Conan Center (`sdl`, `opencv`, `ffmpeg`, `ftxui`, …) for MVP speed.
- If we later choose a library **not** on Center (e.g. **ccap**), this document is the playbook to upstream it—or to run a private recipe until CCI accepts it.
- Do **not** confuse packaging VisionScope itself (application packaging) with packaging a third-party capture library; CCI is primarily for reusable libraries/tools consumers require.

## Official references

- [Contributing to Conan Center (Conan 2 docs)](https://docs.conan.io/2/tutorial/conan_repositories/conan_center.html)
- [CCI: Adding packages](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/README.md)
- [CCI: Developing recipes locally](https://github.com/conan-io/conan-center-index/blob/master/docs/developing_recipes_locally.md)
- [CCI: Folders and files](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/folders_and_files.md)
- [CCI: Dependencies rules](https://github.com/conan-io/conan-center-index/blob/master/docs/adding_packages/dependencies.md)
- [Conan: Creating packages](https://docs.conan.io/2/tutorial/creating_packages.html)
- [Conan Center browser](https://conan.io/center/)
