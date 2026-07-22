# Changelog

版本紀錄依 `RELEASING.md` 的流程維護；日期於發佈 MR 時標上。
Maintained per `RELEASING.md`; dates are stamped in the release MR.

## v1.0.0 — (unreleased)

首個正式發佈：完整的 C17 → WasmVM 工具鏈。
First release: the complete C17 → WasmVM toolchain.

- **編譯管線 Pipeline** — 前處理（C17 轉譯階段 1–7）、手寫遞迴下降剖析器、語意分析、
  直接產生 `WasmModule` 的程式碼產生器（wasm64、LP64、雙記憶體模型與標記指標）。
  Preprocessor (translation phases 1–7), hand-written recursive-descent parser, semantic
  analysis, and direct-to-`WasmModule` codegen (wasm64, LP64, dual-memory model with tagged
  pointers).
- **整合式連結器 Integrated linker** — 多 TU 與 `.o`/`.wasm` 物件合併、符號解析、`.a` 惰性
  載入、重定位、DCE、`crt0` 合成與連結映射表。
  Multi-TU and object merging, symbol resolution, lazy archive members, relocations, DCE,
  `crt0` synthesis, and link maps.
- **libc 執行期 runtime** — 最小化獨立式 C 函式庫，由 wvmcc 自行編譯為 `libc.a` 並安裝於
  sysroot（stdio、stdlib、string、ctype、math with errno 契約等）。
  A minimal freestanding libc built by wvmcc itself into a sysroot (stdio, stdlib, string,
  ctype, math with the errno contract, …).
- **一致性 Conformance** — 以 ISO/IEC 9899:2017 為準的目錄與測試套件
  （`docs/standard/` + `tests/standard/`）：526 條測試，`status-supported` 520/520 全綠；
  兩條 deferred 失敗為預期的一致性訊號（`_Thread_local` 與 VLA 約束診斷）。
  A C17 catalog + suite: 526 tests, status-supported 520/520 green; the two deferred failures
  are the intended conformance signal.
- **工具 Tooling** — `--version`、`-E`、`--ast`、`--map`、sysroot 解析、CPack 打包
  （`.deb`/`.pkg`/`.tar.gz`）與 GitHub Actions 發佈管線。
  `--version`, `-E`, `--ast`, `--map`, sysroot resolution, CPack packaging, and the GitHub
  Actions release pipeline.
