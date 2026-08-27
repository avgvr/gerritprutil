target_sources(gerritprutil
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/socket.cpp
)

target_sources(gerritprutil
    PUBLIC
        FILE_SET HEADERS
        FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/lexer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/socket.h
)
