// Header Windows
#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
#endif

#include <iostream>
#include <vector>
#include <string>
#include <future>
#include <algorithm>
#include <csignal> 
#include <filesystem>

#include "config.h"
#include "utils.h"
#include "progress.h"
#include "backup.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    enableANSI();
    setHighPriority();
    
    int cpuCores = getCPUCoreCount();
    uintmax_t availableRAM = getAvailableRAM();
    
    log(-1, "SYSTEM", "Demarrage - " + std::to_string(cpuCores) + " cores CPU, " + std::to_string(availableRAM) + " MB RAM disponibles");
    
    std::string appDir = getAppDir(argv[0]);
    
    // Chemin universel 
    fs::path iniPath = fs::path(appDir) / "settings.ini";
    
    bool justConfigured = false;

    if (!loadConfig(iniPath.string())) {
        createConfigInteractive(iniPath.string());
        
        if (REMOTE_IP.empty() || SSH_KEY.empty()) {
             log(-1, "ERROR", "Configuration annulee ou invalide.");
             systemPause(); return 1;
        }
        justConfigured = true;
    } else {
        log(-1, "SYSTEM", "Configuration chargee depuis settings.ini");
    }

    std::string zstdPath = findZstdPath(appDir);
    std::string scpPath = findScpPath();

    // Validations
    if (zstdPath.empty()) {
        log(-1, "ERROR", "zstd introuvable");
        log(-1, "INFO", "Installez zstd (apt install zstd) ou placez le binaire dans tools/");
        systemPause(); return 1;
    }
    
    if (scpPath.empty()) {
        log(-1, "ERROR", "SCP introuvable. Installez OpenSSH.");
        systemPause(); return 1;
    }
    
    if (!fs::exists(SSH_KEY)) {
        log(-1, "ERROR", "Cle SSH introuvable: " + SSH_KEY);
        systemPause(); return 1;
    }
    
    // Test connexion
    log(-1, "SYSTEM", "Test connexion SSH vers " + REMOTE_IP + "...");
    if (!testSSHConnection(scpPath)) {
        log(-1, "ERROR", "Impossible de se connecter a " + REMOTE_USER + "@" + REMOTE_IP);
        log(-1, "INFO", "Verifie: cle SSH autorisee, serveur accessible, credentials corrects");
        systemPause(); return 1;
    }
    log(-1, "SYSTEM", "Connexion SSH OK!");
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    if (argc < 2) {
        if (justConfigured) {
            std::cout << "\n============================================\n";
            std::cout << "      INSTALLATION ET CONFIGURATION REUSSIES      \n";
            std::cout << "============================================\n";
            std::cout << "Le programme est maintenant pret.\n\n";
            std::cout << "MODE D'EMPLOI :\n";
            std::cout << "Lancez: ./backup <dossier> [niveau] (Linux)\n";
            std::cout << "ou glissez un dossier sur l'executable (Windows)\n";
            std::cout << "\n";
            systemPause();
            return 0;
        } else {
            log(-1, "ERROR", "Aucun dossier a sauvegarder.");
            log(-1, "INFO", "Usage: ./backup <dossier> [niveau]");
            systemPause(); return 1;
        }
    }

    std::vector<BackupJob> jobs;
    std::vector<std::string> args;

    for (int i = 1; i < argc; ++i) {
        std::string raw = argv[i];
        args.push_back(cleanArg(raw));
    }

    for (size_t i = 0; i < args.size(); ++i) {
        std::string currentArg = args[i];

        if (fs::is_directory(currentArg)) {
            BackupJob job;
            job.id = (int)jobs.size() + 1;
            job.sourceDir = currentArg;
            fs::path p(currentArg);
            job.baseName = p.filename().string();
            job.level = DEFAULT_LEVEL;
            
            jobs.push_back(job);
        }
        else if (looksLikePath(currentArg)) {
            log(-1, "ERROR", "Dossier introuvable: " + currentArg);
        }
        else if (!jobs.empty()) {
            BackupJob& lastJob = jobs.back();
            if (isValidLevel(currentArg)) lastJob.level = currentArg;
            else lastJob.baseName = currentArg;
        }
    }

    if (jobs.empty()) {
        log(-1, "ERROR", "Aucune tache valide");
        systemPause();
        return 1;
    }

    int maxParallel = std::min(MAX_PARALLEL_JOBS, std::max(1, cpuCores / 4));
    if (jobs.size() == 1) maxParallel = 1;
    
    log(-1, "SYSTEM", "Lancement de " + std::to_string(jobs.size()) + " tache(s) - " + std::to_string(maxParallel) + " en parallele max");
    
    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "============================================================" << std::endl;
    }

    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < jobs.size(); ++i) {
        while (futures.size() >= (size_t)maxParallel) {
            auto it = std::find_if(futures.begin(), futures.end(),
                [](std::future<void>& f) {
                    return f.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready;
                });
            
            if (it != futures.end()) {
                it->get();
                futures.erase(it);
            }
        }
        
        futures.push_back(std::async(std::launch::async, runBackupJob, jobs[i], zstdPath, scpPath));
    }

    for (auto& f : futures) {
        f.get();
    }

    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "============================================================" << std::endl;
    }
    
    if (programInterrupted) {
        log(-1, "SYSTEM", "PROGRAMME INTERROMPU");
    } else if (failedJobs.empty()) {
        log(-1, "SYSTEM", "TOUS LES BACKUPS ONT REUSSI !");
    } else {
        log(-1, "SYSTEM", "TERMINE AVEC ERREURS:");
        for (const auto& err : failedJobs) {
            std::cout << "  - " << err << std::endl;
        }
    }
    
    systemPause();
    return failedJobs.empty() ? 0 : 1;
}