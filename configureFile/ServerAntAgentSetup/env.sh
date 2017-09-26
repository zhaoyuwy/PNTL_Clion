#!/bin/sh

# ��ȡ��ǰĿ¼
ROOT_DIR="$(pwd)"

# ������Ϣ
# ��������
SERVICE_NAME="ServerAntAgent"
# �����������ű�����, �ýű���ע�ᵽOS��ʼ��������
SERVICE_SCRIPT_FILE_NAME="${SERVICE_NAME}Service"
# ����������û���, ����ʹ��root�û�����
SERVICE_USER_NAME="root"
# �����������ű���װ��Ϣ
SERVICE_SCRIPT_INSTALL_DIR="/etc/rc.d"
# �������ID, ͨ�����ý����Ƿ�������жϷ����Ƿ���������
SERVICE_PID_FILE="/var/run/${SERVICE_NAME}/${SERVICE_NAME}.pid"

# �����Ϣ
# �����װ·��
PROC_INSTALL_DIR="/opt/huawei/${SERVICE_NAME}"
# ��������ļ�
PROC_CFG_FILE="${ROOT_DIR}/ServerAntAgent.cfg"
# �����ִ���ļ� ��Ϣ
PROC_START_FILE="${ROOT_DIR}/ServerAntAgent"
# PROC_STOP_FILE="${ROOT_DIR}"
# ���������˽�п���Ϣ
PROC_LIB_DIR="${ROOT_DIR}/libs"

# ��־������Ϣ
# ���������־���Ŀ¼
LOG_DIR="/opt/huawei/logs/${SERVICE_NAME}"
# ��־ת��������Ϣ��װ·��
LOG_CFG_INSTALL_DIR="/etc/logrotate.d"
# ��־ת�������ļ���
LOG_CFG_FILE_NAME="${SERVICE_NAME}LogConfig"
# ��־ת�������ļ�
LOG_CFG_FILE="${ROOT_DIR}/${LOG_CFG_FILE_NAME}"

# ͳһOS�ӿ�
ECHO=$(which echo)
TOUCH=$(which touch)
RM=$(which rm)
RMDIR=$(which rmdir)
CP=$(which cp)
MKDIR=$(which mkdir)
CHOWN=$(which chown)
SU=$(which su)
SED=$(which sed)
EGREP=$(which egrep)
CHMOD=$(which chmod)
NOHUP=$(which nohup)
LN=$(which ln)
KILL=$(which kill)
# ����OS����ͨ�û�PATHδ����/sbinĿ¼
SERVICE="/sbin/service"
CHECKPROC="/sbin/checkproc"
KILLPROC="/sbin/killproc"

DEVICE_NULL="/dev/null"


${ECHO}  " Prepare env success"

