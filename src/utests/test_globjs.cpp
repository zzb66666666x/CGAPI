#include<iostream>
#include <assert.h>
#include "../gl/globj.h"
#include "../../include/gl/common.h"

using namespace std;


float data[] = {1,2,3,4,5,6,7,8,9,10};

float more_data[] = {1,2,3,4,5,6,7,8,9,10,11};

int main(){
    // gl manager test
    glManager mgr;
    int id1, id2, id3;
    glObject * ptr1;
    glObject* ptr2;
    id1 = mgr.insertStorage(GL_FLOAT, 10);
    id2 = mgr.insertStorage(GL_FLOAT, 10);
    cout<<id1<<" : "<<id2<<endl;
    int ret = mgr.searchStorage(&ptr1, id1);
    assert(ret==0);
    cout<<"size in float "<<ptr1->getSize()<<endl;
    cout<<"size in bytes "<<ptr1->byteCapacity()<<endl;
    // ptr1->loadElts(data, 10);
    ptr1->loadBytes((void*)data, sizeof(float)*10);
    float * temp = (float*)(ptr1->getDataPtr());
    for (int i=0; i<10; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;

    ret = mgr.searchStorage(&ptr2, id2);
    assert(ret==0);
    ptr2->allocEltSpace(11);
    ptr2->loadElts(more_data, 11);
    temp = (float *)(ptr2->getDataPtr());
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;

    glObject* ptr3;
    id3 = mgr.insertStorage(GL_FLOAT, *ptr2);
    cout<<id1<<id2<<id3<<endl;
    ret = mgr.searchStorage(&ptr3, id3);
    assert(ret==0);
    cout<<"print third glStorage in the manager"<<endl;
    temp = (float *)(ptr3->getDataPtr());
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    cout<<"show them all together"<<endl;
    mgr.searchStorage(&ptr1, id1);
    mgr.searchStorage(&ptr2, id2);
    mgr.searchStorage(&ptr3, id3);
    temp = (float *)ptr1->getDataPtr();
    for (int i=0; i<10; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    temp = (float *)ptr2->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    temp = (float *)ptr3->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    // use gl storage to keep some struct
    glStorage<color_t> frame = glStorage<color_t>(240000, true, GLOBJ_FRAMEBUF, GL_FRAMEBUFFER);
    color_t * data = (color_t*) frame.getDataPtr();
    color_t color = {1,2,3};
    data[100] = color;
    cout<<data[100].R<<data[100].G<<data[100].B<<endl;
    return 0;
}