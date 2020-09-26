/*-------------------------------------------------------------------------
    This source file is a part of m8rscript
    For the latest info, see http:www.marrin.org/
    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "MacTCP.h"

#include "Containers.h"
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

using namespace m8r;

void MacTCP::init(uint16_t port, IPAddr ip, EventFunction func)
{
    TCP::init(port, ip, func);
    
    _socketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_socketFD == -1) {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            addEvent(TCP::Event::Error, -1, "opening TCP socket");
        }
        return;
    }

    int enable = 1;
    if (setsockopt(_socketFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        {
            std::unique_lock<std::mutex> lock(_mutex);
            addEvent(TCP::Event::Error, -1, "opening TCP socket, setsockopt failed");
        }
        return;
    }
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (ip) {
        inet_aton(ip.toString().c_str(), &(sa.sin_addr));
    }

    if (_server) {
        if (bind(_socketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                addEvent(TCP::Event::Error, -1, "TCP bind failed");
            }
            ::close(_socketFD);
            _socketFD = -1;
            return;
        }
      
        if (listen(_socketFD, MaxConnections) == -1) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                addEvent(TCP::Event::Error, -1, "TCP listen failed");
            }
            ::close(_socketFD);
            _socketFD = -1;
            return;
        }

        memset(_clientSockets, 0, MaxConnections * sizeof(int));
    } else {
        if (connect(_socketFD, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                addEvent(TCP::Event::Error, -1, "TCP connect failed");
            }
        } else {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                addEvent(TCP::Event::Connected, 0, nullptr);
            }
        }
    }

    std::thread dispatchThread([this, sa] {
        fd_set readfds;
        int maxsd;
        
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(_socketFD, &readfds);
            maxsd = _socketFD;
            
            if (_server) {
                for (int socket : _clientSockets) {
                    if (socket > 0) {
                        FD_SET(socket, &readfds);
                    }
                    if (socket > maxsd) {
                        maxsd = socket;
                    }
                }
            }
            
            int result = select(maxsd + 1, &readfds, NULL, NULL, NULL);
            if (result < 0 && errno != EINTR) {
                // Select failed, probably because we closed one of the sockets we
                // were waiting on. Ignore it
                //_delegate->TCPevent(this, TCPDelegate::Event::Error, errno, "select failed");
                continue;
            }
            
            if (_server) {
                if (FD_ISSET(_socketFD, &readfds)) {
                    // We have an incoming connection
                    int addrlen;
                    
                    int clientSocket = accept(_socketFD, (struct sockaddr *)&sa, (socklen_t*)&addrlen);
                    if (clientSocket < 0) {
                        {
                            std::unique_lock<std::mutex> lock(_mutex);
                            addEvent(TCP::Event::Error, -1, "accept failed");
                        }
                        continue;
                    }
                    printf("New connection , socket fd=%d, ip=%s, port=%d\n", clientSocket, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
                    int16_t connectionId = -1;
                    for (int& socket : _clientSockets) {
                        if (!socket) {
                            socket = clientSocket;
                            connectionId = &socket - _clientSockets;
                            break;
                        }
                    }
                    
                    if (connectionId < 0) {
                        close(clientSocket);
                        {
                            std::unique_lock<std::mutex> lock(_mutex);
                            addEvent(TCP::Event::Error, -1, "Too many connections on port");
                        }
                        break;
                    }
                    
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        addEvent(TCP::Event::Connected, connectionId, nullptr);
                    }
                }

                for (int& socket : _clientSockets) {
                    if (FD_ISSET(socket, &readfds)) {
                        // Something came in on this client socket
                        int16_t connectionId = &socket - _clientSockets;
                        
                        ssize_t result = read(socket, _receiveBuffer, BufferSize - 1);
                        if (result == 0) {
                            // Disconnect
                            int addrlen;
                            getpeername(socket, (struct sockaddr*) &sa, (socklen_t*) &addrlen);
                            printf("Host disconnected, ip=%s, port=%d\n" , inet_ntoa(sa.sin_addr) , ntohs(sa.sin_port));
                            {
                                std::unique_lock<std::mutex> lock(_mutex);
                                addEvent(TCP::Event::Disconnected, connectionId, nullptr);
                            }

                            close(socket);
                            socket = 0;
                        } else if (result < 0) {
                            {
                                std::unique_lock<std::mutex> lock(_mutex);
                                addEvent(TCP::Event::Error, connectionId, "read error");
                            }
                        } else {
                            {
                                std::unique_lock<std::mutex> lock(_mutex);
                                addEvent(TCP::Event::ReceivedData, connectionId, _receiveBuffer, static_cast<int32_t>(result));
                            }
                        }
                    }
                }
            } else {
                // Client, received data
                ssize_t result = read(_socketFD, _receiveBuffer, BufferSize - 1);
                if (result == 0) {
                    // Disconnect
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        addEvent(TCP::Event::Disconnected, 0, nullptr);
                    }
                    shutdown(_socketFD, SHUT_RDWR);
                    close(_socketFD);
                    _socketFD = -1;
                    break;
                } else if (result < 0) {
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        addEvent(TCP::Event::Error, -1, "read failed");
                    }
                } else {
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        addEvent(TCP::Event::ReceivedData, 0, _receiveBuffer, static_cast<int32_t>(result));
                    }
                }
            }
        }
    });

    _thread = std::move(dispatchThread);
}

MacTCP::~MacTCP()
{
    close(_socketFD);
    for (int& socket : _clientSockets) {
        if (socket) {
            close(socket);
        }
    }
    _thread.join();
}

int32_t MacTCP::sendData(int16_t connectionId, const char* data, uint16_t length)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return -1;
    }
    if (!length) {
        length = ::strlen(data);
    }
    
    int socket = 0;
    {
        socket = _server ? _clientSockets[connectionId] : _socketFD;
    }

    return static_cast<int32_t>(::send(socket, data, length, 0));
}

void MacTCP::disconnect(int16_t connectionId)
{
    if (connectionId < 0 || connectionId >= MaxConnections) {
        return;
    }
    
    int socket = _clientSockets[connectionId];
    _clientSockets[connectionId] = 0;
    close(socket);
    {
        std::unique_lock<std::mutex> lock(_mutex);
        addEvent(TCP::Event::Disconnected, connectionId, nullptr);
    }
}

void MacTCP::handleEvents()
{
    std::unique_lock<std::mutex> lock(_mutex);
    TCP::handleEvents();
}

IPAddr MacTCP::clientIPAddr(int16_t connectionId) const
{
    if (connectionId < 0 || connectionId > MaxConnections || !_clientSockets[connectionId]) {
        return IPAddr();
    }
    
    struct sockaddr_in addr;
    socklen_t len;
    if (getsockname(_clientSockets[connectionId], reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        return IPAddr();
    }
    
    return IPAddr(inet_ntoa(addr.sin_addr));
}
