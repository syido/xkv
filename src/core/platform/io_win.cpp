#ifdef _WIN32 // win

#include "../io.hpp"
#include <shared/config.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <mswsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

namespace xkv {

enum class io_op {
    accept, // 接受连接
    read,   // 读取数据
    write,  // 写入数据
};

// IO操作对象
struct operation {
    OVERLAPPED overlapped{};
    io_op type{};
    SOCKET socket = INVALID_SOCKET;
    WSABUF wsabuf{};
    array<char, static_config::buffer_size> buffer{};
    array<char, (sizeof(sockaddr_storage) + 16) * 2> accept_buffer{};
};

// IO操作对象池
struct operation_pool {
    vector<operation> items;
    vector<operation *> free_list;

    explicit operation_pool(size_t size) : items(size) {
        free_list.reserve(size);
        for (auto &item : items) {
            free_list.push_back(&item);
        }
    }

    operation *acquire(io_op type, SOCKET socket) {
        if (free_list.empty()) {
            static runtime_error error{"IO操作池已耗尽"};
            throw error;
        }

        operation *op = free_list.back();
        free_list.pop_back();
        *op = operation{};
        op->type = type;
        op->socket = socket;
        return op;
    }

    void release(operation *op) {
        free_list.push_back(op);
    }
};

static void set_non_blocking(SOCKET socket) {
    u_long mode = 1;
    if (ioctlsocket(socket, FIONBIO, &mode) == SOCKET_ERROR) {
        throw runtime_error{"设置socket非阻塞失败"};
    }
}

// 创建非阻塞的监听socket，并完成Winsock初始化、绑定和监听
static SOCKET create_listen(int port) {
    WSADATA wsa_data{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        throw runtime_error{"初始化socket失败"};
    }

    SOCKET listen_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (listen_socket == INVALID_SOCKET) {
        WSACleanup();
        throw runtime_error{"创建socket失败"};
    }

    auto close_and_throw = [listen_socket](const char *message) {
        closesocket(listen_socket);
        WSACleanup();
        throw runtime_error{message};
    };

    BOOL opt = TRUE;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&opt), sizeof(opt)) ==
        SOCKET_ERROR) {
        close_and_throw("setting reuseaddr failed 设置端口复用失败");
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(listen_socket, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR ||
        listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        close_and_throw("监听socket失败");
    }

    set_non_blocking(listen_socket);
    return listen_socket;
}

// 将socket关联到IOCP，后续完成事件会投递到该完成端口
static void associate_socket(HANDLE iocp, SOCKET socket) {
    HANDLE handle = CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), iocp, static_cast<ULONG_PTR>(socket), 0);
    if (!handle) {
        throw runtime_error{"关联socket到IOCP失败"};
    }
}

// 关闭客户端连接，并从连接表移除
static void close_connect(connection_map &connections, SOCKET event_socket) {
    io_handle event_handle = static_cast<io_handle>(event_socket);
    auto it = connections.find(event_handle);
    if (it == connections.end()) {
        return;
    }

    it->second.closed = true;
    closesocket(event_socket);
    connections.erase(it);
}

// 投递一次异步AcceptEx，用于接收下一个客户端连接
static void create_connect(operation_pool &pool, SOCKET listen_socket, LPFN_ACCEPTEX accept_ex) {
    SOCKET client_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (client_socket == INVALID_SOCKET) {
        throw runtime_error{"创建socket失败"};
    }

    operation *op = pool.acquire(io_op::accept, client_socket);

    DWORD bytes = 0;
    BOOL ok = accept_ex(listen_socket, op->socket, op->accept_buffer.data(), 0, sizeof(sockaddr_storage) + 16,
                        sizeof(sockaddr_storage) + 16, &bytes, &op->overlapped);
    if (!ok && WSAGetLastError() != ERROR_IO_PENDING) {
        closesocket(op->socket);
        pool.release(op);
        static runtime_error error{"投递AcceptEx失败"};
        throw error;
    }
}

// 投递一次异步读请求
static void read_connect(operation_pool &pool, connection_map &connections, SOCKET client_socket) {
    io_handle client_handle = static_cast<io_handle>(client_socket);
    auto it = connections.find(client_handle);
    if (it == connections.end()) {
        return;
    }

    operation *op = pool.acquire(io_op::read, client_socket);
    op->wsabuf.buf = op->buffer.data();
    op->wsabuf.len = static_cast<ULONG>(op->buffer.size());

    DWORD flags = 0;
    DWORD bytes = 0;
    int ret = WSARecv(op->socket, &op->wsabuf, 1, &bytes, &flags, &op->overlapped, nullptr);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        pool.release(op);
        close_connect(connections, client_socket);
        return;
    }
}

// 投递一次异步写请求；无待写数据时重新投递读请求
static void write_connect(operation_pool &pool, connection_map &connections, SOCKET client_socket) {
    io_handle client_handle = static_cast<io_handle>(client_socket);
    auto it = connections.find(client_handle);
    if (it == connections.end()) {
        return;
    }

    auto &conn = it->second;
    if (conn.outbuf_view.empty()) {
        read_connect(pool, connections, client_socket);
        return;
    }

    operation *op = pool.acquire(io_op::write, client_socket);
    op->wsabuf.buf = const_cast<char *>(conn.outbuf_view.data());
    op->wsabuf.len = static_cast<ULONG>(conn.outbuf_view.size());

    DWORD bytes = 0;
    int ret = WSASend(op->socket, &op->wsabuf, 1, &bytes, 0, &op->overlapped, nullptr);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        pool.release(op);
        close_connect(connections, client_socket);
        return;
    }
}

// 将连接切换到写回流程
static void write_back(operation_pool &pool, connection_map &connections, SOCKET client_socket) {
    write_connect(pool, connections, client_socket);
}

static void create_connect_done(operation_pool &pool, HANDLE iocp, SOCKET listen_socket, LPFN_ACCEPTEX accept_ex,
                                connection_map &connections, SOCKET client_socket) {
    setsockopt(client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char *>(&listen_socket),
               sizeof(listen_socket));
    set_non_blocking(client_socket);
    associate_socket(iocp, client_socket);

    io_handle client_handle = static_cast<io_handle>(client_socket);
    connections.emplace(client_handle, connection{client_handle});
    read_connect(pool, connections, client_socket);
    create_connect(pool, listen_socket, accept_ex);
}

// 处理读完成事件，追加输入缓冲并调用业务回调
static void read_connect_done(operation_pool &pool, io *self, connection_map &connections, const on_recv_func &on_recv,
                              const function<void(connection &)> &check_response, operation &op, DWORD transferred) {
    SOCKET client_socket = op.socket;
    io_handle client_handle = static_cast<io_handle>(client_socket);
    if (transferred == 0) {
        pool.release(&op);
        close_connect(connections, client_socket);
        return;
    }

    auto it = connections.find(client_handle);
    if (it == connections.end()) {
        pool.release(&op);
        return;
    }

    auto &conn = it->second;
    conn.inbuf.append(op.buffer.data(), transferred);
    on_recv(self, conn);
    check_response(conn);
    pool.release(&op);
    write_back(pool, connections, client_socket);
}

// 处理写完成事件，移动待写视图并继续写剩余数据
static void write_connect_done(operation_pool &pool, connection_map &connections, operation *op, DWORD transferred) {
    SOCKET client_socket = op->socket;
    io_handle client_handle = static_cast<io_handle>(client_socket);
    if (transferred == 0) {
        pool.release(op);
        close_connect(connections, client_socket);
        return;
    }

    auto it = connections.find(client_handle);
    if (it == connections.end()) {
        pool.release(op);
        return;
    }

    auto &conn = it->second;
    conn.outbuf_view.remove_prefix(static_cast<size_t>(transferred));
    pool.release(op);
    write_connect(pool, connections, client_socket);
}

void io::loop() {
    SOCKET listen_socket = create_listen(port);
    HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocp) {
        closesocket(listen_socket);
        WSACleanup();
        throw runtime_error{"创建IOCP失败"};
    }

    GUID guid = WSAID_ACCEPTEX;
    LPFN_ACCEPTEX accept_ex = nullptr;
    DWORD bytes = 0;
    int ret = WSAIoctl(listen_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &accept_ex,
                       sizeof(accept_ex), &bytes, nullptr, nullptr);
    if (ret == SOCKET_ERROR) {
        throw runtime_error{"加载AcceptEx失败"};
    }

    associate_socket(iocp, listen_socket);
    operation_pool pool{config.io_pool_size};

    auto check = [this](connection &conn) {
        check_response(conn);
    };

    create_connect(pool, listen_socket, accept_ex);

    while (true) {
        DWORD transferred = 0;
        ULONG_PTR key = 0;
        OVERLAPPED *overlapped = nullptr;
        BOOL ok = GetQueuedCompletionStatus(iocp, &transferred, &key, &overlapped, INFINITE);
        if (!overlapped) {
            if (!ok) {
                cout << "iocp error 错误" << endl;
            }
            continue;
        }

        operation *op = reinterpret_cast<operation *>(overlapped);
        SOCKET socket = op->socket;

        if (!ok) {
            if (op->type == io_op::accept) {
                closesocket(op->socket);
                pool.release(op);
                create_connect(pool, listen_socket, accept_ex);
            } else {
                close_connect(connections, socket);
                pool.release(op);
            }
            continue;
        }

        if (op->type == io_op::accept) {
            pool.release(op);
            create_connect_done(pool, iocp, listen_socket, accept_ex, connections, socket);
        } else if (op->type == io_op::read) {
            read_connect_done(pool, this, connections, on_recv, check, *op, transferred);
        } else if (op->type == io_op::write) {
            write_connect_done(pool, connections, op, transferred);
        }
    }
}

} // namespace xkv

#endif
