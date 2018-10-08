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
          if (string(dir->d_name) != "." && string(dir->d_name) != "..") {
            STAT_t buffer;
            string fpath = path+"/"+string(dir->d_name);
            stat(fpath.c_str(), &buffer);
            _files[string(dir->d_name)] = buffer;
            //printf("%s\n", dir->d_name);
          }
        }
        closedir(d);
    }
    return _files;
}


}
