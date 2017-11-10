#include "Worker.h"

#include <exception>
#include <iostream>
#include <unordered_map>

#ifndef __APPLE__
#   include <sys/epoll.h>
#else
#   include <sys/event.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>

#include "Utils.h"

#define MAXEVENTS 64

namespace Afina {
namespace Network {
namespace NonBlocking {

// See Worker.h
Worker::Worker(std::shared_ptr<Afina::Storage> ps) : pStorage(ps) {
    // TODO: implementation here
}

// See Worker.h
Worker::Worker(const Worker &worker) : pStorage(worker.pStorage) {
    // TODO: implementation here
}

// See Worker.h
Worker::~Worker(void) {
    // TODO: implementation here
}

// See Worker.h
void Worker::Start(int server_socket) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    auto p = new std::pair<Worker *, int>(this, server_socket);
    if (pthread_create(&thread, nullptr, Worker::OnRun, p) < 0) {
        delete p;
        throw std::runtime_error("Could not create server thread");
    }
}

// See Worker.h
void Worker::Stop(void) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    running.store(false);
}

// See Worker.h
void Worker::Join(void) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    pthread_join(this->thread, nullptr);
}

// See Worker.h
void *Worker::OnRun(void *args) {
    std::cout << "network debug: " << __PRETTY_FUNCTION__ << std::endl;
    auto p = static_cast<std::pair<Worker *, int> *>(args);
    Worker &worker = *p->first;
    int server_socket = p->second;
    delete p;
    std::unordered_map<int, size_t> buffer_len;
    std::unordered_map<int, std::string> input;
    std::unordered_map<int, Protocol::Parser> parser;
    std::unordered_map<int, size_t> parsed;
    std::unordered_map<int, uint32_t> body_size;
    int epfd = epoll_create(MAXEVENTS);
    if (epfd == -1) {
        throw std::runtime_error("epoll_create");
    }
    epoll_event ev, events[MAXEVENTS];
    ev.events = EPOLLEXCLUSIVE | EPOLLHUP | EPOLLIN | EPOLLERR;
    ev.data.fd = server_socket; 
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
        throw std::runtime_error("epoll_ctl");
    }
    while (running.load()) {
        int nfds = epoll_wait(epfd, events, MAXEVENTS, -1);
        if (nfds < 0) {
            throw std::runtime_error("epoll_wait");
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_socket) {
                if (events[i].events & EPOLLIN) {
                    int client_socket = accept(server_socket, nullptr, nullptr);
                    make_socket_non_blocking(client_socket);
                    ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
                    ev.data.fd = client_socket;
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
                        throw std::runtime_error("epoll_ctl");
                    }
                    socklen_t option_len = sizeof buffer_len[client_socket];
                    if (getsockopt(client_socket, SOL_SOCKET, SO_RCVBUF, &buffer_len[client_socket], &option_len) == -1) {
                        throw std::runtime_error("Socket getsockopt() failed");
                    }
                    input.insert(std::make_pair(client_socket, std::string()));
                    parsed.emplace(client_socket, 0);
                    parser.insert(std::make_pair(client_socket, Protocol::Parser()));
                } else if ((events[i].events & EPOLLERR) == EPOLLERR && (events[i].events & EPOLLHUP) == EPOLLHUP) {
                    pthread_exit(nullptr);
                }
            } else {
                int client_socket = events[i].data.fd;
                if (events[i].events & EPOLLIN) {
                    try {
                        char buffer[buffer_len[client_socket]];
                        ssize_t count;
                        while ((count = recv(client_socket, buffer, buffer_len[client_socket], 0)) > 0) {
                            input[client_socket].append(buffer, count);
                        }
                        if (count == -1) {
                            if (errno == EAGAIN) {
                                continue;
                            } else {
                                throw std::runtime_error("Socket recv() failed");
                            }
                        }
                        if (parser[client_socket].Parse(input[client_socket], parsed[client_socket])) {
                            auto command = parser.Build(body_size[client_socket]);
                            if (body_size[client_socket] > 0) {
                                while (input[client_socket].size() < body_size[client_socket]) {
                                    count = recv(client_socket, buffer, buffer_len[client_socket], 0);
                                    if (count == -1) {
                                        if (errno == EAGAIN) {
                                            break;
                                        } else {
                                            throw std::runtime_error("Socket recv() failed");
                                        }
                                    }
                                    input[client_socket].append(buffer, count);
                                }
                                if (input[client_socket].size() < body_size[client_socket]) {
                                    continue;
                                }
                                std::string args = input[client_socket].substr(0, body_size[client_socket]), out;
                                input[client_socket].erase(0, body_size[client_socket]);
                                command->Execute(*pStorage, args, out);
                                while (!out.empty()) {
                                    count = send(client_socket, out.data(), out.size(), 0);
                                    if (count == -1 && errno == EAGAIN) {
                                        continue;
                                    } else if (count <= 0) {
                                        throw std::runtime_error("Socket send() failed");
                                    }
                                    out.erase(0, count);
                                }
                            }
                        }
                    } catch (const std::exception &e) {
                        std::string out = "SERVER_ERROR ";
                        out += e.what();
                        out += "\r\n";
                        while ((count = send(client_socket, out.data(), out.size(), 0)) > 0) {
                            out.erase(0, count);
                        }
                    }
                } else {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_socket, nullptr);
                    close(client_socket);
                    buffer_len.erase(client_socket);
                    input.erase(client_socket);
                    parser.erase(client_socket);
                    parsed.erase(client_socket);
                    body_size.erase(client_socket);
                }
            }
        }
    }
    close(epfd);
    return nullptr;
}

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
