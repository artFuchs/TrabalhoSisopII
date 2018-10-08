#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include "FileManager.hpp"

using namespace std;

namespace dropbox{

// estruturas usadas
enum Modification{ MOVED, MODIFIED, ERASED };
struct file_mod{
  STAT_t file_stat;
  Modification mod;
};
typedef struct file_mod FILE_MOD_t; // representa a modificação de um arquivo

class FileMonitor : public FileManager{
private:
    map<string, STAT_t> files;

public:
    FileMonitor(string);

    /* diff_dir : examina o diretório, retornando modificações, se ocorreram
          retorno : mudanças encontradas nos arquivos - map<string, FILE_MOD_t>
    */
    map<string, FILE_MOD_t> diff_dir();

};
}
