#include "lru_cache.h"
using namespace std;

namespace VVR{
    string LRUcache::get(const string& key){
    std::lock_guard<std::mutex> lock(mtx);
    auto it=cacheMap.find(key);
     
    if(it==cacheMap.end()){
        return "";
    }
    long long now = getCurrentTimeMs();
    if (it->second->expiryTime > 0 && now > it->second->expiryTime) {
            // The time has passed! LAZY EVICTION TRIGGERED.
            cacheList.erase(it->second); // Remove from linked list
            cacheMap.erase(it);          // Remove from dictionary
            return "";                   // Act like we never found it
        }
    cacheList.splice(cacheList.begin(),cacheList,it->second);
    return it->second->value;
}
//back front 
void LRUcache::put(const string& key,const string& value,long long ttlMs){
    std::lock_guard<std::mutex> lock(mtx);
    auto it=cacheMap.find(key);
    if(it!=cacheMap.end()){
        it->second->value=value;
        it->second->expiryTime = (ttlMs > 0) ? getCurrentTimeMs() + ttlMs : 0;
        cacheList.splice(cacheList.begin(),cacheList,it->second);
        return;
    }
    if(cacheMap.size()==capacity){
        const Node& lru_node=cacheList.back();
        cacheMap.erase(lru_node.key);
        cacheList.pop_back();
    }
    cacheList.emplace_front(key,value);
    cacheList.front().expiryTime = (ttlMs > 0) ? getCurrentTimeMs() + ttlMs : 0;
    cacheMap[key]=cacheList.begin();
}
}