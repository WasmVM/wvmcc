#!/bin/bash
# Install complexity analysis tools for wvmcc

echo "Installing code complexity tools..."

# Check if Python3 is available
if ! command -v python3 &> /dev/null; then
    echo "Error: Python3 is required but not found."
    exit 1
fi

# Install lizard (cyclomatic complexity)
echo "Installing lizard..."
python3 -m pip install lizard

# Optional: Install other analysis tools
echo ""
echo "Optional tools (uncomment to install):"
echo "# python3 -m pip install radon        # Code metrics"
echo "# python3 -m pip install mccabe       # Complexity checker"
echo "# brew install cloc                   # Count Lines of Code (macOS)"

echo ""
echo "Installation complete!"
echo "Run 'make complexity' in your build directory to analyze code."
