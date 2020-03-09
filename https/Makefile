#
#Makefile c/c++编译脚本
#
#https://blog.csdn.net/hust_keven/article/details/8943909

#编译选项
CROSS_COMPILE =
#编译优化 -O2 debug -g
OPTIMIZE := -O2 -g
#禁用警告
WARNINGS := -w #-Wall -Wno-unused -Wno-format #-Wno-switch -Wno-return-type -Wno-unused-variable \
#-Wno-sizeof-pointer-memaccess -Wno-narrowing -Wno-sign-compare -Wno-maybe-uninitialized -Wno-pointer-to-int-cast
DEFS     := -DMYDEF=1 -UMYDEF2 -D_EVENTLOOP_CONTEXT_ -D_STAT_ORDER_QPS_ -D_STAT_ORDER_QPS_DETAIL_ #-D_NO_LOGIC_PROCESS_
EXTRA_CFLAGS := -Wfatal-errors #-Wno-overflow -Wno-sign-conversion -Wformat-security

#编译.o文件中间目录，必须bin
OBJ_DIR = bin

#编译目标文件路径/文件名
TARGET := ../../bin/ApiServer

#编译成.a静态库
#TARGET_TYPE  := ar
#编译成bin可执行文件
TARGET_TYPE  := app
#编译成.so动态库
#TARGET_TYPE := so

#编译文件类型
SUFFIX = c cpp cc cxx

#包含头文件目录
INC_DIR =. \
../ \
../.. \
../include \
../thirdpart \
../thirdpart/include \
../thirdpart/hiredis-vip \
../thirdpart/Json/include \/usr/include \
/usr/local/include \

#包含源文件目录
SRC_DIR =.

#附加编译依赖文件
EXTRA_SRC = 

#排除掉不编译的文件集合
EXCLUDE_FILES = 

#链接器附加库目录/输入库依赖项
LDFLAGS += -pthread -lrt -lm -lz -ldl \
	-L/usr/lib64 -lsnappy \
	-L/usr/lib64 -lcrypto \
	-L/usr/lib64 -lssl \
	-L/usr/lib64 -lcurl \
	-L/usr/local/lib -lzookeeper_mt \
	-L/usr/local/lib -lglog \
	-L/usr/local/lib -lprotobuf \
	-L/usr/local/lib -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread -lboost_locale -lboost_regex \
	-L../libs -lmuduo \
	-L../libs -lgameCrypto \
	-L../libs -lbsoncxx -lmongocxx \
	-L../libs -lJson \
	-L../libs -lrocketmq \
	-L../libs -lhiredis-vip \
	#-L../public/jsoncpp/lib -ljsoncpp \

define add_newline
$1

endef

define set_src_x
src-$1 = $(filter-out $4,$(foreach d,$2,$(wildcard $d/*.$1)) $(filter %.$1,$3))

endef

define set_obj_x
obj-$1 = $(patsubst %.$1,$3%.o,$(notdir $2))

endef

ifneq ($(OBJ_DIR),)
prefix_objdir  = $(shell echo $(OBJ_DIR)|sed 's:\(\./*\)*::')
prefix_objdir := $(filter-out /,$(prefix_objdir)/)
endif

GCC      := $(CROSS_COMPILE)gcc
G++      := $(CROSS_COMPILE)g++
src_dir  := $(sort . $(SRC_DIR))
inc_dir  := $(foreach d,$(sort $(INC_DIR) $(SRC_DIR)),-I$d)

$(eval $(foreach i,$(SUFFIX),$(call set_src_x,$i,$(src_dir),$(EXTRA_SRC),$(EXCLUDE_FILES))))
$(eval $(foreach i,$(SUFFIX),$(call set_obj_x,$i,$(src-$i),$(prefix_objdir))))
$(eval $(foreach f,$(EXTRA_SRC),$(call add_newline,vpath $(notdir $f) $(dir $f))))
$(eval $(foreach d,$(src_dir),$(foreach i,$(SUFFIX),$(call add_newline,vpath %.$i $d))))

all_objs = $(foreach i,$(SUFFIX),$(obj-$i))
all_srcs = $(foreach i,$(SUFFIX),$(src-$i))

CFLAGS       = $(EXTRA_CFLAGS) $(WARNINGS) $(OPTIMIZE) $(DEFS) -std=c++11
TARGET_TYPE := $(strip $(TARGET_TYPE))

#检查编译归档类型有效性(*.so *.a bin)
ifeq ($(filter $(TARGET_TYPE),so ar app),)
$(error Unexpected TARGET_TYPE `$(TARGET_TYPE)')
endif

ifeq ($(TARGET_TYPE),so)
 CFLAGS  += -fpic -shared
 LDFLAGS += -shared
endif

PHONY = all .mkdir clean

all: .mkdir $(TARGET)
	@echo ""
ifeq ($(TARGET_TYPE),so)
	ldd -r $(TARGET)
	@echo "输出动态库文件 $(TARGET) [OK]"
else ifeq ($(TARGET_TYPE),app)
	ldd -r $(TARGET)
	@echo "输出可执行文件 $(TARGET) [OK]"
else ifeq ($(TARGET_TYPE),ar)
	nm -g --defined-only $(TARGET)
	@echo "输出静态库文件 $(TARGET) [OK]"
endif
	@echo ""
	
define cmd_o
$$(obj-$1): $2%.o: %.$1  $(MAKEFILE_LIST)
	$(GCC) $(inc_dir) -Wp,-MT,$$@ -Wp,-MMD,$$@.d $(CFLAGS) -c -o $$@ $$<

endef
$(eval $(foreach i,$(SUFFIX),$(call cmd_o,$i,$(prefix_objdir))))

ifeq ($(TARGET_TYPE),ar)
$(TARGET): AR := $(CROSS_COMPILE)ar
$(TARGET): $(all_objs)
	rm -f $@
	$(AR) rcvs $@ $(all_objs)
else
$(TARGET): LD = $(if $(strip $(src-cpp) $(src-cc) $(src-cxx)),$(G++),$(GCC))
$(TARGET): $(all_objs)
	$(LD) $(all_objs) $(LDFLAGS) -o $@
endif

.mkdir:
	@if [ ! -d $(OBJ_DIR) ]; then mkdir -p $(OBJ_DIR); fi

clean:
	rm -f $(prefix_objdir)*.o $(prefix_objdir)*.o.d $(TARGET)

-include $(patsubst %.o,%.o.d,$(all_objs))

.PHONY: $(PHONY)

