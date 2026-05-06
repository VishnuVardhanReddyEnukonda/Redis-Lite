#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <chrono>
#include<mutex>
using namespace std;
namespace VVR {

    // Helper function to get exact current time in milliseconds
    inline long long getCurrentTimeMs() {
        return chrono::duration_cast<chrono::milliseconds>(
            chrono::system_clock::now().time_since_epoch()
        ).count();
    }
class LRUcache{
private:
        struct Node{
            string key;
            string value;
            long long expiryTime;
            Node(const string& k,const string& v){
                key=k;
                value=v;
                expiryTime = 0;
            }
        };
        int capacity;
        list<Node> cacheList;
        unordered_map<string,list<Node>::iterator> cacheMap;
        std::mutex mtx;
public:
       LRUcache(int c){
        capacity=c;
       }
       string get(const string& key);
       void put(const string& key,const string& value,long long ttlMs = 0);
       void clear() {
            std::lock_guard<std::mutex> lock(mtx); // <--- Lock the door!
            cacheMap.clear();
            cacheList.clear();
        }
        bool expire(const string& key, long long ttlSeconds) {
            std::lock_guard<std::mutex> lock(mtx); // <--- Lock the door!
            auto it = cacheMap.find(key);
            if (it == cacheMap.end()) return false;
            it->second->expiryTime = getCurrentTimeMs() + (ttlSeconds * 1000);
            return true;
        }
};
}