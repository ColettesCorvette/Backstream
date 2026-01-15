# BackStream

High-performance automated backup tool with zstd compression and secure SSH transfer for Windows and Linux.

## Overview

BackStream is a command-line utility designed to automate the backup workflow:
1. Compress directories using zstd multi-threaded compression
2. Transfer archives to a remote server via SSH/SCP with automatic retry
3. Clean up local archives after successful upload
4. Provide clear, timestamped logs for each operation phase

The tool optimizes compression parameters based on available system resources (CPU cores and RAM) and supports parallel backup jobs for efficient batch operations.

## Features

- **Optimized zstd compression**: Multi-threaded with automatic parameter tuning based on system resources
- **Secure SSH transfer**: SCP upload with 3-attempt retry mechanism and network speed display
- **Timestamped logging**: Clear format `[HH:MM:SS] [JOB X] [PHASE] Message` for easy monitoring
- **Parallel processing**: Multiple backup jobs executed concurrently with intelligent resource management
- **Interruption handling**: Clean Ctrl+C support with automatic temporary file cleanup
- **Comprehensive validation**: Disk space, SSH connectivity, path verification, and argument parsing
- **Automatic detection**: Searches for zstd.exe in multiple locations
- **Detailed statistics**: Size, duration, compression ratio, and network transfer speed

## Prerequisites

### Windows
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.15+
- OpenSSH Client (included in Windows 10/11)
- zstd.exe from [zstd releases](https://github.com/facebook/zstd/releases)

### Linux
- GCC or Clang compiler
- CMake 3.15+
- OpenSSH client
- zstd package

### SSH Key Setup

Generate an ED25519 SSH key:
```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519
```

Copy the public key to your remote server:
```bash
ssh-copy-id user@remote-server
```

## Configuration

On first run, BackStream will automatically prompt for configuration:

1. **SSH Username**: Username for remote server
2. **Remote IP**: IP address or hostname of backup server
3. **Remote Path**: Destination directory on server (e.g., `/mnt/backups`)
4. **SSH Key Path**: Full path to your SSH private key
5. **Default Compression Level**: 1-22 (recommended: 3 for speed, 19 for size)

Configuration is saved to `settings.ini` in the executable directory.

### Example settings.ini
```ini
[General]
REMOTE_USER=your-username
REMOTE_IP=192.168.1.100
REMOTE_PATH=/path/to/backups
SSH_KEY=C:\Users\YourName\.ssh\id_ed25519
DEFAULT_LEVEL=3
```

You can manually edit this file or delete it to reconfigure.

## Building from Source

No configuration needed before building - setup happens on first run.

### Windows

Using PowerShell:
```powershell
.\build.ps1
```

Or double-click `build-quick.bat`

### Linux

```bash
chmod +x build.sh
./build.sh
```

### Manual Build

**Windows:**
```powershell
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

**Linux:**
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

The compiled executable will be located in `build/Portable/`.

## Installation

### Windows - Add to PATH

Run as Administrator:
```powershell
.\install-path.ps1
```

This adds the executable directory to your system PATH, allowing you to run `backup` from any location.

### Linux

```bash
sudo cp build/Portable/backup /usr/local/bin/
sudo chmod +x /usr/local/bin/backup
```

## Usage

### Basic Syntax

```bash
backup <directory> [custom_name] [compression_level]
```

### Examples

```bash
# Backup with default settings
backup "D:\Games\Cyberpunk 2077"

# Backup with custom compression level
backup "D:\Games\GTA V" 19

# Backup with custom name and compression level
backup "D:\Data\Photos" "Photos-2024" 3

# Multiple backups (parallel execution)
backup "D:\Games\Game1" "D:\Games\Game2" "D:\Games\Game3"
```

### Compression Levels

| Level | Speed | Ratio | Use Case |
|-------|-------|-------|----------|
| 1-3   | Very fast | 60% | Daily backups, large volumes |
| 10-15 | Fast | 50% | Balanced speed/size |
| 19    | Slow | 35% | Maximum compression |
| 22    | Very slow | 30% | Long-term archives |

## Output Format

```
[14:23:45] [SYSTEM] Demarrage - 16 cores CPU, 18253 MB RAM disponibles
[14:23:45] [SYSTEM] Configuration chargee depuis settings.ini
[14:23:46] [SYSTEM] Test connexion SSH vers 192.168.1.100...
[14:23:46] [SYSTEM] Connexion SSH OK!
[14:23:46] [SYSTEM] Lancement de 1 tache(s) - 1 en parallele max
============================================================
[14:23:46] [JOB 1] [INIT] Demarrage backup: F:\Data\Project
[14:23:46] [JOB 1] [INIT] Calcul de la taille du dossier...
[14:23:47] [JOB 1] [INIT] Taille totale: 19 GB
[14:23:47] [JOB 1] [COMPRESS] Debut compression (niveau 3)
[14:24:08] [JOB 1] [COMPRESS] Termine en 21s - 19 GB (ratio: 100%)
[14:24:08] [JOB 1] [UPLOAD] Debut transfert vers 192.168.1.100
[14:24:12] [JOB 1] [UPLOAD] 10% - 85.2MB/s
[14:24:16] [JOB 1] [UPLOAD] 20% - 87.1MB/s
[14:28:02] [JOB 1] [UPLOAD] Termine en 234s
[14:28:02] [JOB 1] [CLEANUP] Archive locale supprimee
[14:28:02] [JOB 1] [DONE] Backup termine avec succes!
============================================================
[14:28:02] [SYSTEM] TOUS LES BACKUPS ONT REUSSI !
```

## Archive Format

Archives are created in the format:
```
DirectoryName_YYYY-MM-DD.tar.zst
```

### Extraction

On the remote server:
```bash
zstd -d archive.tar.zst
tar -xf archive.tar
```

Or in one command:
```bash
zstd -dc archive.tar.zst | tar -xf -
```

## Execution Phases

1. **INIT**: Directory validation, size calculation, disk space verification
2. **COMPRESS**: tar + zstd compression with automatic optimization
3. **UPLOAD**: SCP transfer with retry (3 attempts) and progress display
4. **CLEANUP**: Local archive deletion (preserved on upload failure)
5. **DONE**: Success confirmation

## Error Handling

### Common Errors

**zstd.exe not found**
```
[ERROR] zstd.exe not found
[INFO] Copy zstd.exe to C:\Compresseur or C:\Compresseur\bin
```
Solution: Download and place zstd.exe in the specified location.

**SSH connection failed**
```
[ERROR] Cannot connect to user@192.168.1.100
[INFO] Verify: SSH key authorized, server accessible, correct credentials
```
Solution: Check SSH key, network connectivity, and server availability.

**Directory not found**
```
[ERROR] Directory not found: D:\Games\Test
[INFO] Verify there is no trailing backslash (\) in the path
```
Solution: Remove trailing backslash or verify the path.

**Insufficient disk space**
```
[ERROR] Insufficient disk space
```
Solution: Free up disk space or change destination.

### Interruption (Ctrl+C)

The program handles interruptions gracefully:
- Stops ongoing processes
- Cleans up temporary files
- Preserves partial archives when necessary
- Provides clear log messages

## Performance

### Automatic Optimizations

- **zstd threads**: Adjusted based on available CPU cores
- **Memory**: Parameters adapted to available RAM
- **Parallel jobs**: Calculated as `cores / 4` (max `MAX_PARALLEL_JOBS`)
- **Process priority**: `HIGH_PRIORITY_CLASS` for maximum performance
- **Buffer size**: 8KB optimized for command output reading

### Typical Benchmarks

| Source Size | Level | Compression Time | Ratio | Network Speed |
|-------------|-------|------------------|-------|---------------|
| 19 GB | 3 | 21s | 100% | 85 MB/s |
| 50 GB | 10 | 180s | 50% | 90 MB/s |
| 100 GB | 19 | 685s | 44% | 88 MB/s |

*Tests on: 16 cores, 32GB RAM, NVMe SSD, 1Gbps LAN*

## Project Structure

```
├── src/
│   ├── main.cpp           # Entry point and orchestration
│   ├── backup.cpp         # Core backup logic
│   ├── config.cpp         # Configuration management
│   ├── utils.cpp          # System utilities
│   └── progress.cpp       # Logging system
├── include/
│   ├── backup.h
│   ├── config.h
│   ├── utils.h
│   └── progress.h
├── build/                 # CMake build directory
│   └── Portable/
│       ├── backup.exe     # Compiled executable
│       └── zstd.exe       # Auto-copied
├── tools/
│   └── zstd.exe           # Place zstd.exe here
├── CMakeLists.txt         # CMake configuration
├── build.ps1              # Windows build script
├── build.sh               # Linux build script
└── README.md
```

## Development

### Source Files

- **main.cpp**: Argument parsing, orchestration, initial validation
- **backup.cpp**: Backup logic, compression, upload, retry mechanism
- **utils.cpp**: System detection, paths, SSH, zstd optimization
- **progress.cpp**: Thread-safe logging system with timestamps
- **config.h/cpp**: Configuration loading/saving from settings.ini

### Quick Rebuild

```powershell
cd build
cmake --build . --config Release
```

## Troubleshooting

### Configuration Issues

**Need to reconfigure**
- Delete `settings.ini` in the executable directory
- Run the program again to enter new settings

**SSH connection fails on first run**
- Verify SSH key path is correct and absolute
- Test manually: `ssh -i <key_path> user@server`
- Ensure public key is in server's `~/.ssh/authorized_keys`

### Program Issues

**Program won't start**
- Ensure you're using Developer PowerShell for VS 2022 (Windows)
- Verify zstd.exe is present in the executable directory

**Compression very slow**
- Reduce compression level (3-10 recommended)
- Check disk isn't saturated
- Close CPU-intensive applications

**Upload always fails**
- Test SSH connection manually: `ssh user@server`
- Verify remote path exists and has write permissions
- Check server network accessibility
- Review settings.ini for typos

**Corrupted archives**
- Verify sufficient disk space
- Don't interrupt during compression
- Test archive integrity: `zstd -t archive.tar.zst`

## License

Personal project - Free to use

## Author

January 2026
