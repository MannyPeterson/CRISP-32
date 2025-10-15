# VSCode Configuration

This folder contains VSCode configuration files for building and debugging the CRISP-32 VM.

## Files

- **c_cpp_properties.json** - IntelliSense configuration for C89
- **tasks.json** - Build tasks
- **launch.json** - Debug configurations

## Build Tasks

Available tasks (Terminal → Run Build Task):

1. **Build CRISP-32** (Ctrl+Shift+B) - Default release build with optimizations
2. **Build CRISP-32 (Debug)** - Debug build with symbols and no optimizations
3. **Clean Build** - Remove all build artifacts
4. **Rebuild All** - Clean and rebuild everything

## Debug Configurations

Available configurations (Run → Start Debugging or F5):

1. **Debug CRISP-32** - Build with debug symbols and start debugging session
   - Automatically builds debug version before launching
   - Stops at breakpoints
   - Enables GDB pretty-printing

2. **Run CRISP-32 (No Debug)** - Build and run without debugging
   - Faster execution
   - Still builds before running

## Usage

### Building
- Press `Ctrl+Shift+B` to build (release mode)
- Or use Command Palette: `Tasks: Run Build Task`

### Debugging
1. Set breakpoints in source files (click left of line numbers)
2. Press `F5` or Run → Start Debugging
3. Use debug controls:
   - `F5` - Continue
   - `F10` - Step Over
   - `F11` - Step Into
   - `Shift+F11` - Step Out
   - `Shift+F5` - Stop

### Command Line Alternatives
```bash
make              # Release build (optimized)
make debug        # Debug build (with symbols)
make release      # Explicit release build
make clean        # Clean build artifacts
```

## IntelliSense

The C/C++ extension is configured for strict C89 compliance:
- C89 standard
- Freestanding environment
- Custom include paths
- All warnings enabled

IntelliSense will show errors/warnings as you type, matching the build system.
