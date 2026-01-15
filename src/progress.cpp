#include "progress.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>

std::mutex logMutex;

std::string getCurrentTime() {
    auto now = std::time(nullptr);
    struct tm tm;
    
    #ifdef _WIN32
        localtime_s(&tm, &now); // Windows (safe)
    #else
        localtime_r(&now, &tm); // Linux (thread-safe)
    #endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

void log(int jobId, const std::string& phase, const std::string& msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    std::cout << "[" << getCurrentTime() << "] ";
    
    if (jobId >= 0) {
        std::cout << "[JOB " << jobId << "] ";
    }
    
    if (!phase.empty()) {
        std::cout << "[" << phase << "] ";
    }
    
    std::cout << msg << std::endl;
}