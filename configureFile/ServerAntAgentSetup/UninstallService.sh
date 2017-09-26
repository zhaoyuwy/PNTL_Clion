#!/bin/sh

# 卸载服务

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

# 如果服务已经安装, 先停止服务
if [ -f "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ]; then 
    ${ECHO} Stopping ${SERVICE_SCRIPT_FILE_NAME}
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} stop
fi;

# 删除二进制文件, 已经确认过PROC_INSTALL_DIR不是根目录, 也不是空目录. 避免使用root删除目录的风险
if test -d  ${PROC_INSTALL_DIR} ; then
    ${ECHO} Remove Bin file in ${PROC_INSTALL_DIR}
    ${RM} ${PROC_INSTALL_DIR} -fr
fi

# 删除启动脚本    
    ${ECHO} Remove auto boot script from ${SERVICE_SCRIPT_INSTALL_DIR}
    
    if test -d "${SERVICE_SCRIPT_INSTALL_DIR}/rc3.d" ;then 
        ${RM} -f ${SERVICE_SCRIPT_INSTALL_DIR}/rc3.d/*${SERVICE_SCRIPT_FILE_NAME}
    fi
    
    if test -d "${SERVICE_SCRIPT_INSTALL_DIR}/rc5.d" ;then 
        ${RM} -f ${SERVICE_SCRIPT_INSTALL_DIR}/rc5.d/*${SERVICE_SCRIPT_FILE_NAME}
    fi
    
    if test -e "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ; then 
        ${RM} ${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}; 
    fi;

# 删除PID目录
    if test -d $(dirname ${SERVICE_PID_FILE})         ; then ${RMDIR} $(dirname ${SERVICE_PID_FILE}); fi;

# 取消 syslog 日志转储功能
    if test -e "${LOG_CFG_INSTALL_DIR}/${LOG_CFG_FILE_NAME}"          ; then ${RM} ${LOG_CFG_INSTALL_DIR}/${LOG_CFG_FILE_NAME}; fi;

${ECHO} Uninstall  ${SERVICE_SCRIPT_FILE_NAME} OK
