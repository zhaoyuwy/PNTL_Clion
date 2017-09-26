#!/bin/sh

# ׼������
. ./env.sh

${ECHO}  " Checking ${PROC_START_FILE}  ..."

# ��鵱ǰ�û�
if [ _"$(whoami)"_ != _"root"_ -a _"$(whoami)"_ != _${SERVICE_USER_NAME}_ ]; then 
    ${ECHO} " Error: start service with user: root or ${SERVICE_USER_NAME}."
    ${ECHO} " SERVICE_USER_NAME:${SERVICE_USER_NAME} is set in env.sh before run Install script."
    exit 1; 
fi;

# ��鵱ǰĿ¼�Ƿ���
if [ _${ROOT_DIR}_ != _${PROC_INSTALL_DIR}_ ]; then 
    ${ECHO} " Error: Please run this script from path:${PROC_INSTALL_DIR}."
    ${ECHO} " PROC_INSTALL_DIR:${PROC_INSTALL_DIR} is set in env.sh before run Install script."
    exit 1; 
fi;

# ���Service�Ƿ��Ѿ�����
${CHECKPROC} -p ${SERVICE_PID_FILE} ${PROC_START_FILE}

