#!/bin/sh
# ��װ����

# ׼������
. ./env.sh

# ��ҪrootȨ��
if [ _"$(whoami)"_ != _"root"_ ]; then 
    ${ECHO} you are using a non-privileged account
    exit 1; 
fi;

# ��鰲װĿ¼, �����ǿջ��߸�Ŀ¼
if [ _${PROC_INSTALL_DIR}_ == _"/"_  -o _${PROC_INSTALL_DIR}_ == _""_ ]; then
    ${ECHO} "You must set PROC_INSTALL_DIR in env.sh"
    exit 1; 
fi

# ����������, ����Ϊ��
if [ _${SERVICE_SCRIPT_FILE_NAME}_ == _""_ ]; then
    ${ECHO} "You must set SERVICE_SCRIPT_FILE_NAME in env.sh"
    exit 1; 
fi

# ��黷���������Ƿ���Ҫ�����û���. �����, ��ˢ���û���. һ���Ansible�·���������
if [ _${SERVICE_USER_NAME_OVERRIDE}_ != _""_  ]; then
    ${ECHO} "Update SERVICE_USER_NAME to [${SERVICE_USER_NAME_OVERRIDE}]"
    SERVICE_USER_NAME=${SERVICE_USER_NAME_OVERRIDE}
    ${SED} -i "s:^SERVICE_USER_NAME=.*:SERVICE_USER_NAME=\"${SERVICE_USER_NAME}\":g" ./env.sh
fi

# ��� ${SERVICE_USER_NAME} �û����Ƿ����
${EGREP} "^${SERVICE_USER_NAME}" /etc/passwd >& ${DEVICE_NULL}
if [ $? != 0 ]  ; then
    ${ECHO} "Can't find user ${SERVICE_USER_NAME}, ${SERVICE_NAME} need it. Or modify SERVICE_USER_NAME in env.sh"
    exit 1
fi

# ��������Ѿ���װ, ��ֹͣ����
if [ -f "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ]; then 
    ${ECHO} Stopping ${SERVICE_SCRIPT_FILE_NAME}
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} stop
fi;

# ˢ���Զ������ű��еı���
${SED} -i "s:^SERVICE_NAME=.*:SERVICE_NAME=\"${SERVICE_NAME}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_USER_NAME=.*:SERVICE_USER_NAME=\"${SERVICE_USER_NAME}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_INSTALL_DIR=.*:SERVICE_INSTALL_DIR=\"${PROC_INSTALL_DIR}\":g" ${SERVICE_SCRIPT_FILE_NAME}
${SED} -i "s:^SERVICE_PID_FILE=.*:SERVICE_PID_FILE=\"${SERVICE_PID_FILE}\":g" ${SERVICE_SCRIPT_FILE_NAME}

# ������־�����ļ�
# ��ѯ  ${SERVICE_USER_NAME} �û���Ĭ����, ������־ת��ģ��ʱʹ��
SERVICE_USER_GROUP=$(id ${SERVICE_USER_NAME} | awk -F '[()]' '{print $4}')
# ˢ��������־ת�������ļ��е��û���������
${SED} -i "s/^.*su .*/    su ${SERVICE_USER_NAME} ${SERVICE_USER_GROUP}/g" ${LOG_CFG_FILE}
# ˢ��������־ת�������ļ��е���־·��
${SED} -i "s:^.*{:${LOG_DIR}/*.log ${PROC_INSTALL_DIR}/logs/*.log {:g" ${LOG_CFG_FILE}
# Sles 12 ��logrotate���������ļ�Ȩ��, ������ش�ִ��Ȩ�޵��ļ�.
${CHMOD} 644 ${LOG_CFG_FILE}

# ip address �����ȡ�����IP
ConnectIP=$(ip address | grep Mgnt-0 | grep inet[^6] | awk -F ' ' '{print $2}')
ConnectIP=${ConnectIP%\/*}
# �����ȡ�ɹ�, ��ˢ��IP.
if [ _${ConnectIP}_ != _""_  ]; then
    ${ECHO} "Update MgntIP to [${ConnectIP}]"
    ${SED} -i "s/^.*\"MgntIP\".*$/\t\"MgntIP\"\t:\t\"${ConnectIP}\",/g" ${PROC_CFG_FILE}
fi

# ip address �����ȡv-bond��IP��agent ip
ConnectIP=$(ip address | grep Mgnt-0 | grep inet[^6] | awk -F ' ' '{print $2}')
ConnectIP=${ConnectIP%\/*}
# �����ȡ�ɹ�, ��ˢ��IP.
if [ _${ConnectIP}_ != _""_  ]; then
    ${ECHO} "Update AgentIP to [${ConnectIP}]"
    ${SED} -i "s/^.*\"AgentIP\".*$/\t\"AgentIP\"\t:\t\"${ConnectIP}\",/g" ${PROC_CFG_FILE}
fi

#����LogĿ¼����
${SED} -i "s:^.*LOG_DIR.*:\t\t\"LOG_DIR\"\t\: \"${LOG_DIR}\":g" ${PROC_CFG_FILE}

# �´����ļ���Ĭ��Ȩ��755, �ļ�Ϊ644
umask 022

# ��װ�������ļ�
    ${ECHO} Install Bin file to ${PROC_INSTALL_DIR}
    ${MKDIR} -p ${PROC_INSTALL_DIR}
    ${CP} * ${PROC_INSTALL_DIR} -r
    
    # �޸�Ȩ��
    ${CHOWN} ${SERVICE_USER_NAME} ${PROC_INSTALL_DIR} -R
    
    # ������־�ļ�Ŀ¼, ������Ȩ��
    ${MKDIR} -p ${LOG_DIR}
    ${CHOWN} ${SERVICE_USER_NAME} ${LOG_DIR} -R
    
    # ����PID�ļ�Ŀ¼, ������Ȩ��
    ${MKDIR} -p $(dirname ${SERVICE_PID_FILE})
    ${CHOWN} ${SERVICE_USER_NAME} $(dirname ${SERVICE_PID_FILE}) -R

# ��װ�����ű�    
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

# ���� logrotate ��־ת������
if test -d "${LOG_CFG_INSTALL_DIR}" ;then ${CP} ${LOG_CFG_FILE} ${LOG_CFG_INSTALL_DIR}; else ${ECHO} Do not support this log rotate cfg now;  exit 1; fi; 

# ��װ���, ������������
if test -e "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ; then 
    ${ECHO} Install  ${SERVICE_NAME} Sucess
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} start
else 
    ${ECHO} Install  ${SERVICE_NAME} Failed
    exit 1 
fi
