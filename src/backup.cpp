#include "backup.h"
#include "config.h"
#include "utils.h"
#include "progress.h"

// --- CROSS-PLATFORM ---
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #include <unistd.h>
    #define POPEN popen
    #define PCLOSE pclose
    #include <sys/wait.h> // Pour WEXITSTATUS
#endif
// ---------------------------------

#include <csignal>
#include <regex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;
using namespace std::chrono;

std::atomic<bool> programInterrupted(false);
std::vector<std::string> failedJobs;
std::mutex failedJobsMutex;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        programInterrupted = true;
        log(-1, "SYSTEM", "INTERRUPTION - Arret en cours...");
    }
}

std::string runCommandWithProgress(const std::string& cmd, int jobId, 
                                   const std::string& phase, int maxRetries) {
    static const std::regex percentRegex(R"((\d+)%)");
    static const std::regex scpSpeedRegex(R"((\d+)%\s+\S+\s+([\d.]+[KMG]B/s))");
    
    for (int attempt = 1; attempt <= maxRetries; ++attempt) {
        if (programInterrupted) return "INTERRUPTED";
        
        if (attempt > 1) {
            log(jobId, phase, "Tentative " + std::to_string(attempt) + "/" + std::to_string(maxRetries));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        FILE* pipe = POPEN(cmd.c_str(), "r");
        if (!pipe) {
            log(jobId, phase, "Erreur: impossible d'ouvrir le pipe");
            if (attempt == maxRetries) return "PIPE_FAILED";
            continue;
        }
        
        char buffer[PIPE_BUFFER_SIZE];
        std::string output;
        // output.reserve(4096); 
        int lastPercent = -1;
        
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            if (programInterrupted) {
                PCLOSE(pipe);
                return "INTERRUPTED";
            }
            
            std::string line(buffer);
            output += line;
            
            if (phase == "UPLOAD") {
                std::smatch match;
                if (std::regex_search(line, match, scpSpeedRegex)) {
                    int pct = std::stoi(match[1]);
                    std::string speed = match[2].str();
                    if (pct % 10 == 0 && pct != lastPercent) {
                        log(jobId, phase, std::to_string(pct) + "% - " + speed);
                        lastPercent = pct;
                    }
                }
                else if (std::regex_search(line, match, percentRegex)) {
                    int pct = std::stoi(match[1]);
                    if (pct % 10 == 0 && pct != lastPercent) {
                        log(jobId, phase, std::to_string(pct) + "%");
                        lastPercent = pct;
                    }
                }
            }
        }
        
        int result = PCLOSE(pipe);
        
        // Sur Linux, il faut extraire le vrai code de retour
        #ifndef _WIN32
        if (WIFEXITED(result)) result = WEXITSTATUS(result);
        #endif

        if (result == 0) {
            return output;
        }
        
        if (attempt < maxRetries && phase == "UPLOAD") {
            log(jobId, phase, "Echec (code " + std::to_string(result) + "), nouvelle tentative...");
        }
    }
    
    return "FAILED_AFTER_RETRIES";
}

void runBackupJob(BackupJob job, std::string zstdPath, std::string scpPath) {
    // Thread Priority (Windows seulement)
    #ifdef _WIN32
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    #endif
    
    std::string pathToZstd = "\"" + zstdPath + "\"";
    std::string dateStr = getCurrentDate();
    std::string archiveName = job.baseName + "_" + dateStr + ".tar.zst";
    
    if (fs::exists(archiveName)) {
        archiveName = job.baseName + "_" + std::to_string(job.id) + "_" + dateStr + ".tar.zst";
    }
    
    fs::path absArchivePath = fs::absolute(archiveName);
    std::string absArchiveStr = absArchivePath.string();
    
    log(job.id, "INIT", "Demarrage backup: " + job.sourceDir);
    
    if (!fs::exists(job.sourceDir)) {
        log(job.id, "ERROR", "Dossier introuvable: " + job.sourceDir);
        std::lock_guard<std::mutex> lock(failedJobsMutex);
        failedJobs.push_back("JOB " + std::to_string(job.id) + ": Dossier introuvable");
        return;
    }
    
    uintmax_t dirSize = 0;
    try {
        log(job.id, "INIT", "Calcul de la taille du dossier...");
        for (const auto& entry : fs::recursive_directory_iterator(job.sourceDir)) {
            if (fs::is_regular_file(entry)) dirSize += fs::file_size(entry);
        }
        double sizeGB = dirSize / (1024.0 * 1024.0 * 1024.0);
        log(job.id, "INIT", "Taille totale: " + std::to_string((int)sizeGB) + " GB");
    } catch (const std::exception&) {
        log(job.id, "WARN", "Impossible de calculer la taille");
    }
    
    if (dirSize > 0 && !hasEnoughDiskSpace(".", dirSize)) {
        log(job.id, "ERROR", "Espace disque insuffisant");
        std::lock_guard<std::mutex> lock(failedJobsMutex);
        failedJobs.push_back("JOB " + std::to_string(job.id) + ": Espace disque insuffisant");
        return;
    }

    // COMPRESSION
    bool skipCompression = false;
    if (fs::exists(absArchivePath) && fs::file_size(absArchivePath) > 0) {
        log(job.id, "COMPRESS", "Archive existe deja, skip compression");
        skipCompression = true;
    }

    if (!skipCompression) {
        uintmax_t ram = getAvailableRAM();
        std::string zstdParams = getOptimalZstdParams(std::stoi(job.level), ram);
        
        log(job.id, "COMPRESS", "Debut compression (niveau " + job.level + ")");
        
        // --- COMMANDE HYBRIDE ---
        std::string finalCmd;
        #ifdef _WIN32
            // Windows : On passe par cmd.exe pour gérer le pipe |
            std::string innerCmd = "tar -cf - \"" + job.sourceDir + "\" 2>nul | " + pathToZstd + " " + zstdParams + " -q -o \"" + absArchiveStr + "\"";
            finalCmd = "cmd.exe /c \"" + innerCmd + "\"";
        #else
            // Linux : Le shell par défaut gère le pipe, tar gère les erreurs via stderr
            finalCmd = "tar -cf - \"" + job.sourceDir + "\" 2>/dev/null | " + pathToZstd + " " + zstdParams + " -q -o \"" + absArchiveStr + "\"";
        #endif

        auto startComp = steady_clock::now();
        int res = std::system(finalCmd.c_str());
        auto endComp = steady_clock::now();
        
        // Décodage du code retour Linux
        #ifndef _WIN32
        if (WIFEXITED(res)) res = WEXITSTATUS(res);
        #endif

        auto durationSec = duration_cast<seconds>(endComp - startComp).count();
        
        if (programInterrupted) {
            log(job.id, "ERROR", "Interruption detectee");
            if (fs::exists(absArchivePath)) fs::remove(absArchivePath);
            return;
        }
        
        if (res != 0 || !fs::exists(absArchivePath) || fs::file_size(absArchivePath) == 0) {
            log(job.id, "ERROR", "Echec compression (code: " + std::to_string(res) + ")");
            if (fs::exists(absArchivePath)) fs::remove(absArchivePath);
            std::lock_guard<std::mutex> lock(failedJobsMutex);
            failedJobs.push_back("JOB " + std::to_string(job.id) + ": Echec compression");
            return;
        }
        
        uintmax_t archiveSize = fs::file_size(absArchivePath);
        double archiveSizeGB = archiveSize / (1024.0 * 1024.0 * 1024.0);
        double ratio = (dirSize > 0) ? (100.0 * archiveSize / dirSize) : 0;
        
        log(job.id, "COMPRESS", "Termine en " + std::to_string(durationSec) + "s - " 
            + std::to_string((int)archiveSizeGB) + " GB (ratio: " + std::to_string((int)ratio) + "%)");
    }

    // UPLOAD SCP
    log(job.id, "UPLOAD", "Debut transfert vers " + REMOTE_IP);
    
    std::string remoteTarget = REMOTE_USER + "@" + REMOTE_IP + ":" + REMOTE_PATH + "/";
    std::string scpCmd = scpPath + " -i " + SSH_KEY + " \"" + absArchiveStr + "\" " + remoteTarget;
    
    auto startUpload = steady_clock::now();
    std::string scpResult = runCommandWithProgress(scpCmd, job.id, "UPLOAD", 3);
    auto endUpload = steady_clock::now();
    
    if (scpResult == "INTERRUPTED" || programInterrupted) {
        log(job.id, "ERROR", "Transfert interrompu");
        log(job.id, "INFO", "Archive conservee: " + absArchiveStr);
        return;
    }
    
    if (scpResult == "FAILED_AFTER_RETRIES" || scpResult == "PIPE_FAILED") {
        log(job.id, "ERROR", "Echec transfert apres 3 tentatives");
        log(job.id, "INFO", "Archive conservee: " + absArchiveStr);
        std::lock_guard<std::mutex> lock(failedJobsMutex);
        failedJobs.push_back("JOB " + std::to_string(job.id) + ": Echec upload SCP");
        return;
    }

    auto uploadDurationSec = duration_cast<seconds>(endUpload - startUpload).count();
    log(job.id, "UPLOAD", "Termine en " + std::to_string(uploadDurationSec) + "s");

    // NETTOYAGE
    try {
        fs::remove(absArchivePath);
        log(job.id, "CLEANUP", "Archive locale supprimee");
    } catch (const std::exception&) {
        log(job.id, "WARN", "Impossible de supprimer l'archive: " + absArchiveStr);
    }
    
    log(job.id, "DONE", "Backup complete avec succes!");
}