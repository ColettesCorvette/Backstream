#include "config.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::string REMOTE_USER = "";
std::string REMOTE_IP = "";
std::string REMOTE_PATH = "";
std::string SSH_KEY = "";
std::string DEFAULT_LEVEL = "3";

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool loadConfig(const std::string& iniPath) {
    if (!fs::exists(iniPath)) return false; // Fichier introuvable

    std::ifstream file(iniPath);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = trim(line.substr(0, delimiterPos));
            std::string value = trim(line.substr(delimiterPos + 1));

            if (key == "REMOTE_USER") REMOTE_USER = value;
            else if (key == "REMOTE_IP") REMOTE_IP = value;
            else if (key == "REMOTE_PATH") REMOTE_PATH = value;
            else if (key == "SSH_KEY") SSH_KEY = value;
            else if (key == "DEFAULT_LEVEL") DEFAULT_LEVEL = value;
        }
    }
    return true;
}

void saveConfig(const std::string& iniPath) {
    std::ofstream file(iniPath);
    if (file.is_open()) {
        file << "[General]\n";
        file << "REMOTE_USER=" << REMOTE_USER << "\n";
        file << "REMOTE_IP=" << REMOTE_IP << "\n";
        file << "REMOTE_PATH=" << REMOTE_PATH << "\n";
        file << "SSH_KEY=" << SSH_KEY << "\n";
        file << "DEFAULT_LEVEL=" << DEFAULT_LEVEL << "\n";
    }
}

void createConfigInteractive(const std::string& iniPath) {
    std::cout << "\n============================================\n";
    std::cout << "   CONFIGURATION INITIALE (PREMIER LANCEMENT)   \n";
    std::cout << "============================================\n\n";
    
    std::cout << "Aucun fichier de configuration trouve.\n";
    std::cout << "Veuillez entrer les informations de votre serveur.\n\n";

    // On utilise getline pour gerer les espaces eventuels
    std::cout << "1. Utilisateur distant : ";
    std::getline(std::cin, REMOTE_USER);

    std::cout << "2. IP du serveur : ";
    std::getline(std::cin, REMOTE_IP);

    std::cout << "3. Chemin de destination sur le serveur : ";
    std::getline(std::cin, REMOTE_PATH);

    std::cout << "4. Chemin LOCAL de votre cle prive SSH (ex: C:\\Users\\You\\.ssh\\id_ed25519) : ";
    std::getline(std::cin, SSH_KEY);
    // On enleve les guillemets si l'utilisateur a fait "Copier en tant que chemin"
    if (!SSH_KEY.empty() && SSH_KEY.front() == '"') SSH_KEY.erase(0, 1);
    if (!SSH_KEY.empty() && SSH_KEY.back() == '"') SSH_KEY.pop_back();

    std::cout << "5. Niveau de compression par defaut (1-19, defaut: 3) : ";
    std::string lvl;
    std::getline(std::cin, lvl);
    if (!lvl.empty()) DEFAULT_LEVEL = lvl;
    else DEFAULT_LEVEL = "3";

    std::cout << "\n>> Sauvegarde de la configuration dans settings.ini...\n";
    saveConfig(iniPath);
    std::cout << ">> Configuration terminee !\n\n";
}