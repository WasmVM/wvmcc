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
 
2. 在文件方面，本專案以 **台灣正體中文** 為主要使用語言，英文為次要使用語言，其他語言 （例如: 簡體中文）僅能做為參考或翻譯使用。

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