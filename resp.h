#include<iostream>
#include<string>
#include<vector>
using namespace std;

string encodeSimpleString(const string& message) {
    return "+" + message + "\r\n";
}

string encodeError(const string& errorMessage) {
    return "-" + errorMessage + "\r\n";
}

string encodeBulkString(const string& data) {
    return "$" + to_string(data.length()) + "\r\n" + data + "\r\n";
}

string encodeNull() {
    return "$-1\r\n";
}

vector<string> parseRESP(const string& buffer) {
    vector<string> args;
    
    // Check if the message is actually a RESP Array
    if (buffer.empty() || buffer[0] != '*') {
        return args; // Return empty, indicating a bad command
    }

    // Find the end of the first line (*3\r\n)
    size_t pos = buffer.find("\r\n");
    if (pos == string::npos) return args;

    // Extract the number of arguments (the '3' from '*3')
    int numArgs = stoi(buffer.substr(1, pos - 1));
    pos += 2; // Move past the "\r\n"

    // Loop through the array and extract each string
    for (int i = 0; i < numArgs; ++i) {
        // We should be at a '$' now
        if (pos >= buffer.length() || buffer[pos] != '$') break;

        // Find the length of the string
        size_t nextPos = buffer.find("\r\n", pos);
        int strLen = stoi(buffer.substr(pos + 1, nextPos - pos - 1));
        
        pos = nextPos + 2; // Move to the actual data
        
        // Extract the exact number of bytes safely!
        string arg = buffer.substr(pos, strLen);
        args.push_back(arg);
        
        // Move past the data and the final "\r\n" to setup for the next item
        pos += strLen + 2;
    }

    return args;
}