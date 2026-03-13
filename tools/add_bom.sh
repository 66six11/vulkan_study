#!/bin/bash
# Vulkan Engine - Add BOM to Code Files
# This script adds UTF-8 BOM to all code files in the project

echo "========================================"
echo "Vulkan Engine - Add BOM Tool"
echo "========================================"
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null && ! command -v python &> /dev/null; then
    echo "[ERROR] Python not found!"
    echo "Please install Python 3.6 or later."
    echo ""
    exit 1
fi

# Use python3 if available, otherwise python
PYTHON_CMD="python3"
if ! command -v python3 &> /dev/null; then
    PYTHON_CMD="python"
fi

echo "Python found: $($PYTHON_CMD --version)"
echo ""

# Parse command line arguments
MODE="ADD"
case "$1" in
    --check|-c)
        MODE="CHECK"
        ;;
    --remove|-r)
        MODE="REMOVE"
        ;;
    --dry-run|-d)
        MODE="DRYRUN"
        ;;
esac

echo "Mode: $MODE"
echo ""

# Execute Python script
case "$MODE" in
    CHECK)
        echo "Checking which files need BOM..."
        echo ""
        $PYTHON_CMD add_bom.py --check
        ;;
    REMOVE)
        echo "Removing BOM from code files..."
        echo ""
        $PYTHON_CMD add_bom.py --remove --verbose
        ;;
    DRYRUN)
        echo "Dry run - showing what would be done..."
        echo ""
        $PYTHON_CMD add_bom.py --dry-run
        ;;
    *)
        echo "Adding BOM to code files..."
        echo ""
        $PYTHON_CMD add_bom.py --verbose
        ;;
esac

if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Script failed"
    echo ""
    exit 1
fi

echo ""
echo "========================================"
echo "Operation completed successfully!"
echo "========================================"
echo ""
