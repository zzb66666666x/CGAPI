#include <iostream>
#include "../include/gl/gl.h"
#include "../include/glv/glv.h"

using namespace std;


int main(){
    glvInit();
    GLVFile* file1 = glvCreateFile(900, 900, "red");
    GLVFile* file2 = glvCreateFile(900, 900, "green");
    GLVFile* file3 = glvCreateFile(900, 900, "blue");
    glvMakeFileCurrent(file1);
    cout<<"finish creating file"<<endl;
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    cout<<"finish clearing framebuffer, write to output image"<<endl;
    if(glvWriteFile(file1)){
        cout<<"success!"<<endl;
    }
    cout<<"writing to other files"<<endl;
    glvMakeFileCurrent(file2);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glvWriteFile(file2);
    try{
        glvWriteFile(file3);
    }
    catch(std::exception & e){
        cout<<"cannot output to another context without make current"<<endl;
    }
    glvMakeFileCurrent(file3);
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glvWriteFile(file3);
    glvTerminate();
    return 0;
}