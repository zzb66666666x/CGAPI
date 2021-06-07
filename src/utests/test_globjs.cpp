#include<iostream>
#include"../gl/globj.h"
#include <assert.h>

using namespace std;

glManager<float> mgr;

float data[] = {1,2,3,4,5,6,7,8,9,10};

float more_data[] = {1,2,3,4,5,6,7,8,9,10,11};

int main(){
    int id1, id2, id3;
    glStorage<float> * ptr1;
    glStorage<float>* ptr2;
    glStorage<int> * ptr_int;
    id1 = mgr.insertStorage(10);
    id2 = mgr.insertStorage(10);
    cout<<id1<<" : "<<id2<<endl;
    int ret = mgr.searchStorage(&ptr1, id1);
    assert(ret==0);
    cout<<ptr1->getSize()<<endl;
    ptr1->loadElts(data, 10);
    float * temp = ptr1->getDataPtr();
    for (int i=0; i<10; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    try{
        mgr.searchStorage(&ptr_int, id2);
    }
    catch(std::runtime_error &){
        cout<<"error input test passed, cannot search in glManager by another type of pointer"<<endl;
    }
    ret = mgr.searchStorage(&ptr2, id2);
    assert(ret==0);
    ptr2->allocEltSpace(11);
    ptr2->loadElts(more_data, 11);
    temp = ptr2->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    glStorage<float>* ptr3;
    id3 = mgr.insertStorage(*ptr2);
    cout<<id1<<id2<<id3<<endl;
    ret = mgr.searchStorage(&ptr3, id3);
    assert(ret==0);
    cout<<"print third glStorage in the manager"<<endl;
    temp = ptr3->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    cout<<"show them all together"<<endl;
    mgr.searchStorage(&ptr1, id1);
    mgr.searchStorage(&ptr2, id2);
    mgr.searchStorage(&ptr3, id3);
    temp = ptr1->getDataPtr();
    for (int i=0; i<10; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    temp = ptr2->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    temp = ptr3->getDataPtr();
    for (int i=0; i<11; i++){
        cout<<temp[i]<<"   ";
    }
    cout<<endl;
    return 0;
}