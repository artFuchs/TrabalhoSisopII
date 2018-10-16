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
    map<string, vector<string> > file_pieces;
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

    string getPath(){
      return path;
    }

    /*
        is_valid: informa se o FileMonitor foi iniciado corretamente
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
    int create_file(string, string);

    /* append_file : apende dados de um buffer em um arquivo
          parâmetros : name - string : nome do arquivo a ser continuado;
                        contents - string : conteúdo a ser adicionado ao arquivo;
          retorno : int - 0 se a operação for concluida
                        < 0 se ocorrer erro
    */
    int append_file(string, string);

    /* create_file_part : cria um arquivo que é parte de outro arquivo
            parâmetros: name - string : nome da parte do arquivo;
                        buffer - char[] : conteúdo do arquivo;
                        max_size - uint : tamanho limite do buffer;
                        part - int : qual parte do arquivo está sendo criada;
                        total - int : total de partes a serem criadas - deve ser chamado na primeira chamada do método para o arquivo
            retorno : int - 0 se a operação for concluida
                           <0 se ocorreu erro
    */
    int create_file_part(string, char[], uint, int, int);

    /* join_files : junta diversas partes de um arquivo
            parâmetros : name - string : nome do arquivo
            retorno : int - 0 se a operação for concluida
                           <0 se ocorreu erro
    */
    int join_files(string);

    /* clean_parts : deleta as partes do arquivo que foram criadas
            parâmetros : name - string : nome do arquivo a que as partes se referem
            retorno : int - 0 se a operação for concluida
                           <0 se ocorreu erro

    */
    int clean_parts(string);

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
