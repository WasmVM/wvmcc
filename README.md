# WVMCC

#### 為 [WasmVM](https://github.com/LuisHsu/WasmVM) 打造的編譯器

A C compiler for [WasmVM](https://github.com/LuisHsu/WasmVM)

## 先備條件 Prerequisite

* CMake >= 2.6

* 支援 C11 及標準函式庫的 C++ 編譯器
 
  C compiler supporting c11 standard with Standard Library

## 注意事項 Notice

1. 目前只有前處理器 (wvmcpp) 是可以執行的

  There's only preprocessor (wvmcpp) workable currently.
  
2. **預設的 include 目前還不是 `/usr/include`，而是 `wvmcpp` 相同路徑下的 `include` 資料夾**

  **The default include directory is the `include` directory in the same path of `wvmcpp`, instead of `/usr/include`.**
 
3. 在文件方面，本專案以 **台灣正體中文** 為主要使用語言，英文為次要使用語言，其他語言 （例如: 簡體中文）僅能做為參考或翻譯使用。

  This project uses **"Taiwan Traditional Chinese"** as primary, English as secondary language in documents.
  
  Other languages (Ex: Simplified Chinese) are only used as references or translations.

## 編譯 Build

1. 執行 CMake

> mkdir build && cd build && cmake ..

2. 執行 Make

> make -j4
  
## 執行 Run

1. 準備好以`.wasm`為副檔名的 WebAssembly 位元檔

  Prepare your WebAssembly binary file postfixed with `.wasm`
  
2. 執行 Run

> ./wvmcpp 輸入檔名 輸出檔名

> ./wvmcpp INPUTFILE OUTPUTFILE
