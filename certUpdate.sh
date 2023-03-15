#!/usr/bin/env bash
secrets=secrets.h
PYTHON=python3

cwd_abs=$(realpath -e $(pwd))
cwd_rel=$(realpath -e --relative-to=$(dirname $0) $(pwd))
mydir_abs=$(realpath -e $(dirname $0))
mydir_rel=$(realpath -e --relative-to=$(pwd) $(dirname $0))
#echo "cwd_abs: ${cwd_abs}"
#echo "cwd_rel: ${cwd_rel}"
#echo " md_abs: ${mydir_abs}"
#echo " md_rel: ${mydir_rel}"
cmd1="${PYTHON} ${mydir_rel}/h_parser.py -f -d MQTT_HOST ${cwd_abs}/${secrets}"
cmd2="${PYTHON} ${mydir_abs}/h_parser.py -f -d MQTT_PORT ${cwd_abs}/${secrets}"
#echo "$cmd1"
#echo "$cmd2"
MQTT_HOST=$($cmd1 | cut -f2)
MQTT_PORT=$($cmd2 | cut -f2)
echo "wss://${MQTT_HOST}:${MQTT_PORT}"
cmd3="${PYTHON} ${mydir_abs}/cert.py -s ${MQTT_HOST} -p ${MQTT_PORT} -n myhost"
$cmd3 > ${cwd_abs}/certs.h