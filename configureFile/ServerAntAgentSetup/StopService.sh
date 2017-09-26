#!/bin/sh

# 准备环境
. ./env.sh

${ECHO}  " Stopping ${PROC_START_FILE}  ..."

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

# 检查Service是否已经启动
${CHECKPROC} -p ${SERVICE_PID_FILE} ${PROC_START_FILE} || \
    ${ECHO} " Warning: ${PROC_START_FILE} not running. "

# 停止进程, 同时会删除pid文件
${KILLPROC} -p ${SERVICE_PID_FILE} -t 10 ${PROC_START_FILE}

pid=$(ps aux | grep ${PROC_START_FILE} | grep -v grep | awk '{print $2}')
if [ ${pid} ]; then
    ${KILL} -9 ${pid}
fi
# 是否有额外的操作?
if [ _"${PROC_STOP_FILE}"_ != __ ]; then 
    ${PROC_STOP_FILE} || \
    ${ECHO}  " Call  ${PROC_STOP_FILE} failed"
fi

${ECHO}  " Shutting down  ${PROC_START_FILE} success"
# 返回OK
exit 0

