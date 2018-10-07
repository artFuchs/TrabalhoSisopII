#include "FileMonitor.hpp"
using namespace std;

namespace dropbox{

FileMonitor::FileMonitor(string _path) : path(_path){
    int err = check_dir(path);
    if (err < 0) {
        valid = false;
        return;
    } else {
        valid = true;
    }

    files = read_dir();
}

bool FileMonitor::is_valid()
{
    return valid;
}

int FileMonitor::check_dir(string _path){
    int err = -1;
    path = _path;
    DIR *d;
    d = opendir(path.c_str());
    if (d){
        err = 0;
    } else{
        err = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    return err;
}


map<string, STAT_t> FileMonitor::read_dir(){
  map<string, STAT_t> _files;

  if (!valid)
    return _files;

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


// check for diferences in the directory
map<string, FILE_MOD_t> FileMonitor::diff_dir(){
  map<string, FILE_MOD_t> changes; // changes that ocurred in the directory
  map<string, bool> in_dir; // which files are in directory

  if (!valid)
    return changes;

  for (auto it = files.begin(); it!=files.end(); it++)
    in_dir[it->first] = false;

  //open directory;
  DIR *d;
  struct dirent *dir;
  d = opendir(path.c_str());
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (string(dir->d_name) != "." && string(dir->d_name) != "..") {
        // get file info to compare
        STAT_t buffer;
        string fpath = path+"/"+string(dir->d_name);
        stat(fpath.c_str(), &buffer);

        FILE_MOD_t change_buffer;
        string name = string(dir->d_name);
        in_dir[name] = true;
        if (files.find(name)!=files.end()){
          // if the file was in the directory before
          // check if it was modified
          if (files[name].st_mtime < buffer.st_mtime) {
            change_buffer.file_stat = buffer;
            change_buffer.mod = MODIFIED;
            changes[name] = change_buffer;
            files[name] = buffer;
          } else{
            // check if changes in mtime was just an update to the timestamp of
            // the file - ocurr when a file is copied, for example
            bool tc = (files[name].st_ctime == buffer.st_ctime);
            bool ta = (files[name].st_atime == buffer.st_atime);
            bool s = (files[name].st_size == buffer.st_size);
            if (tc && ta && s)
            {
              files[name] = buffer;
            } else{
              change_buffer.file_stat = buffer;
              change_buffer.mod = MODIFIED;
              changes[name] = change_buffer;
              files[name] = buffer;
            }
          }
        } else{
          // file was moved to directory
          change_buffer.file_stat = buffer;
          change_buffer.mod = MOVED;
          changes[name] = change_buffer;
          files[name] = buffer;
        }
      }
    }
    closedir(d);
  }

  // check if any files where erased
  for (auto it = files.begin(); it!=files.end(); it++)
  {
    if (in_dir[it->first] == false)
    {
      FILE_MOD_t deleted;
      deleted.mod = ERASED;
      changes[it->first] = deleted;
      files.erase(it->first);
    }
  }

  return changes;
}
}
