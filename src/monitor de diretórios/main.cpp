#include "folder_monitor.h"
using namespace std;

int main(void) {
  map<string,STAT_t> files;

  string path = "./dir";
  read_dir(path, files);

  while (true)
  {
    map<string, FILE_MOD_t> m;
    m = diff_dir(path,files);
    if (!m.empty())
    {
      cout << "CHANGES!" << endl;
      for (auto it = m.begin(); it!=m.end(); it++)
      {
          cout << it->first;
          switch (it->second.mod){
            case MOVED:
              cout << " moved/created" << endl;
              break;
            case MODIFIED:
              cout << " modified" << endl;
              break;
            case ERASED:
              cout << " erased" << endl;
              break;
          }
          if (it->second.mod!=ERASED){
            cout << "\t tamanho: " << it->second.file_stat.st_size << endl;
            cout << "\t mtime:" << ctime(&it->second.file_stat.st_mtime);
            cout << "\t atime:" << ctime(&it->second.file_stat.st_atime);
            cout << "\t ctime:" << ctime(&it->second.file_stat.st_ctime);
            files[it->first] = it->second.file_stat;
          }
      }
      cout << endl;
      sleep(3);
    }
  }

  return(0);
}
