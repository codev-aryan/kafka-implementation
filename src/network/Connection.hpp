#pragma once

class Connection {
public:
    explicit Connection(int fd);
    ~Connection();
    void handle();

private:
    int client_fd;
};