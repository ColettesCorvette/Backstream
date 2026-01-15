#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

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

#endif // UTILS_H
