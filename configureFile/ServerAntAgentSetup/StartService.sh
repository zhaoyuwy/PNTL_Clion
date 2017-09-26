#!/bin/sh

# ׼������
. ./env.sh

${ECHO}  " Starting ${PROC_START_FILE}  ..."

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

# ���PID�ļ�Ŀ¼�Ƿ����.
if [ ! -d  $(dirname ${SERVICE_PID_FILE}) ]; then
    ${ECHO} " Error: Please run install script with user root first."
    exit 1
fi

# ���PID�ļ�Ŀ¼�Ƿ���дȨ��
${TOUCH} $(dirname ${SERVICE_PID_FILE})/test > ${DEVICE_NULL} 2>&1
if [ $? != 0 ]; then
    ${ECHO} " user:$(whoami) can't write to $(dirname ${SERVICE_PID_FILE}) ."
    ${ECHO} " Please update SERVICE_USER_NAME:${SERVICE_USER_NAME} in env.sh and run install script again."
    exit 1
fi
# ɾ�������ļ�
${RM} $(dirname ${SERVICE_PID_FILE})/test -f

# ���PID�ļ�����, ����Ƿ���дȨ��
if [ -f  ${SERVICE_PID_FILE} ]; then
    ${TOUCH} ${SERVICE_PID_FILE} > ${DEVICE_NULL} 2>&1
    if [ $? != 0 ]; then
        ${ECHO} " user:$(whoami) can't write to ${SERVICE_PID_FILE} ."
        ${ECHO} " Please update SERVICE_USER_NAME:${SERVICE_USER_NAME} in env.sh and run install script again."
        exit 1
    fi
fi

# �趨��̬������·��, ����ʹ��OS�ṩ�Ķ�̬��, �Ҳ��������ʹ���Լ��ṩ�Ŀ��ļ�
export LD_LIBRARY_PATH=${PROC_LIB_DIR}:${LD_LIBRARY_PATH}

# ���Service�Ƿ��Ѿ�����
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

# �����������
${PROC_START_FILE} $*

# ��ȡ����ID
pid=$(ps aux | grep ${PROC_START_FILE} | grep -v grep | awk '{print $2}')
# ������IDд���ļ� 
if [ ${pid} ]; then
    ${ECHO} ${pid} > "${SERVICE_PID_FILE}"
    ${ECHO}  " Start ${PROC_START_FILE} success"
    exit 0
else 
    rm -f ${SERVICE_PID_FILE}
    ${ECHO}  " Start ${PROC_START_FILE} failed"
    exit 1
fi
