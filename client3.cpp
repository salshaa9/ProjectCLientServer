#include <iostream>
#include <cstring>
#include <SDL2/SDL_net.h>
#include <vector>
#include <sstream>

using namespace std;

#define BUFFER_SIZE 1024

struct Question {
    int level;
     string content;
     vector< string> answerList;
};

Question decodeQuestion(const  string& message) {
    Question question;
     istringstream iss(message);
     string line;

    // Read the question level
     getline(iss, line);
    question.level =  stoi(line.substr(line.find(":") + 1));

    // Read the question content
     getline(iss, question.content);

    // Read the answer options
    while ( getline(iss, line)) {
        question.answerList.push_back(line);
    }

    return question;
}

int main(int argc, char* argv[]) {
    if (SDLNet_Init() == -1) {
         cerr << "SDLNet_Init failed: " << SDLNet_GetError() <<  endl;
        return -1;
    }

    IPaddress ip;
    TCPsocket clientSocket;
    const char* serverAddress = "127.0.0.1";
    int port = 45951;

    if (SDLNet_ResolveHost(&ip, serverAddress, port) == -1) {
         cerr << "SDLNet_ResolveHost failed: " << SDLNet_GetError() <<  endl;
        SDLNet_Quit();
        return -1;
    }

    // Open a connection to the server
    clientSocket = SDLNet_TCP_Open(&ip);
    if (!clientSocket) {
         cerr << "SDLNet_TCP_Open failed: " << SDLNet_GetError() <<  endl;
        SDLNet_Quit();
        return -1;
    }

     cout << "Connected to the server" <<  endl;

    // Receive welcome message from the server
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
         cerr << "Error receiving welcome message: " << SDLNet_GetError() <<  endl;
        SDLNet_TCP_Close(clientSocket);
        SDLNet_Quit();
        return -1;
    }

     cout << buffer <<  endl;

    // Send client's name to the server
     string name;
     cout << "Enter your name: ";
     getline( cin, name);
    if (SDLNet_TCP_Send(clientSocket, name.c_str(), name.length()) <= 0) {
         cerr << "Error sending name: " << SDLNet_GetError() <<  endl;
        SDLNet_TCP_Close(clientSocket);
        SDLNet_Quit();
        return -1;
    }

    // Game loop
    while (true) {
        int choice;
         cout << "Who wants to be a millionaire" <<  endl;
         cout << "1. Start Game" <<  endl;
         cout << "2. Challenge another player" <<  endl;
         cout << "Other to quit" <<  endl;
         cin >> choice;
         cin.ignore(); // Add this line to discard the newline character

        switch (choice) {
            case 1:
                if (SDLNet_TCP_Send(clientSocket, "START_GAME", strlen("START_GAME")) <= 0) {
                     cerr << "Error sending START_GAME message: " << SDLNet_GetError() <<  endl;
                    SDLNet_TCP_Close(clientSocket);
                    SDLNet_Quit();
                    return -1;
                }

        
        while (true) {
       memset(buffer, 0, BUFFER_SIZE);
                    if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
                        std::cerr << "Error receiving server message: " << SDLNet_GetError() << std::endl;
                        SDLNet_TCP_Close(clientSocket);
                        SDLNet_Quit();
                        return -1;
                    } else if (strcmp(buffer, "GAME_OVER") == 0) {
                        std::cout << "You answered incorrectly. " << buffer << std::endl;
                        memset(buffer, 0, BUFFER_SIZE);
                        if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
                            std::cerr << "Error receiving score message: " << SDLNet_GetError() << std::endl;
                            SDLNet_TCP_Close(clientSocket);
                            SDLNet_Quit();
                            return -1;
                        }
                        std::cout << buffer << std::endl;
                        break;
                    } else {
                        // Decode the server's question message
                        Question question = decodeQuestion(buffer);

                        // Print the question
                        std::cout << "Level: " << question.level << std::endl;
                        std::cout << "Question: " << question.content << std::endl;
                        std::cout << "Options:" << std::endl;
                        for (const std::string& option : question.answerList) {
                            std::cout << option << std::endl;
                        }

                        std::string clientAnswer;
                        std::getline(std::cin, clientAnswer);
                        if (SDLNet_TCP_Send(clientSocket, clientAnswer.c_str(), clientAnswer.length()) <= 0) {
                            std::cerr << "Error sending answer: " << SDLNet_GetError() << std::endl;
                            SDLNet_TCP_Close(clientSocket);
                            SDLNet_Quit();
                            return -1;
                        }
                    }
        }
    }

    // Close the client socket
    SDLNet_TCP_Close(clientSocket);
    SDLNet_Quit();

    return 0;
}
}
