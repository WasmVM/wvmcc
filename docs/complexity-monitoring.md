# Code Complexity Monitoring

## Overview

This project includes automated code complexity monitoring integrated into the CMake build system. These tools help maintain code quality by identifying overly complex functions and files.

## Available Targets

### Basic Usage

After configuring with CMake, run these commands from your build directory:

```bash
# View complexity analysis
make complexity

# Generate HTML complexity report
make complexity-report

# Check complexity against thresholds
make complexity-check

# Count lines of code
make loc

# Combined metrics (LOC + complexity)
make metrics
```

## Metrics Explained

### Cyclomatic Complexity (CCN)
- **< 10**: Simple function, low risk
- **10-20**: Moderate complexity, medium risk
- **20-50**: Complex function, high risk
- **> 50**: Very complex, very high risk - consider refactoring

### Function Length
- **< 50 lines**: Good
- **50-100 lines**: Consider splitting
- **> 100 lines**: Should be refactored

### Parameter Count
- **< 4**: Good
- **4-7**: Acceptable
- **> 7**: Too many, consider using structs/objects

## Thresholds

The `complexity-check` target enforces these thresholds:
- **CCN < 15**: Cyclomatic complexity
- **Length < 1000**: Maximum lines per function
- **Args < 5**: Maximum function parameters

## Installation

### Install Lizard (Required)

```bash
# Using pip
pip install lizard

# Or run the install script
chmod +x scripts/install-complexity-tools.sh
./scripts/install-complexity-tools.sh
```

### Optional Tools

```bash
# For more detailed metrics
pip install radon

# For even more analysis
brew install cloc  # macOS
apt-get install cloc  # Ubuntu/Debian
```

## Integration with CI/CD

Add to your CI pipeline:

```yaml
# Example for GitHub Actions
- name: Check Code Complexity
  run: |
    cd build
    make complexity-check
```

## Example Output

```
=== Source Lines of Code ===
   Total: 2847 lines

=== Complexity Analysis ===
NLOC    CCN   token  PARAM  length  location  
------  -----  -----  -----  ------  --------------------------------------
    45      8     89      3      52  Preprocessor::run@src/pp/Preprocessor.cpp
    32      6     78      2      38  Tokenizer::nextToken@src/pp/Tokenizer.cpp
    28      5     65      1      35  MacroTable::expandMacros@src/pp/MacroTable.cpp
```

## Recommendations

1. **Run before commits**: `make complexity-check`
2. **Review weekly**: `make complexity-report` and review HTML output
3. **Set CI gates**: Fail builds if CCN > 20
4. **Refactor regularly**: Target CCN < 10 for critical functions

## Tools Reference

- **Lizard**: https://github.com/terryyin/lizard
- **Radon**: https://radon.readthedocs.io/
- **CLOC**: https://github.com/AlDanial/cloc
