#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <filesystem>
#include <vector>

// --- CROSS-PLATFORM ---
#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

namespace fs = std::filesystem;

// Initialisation système
void enableANSI();
void setHighPriority();
int getCPUCoreCount();
uintmax_t getAvailableRAM();

// Utilitaires chemins
std::string getAppDir(char* argv0);
std::string getCurrentDate();
std::string findScpPath();
std::string findZstdPath(const std::string& appDir);

// Validation
bool hasEnoughDiskSpace(const std::string& path, uintmax_t requiredBytes);
bool testSSHConnection(const std::string& scpPath);
bool isValidLevel(const std::string& s);
bool looksLikePath(const std::string& s);

// Traitement arguments
std::string cleanArg(std::string str);
std::string getOptimalZstdParams(int level, uintmax_t availableRAM);

// Pause (Windows & Linux)
void systemPause();

#endif // UTILS_H