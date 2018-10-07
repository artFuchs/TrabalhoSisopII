#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <map>
#include <string>

using namespace std;

namespace dropbox{

// estruturas usadas
enum Modification{ MOVED, MODIFIED, ERASED };
typedef struct stat STAT_t; // representa um arquivo
struct file_mod{
  STAT_t file_stat;
  Modification mod;
};
typedef struct file_mod FILE_MOD_t; // representa a modificação de um arquivo

class FileMonitor{
private:
    string path;
    bool valid;
    map<string, STAT_t> files;

public:
    FileMonitor(string);

    /*
        is_valid: informa se o FileMonitor foi iniciado corretamente
            retorno: valid - bool : se é valido (true) ou não (false)
    */
    bool is_valid();

    /* check_dir : verifica se um diretório existe.
                    se o diretório não existir, cria ele.
            parâmetros : path - string : caminho do diretório
            retorno : -1 - erro
                      0 - diretório existe
                      1 - diretório foi criado
    */
    int check_dir(string);

    /* read_dir : lê o diretório alvo, retornando um dicionário contendo os
                  arquivos contidos nele.
          retorno : arquivos do diretório - map<string, STAT_t>
    */
    map<string, STAT_t>read_dir();

    /* diff_dir : examina o diretório, retornando modificações, se ocorreram
          retorno : mudanças encontradas nos arquivos - map<string, FILE_MOD_t>
    */
    map<string, FILE_MOD_t> diff_dir();

};
}
