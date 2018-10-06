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
typedef struct stat STAT_t;
struct file_mod{
  STAT_t file_stat;
  Modification mod;
};
typedef struct file_mod FILE_MOD_t;

// =============================================================================
// funções

/* check_dir : verifica se um diretório existe.
                se o diretório não existir, cria ele.
        parâmetros : path - string : caminho do diretório
        retorno : -1 - erro
                  0 - diretório existe
                  1 - diretório foi criado
        */
int check_dir(string);



/* read_dir : lê um diretório, retornando um dicionário contendo os
              arquivos contidos nele.
      parâmetros : path - string : caminho do diretório a ser lido.
      retorno : arquivos do diretório - map<string, STAT_t>
*/
map<string, STAT_t>read_dir(string);

/* diff_dir : examina um diretório, retornando modificações que nele ocorram
      parâmetros : path - string : caminho do diretório a ser lido
                   files - map <string, STAT_t : dicionário contendo os arquivos
                           já lidos do diretório, necessário para examinar as
                           diferenças.
      retorno : mudanças encontradas nos arquivos - map<string, FILE_MOD_t>
*/
map<string, FILE_MOD_t> diff_dir(string, map<string, STAT_t>);

}
