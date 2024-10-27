# Multithread Client-Server Application

## List of contents
- [Overview](#Overview)
- [Features](#Features)
- [Requirements](#Requirements)
- [Usage](#Usage)
- [Architecture](#Architecture)
- [Screenshots](#Screenshots)
- [Contact](#Contact)

## Overview
This project implements a multithreaded client-server application in C++ for the Windows platform. It utilizes Winsock API for network communication and demonstrates basic socket programming concepts such as TCP/IP connections and multithreading.

## Features
- **Multithreaded Server**: Handles multiple client connections concurrently using threads.
- **Client Application**: Connects to the server and sends/receives messages.
- **Simple Protocol**: Defines a basic protocol for communication between clients and the server.
- **Error Handling**: Includes error checking and handling for socket operations.

## Requirements
- Windows OS
- Visual Studio (or another C++ compiler with Winsock support)

## Usage
1. **Clone the Repository**:
   ```sh
      git clone https://github.com/Neamen1/Multithread-client-server.git
   ```
2. Build the Project:
  - Open the project in Visual Studio.
  - Build the solution to compile both the client and server applications.

3. Run the Server:
  - Navigate to the server executable.
  - Run the executable to start the server.

4. Run the Client:
  - Navigate to the client executable.
  - Run the executable to start the client.

5. Interact with the Client-Server Application:
  - Use the client interface to connect to the server, send messages, and receive responses.

## Architecture
The project is structured into two main components:

- Server: Implements the server-side logic, manages incoming client connections, and handles communication using threads.
- Client: Provides a simple command-line interface for users to connect to the server, send messages, and display responses.

## Screenshots
<div align="center">
  <img src="https://github.com/user-attachments/assets/bc5d054b-4116-49c1-9ba8-83539db22fe9" alt="Server and 4 clients working simultaneously"/>
  <p>Figure 1: Server and 4 clients working simultaneously</p>
</div>

## Contact
- [Oleksii](mailto:o.rakytskyi@gmail.com)
