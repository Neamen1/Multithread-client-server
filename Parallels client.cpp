#include <iostream>
#include <vector>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#include <random>

using namespace std;

#pragma comment(lib, "ws2_32.lib") // Çâ'ÿçîê ç á³áë³îòåêîþ Winsock

void initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed." << endl;
        exit(1);
    }
}

SOCKET createSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        cerr << "Failed to create socket." << endl;
        WSACleanup();
        exit(2);
    }
    return sock;
}

void connectToServer(SOCKET sock, const char* serverIP, int port) {
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIP, &serverAddr.sin_addr);

    //serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to connect to the server." << endl;
        int iError = WSAGetLastError();
        if (iError == WSAEWOULDBLOCK)
            printf("recv failed with error: WSAEWOULDBLOCK\n");
        else
            printf("recv failed with error: %ld\n", iError);
        closesocket(sock);
        WSACleanup();
        system("pause");
        exit(3);
    }
}

void fillMatrixWithRandomNumbers(vector<vector<int>>& matrix) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(1, 100);

    for (auto& row : matrix) {
        for (auto& elem : row) {
            elem = dis(gen);
        }
    }
}

bool sendMatrixRow(SOCKET sock, const vector<int>& row) {
    int bytesToSend = row.size() * sizeof(int);
    const char* buffer = reinterpret_cast<const char*>(row.data());
    while (bytesToSend > 0) {
        int sent = send(sock, buffer, bytesToSend, 0);
        if (sent == SOCKET_ERROR) {
            cerr << "Failed to send data." << endl;
            return false;
        }
        buffer += sent;
        bytesToSend -= sent;
    }
    return true;
}

bool receiveMatrixRow(SOCKET sock, vector<int>& row) {
    int bytesToReceive = row.size() * sizeof(int);
    char* buffer = reinterpret_cast<char*>(row.data());
    while (bytesToReceive > 0) {
        int received = recv(sock, buffer, bytesToReceive, 0);
        if (received == SOCKET_ERROR || received == 0) {
            return false;
        }
        buffer += received;
        bytesToReceive -= received;
    }
    return true;
}

bool performHandshake(SOCKET sock) {
    const char* helloMessage = "HELLO_SERVER";
    if (send(sock, helloMessage, strlen(helloMessage) + 1, 0) == SOCKET_ERROR) {
        cerr << "Failed to send handshake message." << endl;
        return false;
    }

    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cerr << "Failed to receive handshake response or connection closed." << endl;
        return false;
    }

    if (strcmp(buffer, "HELLO_CLIENT") != 0) {
        cerr << "Invalid handshake response." << endl;
        return false;
    }
    cout << buffer << endl;
    return true;
}

struct MessageHeader {
    uint8_t type;
    uint32_t size; // Ðîçì³ð ìàòðèö³ ó îäèíèöÿõ
};

enum MessageType {
    MATRIX_REQUEST = 1,
    STATUS_CHECK = 2,
    RESULT_REQUEST = 3,
    RESULT_NOT_READY = 4
};

bool sendHeader(SOCKET sock, uint32_t size, MessageType type) {
    MessageHeader header;
    header.type = type; // e.g. REQUEST_MATRIX
    header.size = size;

    // Â³äïðàâêà çàãîëîâêà
    int sent = send(sock, reinterpret_cast<char*>(&header), sizeof(header), 0);
    if (sent == SOCKET_ERROR) {
        return false;
    }
    return true;
}

bool checkProcessingStatus(SOCKET sock) {
    if (!sendHeader(sock, 0, STATUS_CHECK)) {
        cerr << "Failed to send status check header." << endl;
        return false;
    }

    char buffer[2];
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cerr << "Failed to receive status response or connection closed." << endl;
        return false;
    }

    if (buffer[0] == '1') {
        cout << "Matrix is still being processed." << endl;
    } else {
        cout << "Matrix processing is complete." << endl;
    }

    return true;
}

bool requestProcessedMatrix(SOCKET sock, vector<vector<int>>& matrix) {
    uint32_t size = matrix.size();
    if (!sendHeader(sock, size, RESULT_REQUEST)) {
        cerr << "Failed to send result request header." << endl;
        return false;
    }

    // Receive the result status
    char buffer[2];
    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cerr << "Failed to receive result status or connection closed." << endl;
        return false;
    }

    if (buffer[0] == '0') {
        //cout << "Matrix has not been processed yet." << endl;
        return false;
    }
    else if (buffer[0] == '1') {
        for (auto& row : matrix) {
            if (!receiveMatrixRow(sock, row)) {
                cerr << "Failed to receive matrix row." << endl;
                return false;
            }
        }
        return true;
    }

    return false;
}

void cleanExit(SOCKET &sock) {
    closesocket(sock);
    WSACleanup();
}

void userInteractionLoop(SOCKET sock) {
    while (true) {
        cout << "Enter the size of the matrix or 0 to exit: ";
        uint32_t size;
        cin >> size;

        if (size == 0) {
            break;
        }

        vector<vector<int>> matrix(size, vector<int>(size));
        fillMatrixWithRandomNumbers(matrix);
        if (size < 20) {
            cout << "Generated matrix:" << endl;
            for (const auto& row : matrix) {
                for (int val : row) {
                    cout << val << " ";
                }
                cout << endl;
            }
        }
        else {
            cout << "Generated matrix." << endl;
        }

        if (!sendHeader(sock, size, MATRIX_REQUEST)) {
            cerr << "Failed to send matrix header." << endl;
            closesocket(sock);
            WSACleanup();
            return;
        }

        // Â³äïðàâëåííÿ ìàòðèö³
        for (auto& row : matrix) {
            if (!sendMatrixRow(sock, row)) {
                cerr << "Failed to send matrix row." << endl;
                continue;
            }
        }

        cout << "Matrix sent. You can check the status or wait for processing to complete." << endl;

        // Poll for status
        while (true) {
            cout << "Do you want to check the status of matrix processing or request the result? (status/result/exit): ";
            string answer="";
            cin >> answer;
            if (answer == "status") {
                checkProcessingStatus(sock);
            }
            else if (answer == "result") {
                if (requestProcessedMatrix(sock, matrix)) {
                    if (size < 20) {
                        cout << "Processed matrix:" << endl;
                        for (const auto& row : matrix) {
                            for (int val : row) {
                                cout << val << " ";
                            }
                            cout << endl;
                        }
                    }
                    else {
                        cout << "Matrix processed and received." << endl; 
                    }
                    break;
                }
                else {
                    cout << "Matrix not ready yet or failed to receive the processed matrix." << endl;
                }
                
            }
            else if (answer == "exit") {
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }
}

int main() {
    initializeWinsock();

    SOCKET sock = createSocket();
    connectToServer(sock, "127.0.0.1", 12345); // Ï³äêëþ÷åííÿ äî ñåðâåðà çà àäðåñîþ 127.0.0.1 ³ ïîðòîì 12345

    if (!performHandshake(sock)) {
        cerr << "Handshake failed." << endl;
        cleanExit(sock);
        return 1;
    }
    thread interactionThread(userInteractionLoop, sock);
    interactionThread.join();

    cleanExit(sock);
    return 0;
}
