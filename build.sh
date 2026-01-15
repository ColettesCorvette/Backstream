#!/usr/bin/env bash

set -e

echo "================================"
echo "  BackStream - Build"
echo "================================"
echo ""

OS=$(uname -s)
echo "Detected OS: $OS"
echo ""

if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is not installed"
    echo "   Ubuntu/Debian: sudo apt install cmake"
    echo "   Fedora/RHEL: sudo dnf install cmake"
    echo "   macOS: brew install cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "ERROR: No C++ compiler found"
    echo "   Ubuntu/Debian: sudo apt install build-essential"
    echo "   Fedora/RHEL: sudo dnf groupinstall 'Development Tools'"
    echo "   macOS: xcode-select --install"
    exit 1
fi

echo "OK CMake: $(cmake --version | head -n1)"
if command -v g++ &> /dev/null; then
    echo "OK Compiler: $(g++ --version | head -n1)"
elif command -v clang++ &> /dev/null; then
    echo "OK Compiler: $(clang++ --version | head -n1)"
fi
echo ""

if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

echo "Creating build directory..."
mkdir -p build
cd build

echo ""
echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo ""
echo "Building in Release mode..."
cmake --build . --config Release -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

if [ $? -ne 0 ]; then
    echo ""
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "================================"
echo "BUILD SUCCESS"
echo "================================"
echo ""
echo "Executable: $(pwd)/Portable/backup"
echo ""

if [ -f "Portable/backup" ]; then
    echo "OK File created successfully"
    ls -lh Portable/backup
else
    echo "WARNING: Executable not found at expected location"
fi

echo ""
echo "Pour tester:"
echo "  cd build/Portable"
echo "  ./backup --help"
echo ""
