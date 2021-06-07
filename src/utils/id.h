#ifndef _ID_H
#define _ID_H

#include <boost/numeric/interval.hpp>
#include <limits>
#include <set>
#include <iostream>

class id_interval 
{
public:
    id_interval(int ll, int uu) : value_(ll,uu)  {}
    bool operator < (const id_interval& ) const;
    int left() const { return value_.lower(); }
    int right() const {  return value_.upper(); }
private:
    boost::numeric::interval<int> value_;
};

class IdManager {
public:
    IdManager();
    int AllocateId();          // Allocates an id
    void FreeId(int id);       // Frees an id so it can be used again
    bool MarkAsUsed(int id);   // Let's the user register an id. 
private: 
    typedef std::set<id_interval> id_intervals_t;
    id_intervals_t free_;
};



#endif
