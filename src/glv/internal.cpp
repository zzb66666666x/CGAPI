#include "internal.h"

std::array<bool, MAX_GLVFILE_NUM> glv_file_usage;
std::array<_GLVFile*, MAX_GLVFILE_NUM> glv_files;
_GLVFile *_glv_file_current = nullptr;

int _glv_files_free_slot(){
    for (int i=0; i<MAX_GLVFILE_NUM; i++){
        if (!glv_file_usage[i]){
            return i;
        }
    }
    return -1;
}