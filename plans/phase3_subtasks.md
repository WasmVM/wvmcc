# Phase 3: Control Flow Completeness Implementation Subtasks

## Overview
This document outlines the detailed implementation subtasks for Phase 3 of the C17 to WasmVM lowering pipeline, which adds support for complete control flow handling including switch statements, loops, and proper branching.

## Subtasks

### Step 3.1 — ControlFlowStack implementation
- [ ] Add `ControlFlowStack` to `FunctionCodegen`:
  - Implement stack-based tracking of current control flow context (loops, switches)
  - Track break/continue targets for each control structure
  - Track switch case labels and default handling
- [ ] Update `FunctionCodegen` to maintain this stack during code generation
- [ ] Implement helper methods for pushing/popping control flow contexts

### Step 3.2 — Switch statement handling
- [ ] Implement `emitStmt` for:
  - `Switch` statements → emit switch logic with proper case handling
- [ ] Implement `SwitchCaseCollector`:
  - Collect all switch cases and their labels
  - Determine if switch is dense or sparse based on case distribution
- [ ] Implement dense case handling (`max−min ≤ 4 × num_cases`):
  - Generate jump tables for efficient case dispatch
  - Handle fall-through between cases by not emitting `Br` at case end
- [ ] Implement sparse case handling (`max−min > 4 × num_cases`):
  - Generate if-else chains for sparse case distributions
  - Properly handle break/continue in switch contexts

### Step 3.3 — Loop statement handling
- [ ] Implement `emitStmt` for:
  - `While` loops → emit condition check and loop body
  - `DoWhile` loops → emit body first, then condition check
  - `For` loops → emit initialization, condition, and increment
- [ ] Implement proper break/continue handling:
  - Track loop targets for break/continue statements
  - Generate appropriate branch instructions to loop exit or continue point

### Step 3.4 — Expression handling for control flow
- [ ] Implement `emitExpr` for:
  - `Break` expressions → emit branch to outermost loop break target
  - `Continue` expressions → emit branch to innermost loop continue target
- [ ] Implement `emitExpr` for:
  - `Goto` expressions → emit branch to labeled target (if supported)
- [ ] Implement `emitExpr` for:
  - `Label` expressions → emit label definitions for goto targets

### Step 3.5 — Verification and testing
- [ ] Compile switch/loop-heavy C file; inspect control flow with `readwasm --func output.wasm`
- [ ] Run via `wasmvm output.wasm`; regression: all unit tests pass