add_library(my_reactor_net SHARED 
                                Acceptor.cpp
                                Buffer.cpp
                                Channel.cpp
                                Connection.cpp
                                Epoll.cpp
                                EventFd.cpp
                                EventLoop.cpp
                                InetAddress.cpp
                                Log.cpp
                                Socket.cpp
                                TcpServer.cpp
                                ThreadPool.cpp
                                Timer.cpp
                                TimesTamp.cpp
                                AsyncLogging.cpp)

                                
# 指定生成动态库所需要的头文件，使用PUBLIC限定符，若通过target_link_libraries连接库，会自动获取到这些头文件所在的路径
target_include_directories(my_reactor_net 
                                        PUBLIC
                                                ${CMAKE_CURRENT_SOURCE_DIR}/include/)

#设置动态库输出路径
set_target_properties(my_reactor_net PROPERTIES
                                        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)