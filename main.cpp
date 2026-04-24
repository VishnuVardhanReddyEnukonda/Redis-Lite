#include <iostream>
#include <string>
#include <sstream>
#include "lru_cache.h"

using namespace std;

// Your existing parser goes here:
string handleClientCommand(const string& command, LRUcache& db) {
    stringstream ss(command);
    string action, key, value;
    ss >> action;
    
    if(action == "PUT") {
        ss >> key >> value;
        if(key.empty() || value.empty()) {
            return "ERROR in key value format as action <key> <value>\n";
        }
        db.put(key, value);
        return "ok \n";
    }
    else if(action == "GET") {
        ss >> key;
        if(key.empty()) {
            return "error is in the key \n";
        }
        string result = db.get(key);
        if(result == "") {
            return "Not Found \n";
        }
        else {
            return result + "\n"; 
        }
    }
    
    return "unknown error use get or put\n"; 
}

// The new interactive terminal loop
int main() {
    // Let's set capacity to 2 so we can easily test the eviction logic!
    LRUcache db(2); 

    cout << "--- Interactive In-Memory Database Started ---" << endl;
    cout << "Commands: PUT <key> <value>  |  GET <key>  |  EXIT" << endl;
    cout << "----------------------------------------------" << endl;

    string input_command;
    
    // Infinite loop to keep asking for commands
    while (true) {
        cout << "db> "; // Print a prompt like a real database terminal
        
        // Read the entire line the user types
        getline(cin, input_command);

        // Allow the user to quit the program cleanly
        if (input_command == "EXIT" || input_command == "exit") {
            cout << "Shutting down database..." << endl;
            break; 
        }

        // Process the command and print the result
        if (!input_command.empty()) {
            string response = handleClientCommand(input_command, db);
            cout << response;
        }
    }

    return 0;
}