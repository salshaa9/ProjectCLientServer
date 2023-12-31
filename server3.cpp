#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <mutex>
#include <algorithm>
#include "SDL2/SDL_net.h"

#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024

struct Client {
    TCPsocket socket;
    std::string name;
    int answeredQuestions; // Tambahkan atribut untuk melacak berapa banyak pertanyaan yang telah dijawab oleh pemain.
    int score;
};


std::mutex mutex;

struct Question {
    int level;
    std::string content;
    std::vector<std::string> answerList;
    std::string correctAnswer;
};

std::vector<Question> questions = {
    {1, "What is the capital of France?", {"London", "Paris", "Berlin", "Madrid"}, "Paris"},
    {2, "Who wrote the novel 'Pride and Prejudice'?", {"Jane Austen", "Charles Dickens", "Mark Twain", "Leo Tolstoy"}, "Jane Austen"},
    {3, "What is the chemical symbol for the element Gold?", {"Go", "Gd", "Au", "Ag"}, "Au"},
    {4, "Which planet is known as the Red Planet?", {"Mars", "Venus", "Jupiter", "Saturn"}, "Mars"},
    {5, "What is the largest ocean on Earth?", {"Atlantic Ocean", "Indian Ocean", "Arctic Ocean", "Pacific Ocean"}, "Pacific Ocean"}
};

std::vector<Client> clients;

std::string generateScoreboard() {
                std::string scoreboard = "Scoreboard:\n";

                for (const Client& client : clients) {
                    scoreboard += client.name + " - Score: " + std::to_string(client.score) + "\n";
                }

                return scoreboard;
                }



void handleClient(void* data) {
    TCPsocket clientSocket = static_cast<TCPsocket>(data);
    
    if (SDLNet_TCP_Send(clientSocket, "Welcome Client", sizeof("Welcome Client")) < sizeof("Welcome Client")) {
        std::cerr << "Error sending welcome message: " << SDLNet_GetError() << std::endl;
        SDLNet_TCP_Close(clientSocket);
        return;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
        std::cout << "Error receiving client name: " << SDLNet_GetError() << std::endl;
        SDLNet_TCP_Close(clientSocket);
        return;
    }

    mutex.lock();
    Client client;
    client.socket = clientSocket;
    client.name = buffer;
    client.score = 0;
    clients.push_back(client);
    mutex.unlock();

    std::cout << "Client connected: " << client.name << std::endl;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
            std::cout << "Error receiving client message: " << SDLNet_GetError() << std::endl;
            SDLNet_TCP_Close(clientSocket);
            return;
        } else if (strcmp(buffer, "START_GAME") == 0) {
            int questionIndex = 0;
            while (true) {
                if (questionIndex >= questions.size()) {
                    break;
                }

                Question currentQuestion = questions[questionIndex];

                std::string message = "Question Level: " + std::to_string(currentQuestion.level) + "\n";
                message += currentQuestion.content + "\n";

                for (const std::string& answer : currentQuestion.answerList) {
                    message += answer + "\n";
                }

                if (SDLNet_TCP_Send(clientSocket, message.c_str(), message.length()) < message.length()) {
                    std::cout << "Error sending question: " << SDLNet_GetError() << std::endl;
                    break;
                }

                memset(buffer, 0, BUFFER_SIZE);
                if (SDLNet_TCP_Recv(clientSocket, buffer, BUFFER_SIZE) <= 0) {
                    std::cout << "Error receiving answer: " << SDLNet_GetError() << std::endl;
                    break;
                }


                
                std::string clientAnswer = buffer;

                if (clientAnswer == currentQuestion.correctAnswer) {
                    std::cout << "Client answered correctly" << std::endl;
                    // Menambahkan 100 poin jika pemain menjawab benar
                    client.score += 100;
                    client.answeredQuestions++;
                } else {
                    std::cout << "Client answered incorrectly" << std::endl;
                    // Mengurangkan 100 poin jika pemain menjawab salah
                    client.score -= 100;
                    client.answeredQuestions++;
                }

                // Setelah pemain menjawab, tampilkan papan skor
                std::string scoreboard = generateScoreboard();

                // Kirim papan skor ke semua pemain
                for (const Client& otherClient : clients) {
                    if (SDLNet_TCP_Send(otherClient.socket, scoreboard.c_str(), scoreboard.length()) < scoreboard.length()) {
                        std::cerr << "Error sending scoreboard: " << SDLNet_GetError() << std::endl;
                    }
                }

                // if (clientAnswer == currentQuestion.correctAnswer) {
                //     std::cout << "Client answered correctly" << std::endl;
                //     client.score++;
                // } else {
                //     std::cout << "Client answered incorrectly" << std::endl;

                //     if (SDLNet_TCP_Send(clientSocket, "GAME_OVER", sizeof("GAME_OVER")) < sizeof("GAME_OVER")) {
                //         std::cout << "Error sending GAME_OVER message: " << SDLNet_GetError() << std::endl;
                //         break;
                //     }

                //     std::string score_msg = "SCORE: " + std::to_string(client.score);
                //     if (SDLNet_TCP_Send(clientSocket, score_msg.c_str(), score_msg.length()) < score_msg.length()) {
                //         std::cout << "Error sending SCORE message: " << SDLNet_GetError() << std::endl;
                //         break;
                //     }
                //     break;
                // }

                questionIndex++;
            }
        }
    }

    SDLNet_TCP_Close(clientSocket);

    mutex.lock();
    clients.erase(std::remove_if(clients.begin(), clients.end(), [clientSocket](const Client& c) { return c.socket == clientSocket; }), clients.end());
    mutex.unlock();
}

int main(int argc, char* argv[]) {
    if (SDLNet_Init() < 0) {
        std::cerr << "SDLNet_Init failed: " << SDLNet_GetError() << std::endl;
        return -1;
    }


    IPaddress ip;
if (SDLNet_ResolveHost(&ip, "0.0.0.0", 54321) < 0) {
    std::cerr << "SDLNet_ResolveHost failed: " << SDLNet_GetError() << std::endl;
    SDLNet_Quit();
    return -1;
}


    TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
    if (!serverSocket) {
        std::cerr << "SDLNet_TCP_Open failed: " << SDLNet_GetError() << std::endl;
        SDLNet_Quit();
        return -1;
    }

    std::cout << "Server started. Listening on port 45951" << std::endl;

    while (true) {
        TCPsocket clientSocket = SDLNet_TCP_Accept(serverSocket);
        if (clientSocket == nullptr) {
            std::cerr << "Failed to accept client connection: " << SDLNet_GetError() << std::endl;
            continue;
        }

        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    SDLNet_TCP_Close(serverSocket);
    SDLNet_Quit();

    return 0;
}
