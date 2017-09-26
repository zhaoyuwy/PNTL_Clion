#!/bin/sh
# 安装服务

# 准备环境
. ./env.sh

# 需要root权限
if [ _"$(whoami)"_ != _"root"_ ]; then 
    ${ECHO} you are using a non-privileged account
    exit 1; 
fi;

# 检查安装目录, 不能是空或者根目录
if [ _${PROC_INSTALL_DIR}_ == _"/"_  -o _${PROC_INSTALL_DIR}_ == _""_ ]; then
    ${ECHO} "You must set PROC_INSTALL_DIR in env.sh"
    exit 1; 
fi

# 检查服务名称, 不能为空
if [ _${SERVICE_SCRIPT_FILE_NAME}_ == _""_ ]; then
    ${ECHO} "You must set SERVICE_SCRIPT_FILE_NAME in env.sh"
    exit 1; 
fi

# 检查环境变量中是否需要覆盖用户名. 如果有, 则刷新用户名. 一般从Ansible下发环境变量
if [ _${SERVICE_USER_NAME_OVERRIDE}_ != _""_  ]; then
    ${ECHO} "Update SERVICE_USER_NAME to [${SERVICE_USER_NAME_OVERRIDE}]"
    SERVICE_USER_NAME=${SERVICE_USER_NAME_OVERRIDE}
    ${SED} -i "s:^SERVICE_USER_NAME=.*:SERVICE_USER_NAME=\"${SERVICE_USER_NAME}\":g" ./env.sh
fi

# 检查 ${SERVICE_USER_NAME} 用户名是否存在
${EGREP} "^${SERVICE_USER_NAME}" /etc/passwd >& ${DEVICE_NULL}
if [ $? != 0 ]  ; then
    ${ECHO} "Can't find user ${SERVICE_USER_NAME}, ${SERVICE_NAME} need it. Or modify SERVICE_USER_NAME in env.sh"
    exit 1
fi

# 如果服务已经安装, 先停止服务
if [ -f "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ]; then 
    ${ECHO} Stopping ${SERVICE_SCRIPT_FILE_NAME}
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} stop
fi;

# 刷新自动启动脚本中的变量
${SED} -i "s:^SERVICE_NAME=.*:SERVICE_NAME=\"${SERVICE_NAME}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_USER_NAME=.*:SERVICE_USER_NAME=\"${SERVICE_USER_NAME}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_INSTALL_DIR=.*:SERVICE_INSTALL_DIR=\"${PROC_INSTALL_DIR}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_PID_FILE=.*:SERVICE_PID_FILE=\"${SERVICE_PID_FILE}\":g" ${SERVICE_SCRIPT_FILE_NAME}

# 修正日志配置文件
# 查询  ${SERVICE_USER_NAME} 用户名默认组, 配置日志转储模块时使用
SERVICE_USER_GROUP=$(id ${SERVICE_USER_NAME} | awk -F '[()]' '{print $4}')
# 刷新启动日志转储配置文件中的用户名和组名
${SED} -i "s/^.*su .*/    su ${SERVICE_USER_NAME} ${SERVICE_USER_GROUP}/g" ${LOG_CFG_FILE}
# 刷新启动日志转储配置文件中的日志路径
${SED} -i "s:^.*{:${LOG_DIR}/*.log ${PROC_INSTALL_DIR}/logs/*.log {:g" ${LOG_CFG_FILE}
# Sles 12 上logrotate会检查配置文件权限, 不会加载带执行权限的文件.
${CHMOD} 644 ${LOG_CFG_FILE}

# ip address 命令获取管理口IP
ConnectIP=$(ip address | grep Mgnt-0 | grep inet[^6] | awk -F ' ' '{print $2}')
ConnectIP=${ConnectIP%\/*}
# 如果获取成功, 则刷新IP.
if [ _${ConnectIP}_ != _""_  ]; then
    ${ECHO} "Update MgntIP to [${ConnectIP}]"
    ${SED} -i "s/^.*\"MgntIP\".*$/\t\"MgntIP\"\t:\t\"${ConnectIP}\",/g" ${PROC_CFG_FILE}
fi

# ip address 命令获取v-bond口IP，agent ip
ConnectIP=$(ip address | grep Mgnt-0 | grep inet[^6] | awk -F ' ' '{print $2}')
ConnectIP=${ConnectIP%\/*}
# 如果获取成功, 则刷新IP.
if [ _${ConnectIP}_ != _""_  ]; then
    ${ECHO} "Update AgentIP to [${ConnectIP}]"
    ${SED} -i "s/^.*\"AgentIP\".*$/\t\"AgentIP\"\t:\t\"${ConnectIP}\",/g" ${PROC_CFG_FILE}
fi

#更新Log目录配置
${SED} -i "s:^.*LOG_DIR.*:\t\t\"LOG_DIR\"\t\: \"${LOG_DIR}\":g" ${PROC_CFG_FILE}

# 新创建文件夹默认权限755, 文件为644
umask 022

# 安装二进制文件
    ${ECHO} Install Bin file to ${PROC_INSTALL_DIR}
    ${MKDIR} -p ${PROC_INSTALL_DIR}
    ${CP} * ${PROC_INSTALL_DIR} -r
    
    # 修改权限
    ${CHOWN} ${SERVICE_USER_NAME} ${PROC_INSTALL_DIR} -R
    
    # 创建日志文件目录, 并赋予权限
    ${MKDIR} -p ${LOG_DIR}
    ${CHOWN} ${SERVICE_USER_NAME} ${LOG_DIR} -R
    
    # 创建PID文件目录, 并赋予权限
    ${MKDIR} -p $(dirname ${SERVICE_PID_FILE})
    ${CHOWN} ${SERVICE_USER_NAME} $(dirname ${SERVICE_PID_FILE}) -R

# 安装启动脚本    
if test -d "${SERVICE_SCRIPT_INSTALL_DIR}"; then
    ${ECHO} Install auto boot script to ${SERVICE_SCRIPT_INSTALL_DIR}
    ${CP} ${SERVICE_SCRIPT_FILE_NAME} ${SERVICE_SCRIPT_INSTALL_DIR}
    
    if test -d "${SERVICE_SCRIPT_INSTALL_DIR}/rc3.d" ;then 
        ${LN} -fs ${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME} ${SERVICE_SCRIPT_INSTALL_DIR}/rc3.d/S99${SERVICE_SCRIPT_FILE_NAME}; 
        ${LN} -fs ${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME} ${SERVICE_SCRIPT_INSTALL_DIR}/rc3.d/K99${SERVICE_SCRIPT_FILE_NAME}; 
    else 
        ${ECHO} Do not support this init script now;  
        exit 1; 
    fi
        
    if test -d "${SERVICE_SCRIPT_INSTALL_DIR}/rc5.d" ;then 
        ${LN} -fs ${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME} ${SERVICE_SCRIPT_INSTALL_DIR}/rc5.d/S99${SERVICE_SCRIPT_FILE_NAME}; 
        ${LN} -fs ${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME} ${SERVICE_SCRIPT_INSTALL_DIR}/rc5.d/K99${SERVICE_SCRIPT_FILE_NAME}; 
    else 
        ${ECHO} Do not support this init script now;  
        exit 1; 
    fi
else 
    ${ECHO} Do not support this os now
    exit 1
fi

# 配置 logrotate 日志转储功能
if test -d "${LOG_CFG_INSTALL_DIR}" ;then ${CP} ${LOG_CFG_FILE} ${LOG_CFG_INSTALL_DIR}; else ${ECHO} Do not support this log rotate cfg now;  exit 1; fi; 

# 安装完成, 重新启动服务
if test -e "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ; then 
    ${ECHO} Install  ${SERVICE_NAME} Sucess
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} start
else 
    ${ECHO} Install  ${SERVICE_NAME} Failed
    exit 1 
fi
