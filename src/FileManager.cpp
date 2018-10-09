#include "FileManager.hpp"

using namespace std;

namespace dropbox{

FileManager::FileManager(){
    valid = false;
}

FileManager::FileManager(string directory) : path(directory){
    check_dir(path);
}

bool FileManager::is_valid()
{
    return valid;
}

void FileManager::check_dir(string _path){
    valid = false;
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
                string fpath = path+"/"+string(dir->d_name);
                stat(fpath.c_str(), &buffer);
                _files[string(dir->d_name)] = buffer;
            }
        }
        closedir(d);
    }
    return _files;
}

int FileManager::create_file(string name, char contents[]){
    ofstream outFile;
    string fpath = path+string("/")+name;
    try{
        outFile.open(fpath);
        outFile << contents;
        outFile.close();
    } catch (std::ifstream::failure e){
        return -1;
    }
    return 0;
}

int FileManager::create_file_part(string name, char contents[], int part , int total){
    ofstream outFile;
    string _path = path+string("/.")+name+to_string(part);

    // if it's the first call, init the file_pieces[name] entry
    if (file_pieces.find(name)==file_pieces.end())
        file_pieces[name] = vector<string>(total, string());

    try{
        outFile.open(_path);
        outFile << contents;
        outFile.close();
        file_pieces[name][part] = "." + name + to_string(part);
    } catch (std::ifstream::failure e){
        return -1;
    }

    return 0;
}

int FileManager::join_files(string name){
    ifstream filePart;
    ofstream file;

    //check if the name entry exists in file_pieces
    if (file_pieces.find(name)==file_pieces.end())
        return -2;

    try {
        file.open(path + "/" + name);
        for (uint i=0; i<file_pieces[name].size(); i++)
        {
            filePart.open(path + "/" + file_pieces[name][i]);
            string buffer;
            buffer.assign( std::istreambuf_iterator<char>(filePart),
                           std::istreambuf_iterator<char>());
            file << buffer;
            filePart.close();
        }
        file.close();
    } catch (std::ifstream::failure e){
        return -1;
    }

    return 0;
}

int FileManager::clean_parts(string name){
    //check if the name entry exists in file_pieces
    if (file_pieces.find(name)==file_pieces.end())
        return -2;

    for (auto part_name = file_pieces[name].begin(); part_name!=file_pieces[name].end(); part_name++)
    {
        if (delete_file(*part_name)!=0)
            return -1; // error deleting file
    }
    file_pieces.erase(name);
    return 0;
}

int FileManager::delete_file(string name){
    string filePath(path+"/"+name);
    return remove(filePath.c_str());
}



}
