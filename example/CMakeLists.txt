add_executable(client.out 
                        client.cpp)
add_executable(tcpepoll.out 
                            tcpepoll.cpp 
                            EchoServer.cpp)

set_target_properties(client.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/example/bin/)
set_target_properties(tcpepoll.out PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/example/bin/)

target_include_directories(client.out PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_include_directories(tcpepoll.out PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

target_link_libraries(client.out my_reactor_net)
target_link_libraries(tcpepoll.out my_reactor_net pthread)