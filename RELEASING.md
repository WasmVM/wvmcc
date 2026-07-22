# 發佈流程 Releasing

wvmcc 採 **不定期發佈**：由維護者自行決定何時發佈。GitLab（canonical）只保存標籤與發佈說明；
二進位套件僅發佈於 GitHub mirror（[WasmVM/wvmcc](https://github.com/WasmVM/wvmcc)）的 Release。

wvmcc releases are **ad hoc** — the maintainer decides when to cut one. GitLab (canonical) holds
the tag and the release notes only; binary packages are published exclusively on the GitHub
mirror's Releases.

## 版本規則 Versioning

`CMakeLists.txt` 的 `project(wvmcc VERSION x.y.z)` 是唯一的版本來源（`wvmcc --version` 由此注入）。
The `project(wvmcc VERSION x.y.z)` in `CMakeLists.txt` is the single source of truth
(`wvmcc --version` is injected from it).

## 檢查表 Checklist

### 1. 發佈前 Preconditions

- [ ] master 在 GitLab **與** GitHub CI 全綠（GitHub 另建 clang 組建）。
      master green on both GitLab **and** GitHub CI (GitHub adds the clang builds).
- [ ] 一致性把關全過：`ninja -C build check-supported`（`status-supported` 必須 100%）。
      Conformance gate: `ninja -C build check-supported` (status-supported must be 100%).
- [ ] 文件為最新（`README.md`、`docs/`、`docs/standard/` 目錄）。Docs current.

### 2. 發佈 MR Release MR

- [ ] `CMakeLists.txt`：更新 `project(wvmcc VERSION x.y.z)`。Bump the version.
- [ ] `CHANGELOG.md`：把該版段落補齊並標上日期。Finalize the section and stamp the date.
- [ ] `RELEASE.md`：整份重寫為本次發佈說明——**第一行必須是** `` `vX.Y.Z` ``（反引號包住的
      標籤名，`WasmVM/release-action` 以此決定標籤），其後為發佈重點。
      Rewrite as this release's notes — **line 1 must be** `` `vX.Y.Z` `` (the backtick-quoted
      tag name; `WasmVM/release-action` derives the tag from it), followed by the highlights.
- [ ] 在 `RELEASE.md` 記錄 **WasmVM 相容版本**（本次以哪個 WasmVM release 建置與通過煙霧測試）。
      Record the **WasmVM compatibility pin** (which WasmVM release this was built and
      smoke-tested against).
- [ ] MR 合併至 master。Merge the MR to master.

### 3. 標籤與 GitLab 發佈（僅說明）Tag + GitLab release (notes-only)

```sh
git fetch origin master
git tag -a vX.Y.Z origin/master -m "wvmcc vX.Y.Z"
git push origin vX.Y.Z
# GitLab release entry (notes only — no binaries). glab api 繞過 SSH-alias 問題：
glab api --hostname git.luishsu.me --method POST "projects/wasmvm%2Fwvmcc/releases" \
  -f tag_name=vX.Y.Z -f name="wvmcc vX.Y.Z" -f "description=$(cat RELEASE.md)"
```

### 4. 鏡像與二進位發佈 Mirror + binary release (GitHub)

```sh
git push github origin/master:refs/heads/master   # fast-forward only
git push github vX.Y.Z
git push github origin/master:refs/heads/release  # triggers .github/workflows/pack.yml
```

`pack.yml` 會建置 `.deb`（Ubuntu）、`.pkg`（macOS）與 `.tar.gz`，以**打包後的成品**跑
煙霧測試（安裝 wasmvm 與 wvmcc 的 .deb，編譯並執行 hello world），再以 `RELEASE.md` 為
說明發佈 GitHub Release。從其他分支 `workflow_dispatch` 則為乾跑（只打包與煙測，不發佈）。

`pack.yml` builds the `.deb` (Ubuntu), `.pkg` (macOS), and `.tar.gz` packages, smoke-tests the
**packaged artifact** (installs the wasmvm + wvmcc `.deb`s, compiles and runs hello world), then
publishes the GitHub Release with `RELEASE.md` as its body. A `workflow_dispatch` from any other
branch is a dry run (package + smoke only, no publish).

> **Windows。** `cmake/pack.cmake` 已含 NSIS 設定，可在 Windows 上以本機 `cpack` 打包；但
> wvmcc 尚無 Windows CI（從未在 MSVC 驗證過），因此發佈管線不產出 Windows 安裝檔。待
> Windows 組建於 CI 驗證後，再於 `pack.yml` 加入 NSIS 工作。
>
> **Windows.** `cmake/pack.cmake` carries the NSIS settings, so a local `cpack` on Windows can
> build an installer; but wvmcc has no Windows CI (never verified under MSVC), so the release
> pipeline publishes no Windows installer. Add an NSIS job to `pack.yml` only after a Windows
> build is CI-verified.

### 5. 驗證 Verify

- [ ] `gh release view vX.Y.Z --repo WasmVM/wvmcc` 列出 `.deb` / `.pkg` / `.tar.gz` 資產。
      Assets are listed on the GitHub release.
- [ ] GitLab release 頁面顯示說明。The GitLab release entry shows the notes.
