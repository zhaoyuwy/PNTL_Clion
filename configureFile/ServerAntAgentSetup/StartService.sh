#!/bin/sh

# 准备环境
. ./env.sh

${ECHO}  " Starting ${PROC_START_FILE}  ..."

# 检查当前用户
if [ _"$(whoami)"_ != _"root"_ -a _"$(whoami)"_ != _${SERVICE_USER_NAME}_ ]; then 
    ${ECHO} " Error: start service with user: root or ${SERVICE_USER_NAME}."
    ${ECHO} " SERVICE_USER_NAME:${SERVICE_USER_NAME} is set in env.sh before run Install script."
    exit 1; 
fi;

# 检查当前目录是否是
if [ _${ROOT_DIR}_ != _${PROC_INSTALL_DIR}_ ]; then 
    ${ECHO} " Error: Please run this script from path:${PROC_INSTALL_DIR}."
    ${ECHO} " PROC_INSTALL_DIR:${PROC_INSTALL_DIR} is set in env.sh before run Install script."
    exit 1; 
fi;

# 检查PID文件目录是否存在.
if [ ! -d  $(dirname ${SERVICE_PID_FILE}) ]; then
    ${ECHO} " Error: Please run install script with user root first."
    exit 1
fi

# 检查PID文件目录是否有写权限
${TOUCH} $(dirname ${SERVICE_PID_FILE})/test > ${DEVICE_NULL} 2>&1
if [ $? != 0 ]; then
    ${ECHO} " user:$(whoami) can't write to $(dirname ${SERVICE_PID_FILE}) ."
    ${ECHO} " Please update SERVICE_USER_NAME:${SERVICE_USER_NAME} in env.sh and run install script again."
    exit 1
fi
# 删除测试文件
${RM} $(dirname ${SERVICE_PID_FILE})/test -f

# 如果PID文件存在, 检查是否有写权限
if [ -f  ${SERVICE_PID_FILE} ]; then
    ${TOUCH} ${SERVICE_PID_FILE} > ${DEVICE_NULL} 2>&1
    if [ $? != 0 ]; then
        ${ECHO} " user:$(whoami) can't write to ${SERVICE_PID_FILE} ."
        ${ECHO} " Please update SERVICE_USER_NAME:${SERVICE_USER_NAME} in env.sh and run install script again."
        exit 1
    fi
fi

# 设定动态库搜索路径, 优先使用OS提供的动态库, 找不到的情况使用自己提供的库文件
export LD_LIBRARY_PATH=${PROC_LIB_DIR}:${LD_LIBRARY_PATH}

# 检查Service是否已经启动
${CHECKPROC} -p ${SERVICE_PID_FILE} ${PROC_START_FILE}
case $? in
    0) 
        ${ECHO} " Warning: ${PROC_START_FILE} already running. " 
        exit 0
        ;;
    1) 
        ${ECHO} " Warning: ${PROC_START_FILE} is not running but ${SERVICE_PID_FILE} exists. " 
        ;;
esac

# 启动软件进程
${PROC_START_FILE} $*

# 获取进程ID
pid=$(ps aux | grep ${PROC_START_FILE} | grep -v grep | awk '{print $2}')
# 将进程ID写入文件 
if [ ${pid} ]; then
    ${ECHO} ${pid} > "${SERVICE_PID_FILE}"
    ${ECHO}  " Start ${PROC_START_FILE} success"
    exit 0
else 
    rm -f ${SERVICE_PID_FILE}
    ${ECHO}  " Start ${PROC_START_FILE} failed"
    exit 1
fi
