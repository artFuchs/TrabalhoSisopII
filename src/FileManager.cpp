#include "FileManager.hpp"

using namespace std;

namespace dropbox{

FileManager::FileManager(){
    valid = false;
}

FileManager::FileManager(string directory) : path(directory){
    check_dir(path);
}

FileManager::~FileManager(){
    if (!opened_files.empty())
    {
        // close all opened files
        for (auto it = opened_files.begin(); it != opened_files.end(); it++)
        {
            it->second.close();
            opened_files.erase(it->first);
        }
    }
}

bool FileManager::is_valid()
{
    return valid;
}

void FileManager::check_dir(string _path){
    valid = false;
    if (_path.back() != '/')
    {
        _path = _path + "/";
    }
    path = _path;
    DIR *d;
    d = opendir(path.c_str());
    if (d){
        valid = true;
    } else{
        int err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (err > -1)
            valid = true;
    }
}

map<string, STAT_t> FileManager::read_dir(){
    map<string, STAT_t> _files;

    if (!is_valid())
    {
        return _files;
    }

    //open directory;
    DIR *d;
    struct dirent *dir;
    d = opendir(path.c_str());
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            //read the entries of the directory
            if (string(dir->d_name) != "." && string(dir->d_name) != "..") {
                STAT_t buffer;
                string fpath = path+string(dir->d_name);
                stat(fpath.c_str(), &buffer);
                _files[string(dir->d_name)] = buffer;
            }
        }
        closedir(d);
    }
    return _files;
}

int FileManager::create_file(string name, char contents[], uint max_size){
    ofstream outFile;
    string fpath = path+name;
    try{
        outFile.open(fpath, fstream::binary);
        outFile.write(contents,max_size);
        outFile.close();
    } catch (std::ifstream::failure e){
        return -1;
    }
    return 0;
}

int FileManager::append_file(string name, char contents[], uint max_size){
    ofstream outFile;
    string fpath = path+name;
    try{
        outFile.open(fpath, fstream::app | fstream::binary);
        outFile.write(contents,max_size);
        outFile.close();
    } catch (std::ifstream::failure e){
        return -1;
    }
    return 0;
}

int FileManager::delete_file(string name){
    string filePath(path+name);
    return remove(filePath.c_str());
}

uint FileManager::read_file(string name, char* buffer, uint n){

    //if file is not opened already, open it
    if (opened_files.find(name)==opened_files.end()){
        opened_files[name] = ifstream();
        opened_files[name].open(path+name);
    }

    ifstream *file = &opened_files[name];

    if (file){
        file->read(buffer, n);
        uint read = file->gcount();
        if (file->eof()){
            file->close();
            opened_files.erase(name);
        }
        return read;
    }
    return 0;
}

}
