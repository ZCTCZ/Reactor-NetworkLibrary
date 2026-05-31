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
target_include_directories(my_reactor_net PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
        $<INSTALL_INTERFACE:include/my_reactor_net>)

#设置动态库输出路径
set_target_properties(my_reactor_net PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# 安装动态库
install(TARGETS my_reactor_net
        EXPORT ReactorNetTargets
        LIBRARY DESTINATION my_reactor_net)

# 安装 CMake targets 文件
install(EXPORT ReactorNetTargets
        DESTINATION my_reactor_net/cmake/ReactorNet)

# 安装头文件
install(DIRECTORY include/
        DESTINATION include/my_reactor_net)

# 生成 CMake 配置文件
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/ReactorNetConfig.cmake.in
    ${CMAKE_BINARY_DIR}/ReactorNetConfig.cmake
    INSTALL_DESTINATION my_reactor_net/cmake/ReactorNet)

install(FILES ${CMAKE_BINARY_DIR}/ReactorNetConfig.cmake
        DESTINATION my_reactor_net/cmake/ReactorNet)

