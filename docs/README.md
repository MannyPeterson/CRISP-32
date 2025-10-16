# CRISP-32 Documentation

This directory contains configuration for generating API documentation using Doxygen.

## Prerequisites

Install Doxygen on your system:

```bash
# Ubuntu/Debian
sudo apt-get install doxygen

# macOS
brew install doxygen

# Fedora/RHEL
sudo dnf install doxygen
```

## Generating Documentation

From the project root directory, run:

```bash
doxygen
```

This will generate HTML documentation in `docs/html/`.

## Viewing Documentation

Open the generated documentation:

```bash
# Linux
xdg-open docs/html/index.html

# macOS
open docs/html/index.html

# Windows
start docs/html/index.html
```

## Documentation Coverage

The generated documentation includes:

- **VM Core** (`c32_vm.h`, `c32_vm.c`) - Virtual machine implementation
- **Assembler** (`c32_asm.h`) - Two-pass assembler API
- **Types** (`c32_types.h`) - Freestanding C89 type definitions
- **Opcodes** (`c32_opcodes.h`) - All 80+ instruction opcodes
- **String Functions** (`c32_string.h`) - Freestanding string/memory operations
- **Test Framework** (`c32_test.h`) - Unit testing framework

## Configuration

The Doxyfile is located in the project root and is configured with:

- **Output Format**: HTML (by default)
- **Input Directories**: `include/`, `src/vm/`, `src/common/`, `src/test/`, `src/asm/`
- **Special Files**: `README.md`, `docs/crisp32-spec.md`
- **Optimization**: Configured for C projects
- **Source Browsing**: Enabled
- **Search Engine**: Enabled

## Customization

To customize the documentation output, edit the `Doxyfile` in the project root:

- Change `PROJECT_NUMBER` to update the version
- Modify `HTML_COLORSTYLE_*` for different color schemes
- Enable `GENERATE_LATEX = YES` for PDF output
- Enable `GENERATE_MAN = YES` for man pages

## Cleaning Documentation

To remove generated documentation:

```bash
rm -rf docs/html docs/latex
```

## Notes

- All header files use Doxygen-style comments (@file, @brief, @param, etc.)
- The documentation is organized into logical groups using @defgroup
- Examples from `src/test/unit/*.asm` are automatically included
