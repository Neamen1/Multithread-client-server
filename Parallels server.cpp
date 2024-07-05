#include <vector>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <string>
#include <random>
#include <mutex>

using namespace std;

#pragma comment(lib, "ws2_32.lib") // Зв'язок з бібліотекою Winsock

// Ініціалізація WinSock
void initWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed.\n";
        exit(1);
    }
}

// Помічник для читання даних з сокета
bool readFromSocket(SOCKET sock, char* buffer, int length) {
    int bytesReceived = 0;
    while (bytesReceived < length) {
        int result = recv(sock, buffer + bytesReceived, length - bytesReceived, 0);
        if (result == SOCKET_ERROR) {
            cout << "Socket error while receiving data." << endl;
            return false;
        }
        bytesReceived += result;
    }
    return true;
}

// Помічник для відправлення даних через сокет
bool sendToSocket(SOCKET sock, const char* buffer, int length) {
    int bytesSent = 0;
    while (bytesSent < length) {
        int result = send(sock, buffer + bytesSent, length - bytesSent, 0);
        if (result == SOCKET_ERROR) {
            cout << "Socket error while sending data." << endl;
            return false;
        }
        bytesSent += result;
    }
    return true;
}

// Обробка матриці
void processMatrix(vector<vector<int>>& matrix) {
    int n = matrix.size();
    for (int i = 0; i < n; ++i) {
        int maxElementIndex = 0;
        for (int j = 0; j < n; ++j) {
            if (matrix[i][j] > matrix[i][maxElementIndex]) {
                maxElementIndex = j;
            }
        }
        swap(matrix[i][i], matrix[i][maxElementIndex]);
    }
}

// Відправлення рядка матриці
bool sendMatrixRow(SOCKET sock, const vector<int>& row) {
    const char* buffer = reinterpret_cast<const char*>(row.data());
    int length = row.size() * sizeof(int);
    return sendToSocket(sock, buffer, length);
}

// Читання рядка матриці
bool readMatrixRow(SOCKET sock, vector<int>& row) {
    char* buffer = reinterpret_cast<char*>(row.data());
    int length = row.size() * sizeof(int);
    return readFromSocket(sock, buffer, length);
}

struct MessageHeader {
    uint8_t type;
    uint32_t size;  // Розмір матриці у байтах
};

bool receiveHeader(SOCKET clientSocket, MessageHeader& header) {
    char buffer[sizeof(MessageHeader)];
    int bytesReceived = 0;
    int totalBytesToReceive = sizeof(MessageHeader);

    // Цикл для прийому повного заголовка
    while (bytesReceived < totalBytesToReceive) {
        int result = recv(clientSocket, buffer + bytesReceived, totalBytesToReceive - bytesReceived, 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Socket error while receiving data." << std::endl;
            return false;
        }
        else if (result == 0) {
            std::cerr << "Connection closed by the client." << std::endl;
            return false;
        }
        bytesReceived += result;
    }

    // Копіювання отриманих даних у структуру заголовка
    memcpy(&header, buffer, sizeof(MessageHeader));

    return true;
}

enum MessageType {
    MATRIX_REQUEST = 1,
    STATUS_CHECK = 2,
    RESULT_REQUEST = 3,
    RESULT_NOT_READY = 4
};

void handleClient(SOCKET clientSocket) {
    char buffer[1024];

    // Receive handshake message
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cerr << "Failed to receive handshake or connection closed." << endl;
        closesocket(clientSocket);
        return;
    }

    // Check if handshake message is correct
    if (strcmp(buffer, "HELLO_SERVER") != 0) {
        cerr << "Invalid handshake message." << endl;
        closesocket(clientSocket);
        return;
    }
    cout << buffer << endl;

    // Send handshake acknowledgment
    const char* ackMessage = "HELLO_CLIENT";
    if (send(clientSocket, ackMessage, strlen(ackMessage) + 1, 0) == SOCKET_ERROR) {
        cerr << "Failed to send acknowledgment." << endl;
        closesocket(clientSocket);
        return;
    }

    // Variables for processing state
    vector<vector<int>> matrix;
    bool processing = false;
    bool processed = false;
    mutex mtx;
    thread processingThread;

    while (true) {
        // Отримання розміру заголовку матриці (тип повідомлення та розмір матриці)
        MessageHeader header;
        if (!receiveHeader(clientSocket, header)) {
            closesocket(clientSocket);
            break;
        }

        // Перевірка типу повідомлення
        if (header.type == MATRIX_REQUEST) {
            
            unique_lock<mutex> lock(mtx);
            if (processing == false) {
                lock.unlock();
                matrix.resize(header.size, vector<int>(header.size));
                for (auto& row : matrix) {
                    row.resize(header.size);
                }

                // Receive the matrix
                for (uint32_t i = 0; i < header.size; ++i) {
                    if (!readMatrixRow(clientSocket, matrix[i])) {
                        cerr << "Failed to receive matrix row data." << endl;
                        continue;
                    }
                    if (header.size < 20) {
                        for (int j : matrix[i])
                            std::cout << j << ' ';
                        std::cout << endl;
                    }
                }

                // Start a new thread to process the matrix
                {
                    lock_guard<mutex> lock(mtx);
                    processing = true;
                    processed = false;
                }
                if (processingThread.joinable()) {
                    processingThread.join();
                }
                processingThread = thread([&matrix, &processing, &processed, &mtx]() {
                    processMatrix(matrix);
                    this_thread::sleep_for(chrono::milliseconds(25000)); // Simulate processing time
                    lock_guard<mutex> lock(mtx);
                    processing = false;
                    processed = true;
                    });
            }
            else {

            }
            
            
        }
        else if (header.type == STATUS_CHECK) {
            bool isProcessing;
            {
                lock_guard<mutex> lock(mtx);
                isProcessing = processing;
            }
            char statusMessage[2] = { isProcessing ? '1' : '0', '\0' };
            if (send(clientSocket, statusMessage, strlen(statusMessage) + 1, 0) == SOCKET_ERROR) {
                cerr << "Failed to send status response." << endl;
                closesocket(clientSocket);
                break;
            }
        }
        else if (header.type == RESULT_REQUEST) {
            {
                lock_guard<mutex> lock(mtx);
                if (!processed) {
                    char resultNotReadyMessage[2] = { '0', '\0' };
                    if (send(clientSocket, resultNotReadyMessage, strlen(resultNotReadyMessage) + 1, 0) == SOCKET_ERROR) {
                        cerr << "Failed to send result not ready response." << endl;
                        closesocket(clientSocket);
                        break;
                    }
                    continue;
                }
            }
            char resultReadyMessage[2] = { '1', '\0' };
            if (send(clientSocket, resultReadyMessage, strlen(resultReadyMessage) + 1, 0) == SOCKET_ERROR) {
                cerr << "Failed to send result ready response." << endl;
                closesocket(clientSocket);
                break;
            }
            for (uint32_t i = 0; i < header.size; ++i) {
                if (!sendMatrixRow(clientSocket, matrix[i])) {
                    cerr << "Failed to send matrix row data." << endl;
                    break;
                }
            }
        }
        else {
            cerr << "Unexpected message type." << endl;
            break;
        }

    }
    if (processingThread.joinable()) {
        processingThread.join();
    }
    closesocket(clientSocket);
}

// Головна функція сервера
int main() {
    initWinsock();

    SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);
    serverAddress.sin_port = htons(12345); // Використовувати порт 12345

    bind(listeningSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(listeningSocket, 5);   

    while (true) {
        SOCKET clientSocket = accept(listeningSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Accept failed.\n";
            int iError = WSAGetLastError();
            printf("recv failed with error: %ld\n", iError);
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    closesocket(listeningSocket);
    WSACleanup();
    return 0;
}
