#include <iostream>
#include "FileManager.hpp"
using namespace std;
using namespace dropbox;
int main()
{
    FileManager fm("./test");
    if (!fm.is_valid())
        return -1;
    fm.create_file("test1", (char*)"testando a criação de arquivos.");
    fm.create_file_part("test2", (char*)"testando ", 0, 7);
    fm.create_file_part("test2", (char*)"a ", 1, 7);
    fm.create_file_part("test2", (char*)"criacao ", 2, 7);
    fm.create_file_part("test2", (char*)"de ", 3, 7);
    fm.create_file_part("test2", (char*)"arquivos ", 4, 7);
    fm.create_file_part("test2", (char*)"em ", 5, 7);
    fm.create_file_part("test2", (char*)"partes.", 6, 7);
    fm.join_files("test2");
    
    int err = fm.clean_parts("test2");
    if (err != 0)
    {
        cout << "error cleanning temporary files: " << err << endl;
    }

    char buffer[64];
    if (fm.read_file("test2",buffer,64)>0)
    {
        cout << buffer << endl;
    }      
    

    return 0;
}
