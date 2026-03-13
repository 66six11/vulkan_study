#!/usr/bin/env python3
"""
Vulkan Engine - Add BOM to Code Files

This script adds UTF-8 BOM (Byte Order Mark) to all code files in the project.
UTF-8 BOM is: EF BB BF (3 bytes)

Usage:
    python add_bom.py              # Add BOM to all code files
    python add_bom.py --check     # Check which files need BOM
    python add_bom.py --remove    # Remove BOM from all files
"""

import os
import sys
from pathlib import Path

# UTF-8 BOM (3 bytes)
UTF8_BOM = b'\xef\xbb\xbf'

# File extensions to process
CODE_EXTENSIONS = {
    # C/C++ files
    '.cpp', '.hpp', '.c', '.h', '.cxx', '.hxx',
    # CMake files
    '.cmake', '.txt',
    # Python files
    '.py',
    # Shell/Batch files
    '.sh', '.bat',
    # Markdown files
    '.md',
    # JSON files
    '.json',
    # GLSL shader files
    '.vert', '.frag', '.geom', '.comp', '.tesc', '.tese',
}

# Directories to exclude
EXCLUDE_DIRS = {
    'build', 'build_vs', 'build_test', 'build_conan',
    '.git', '.vs', '.vscode',
    'CMakeFiles', 'CMakeCache.txt',
    '__pycache__', '*.egg-info',
}

# Files to exclude
EXCLUDE_FILES = {
    'vcpkg.json',  # If exists, keep as UTF-8 without BOM
}


def has_bom(file_path):
    """Check if file already has UTF-8 BOM."""
    try:
        with open(file_path, 'rb') as f:
            first_three = f.read(3)
            return first_three == UTF8_BOM
    except Exception as e:
        print(f"Error reading {file_path}: {e}")
        return False


def add_bom(file_path):
    """Add UTF-8 BOM to file if it doesn't already have one."""
    if has_bom(file_path):
        return False, "Already has BOM"

    try:
        # Read file content
        with open(file_path, 'rb') as f:
            content = f.read()

        # Write back with BOM
        with open(file_path, 'wb') as f:
            f.write(UTF8_BOM + content)

        return True, "BOM added"
    except Exception as e:
        return False, f"Error: {e}"


def remove_bom(file_path):
    """Remove UTF-8 BOM from file if it exists."""
    if not has_bom(file_path):
        return False, "No BOM to remove"

    try:
        # Read file content (skip BOM)
        with open(file_path, 'rb') as f:
            f.read(3)  # Skip BOM
            content = f.read()

        # Write back without BOM
        with open(file_path, 'wb') as f:
            f.write(content)

        return True, "BOM removed"
    except Exception as e:
        return False, f"Error: {e}"


def should_process_file(file_path):
    """Check if file should be processed."""
    # Check file extension
    if file_path.suffix.lower() not in CODE_EXTENSIONS:
        return False

    # Check if file is in exclude list
    if file_path.name in EXCLUDE_FILES:
        return False

    # Check if file is in exclude directory
    for part in file_path.parts:
        if part in EXCLUDE_DIRS or part.startswith('.'):
            return False

    return True


def find_code_files(root_dir):
    """Find all code files in the project."""
    root_path = Path(root_dir)
    code_files = []

    for file_path in root_path.rglob('*'):
        if file_path.is_file() and should_process_file(file_path):
            code_files.append(file_path)

    return sorted(code_files)


def main():
    """Main function."""
    import argparse

    parser = argparse.ArgumentParser(
        description='Add/Remove/Check UTF-8 BOM for code files',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python add_bom.py              # Add BOM to all code files
  python add_bom.py --check     # Check which files need BOM
  python add_bom.py --remove    # Remove BOM from all files
  python add_bom.py --dry-run   # Show what would be done without changing files
        """
    )

    parser.add_argument(
        '--check', '-c',
        action='store_true',
        help='Check which files need BOM (read-only)'
    )

    parser.add_argument(
        '--remove', '-r',
        action='store_true',
        help='Remove BOM from files instead of adding'
    )

    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be done without making changes'
    )

    parser.add_argument(
        '--dir', '-d',
        default='.',
        help='Root directory to process (default: current directory)'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Show detailed output'
    )

    args = parser.parse_args()

    # Find all code files
    root_dir = Path(args.dir).resolve()
    print(f"Scanning directory: {root_dir}")
    print()

    code_files = find_code_files(root_dir)

    if not code_files:
        print("No code files found to process.")
        return 0

    print(f"Found {len(code_files)} code files")
    print()

    if args.check:
        # Check mode - only show which files need BOM
        files_without_bom = []
        for file_path in code_files:
            if not has_bom(file_path):
                files_without_bom.append(file_path)

        if files_without_bom:
            print(f"Files without BOM ({len(files_without_bom)}):")
            for file_path in files_without_bom:
                print(f"  {file_path}")
            print()
            print(f"Total: {len(files_without_bom)} files need BOM")
        else:
            print("All files already have BOM!")
        return 0

    if args.remove:
        # Remove BOM mode
        action_name = "Removing BOM"
        action_func = remove_bom
    else:
        # Add BOM mode (default)
        action_name = "Adding BOM"
        action_func = add_bom

    if args.dry_run:
        print(f"[DRY RUN] Would {action_name.lower()} from {len(code_files)} files:")
        print()

    success_count = 0
    skip_count = 0
    error_count = 0

    for file_path in code_files:
        if args.dry_run:
            print(f"  {file_path}")
            continue

        success, message = action_func(file_path)

        if success:
            success_count += 1
            if args.verbose:
                print(f"[OK] {file_path} - {message}")
        else:
            if "Already" in message or "No BOM" in message:
                skip_count += 1
            else:
                error_count += 1
                skip_or_error = 'SKIP' if ('Already' in message or 'No BOM' in message) else 'ERROR'
                print(f"[{skip_or_error}] {file_path} - {message}")

    if not args.dry_run:
        print()
        print("=" * 60)
        print(f"Summary: {action_name}")
        print("=" * 60)
        print(f"  Total files:  {len(code_files)}")
        print(f"  Success:      {success_count}")
        print(f"  Skipped:      {skip_count}")
        print(f"  Errors:       {error_count}")
        print()

        if error_count == 0:
            print("All operations completed successfully!")
        else:
            print(f"{error_count} errors occurred")
            return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
