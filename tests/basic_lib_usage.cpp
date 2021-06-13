#include <iostream>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"

using namespace std;


int main(){
    glvInit();
    GLVFile* file = glvCreateFile(300, 290, "result.jpg");
    cout<<"finish creating file"<<endl;
    glClearColor(0.2f, 0.5f, 0.6f, 1.0f);
    cout<<"finish clearing framebuffer, write to output image"<<endl;
    glvWriteFile(file);
    glvTerminate();
    return 0;
}