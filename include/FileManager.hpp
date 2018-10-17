#pragma once

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <fstream>

#include <iostream>
#include <ctime>
#include <map>
#include <string>
#include <vector>

using namespace std;

namespace dropbox{

typedef struct stat STAT_t; // representa um arquivo

class FileManager{
private:
    bool valid;
    map<string,ifstream> opened_files;
protected:
    string path; // caminho para o diretório - pode ser relativo
public:
    FileManager();
    FileManager(string directory);
    ~FileManager();

    /* check_dir : verifica se um diretório existe.
                    se o diretório não existir, cria ele.
                    se ocorrer algum erro, configura a flag valid para false.
                    c.c., configura a flag valid para true;
          parâmetros : path - string : caminho do diretório
    */
    void check_dir(string);

    /* getPath : retorna o caminho do diretório atual */
    string getPath(){
      return path;
    }

    /* is_valid: informa se o FileMonitor foi iniciado corretamente
          retorno: valid - bool : se é valido (true) ou não (false)
    */
    bool is_valid();

    /* read_dir : lê o diretório alvo, retornando um dicionário contendo os
                  arquivos contidos nele.
          retorno : arquivos do diretório - map<string, STAT_t>
    */
    virtual map<string, STAT_t> read_dir();

    /* create_file : cria um arquivo
            parâmetros: name - string : nome do arquivo;
                        contents - char[] : conteúdo do arquivo;
                        max_size - uint : tamanho limite do buffer;
            retorno : int - 0 se a operação for concluida
                            <0 se ocorreu erro
    */
    int create_file(string, char[], uint);

    /* append_file : apende dados de um buffer em um arquivo
          parâmetros : name - string : nome do arquivo a ser continuado;
                        contents - char[]: buffer contendo dados a serem adicionados ao arquivo;
                        max_size - uint : tamanho do buffer contents
          retorno : int - 0 se a operação for concluida
                        < 0 se ocorrer erro
    */
    int append_file(string, char[], uint);

    /* delete_file : deleta um arquivo
            parâmetros : name - string : nome do arquivo a ser deletado
            retorno : int - 0 se a operação for concluida
                            <0 se ocorreu erro
    */
    int delete_file(string);

    /* read_file : lê n bytes de um arquivo
            parâmetros : name - string : nome do arquivo a ser lido;
                         buffer - char* : buffer onde armazenar o arquivo;
                         n - uint : numero de bytes que devem ser lidos do arquivo.
            retorno : numero de bytes lidos do arquivo
    */
    uint read_file(string, char*, uint);
};

}
