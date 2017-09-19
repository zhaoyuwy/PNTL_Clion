cmake_minimum_required(VERSION 3.8)
project(PNTL_Agent)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES src/AgentCommon.h src/AgentJsonAPI.cpp src/AgentJsonAPI.h src/DetectWorker.cpp src/DetectWorker.h src/FileNotifier.cpp src/FileNotifier.h src/FlowManager.cpp src/FlowManager.h src/GetLocalCfg.cpp src/GetLocalCfg.h src/Log.cpp src/Log.h src/main.cpp src/MessagePlatform.cpp src/MessagePlatform.h src/Sal.cpp src/Sal.h src/ServerAntAgentCfg.cpp src/ServerAntAgentCfg.h src/ThreadClass.cpp src/ThreadClass.h src/common/common.cpp src/MessagePlatformClient.cpp src/MessagePlatformClient.h)
add_executable(PNTL_Agent ${SOURCE_FILES})

#Set link library.
TARGET_LINK_LIBRARIES(PNTL_Agent pthread curl)