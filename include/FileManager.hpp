#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

namespace dropbox{

typedef struct stat STAT_t; // representa um arquivo

class FileManager{
private:
    bool valid;
protected:
    string path;
public:
    FileManager();
    FileManager(string directory);

    /* check_dir : verifica se um diretório existe.
                    se o diretório não existir, cria ele.
                    se ocorrer algum erro, configura a flag valid para false.
                    c.c., configura a flag valid para true;
            parâmetros : path - string : caminho do diretório
    */
    void check_dir(string);

    /*
        is_valid: informa se o FileMonitor foi iniciado corretamente
            retorno: valid - bool : se é valido (true) ou não (false)
    */
    bool is_valid();

    /* read_dir : lê o diretório alvo, retornando um dicionário contendo os
                  arquivos contidos nele.
          retorno : arquivos do diretório - map<string, STAT_t>
    */
    map<string, STAT_t> read_dir();

};

}
