#include "utils.h"
#include "config.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

// --- HEADERS ---
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
    #pragma comment(lib, "psapi.lib")
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/sysinfo.h>
    #include <limits>
#endif

void enableANSI() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    // Sur Linux, le support ANSI est natif, rien à faire.
}

void setHighPriority() {
#ifdef _WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#else
    // Sur Linux, augmenter la priorité demande souvent 'sudo', on ignore pour éviter les erreurs.
#endif
}

int getCPUCoreCount() {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

uintmax_t getAvailableRAM() {
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys / (1024 * 1024); // MB
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (pages * page_size) / (1024 * 1024); // MB
#endif
}

std::string getAppDir(char* argv0) {
    fs::path exePath = fs::absolute(argv0);
    return exePath.parent_path().string();
}

std::string findZstdPath(const std::string& appDir) {
#ifdef _WIN32
    std::string binName = "zstd.exe";
#else
    std::string binName = "zstd";
#endif

    std::vector<std::string> searchPaths = {
        (fs::path(appDir) / "tools" / binName).string(), // Dossier tools/ (Structure Pro)
        (fs::path(appDir) / binName).string(),           // Racine
        "/usr/bin/zstd",                                 // Linux standard
        "/usr/local/bin/zstd"                            // Linux local
    };
    
    for (const auto& path : searchPaths) {
        if (fs::exists(path)) {
            return fs::absolute(path).string();
        }
    }
    
    // Si introuvable mais sur Linux, on renvoie juste la commande systeme
#ifndef _WIN32
    return "zstd"; 
#else
    return "";
#endif
}

std::string getCurrentDate() {
    std::time_t now = std::time(nullptr);
    std::tm ltm;
#ifdef _WIN32
    localtime_s(&ltm, &now); // Version sécurisée Windows
#else
    localtime_r(&now, &ltm); // Version Thread-safe Linux (arguments inversés !)
#endif
    std::stringstream ss;
    ss << (1900 + ltm.tm_year) << "-" 
       << std::setw(2) << std::setfill('0') << (1 + ltm.tm_mon) << "-" 
       << std::setw(2) << std::setfill('0') << ltm.tm_mday;
    return ss.str();
}

std::string findScpPath() {
#ifdef _WIN32
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
#else
    return "scp"; // Sur Linux, c'est standard
#endif
}

bool hasEnoughDiskSpace(const std::string& path, uintmax_t requiredBytes) {
    try {
        fs::space_info si = fs::space(path);
        return si.available > requiredBytes * 2;
    } catch (...) {
        return false;
    }
}

bool testSSHConnection(const std::string& scpPath) {
    std::string sshPath = scpPath;
    
#ifdef _WIN32
    // Windows logic
    size_t pos = sshPath.find("scp.exe");
    if (pos != std::string::npos) sshPath.replace(pos, 7, "ssh.exe");
    else sshPath = "ssh";
    
    // cmd.exe /c est nécessaire pour que std::system() fonctionne correctement avec des chemins contenant des espaces
    std::string testCmd = "cmd.exe /c " + sshPath + " -i \"" + SSH_KEY + "\" -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o BatchMode=yes " + REMOTE_USER + "@" + REMOTE_IP + " exit >nul 2>&1";
#else
    // Linux logic
    if (sshPath == "scp") sshPath = "ssh";
    
    // /dev/null pour cacher la sortie
    std::string testCmd = sshPath + " -i \"" + SSH_KEY + "\" -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o BatchMode=yes " + REMOTE_USER + "@" + REMOTE_IP + " exit >/dev/null 2>&1";
#endif
    
    int result = std::system(testCmd.c_str());
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
        params += "=31"; 
    } else if (availableRAM > 8000) {
        params += "=29"; 
    } else {
        params += "=27"; 
    }
    
    params += " -T0";
    return params;
}

void systemPause() {
#ifdef _WIN32
    std::system("pause > nul");
#else
    std::cout << "Appuyez sur Entree pour continuer..." << std::endl;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
#endif
}