#!/bin/sh

# 获取当前目录
ROOT_DIR="$(pwd)"

# 服务信息
# 服务名称
SERVICE_NAME="ServerAntAgent"
# 服务自启动脚本名称, 该脚本会注册到OS初始化流程中
SERVICE_SCRIPT_FILE_NAME="${SERVICE_NAME}Service"
# 启动服务的用户名, 避免使用root用户启动
SERVICE_USER_NAME="root"
# 服务自启动脚本安装信息
SERVICE_SCRIPT_INSTALL_DIR="/etc/rc.d"
# 服务进程ID, 通过检测该进程是否存在来判断服务是否正在运行
SERVICE_PID_FILE="/var/run/${SERVICE_NAME}/${SERVICE_NAME}.pid"

# 软件信息
# 软件安装路径
PROC_INSTALL_DIR="/opt/huawei/${SERVICE_NAME}"
# 软件配置文件
PROC_CFG_FILE="${ROOT_DIR}/ServerAntAgent.cfg"
# 软件可执行文件 信息
PROC_START_FILE="${ROOT_DIR}/ServerAntAgent"
# PROC_STOP_FILE="${ROOT_DIR}"
# 软件依赖的私有库信息
PROC_LIB_DIR="${ROOT_DIR}/libs"

# 日志配置信息
# 软件运行日志存放目录
LOG_DIR="/opt/huawei/logs/${SERVICE_NAME}"
# 日志转储配置信息安装路径
LOG_CFG_INSTALL_DIR="/etc/logrotate.d"
# 日志转储配置文件名
LOG_CFG_FILE_NAME="${SERVICE_NAME}LogConfig"
# 日志转储配置文件
LOG_CFG_FILE="${ROOT_DIR}/${LOG_CFG_FILE_NAME}"

# 统一OS接口
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
# 部分OS下普通用户PATH未包括/sbin目录
SERVICE="/sbin/service"
CHECKPROC="/sbin/checkproc"
KILLPROC="/sbin/killproc"

DEVICE_NULL="/dev/null"


${ECHO}  " Prepare env success"

