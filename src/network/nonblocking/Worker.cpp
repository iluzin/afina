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
    return nullptr;
}

} // namespace NonBlocking
} // namespace Network
} // namespace Afina
