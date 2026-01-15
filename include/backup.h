#ifndef BACKUP_H
#define BACKUP_H

#include <string>
#include <vector>
#include <mutex>
#include <atomic>

struct BackupJob {
    std::string sourceDir;
    std::string baseName;
    std::string level;
    int id;
};

// Variables globales
extern std::atomic<bool> programInterrupted;
extern std::vector<std::string> failedJobs;
extern std::mutex failedJobsMutex;

// Fonctions principales
void signalHandler(int signal);
std::string runCommandWithProgress(const std::string& cmd, int jobId, 
                                   const std::string& phase, int maxRetries = 3);
void runBackupJob(BackupJob job, std::string zstdPath, std::string scpPath);

#endif // BACKUP_H
