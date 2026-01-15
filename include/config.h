#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Configuration du serveur distant
extern std::string REMOTE_USER; 
extern std::string REMOTE_IP;   
extern std::string REMOTE_PATH; 
extern std::string DEFAULT_LEVEL; 
extern std::string SSH_KEY;

// Optimisations
const int MAX_PARALLEL_JOBS = 2;
const size_t PIPE_BUFFER_SIZE = 8192;

bool loadConfig(const std::string& iniPath);
void saveConfig(const std::string& iniPath); 
void createConfigInteractive(const std::string& iniPath); 

#endif // CONFIG_H
