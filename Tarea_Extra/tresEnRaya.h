#include <iostream>
#include <unistd.h>


struct MyStruct{
    char v1{' '},v2{' '},v3{' '},v4{' '},v5{' '},v6{' '},v7{' '},v8{' '},v9{' '};
    char current_turn = 'X';
    bool was_winned = false;
};

class Game{
public:

    MyStruct data;
    bool start;
    int socket;

    Game(int _socket, bool _start = false): socket(_socket), start(_start){}

    char changeTurn(){
        char last_turn = data.current_turn;

        if(data.current_turn == 'X'){
            data.current_turn = 'O';
        }else{
            data.current_turn = 'X';
        }

        return last_turn;
    }

    void show(){
        std::cout << data.v1 << '|' <<  data.v2 << '|' << data.v3 << std::endl;
        std::cout << "-----"<< std::endl;
        std::cout << data.v4 << '|' <<  data.v5 << '|' << data.v6 << std::endl;
        std::cout << "-----"<< std::endl;
        std::cout << data.v7 << '|' <<  data.v8 << '|' << data.v9 << std::endl;
    }

    void move(){
        std::cout << "Jugador, elige una casilla del 1 al 9 " << data.current_turn << ": ";
        int input;
        std::cin >> input;

        if(input == 1 && data.v1 == ' '){
            data.v1 = changeTurn();
        }else if(input == 2 && data.v2 == ' '){
            data.v2 = changeTurn();
        }else if(input == 3 && data.v3 == ' '){
            data.v3 = changeTurn();
        }else if(input == 4 && data.v4 == ' '){
            data.v4 = changeTurn();
        }else if(input == 5 && data.v5 == ' '){
            data.v5 = changeTurn();
        }else if(input == 6 && data.v6 == ' '){
            data.v6 = changeTurn();
        }else if(input == 7 && data.v7 == ' '){
            data.v7 = changeTurn();
        }else if(input == 8 && data.v8 == ' '){
            data.v8 = changeTurn();
        }else if(input == 9 && data.v9 == ' '){
            data.v9 = changeTurn();
        }else{
            move();
        }
    }

    void winnerCheck(){
        data.was_winned = 
            (data.v1 == data.v2 && data.v2 == data.v3 && data.v3 != ' ') || 
            (data.v4 == data.v5 && data.v5 == data.v6 && data.v6 != ' ') ||
            (data.v7 == data.v8 && data.v8 == data.v9 && data.v9 != ' ') ||
            (data.v1 == data.v4 && data.v4 == data.v7 && data.v7 != ' ') ||
            (data.v2 == data.v5 && data.v5 == data.v8 && data.v8 != ' ') ||
            (data.v3 == data.v6 && data.v6 == data.v9 && data.v9 != ' ') ||
            (data.v1 == data.v5 && data.v5 == data.v9 && data.v9 != ' ') ||
            (data.v3 == data.v5 && data.v5 == data.v7 && data.v7 != ' ');
    }

    void gameLoop(){
        if (start && !data.was_winned){
            show();
            move();
            winnerCheck();
            write(socket, (char*)&data, sizeof(data));
        }else{
            start = true;
        }
        if (data.was_winned){
            show();
            changeTurn();
            std::cout << "El ganador es el jugador " <<  data.current_turn << std::endl;
            return;
        }else{
            ssize_t total = 0;
            ssize_t n;

            while (total < sizeof(data)) {

                n = read(socket,
                    ((char*)&data) + total,
                    sizeof(data) - total);

                if (n <= 0) {
                    printf("Connection closed or error\n");
                    break;
                }

                total += n;
            }

            gameLoop();
        }
    }
};
