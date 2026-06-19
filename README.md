# WVMCC

#### 為 [WasmVM](https://github.com/WasmVM/WasmVM) 打造的 C 編譯器

一個獨立式（freestanding）的 **C17 → WebAssembly** 編譯器、無組譯器的工具鏈，以及最小化
libc，目標平台為 [WasmVM](https://github.com/WasmVM/WasmVM)。

A freestanding **C17 → WebAssembly** compiler, assembler-free toolchain, and minimal libc,
targeting [WasmVM](https://github.com/WasmVM/WasmVM).

wvmcc 直接將 C 原始碼編譯為 WasmVM 的 `WasmModule`，不需外部組譯器或連結器；前處理、剖析、
語意分析、程式碼產生與連結全都包在同一個執行檔內。

wvmcc takes C source and emits a WasmVM `WasmModule` directly — no external assembler or linker.
Preprocessing, parsing, semantic analysis, code generation, and linking all live in one binary.

---

## 注意事項 Notice

在文件方面，本專案以 **台灣正體中文** 為主要使用語言，英文為次要使用語言，其他語言
（例如：簡體中文）僅能做為參考或翻譯使用。

This project uses **Taiwan Traditional Chinese** as its primary documentation language and English
as secondary. Other languages (e.g. Simplified Chinese) are for reference or translation only.

> **設計原則。** wvmcc 是為新目標打造的新編譯器；其行為以 **C17 標準**
> (ISO/IEC 9899:2017) 與本專案自身的設計文件 (`docs/`) 為準，**不**沿用 GCC / Clang 的行為。
>
> **Design rule.** wvmcc is a new compiler for a new target — expected behavior derives from the
> C17 standard and this project's own design docs, never from GCC/Clang.

---

## 功能 What it does

一條自成一體的編譯管線，不依賴任何第三方組譯器或連結器：

A self-contained pipeline, with no third-party assembler/linker:

- **前處理器 Preprocessor** — 完整的 C17 轉譯階段 1–7：三連符、行接續、完整指令集
  （`#define/#undef/#include/#if/#elif/#ifdef/#error/#warning/#line/#pragma`）、物件式與
  函式式巨集、可變參數巨集、`#`/`##`、塗色語意與重新掃描，以及 `#if` 常數運算式求值。

  Full C17 translation phases 1–7: trigraphs, line splicing, the complete directive set, object-
  and function-like macros, variadic macros, `#`/`##`, paint semantics and rescanning, and `#if`
  constant-expression evaluation.

- **剖析器與 AST Parser & AST** — 手寫遞迴下降剖析器，涵蓋 C17 宣告子文法、struct/union/enum、
  初始化器（指定子、複合字面值），以及完整的陳述式集合。

  A hand-written recursive-descent parser covering the C17 declarator grammar, struct/union/enum,
  initializers (designators, compound literals), and the full statement set.

- **語意分析 Semantic analysis** — 範圍／標籤／typedef 解析、型別系統、一般算術轉換、
  整數常數運算式求值、暫時定義 (6.9.2)、`_Generic`、`_Static_assert`，以及約束診斷。

  Scope/tag/typedef resolution, the type system, usual arithmetic conversions, integer-constant-
  expression evaluation, tentative definitions (6.9.2), `_Generic`, `_Static_assert`, and
  constraint diagnostics.

- **程式碼產生 Code generation** — 將 AST 降階為 WasmVM 的 `WasmModule`（並以
  `WasmVM::module_validate` 驗證）。在 wasm64 上採 LP64 資料模型：`int` 為 32 位元，
  `long`、指標、`size_t`、`ptrdiff_t` 為 64 位元。

  Lowers the AST to a WasmVM `WasmModule` (validated via `WasmVM::module_validate`). LP64 data
  model on wasm64: `int` is 32-bit; `long`, pointers, `size_t`, and `ptrdiff_t` are 64-bit.

- **整合式連結器 Integrated linker** — 合併多個轉譯單元與 `.o`/`.wasm` 物件、解析符號、
  從 `.a` 封存檔惰性拉入成員、套用重定位、消除無用程式碼、合成 `crt0`，並可輸出連結映射表。

  Merges multiple translation units and `.o`/`.wasm` objects, resolves symbols, pulls members
  lazily from `.a` archives, applies relocations, eliminates dead code, synthesizes `crt0`, and
  can emit a link map.

- **libc 執行期 libc runtime** — 一套最小化的獨立式 C 函式庫（`runtime/`），由 wvmcc 自行
  編譯並打包成 `libc.a`，安裝於 sysroot 之下。

  A minimal freestanding C library (`runtime/`) built by wvmcc itself and bundled into `libc.a`,
  installed under a sysroot.

資料模型與 ABI 見 [`docs/spec.md`](docs/spec.md)，降階設計見
[`docs/lowering-plan.md`](docs/lowering-plan.md)，程式碼產生實作見 [`docs/codegen.md`](docs/codegen.md)。

See [`docs/spec.md`](docs/spec.md) for the data model and ABI,
[`docs/lowering-plan.md`](docs/lowering-plan.md) for the lowering design, and
[`docs/codegen.md`](docs/codegen.md) for the code-generation implementation.

---

## 先備條件 Prerequisites

- **CMake** ≥ 3.16 與 **Ninja** / and **Ninja**
- 支援 **C11** 與 **C++20** 的 C/C++ 工具鏈 / A C/C++ toolchain supporting **C11** and **C++20**
- **[WasmVM](https://github.com/WasmVM/WasmVM)** — 提供 `WasmModule` 函式庫（由
  `cmake/FindWasmVM.cmake` 尋找），以及建置與檢視執行期所用的 `wasmvm-ar` 封存工具與 `readwasm`。

  Provides the `WasmModule` library (found via `cmake/FindWasmVM.cmake`) and the `wasmvm-ar`
  archiver / `readwasm` tools used to build and inspect the runtime.

---

## 編譯 Build

以 Ninja 進行外置式建置 / Out-of-source build with Ninja:

```sh
cmake -G Ninja -S . -B build
ninja -C build
```

這會建置 `wvmcc` 驅動程式，並（預設）在 `build/runtime/` 下建置 libc 執行期 (`libc.a`)。

This builds the `wvmcc` driver and, by default, the libc runtime (`libc.a`) under `build/runtime/`.

常用選項 / Useful options:

- `-DENABLE_SANITIZERS=ON` — 以 AddressSanitizer + UBSan 建置以利除錯。/ Build with
  AddressSanitizer + UBSan for debugging.
- `-DWVMCC_BUILD_RUNTIME=OFF` — 略過 libc 執行期建置。/ Skip the libc runtime build.
- `-DWASMVM_AR=/path/to/wasmvm-ar` — 當 `wasmvm-ar` 不在 `PATH` 上時指定其路徑。/ Point at
  `wasmvm-ar` if it is not on `PATH`.

安裝（驅動程式至 `bin/`，標頭與 `libc.a` 至 `share/wvmcc/`）：

Install (driver to `bin/`, headers and `libc.a` to `share/wvmcc/`):

```sh
ninja -C build install
```

---

## 執行 Usage

```sh
wvmcc [options] <input...>
```

輸入可混用 `.c` 原始碼（於記憶體中編譯）與 `.o`/`.wasm` 物件（直接連結）。

Inputs may mix `.c` sources (compiled in-memory) and `.o`/`.wasm` objects (linked directly).

| 選項 Option | 說明 Meaning |
|---|---|
| `-o <file>` | 輸出 wasm 至 `<file>`（預設 `a.wasm`）。/ Write output wasm to `<file>` (default `a.wasm`). |
| `-c` | 僅編譯——輸出可連結的 `.wasm` 物件，不進行連結。/ Compile only — emit a linkable object, no link. |
| `-E` | 僅執行前處理器，輸出至 stdout。/ Run the preprocessor only; write to stdout. |
| `--ast` | 傾印簡易 XML AST。/ Dump a simple XML AST. |
| `-I <path>` | 新增標頭搜尋路徑（可重複；亦接受 `-I<path>`）。/ Add a header search path (repeatable). |
| `-isystem <dir>` | 新增系統標頭搜尋路徑。/ Add a system header search path. |
| `-L <dir>` | 新增 `-l` 用的封存檔搜尋路徑。/ Add an archive search path for `-l`. |
| `-l<name>` | 從 `-L` 目錄或 `<sysroot>/lib` 連結 `lib<name>.a`。/ Link `lib<name>.a`. |
| `-nostdlib` | 略過預設 libc 連結與 `crt0` 注入。/ Skip default libc linking and `crt0` injection. |
| `-ffreestanding` | 輸出自成一體、無連結步驟的模組。/ Emit a self-contained module, no linker step. |
| `--map=<path>` | 輸出描述連結結果的連結映射表。/ Write a linker map. |
| `--sysroot=<dir>` | 覆寫 sysroot（亦可用 `WVMCC_SYSROOT`，再者為相對 argv0 的 `../share/wvmcc`）。/ Override the sysroot. |
| `-v` | 詳細模式：將連結階段邊界輸出至 stderr。/ Verbose: link-phase boundaries to stderr. |
| `-h`, `--help` | 顯示說明。/ Show help. |

### 範例 Examples

```sh
# 獨立式單一模組（無 libc、無連結器）
# Freestanding single module (no libc, no linker)
wvmcc -ffreestanding -o hello.wasm hello.c

# 將一個轉譯單元編譯為可連結物件，再與 sysroot 中的 libc 連結
# Compile a TU to a linkable object, then link against libc from a sysroot
wvmcc -c -o foo.o foo.c
wvmcc --sysroot=build/runtime -o app.wasm foo.o

# 以 WasmVM 工具檢視結果 / Inspect the result with the WasmVM tools
readwasm app.wasm
```

---

## 專案結構 Layout

| 路徑 Path | 內容 Contents |
|---|---|
| `src/pp/` | 語彙化器與前處理器（轉譯階段 1–7）。/ Tokenizer and preprocessor (phases 1–7). |
| `src/parser/` | 詞法器、剖析器、AST 與語意分析。/ Lexer, parser, AST, and semantic analysis. |
| `src/codegen/` | AST → `WasmModule` 降階、佈局、符號表、重定位區段。/ AST → `WasmModule` lowering, layout, symbol table, reloc. |
| `src/link/` | 整合式連結器：符號解析、封存／物件載入、重定位、DCE、`crt0`。/ Integrated linker. |
| `src/exec/` | CLI 驅動程式 (`main.cpp`) 與 sysroot 解析。/ CLI driver and sysroot resolution. |
| `runtime/` | 最小化獨立式 libc——標頭 (`include/`) 與原始碼 (`src/`)，建置為 `libc.a`。/ Minimal freestanding libc. |
| `docs/` | 規格、降階計畫、程式碼產生、前處理器設計，與 C17 一致性目錄 (`docs/standard/`)。/ Spec, lowering plan, codegen, preprocessor design, conformance catalog. |
| `tests/` | 單元 (`tests/unit/`)、整合、與標準一致性 (`tests/standard/`) 測試套件。/ Unit, integration, and standard-conformance suites. |

---

## 測試 Tests

```sh
ctest --test-dir build --output-on-failure
```

一致性以 ISO/IEC 9899:2017 為基準，記錄於 [`docs/standard/`](docs/standard/README.md)，
該目錄詳列所有可觸及的 C17 行為（第 4–7 章及附錄 J），並於 `tests/standard/` 下實作。

Conformance is tracked against ISO/IEC 9899:2017 in [`docs/standard/`](docs/standard/README.md),
which catalogs every reachable C17 behavior (Clauses 4–7 plus Annex J), materialized under
`tests/standard/`.

---

## 授權 License

見 [LICENSE](LICENSE)。貢獻請遵循 [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) 與
[CONTRIBUTING.md](CONTRIBUTING.md)。

See [LICENSE](LICENSE). Contributions follow [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) and
[CONTRIBUTING.md](CONTRIBUTING.md).
