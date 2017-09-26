#!/bin/sh

# ж�ط���

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

# ��������Ѿ���װ, ��ֹͣ����
if [ -f "${SERVICE_SCRIPT_INSTALL_DIR}/${SERVICE_SCRIPT_FILE_NAME}" ]; then 
    ${ECHO} Stopping ${SERVICE_SCRIPT_FILE_NAME}
    ${SERVICE} ${SERVICE_SCRIPT_FILE_NAME} stop
fi;

# ɾ���������ļ�, �Ѿ�ȷ�Ϲ�PROC_INSTALL_DIR���Ǹ�Ŀ¼, Ҳ���ǿ�Ŀ¼. ����ʹ��rootɾ��Ŀ¼�ķ���
if test -d  ${PROC_INSTALL_DIR} ; then
    ${ECHO} Remove Bin file in ${PROC_INSTALL_DIR}
    ${RM} ${PROC_INSTALL_DIR} -fr
fi

# ɾ�������ű�    
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

# ɾ��PIDĿ¼
    if test -d $(dirname ${SERVICE_PID_FILE})         ; then ${RMDIR} $(dirname ${SERVICE_PID_FILE}); fi;

# ȡ�� syslog ��־ת������
    if test -e "${LOG_CFG_INSTALL_DIR}/${LOG_CFG_FILE_NAME}"          ; then ${RM} ${LOG_CFG_INSTALL_DIR}/${LOG_CFG_FILE_NAME}; fi;

${ECHO} Uninstall  ${SERVICE_SCRIPT_FILE_NAME} OK
