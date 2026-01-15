#include "utils.h"
#include "config.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

void enableANSI() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

void setHighPriority() {
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

int getCPUCoreCount() {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

uintmax_t getAvailableRAM() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys / (1024 * 1024); // MB
}

std::string getAppDir(char* argv0) {
    fs::path exePath = fs::absolute(argv0);
    return exePath.parent_path().string();
}

std::string findZstdPath(const std::string& appDir) {
    std::vector<std::string> searchPaths = {
        appDir + "\\zstd.exe",                          // build/bin/Release
        appDir + "\\..\\..\\..\\zstd.exe",             // racine depuis build/bin/Release
        appDir + "\\..\\..\\..\\bin\\zstd.exe",       // bin depuis build/bin/Release
        "C:\\Compresseur\\zstd.exe",                   // racine absolue
        "C:\\Compresseur\\bin\\zstd.exe"              // bin absolue
    };
    
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return fs::absolute(path).string();
        }
    }
    
    return "";
}

std::string getCurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
    localtime_s(&ltm, &now);
    std::stringstream ss;
    ss << (1900 + ltm.tm_year) << "-" 
       << std::setw(2) << std::setfill('0') << (1 + ltm.tm_mon) << "-" 
       << std::setw(2) << std::setfill('0') << ltm.tm_mday;
    return ss.str();
}

std::string findScpPath() {
    std::vector<std::string> candidates = {
        "C:\\Windows\\Sysnative\\OpenSSH\\scp.exe", 
        "C:\\Windows\\System32\\OpenSSH\\scp.exe", 
        "scp" 
    };
    for (const auto& path : candidates) {
        if (path == "scp") return path; 
        if (fs::exists(path)) return path;
    }
    return "";
}

bool hasEnoughDiskSpace(const std::string& path, uintmax_t requiredBytes) {
    try {
        fs::space_info si = fs::space(path);
        return si.available > requiredBytes * 2;
    } catch (...) {
        return false;
    }
}

// Dans utils.cpp

bool testSSHConnection(const std::string& scpPath) {
    std::string sshPath = scpPath;
    size_t pos = sshPath.find("scp.exe");
    if (pos != std::string::npos) {
        sshPath.replace(pos, 7, "ssh.exe");
    } else {
        sshPath = "ssh";
    }
    
    std::string testCmd = "\"" + sshPath + "\" -i \"" + SSH_KEY + "\" -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o BatchMode=yes " + REMOTE_USER + "@" + REMOTE_IP + " \"exit 0\" < NUL >nul 2>&1";
    int result = std::system(("cmd.exe /c \"" + testCmd + "\"").c_str());
    return result == 0;
}

bool isValidLevel(const std::string& s) {
    if (s.empty()) return false;
    if (std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) != s.end()) return false;
    try {
        int lvl = std::stoi(s);
        return lvl >= 1 && lvl <= 22;
    } catch (...) { return false; }
}

bool looksLikePath(const std::string& s) {
    return s.find('\\') != std::string::npos || s.find('/') != std::string::npos;
}

std::string cleanArg(std::string str) {
    if (str.empty()) return "";
    if (str.front() == '"' || str.front() == '\'') str.erase(0, 1);
    if (!str.empty() && (str.back() == '"' || str.back() == '\'')) str.pop_back();
    while (!str.empty() && (str.back() == '\\' || str.back() == '/')) {
        str.pop_back();
    }
    return str;
}

std::string getOptimalZstdParams(int level, uintmax_t availableRAM) {
    std::string params = "-" + std::to_string(level) + " --long";
    
    if (availableRAM > 16000) {
        params += "=31"; // 2GB window
    } else if (availableRAM > 8000) {
        params += "=29"; // 512MB window
    } else {
        params += "=27"; // 128MB window
    }
    
    params += " -T0";
    return params;
}
