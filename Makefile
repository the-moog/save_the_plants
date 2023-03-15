
project			=	save_the_plants
app				=	mqtt

HOME			=	/mnt/c/Users/jasomorg
PROGS			=	/mnt/c/Program Files
ARDUINO_WORK	=	${HOME}/Documents/Arduino
ARDUINO_CLI		=	${ARDUINO_WORK}/arduino-cli
ARDUINO_DATA	=	${HOME}/Documents/ArduinoData
ARDUIMO_TMP		=	${HOME}/AppData/Local/Temp
ARDUINO_BASE	=	${PROGS}/WindowsApps/ArduinoLLC.ArduinoIDE_1.8.57.0_x86__mdqgnx93n4wtt
BUILDER			=	${ARDUINO_BASE}/arduino-builder
HARDWARE		=	${ARDUINO_BASE}/hardware
LIBS			=	${ARDUINO_WORK}/libraries
TEMP_BASE		=	${HOME}/AppData/Local/Temp
ARDUINO_PROJ	=	${ARDUINO_WORK}/${project}
INOFILE			=	${ARDUINO_PROJ}/${project}.ino

FQBN			= esp8266:esp8266:nodemcuv2
FQBN_OPS		= xtal=80,vt=flash,exception=disabled,\
stacksmash=disabled,ssl=all,mmu=3232,non32xfer=fast,eesz=4M2M,led=2,\
ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=115200

CMDLINE			=

CMDLINE	+=	-dump-prefs
CMDLINE	+=	-logger=machine
CMDLINE	+=	-hardware ${HARDWARE}
CMDLINE	+=	-hardware ${ARDUINO_DATA}/packages

CMDLINE	+=	-tools ${ARDUINO_BASE}/tools-builder
CMDLINE	+=	-tools ${ARDUINO_BASE}/hardware\tools\avr
CMDLINE	+=	-tools ${ARDUINO_DATA}/packages

CMDLINE	+=	-built-in-libraries ${ARDUINO_BASE}/libraries
CMDLINE	+=	-libraries ${ARDUINO_WORK}/libraries
CMDLINE	+=	-fqbn=${FQBN}:${FQBN_OPS}
CMDLINE	+=	-vid-pid=10C4_EA60
CMDLINE	+=	-ide-version=10819
CMDLINE	+=	-build-path ${TEMP_BASE}/arduino_build_247172
CMDLINE	+=	-warnings=3
CMDLINE	+=	-build-cache ${TEMP_BASE}/arduino_cache_57814

CMDLINE	+=	-prefs=build.warn_data_percentage=75

CMDLINE	+=	-prefs=runtime.tools.mkspiffs.path=\
${ARDUINO_DATA}/packages/esp8266/tools/mkspiffs/3.1.0-gcc10.3-e5f9fec

CMDLINE	+=	-prefs=runtime.tools.mkspiffs-3.1.0-gcc10.3-e5f9fec.path=\
${ARDUINO_DATA}/packages/esp8266/tools/mkspiffs/3.1.0-gcc10.3-e5f9fec

CMDLINE	+=	-prefs=runtime.tools.xtensa-lx106-elf-gcc.path=\
${ARDUINO_DATA}/packages/esp8266/tools/xtensa-lx106-elf-gcc/3.1.0-gcc10.3-e5f9fec

CMDLINE	+=	-prefs=runtime.tools.xtensa-lx106-elf-gcc-3.1.0-gcc10.3-e5f9fec.path=\
${ARDUINO_DATA}/packages/esp8266/tools/xtensa-lx106-elf-gcc/3.1.0-gcc10.3-e5f9fec

CMDLINE	+=	-prefs=runtime.tools.mklittlefs.path=\
${ARDUINO_DATA}/packages/esp8266/tools/mklittlefs/3.1.0-gcc10.3-e5f9fec 

CMDLINE	+=	-prefs=runtime.tools.mklittlefs-3.1.0-gcc10.3-e5f9fec.path=\
${ARDUINO_DATA}/packages/esp8266/tools/mklittlefs/3.1.0-gcc10.3-e5f9fec

CMDLINE	+=	-prefs=runtime.tools.python3.path\
${ARDUINO_DATA}/packages/esp8266/tools/python3/3.7.2-post1

CMDLINE	+=	-prefs=runtime.tools.python3-3.7.2-post1.path=\
${ARDUINO_DATA}/packages/esp8266/tools/python3/3.7.2-post1

CMDLINE	+=	-verbose ${INOFILE}

CMDLINE	+=	CreateFile ${TEMP_BASE}/arduino_cache_57814


.PHONY:default
default:
	echo "use make arduino_reset"


.PHONY: arduino_reset
arduino_reset:
	rm -rf ${ARDUIMO_TMP}/arduino
	rm -rf ${ARDUIMO_TMP}/arduino_build_*
	rm -rf ${ARDUIMO_TMP}/arduino_cache_*
