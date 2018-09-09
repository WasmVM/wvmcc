# WVMCC

[![Build Status](https://travis-ci.org/LuisHsu/wvmcc.svg?branch=master)](https://travis-ci.org/LuisHsu/wvmcc)

#### 為 [WasmVM](https://github.com/LuisHsu/WasmVM) 打造的編譯器

A C compiler for [WasmVM](https://github.com/LuisHsu/WasmVM)


## 先備條件 Prerequisite

* CMake >= 2.6

* 支援 C11 及標準函式庫的 C++ 編譯器
 
  C compiler supporting c11 standard with Standard Library
  
### 以下為開發者模式需要 For Developer build
  
* SkyPat >= 3.1.1

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

### 開發者模式 Developer build

1. 執行 CMake

> mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..

2. 執行 Make

> make -j4

## 安裝 & 執行 Install & run

1. 安裝

> make install

2. 執行

> wvmcc

## 測試 Tests

* 單元測試 Unit tests

> ctest
