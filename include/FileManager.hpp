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
protected:
    string path; // caminho para o diretório - pode ser relativo
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

    /* create_file : cria um arquivo
            parâmetros: name - string : nome do arquivo;
                        buffer - char[] : conteúdo do arquivo;
            retorno : int - 0 se não ocorreu erro
                            <0 se ocorreu erro
    */
    int create_file(string, char[]);

    /* create_file_part : cria um arquivo que é parte de outro arquivo
            parâmetros: name - string : nome da parte do arquivo;
                        buffer - char[] : conteúdo do arquivo;
                        part - int : qual parte do arquivo está sendo criada;
                        total - int : total de partes a serem criadas - deve ser chamado na primeira chamada do método para o arquivo
            retorno : int - 0 se não ocorreu erro
                           <0 se ocorreu erro
    */
    int create_file_part(string, char[], int, int);

    /* join_files : junta diversas partes de um arquivo
            parâmetros : name - string : nome do arquivo
            retorno : int - 0 se não ocorreu erro
                           <0 se ocorreu erro
    */
    int join_files(string);

    /* clean_parts : deleta as partes do arquivo que foram criadas
            parâmetros : name - string : nome do arquivo a que as partes se referem
            retorno : int - 0 se não ocorreu erro
                           <0 se ocorreu erro

    */
    int clean_parts(string);

    /* delete_file : deleta um arquivo
            parâmetros : name - string : nome do arquivo a ser deletado
            retorn : int - 0 se obteve sucesso
                            <0 c.c.
    */
    int delete_file(string);
};

}
