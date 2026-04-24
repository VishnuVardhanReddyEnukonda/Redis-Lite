#include "lru_cache.h"
using namespace std;

string LRUcache::get(const string& key){
    auto it=cacheMap.find(key);
    if(it==cacheMap.end()){
        return "";
    }
    cacheList.splice(cacheList.begin(),cacheList,it->second);
    return it->second->value;
}
//back front 
void LRUcache::put(const string& key,const string& value){
    auto it=cacheMap.find(key);
    if(it!=cacheMap.end()){
        it->second->value=value;
        cacheList.splice(cacheList.begin(),cacheList,it->second);
        return;
    }
    if(cacheMap.size()==capacity){
        const Node& lru_node=cacheList.back();
        cacheMap.erase(lru_node.key);
        cacheList.pop_back();
    }
    cacheList.emplace_front(key,value);
    cacheMap[key]=cacheList.begin();
}