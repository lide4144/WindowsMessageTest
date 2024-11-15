cmake_minimum_required(VERSION 3.16)

# 添加 vcpkg 工具链文件
set(CMAKE_TOOLCHAIN_FILE "E:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# 设置 Qt 安装路径
list(APPEND CMAKE_PREFIX_PATH 
    "E:/Qt/6.8.0/msvc2022_64"
    "E:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6"
    "E:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6Core"
    "E:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6Gui"
    "E:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6Widgets"
)

# 显式设置 Qt6_DIR
set(Qt6_DIR "E:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6")

project(ChatApp VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt 自动生成设置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Windows 特定设置
if(MSVC)
    # 使用 UTF-8 编码
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    
    # 使用 Unicode 字符集
    add_definitions(-DUNICODE -D_UNICODE)
    
    # 设置 Windows 版本
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7 或更高版本
    
    # 禁用一些警告
    add_compile_options(/wd4819 /wd4996)
    
    # 启用多处理器编译
    add_compile_options(/MP)
    
    # 设置运行时库
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# 为所有目标添加 Windows 版本定义
add_compile_definitions(
    $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0601>
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
)

# 查找包
find_package(asio CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS 
    Widgets 
    Core 
    Gui 
    Quick 
    QuickControls2
    Qml
)

# 添加 QML 资源文件
qt_add_resources(QML_RESOURCES
    qml.qrc
)

# 添加可执行文件
add_executable(ChatApp WIN32
    src/main.cpp
    src/ui/login_dialog.cpp
    src/ui/login_dialog.hpp
    src/ui/main_window.cpp
    src/ui/main_window.hpp
    src/network/chat_client.cpp
    src/network/chat_client.hpp
    src/network/message.cpp
    src/network/message.hpp
    src/database/message_store.cpp
    src/database/message_store.hpp
)

# 链接库
target_link_libraries(ChatApp PRIVATE 
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    Qt::Qml
    SQLite::SQLite3
    asio::asio
)

# 复制 Qt DLLs
if(WIN32)
    # 获取 Qt bin 目录
    get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    
    # 确保目标目录存在
    add_custom_command(TARGET ChatApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ChatApp>"
    )
    
    # 复制 Debug DLLs
    foreach(DLL_NAME
        Qt6Cored Qt6Guid Qt6Widgetsd Qt6Quickd 
        Qt6QuickControls2d Qt6Qmld Qt6Networkd Qt6OpenGLd
        Qt6QuickTemplates2d Qt6QmlMetad Qt6QmlWorkerScriptd
        Qt6QmlModelsd)
        add_custom_command(TARGET ChatApp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_qt_bin_dir}/${DLL_NAME}.dll"
                "$<TARGET_FILE_DIR:ChatApp>"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endforeach()
    
    # 复制平台插件
    add_custom_command(TARGET ChatApp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:ChatApp>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_qt_bin_dir}/../plugins/platforms/qwindowsd.dll"
            "$<TARGET_FILE_DIR:ChatApp>/platforms"
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

# 服务器端
add_executable(ChatServer
    src/server_main.cpp
    src/network/chat_server.cpp
    src/network/chat_server.hpp
    src/network/chat_session.cpp
    src/network/chat_session.hpp
    src/network/message.cpp
    src/network/message.hpp
    src/database/message_store.cpp
    src/database/message_store.hpp
)

target_link_libraries(ChatServer PRIVATE 
    SQLite::SQLite3
    asio::asio
)

# 修改链接选项
if(WIN32)
    target_link_options(ChatApp PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/SUBSYSTEM:WINDOWS>
        $<$<CXX_COMPILER_ID:MSVC>:/ENTRY:mainCRTStartup>
    )
    
    # Windows 系统库
    target_link_libraries(ChatApp PRIVATE 
        $<$<CXX_COMPILER_ID:MSVC>:ws2_32>
        $<$<CXX_COMPILER_ID:MSVC>:mswsock>
        $<$<CXX_COMPILER_ID:MSVC>:wsock32>
        $<$<CXX_COMPILER_ID:MSVC>:kernel32>
        $<$<CXX_COMPILER_ID:MSVC>:user32>
        $<$<CXX_COMPILER_ID:MSVC>:gdi32>
        $<$<CXX_COMPILER_ID:MSVC>:winspool>
        $<$<CXX_COMPILER_ID:MSVC>:shell32>
        $<$<CXX_COMPILER_ID:MSVC>:ole32>
        $<$<CXX_COMPILER_ID:MSVC>:oleaut32>
        $<$<CXX_COMPILER_ID:MSVC>:uuid>
        $<$<CXX_COMPILER_ID:MSVC>:comdlg32>
        $<$<CXX_COMPILER_ID:MSVC>:advapi32>
    )
endif()

# 编译器选项
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ChatApp PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -fms-compatibility
        -fms-extensions
        -fdelayed-template-parsing
    )
    
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(ChatApp PRIVATE -g)
    else()
        target_compile_options(ChatApp PRIVATE -O2)
    endif()
endif() 