#include "asuka.h"

int ObjectManager::acquire(void* me) {
  c++;
  //printf("creating id %d\n",c);
  items.push_back(ObjectInfo(c,me));
  return c;
}

bool ObjectManager::remove(int id) {
  //printf("deleting id %d\n",id);
  for (size_t i=0; i<items.size(); i++) {
    if (items[i].id==id) {
      items.erase(items.begin()+i);
      //printf("success\n");
      return true;
    }
  }
  //printf("did not find anything\n");
  return false;
}

bool ObjectManager::has(void* p) {
  for (ObjectInfo& i: items) {
    if (i.ptr==p) return true;
  }
  return false;
}
