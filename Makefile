CC = gcc
FLAGS = -Wall -Wextra -pedantic -std=c17 -Iinclude -D_GNU_SOURCE
DEBUG_FLAG = -g 
LINK_FLAG = -lxbps  -lnotcurses -lnotcurses-core
OPTIMIZE_FLAG = -O3

ifeq ($(CC),clang) 
	FLAGS += -fblocks
	LINK_FLAG += -lBlocksRuntime
endif

# XBPS-UI
NAME = xui 

SRC_DIR = src
BUILD_DIR = build

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst ${SRC_DIR}/%.c, ${BUILD_DIR}/%.o, $(SRC))

ifeq ($(debug),1)
	FLAGS += $(DEBUG_FLAG)
endif

ifeq ($(optimize),1)
	FLAGS += $(OPTIMIZE_FLAG)
endif

.PHONY: all clean

all: dir ${NAME}

${NAME}: ${OBJ}
	${CC} ${FLAGS} $^ -o $@ $(LINK_FLAG)

${BUILD_DIR}/%.o : $(SRC_DIR)/%.c
	${CC} ${FLAGS} -c $< -o $@

dir: 
	mkdir -p ${BUILD_DIR}
	
clean: 
	rm -rf ${BUILD_DIR}
	rm -f ${NAME}

