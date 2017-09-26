#!/bin/sh

# ׼������
. ./env.sh

${ECHO}  " Stopping ${PROC_START_FILE}  ..."

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

# ���Service�Ƿ��Ѿ�����
${CHECKPROC} -p ${SERVICE_PID_FILE} ${PROC_START_FILE} || \
    ${ECHO} " Warning: ${PROC_START_FILE} not running. "

# ֹͣ����, ͬʱ��ɾ��pid�ļ�
${KILLPROC} -p ${SERVICE_PID_FILE} -t 10 ${PROC_START_FILE}

pid=$(ps aux | grep ${PROC_START_FILE} | grep -v grep | awk '{print $2}')
if [ ${pid} ]; then
    ${KILL} -9 ${pid}
fi
# �Ƿ��ж���Ĳ���?
if [ _"${PROC_STOP_FILE}"_ != __ ]; then 
    ${PROC_STOP_FILE} || \
    ${ECHO}  " Call  ${PROC_STOP_FILE} failed"
fi

${ECHO}  " Shutting down  ${PROC_START_FILE} success"
# ����OK
exit 0

