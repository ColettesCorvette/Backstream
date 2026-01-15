#ifndef PROGRESS_H
#define PROGRESS_H

#include <string>
#include <mutex>

// Variables globales
extern std::mutex logMutex;

// Fonctions d'affichage
void log(int jobId, const std::string& phase, const std::string& msg);
std::string getCurrentTime();

#endif // PROGRESS_H
