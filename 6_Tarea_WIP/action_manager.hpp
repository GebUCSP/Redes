#include <iostream>
#include <string>
#include <unistd.h>

using namespace std;

class ActionManager{
public:
    void displayMessage(string nickname, string msg) {
        cout << "[" << nickname << "]: " << msg << endl;
    }

    void displayWisper(string source,string dest, string msg) {
        cout << "[" << source << "] to [" << dest << "]: " << msg << endl;
    }

    string lengthString(string s, int size){
        string s_size = to_string(s.size());
        s_size.insert(0, size - s_size.size(), '0');
        return s_size;
    }

    string readString(int socketFD, int size){
        string buffer(256,0);
        int n = read(socketFD, buffer.data(),size);
        string s = string(buffer.data(),n);
        return s;
    }

    int readInt(int socketFD, int size){
        string buffer(256,0);
        int n = read(socketFD, buffer.data(),size);
        int s = stoi(string(buffer.data(),n));
        return s;
    }
};