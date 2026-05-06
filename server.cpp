#include<iostream>
#include<winsock2.h>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#pragma comment(lib, "ws2_32.lib")
#include "lru_cache.h"
#include "resp.h"
using namespace std;
using namespace VVR;
queue<pair<SOCKET, string>> jobQueue;
mutex queueMutex;
condition_variable queueCondVar;
// ==========================================
// PHASE 5: THE WORKER THREAD LOGIC
// ==========================================
void appendToAOF(const string& rawCommand) {
    // Open the file in Append mode
    ofstream aofFile("database.aof", ios::app); 
    
    if (aofFile.is_open()) {
        aofFile << rawCommand;
        aofFile.close();
    } else {
        cerr << "[WARNING] Failed to write to AOF log!" << endl;
    }
}
void loadFromAOF(LRUcache& db) {
    ifstream aofFile("database.aof");
    
    // If the file doesn't exist yet (first time running), just skip.
    if (!aofFile.is_open()) {
        cout << "[AOF] No existing database found. Starting fresh." << endl;
        return;
    }

    string line;
    int restoredCount = 0;
    
    // Read the file line by line
    while (getline(aofFile, line)) {
        // Fix for Windows: getline removes \n, but leaves the \r. We must remove it!
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty()) continue;

        // If we see an Array marker (*), a command is starting!
        if (line[0] == '*') {
            int numArgs = stoi(line.substr(1)); // Extract the number (e.g., '3')
            vector<string> args;
            
            // Loop to read the next lines based on how many arguments there are
            for (int i = 0; i < numArgs; ++i) {
                string lenLine, dataLine;
                getline(aofFile, lenLine);  // Read the $length line (we can ignore it here)
                getline(aofFile, dataLine); // Read the actual data!
                
                if (!dataLine.empty() && dataLine.back() == '\r') dataLine.pop_back();
                args.push_back(dataLine);
            }

            // Directly inject the command into the DB, bypassing the network entirely!
            if (args.size() >= 3 && args[0] == "PUT") {
                db.put(args[1], args[2],60000);
                restoredCount++;
            }
        }
    }
    cout << "[AOF] Boot sequence complete. Restored " << restoredCount << " operations into RAM!" << endl;
}
string handleClientCommand(const string& command, LRUcache& db) {
    // 1. Call our new parser! 
    vector<string> args = parseRESP(command);
    
    // 2. Protect against empty or garbage data
    if (args.empty()) {
        return encodeError("ERR protocol error");
    }

    string action = args[0]; // "PUT" or "GET"
    
    // 3. Process the clean arguments
    if (action == "PUT" && args.size() >= 3) {
        string key = args[1];
        string value = args[2];
        db.put(key, value,60000);
        appendToAOF(command);
        return encodeSimpleString("OK");
    }
    else if (action == "GET" && args.size() >= 2) {
        string key = args[1];
        string result = db.get(key);
        if (result == "") {
            return encodeNull();
        } else {
            return encodeBulkString(result);
        }
    }
    
    return encodeError("ERR unknown command");
}
void workerThread(LRUcache* db, int threadID) {
    cout << "[SYSTEM] Worker Thread " << threadID << " is online and waiting." << endl;
    
    // The worker lives forever in this loop
    while (true) {
        pair<SOCKET, string> job;
        
        // --- STEP 1 & 2: GET THE TICKET ---
        { 
            // We use unique_lock here instead of lock_guard because condition_variables require it
            unique_lock<mutex> lock(queueMutex);
            
            // Go to sleep! The OS pauses this thread (0% CPU) until the bell rings AND the queue is not empty.
            queueCondVar.wait(lock, []{ return !jobQueue.empty(); });
            
            // We woke up! Grab the ticket at the front of the line.
            job = jobQueue.front();
            jobQueue.pop();
            
        } // <--- MAGIC HAPPENS HERE: The lock is automatically released so other workers can grab tickets!

        // --- STEP 3: COOK AND SERVE ---
        SOCKET client = job.first;
        string command = job.second;

        // Process the command. (Remember, handleClientCommand will safely lock the DB for us!)
        string response = handleClientCommand(command, *db);

        // Send the response back to the client over the network
        send(client, response.c_str(), response.length(), 0);
    }
}
int main(){
    LRUcache db(100);
    loadFromAOF(db);
    cout << "--- Starting Asynchronous Redis-Lite Server ---" << endl;
    int NUM_THREADS = 4;
    vector<thread> threadPool;
    for (int i = 0; i < NUM_THREADS; i++) {
        // Spawn a thread, give it the workerThread function, a pointer to the DB, and an ID
        threadPool.push_back(thread(workerThread, &db, i + 1));
        
        // Detach tells the OS to let this thread run independently in the background forever
        threadPool.back().detach(); 
    }
    WSADATA wsaData;
    int startupResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (startupResult != 0) {
        cerr << "FATAL ERROR: Windows networking failed to start." << endl;
        return 1;
    }
    cout << "[OK] Network drivers loaded." << endl;
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "FATAL ERROR: Could not create socket." << endl;
        WSACleanup(); // Clean up the drivers before exiting
        return 1;
    }
    cout << "[OK] TCP Socket created successfully." << endl;
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_port = htons(8080); // The port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, SOMAXCONN);
    cout << "[OK] Server is now listening on Port 8080..." << endl;

    cout << "Waiting for a client to connect..." << endl;
    vector<SOCKET> activeClients;
    while(1){
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);
        for (SOCKET client : activeClients) {
            FD_SET(client, &readSet);
        }
        select(0, &readSet, nullptr, nullptr, nullptr);
        if (FD_ISSET(serverSocket, &readSet)) {
            SOCKET newClient = accept(serverSocket, nullptr, nullptr);
            activeClients.push_back(newClient); // Add them to our active list
            cout << "[NETWORK] New client joined! Total active: " << activeClients.size() << endl;
        }
        for (int i = 0; i < activeClients.size(); i++) {
            SOCKET client = activeClients[i];
            
            if (FD_ISSET(client, &readSet)) {
                char buffer[1024] = {0};
                int bytesReceived = recv(client, buffer, sizeof(buffer), 0);
                
                if (bytesReceived <= 0) {
                    // If recv gets 0 or less, the client closed the connection
                    cout << "[NETWORK] Client disconnected." << endl;
                    closesocket(client);
                    activeClients.erase(activeClients.begin() + i);
                    i--; // Adjust our loop index since we removed an item
                } else {
                    // We received a valid message! Process it.
                    string clientMessage(buffer);
                    { // Lock the queue just long enough to drop the ticket
                        lock_guard<mutex> lock(queueMutex);
                        jobQueue.push({client, clientMessage});
                    } // Lock automatically releases here
                    queueCondVar.notify_one();
                }
            }
        }
    }
    closesocket(serverSocket);
    WSACleanup();
    cout << "Server shut down cleanly." << endl;
    return 0;
}