add_library(lars_reactor STATIC
        tcp_server.cc
        io_buffer.cc
        buffer_pool.cc
        reactor_buffer.cc
        event_loop.cc
    tcp_conn.cc)
