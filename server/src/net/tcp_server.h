#pragma once

#include <cstdint>
#include <functional>
#include <string>

// Forward declare to avoid pulling <sys/socket.h> into the header
struct sockaddr_in;

namespace erynfall {

class TcpServer {
public:
    using ConnectionCallback = std::function<void(int client_fd, sockaddr_in client_addr)>;

    TcpServer();
    ~TcpServer();

    // Bind to port. Returns false on error.
    bool bind(uint16_t port);

    // Start listening (non-blocking — call acceptPending in your loop)
    bool listen(int backlog = 16);

    // Accept one pending connection (if any). Calls callback.
    void acceptPending(ConnectionCallback callback);

    // Close the server socket
    void close();

    // Get the port we're bound to (useful if port=0 was given)
    uint16_t boundPort() const;

    bool isBound() const { return fd_ >= 0; }

private:
    int fd_ = -1;
    uint16_t port_ = 0;
};

} // namespace erynfall
