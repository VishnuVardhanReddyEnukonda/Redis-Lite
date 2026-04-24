#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
using namespace std;
class LRUcache{
private:
        struct Node{
            string key;
            string value;
            Node(const string& k,const string& v){
                key=k;
                value=v;
            }
        };
        int capacity;
        list<Node> cacheList;
        unordered_map<string,list<Node>::iterator> cacheMap;
public:
       LRUcache(int c){
        capacity=c;
       }
       string get(const string& key);
       void put(const string& key,const string& value);
};