#include <iostream>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"

using namespace std;


int main(){
    glvInit();
    GLVFile* file = glvCreateFile(300, 300, "result");
    cout<<"finish creating file"<<endl;
    glClearColor(0.2f, 0.5f, 0.6f, 1.0f);
    cout<<"finish clearing framebuffer, write to output image"<<endl;
    if(glvWriteFile(file)){
        cout<<"success!";
    }
    glvTerminate();
    return 0;
}