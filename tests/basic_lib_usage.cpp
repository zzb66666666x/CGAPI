#include <iostream>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"

using namespace std;

typedef struct{

    int width;
    int height;
    char *filename;
    int type;

}ptr_parser;


int main(){
    glHelloWorld();
    GLVFile* ptr = glvCreateFile(0, 0, NULL);
    ptr_parser* pp = (ptr_parser*)ptr;
    cout<<"ptr contains: "<<pp->width<<" "<<pp->height<<"  "<<pp->filename<<"  "<<pp->type<<endl;
    return 0;
}