#include "net/tcp_server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace erynfall {

TcpServer::TcpServer() = default;

TcpServer::~TcpServer() {
    close();
}

bool TcpServer::bind(uint16_t port) {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        return false;
    }

    // Allow address reuse (fast restart)
    int opt = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    port_ = port;

    // Set non-blocking for acceptPending
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

    return true;
}

bool TcpServer::listen(int backlog) {
    if (fd_ < 0) return false;
    return ::listen(fd_, backlog) == 0;
}

void TcpServer::acceptPending(ConnectionCallback callback) {
    if (fd_ < 0 || !callback) return;

    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int client_fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);

    if (client_fd >= 0) {
        callback(client_fd, client_addr);
    }
    // EAGAIN/EWOULDBLOCK means no pending connections — that's fine
}

void TcpServer::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

uint16_t TcpServer::boundPort() const {
    if (fd_ < 0) return 0;
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len) == 0) {
        return ntohs(addr.sin_port);
    }
    return port_;
}

} // namespace erynfall
