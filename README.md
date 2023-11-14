# WVMCC

#### 為 [WasmVM](https://github.com/WasmVM/WasmVM) 打造的編譯器

A C compiler for [WasmVM](https://github.com/WasmVM/WasmVM)


## 先備條件 Prerequisite

* CMake >= 3.16

* 支援 C11 和 C++20 的 C/C++ 編譯器
 
  C/C++ compiler supporting c11 and c++20 standard with Standard Library

## 注意事項 Notice

1. 在文件方面，本專案以 **台灣正體中文** 為主要使用語言，英文為次要使用語言，其他語言 （例如: 簡體中文）僅能做為參考或翻譯使用。

  This project uses **"Taiwan Traditional Chinese"** as primary, English as secondary language in documents.
  
  Other languages (Ex: Simplified Chinese) are only used as references or translations.

## 編譯 Build

### 一般模式 Normal build

1. 執行 CMake

> mkdir build && cd build && cmake ..

2. 執行 Make

> make -j4

3. 安裝

> make install

## 安裝 & 執行 Install & run

1. 安裝

> make install

2. 執行

> wvmcc
