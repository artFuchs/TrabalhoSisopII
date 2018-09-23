#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>
#include <iostream>
#include <map>
#include <string>

using namespace std;

// structures used
enum Modification{ MOVED, MODIFIED, ERASED };
typedef struct stat STAT_t;
struct file_mod{
  STAT_t file_stat;
  Modification mod;
};
typedef struct file_mod FILE_MOD_t;

// functions
void read_dir(string, map<string, STAT_t>&);
map<string, FILE_MOD_t> diff_dir(string, map<string, STAT_t>&);
