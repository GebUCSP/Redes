#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <map>

using namespace std;

struct Datagram{
    bool checksum_valid;
    string identifier;
    int seq_number;
    string payload;
};


class DatagramManager{
private:
    const int DATAGRAM_SIZE = 500;
    const int HEADER_SIZE = 7;
    const int PAYLOAD_SIZE = 493;

    map<int,string> chunks;
public:

    int checksum(string payload){
        int sum = 0;
        for(const char c: payload)
            sum += (unsigned char)c;
        
        return (sum % 7);
    }

    string lengthString(string s, int size){
        string s_size = to_string(s.size());
        s_size.insert(0, size - s_size.size(), '0');
        return s_size;
    }

        void displayMessage(string nickname, string msg) {
        cout << "[" << nickname << "]: " << msg << endl;
    }

    void displayWisper(string source,string dest, string msg) {
        cout << "[" << source << "] to [" << dest << "]: " << msg << endl;
    }

    string readDatagram(int socketFD){
        string buffer(DATAGRAM_SIZE, 0);
        sockaddr_in senderAddr{};
        socklen_t len = sizeof(senderAddr);
        int n = recvfrom(socketFD, buffer.data(), buffer.size(),0, (sockaddr*)&senderAddr, &len);
        if (n <= 0)
            return "";
        return buffer;
    }

    void sendDatagram(int socketFD, sockaddr_in& senderAddr, string datagram){
        sendto(socketFD, datagram.data(), datagram.size(),0, (sockaddr*)&senderAddr, sizeof(senderAddr));
    }

    vector<string> wrap(const string& data){
        vector<string> datagrams;

        int total = data.size();
        int offset = 0;
        int chunk_index = 0;
        int total_chunks = (total + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE;
        
        while (offset < total){
            string datagram;

            int chunk_size = min(total - offset, PAYLOAD_SIZE);

            if ((offset == 0 && offset + chunk_size >= total) || offset + chunk_size >= total)
            {
                datagram = "11";
            }else if (offset == 0) {
                datagram = "01";
            }else{
                datagram = "00";
            }

            datagram = datagram + lengthString(to_string(chunk_index), 4) + data.substr(offset, chunk_size);             
            datagram += string(PAYLOAD_SIZE - chunk_size, '#');
            datagram = to_string(checksum(datagram)) + datagram; 

            datagrams.push_back(datagram);
            chunk_index++;
            offset += chunk_size;
        }

        return datagrams;
    }

    Datagram parse(const string& datagram){
        int own_checksum = checksum(datagram.substr(1, DATAGRAM_SIZE - 1));

        int datagram_checksum = datagram[0] - '0';

        if(datagram_checksum != own_checksum){
            return Datagram{false, "", 0, ""};
        }

        string identifier = datagram.substr(1,2); 
        int seq_number = stoi(datagram.substr(3,4));
        string payload = datagram.substr(7, PAYLOAD_SIZE);

        for(int i = 0; i < payload.size(); i++){
            if(payload[i] == '#'){
                payload = payload.substr(0,i);
                break;
            }
        }
        return Datagram{true,identifier,seq_number,payload};
    } 

    string reassemble(Datagram datagram){
        chunks[datagram.seq_number] = datagram.payload;
        
        if(datagram.identifier == "11"){
            string msg = "";
            for(int i = 0; i <= datagram.seq_number; i++){
                msg += chunks[i];
            }
            chunks.clear();
            return msg;
        }

        return "";
    }   
};