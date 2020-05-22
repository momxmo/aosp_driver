LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := hello.default

#LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw 
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := hello.c
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils
LOCAL_CFLAGS += -Wno-implicit-function-declaration
#LOCAL_CFLAGS += -Wno-error
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
# LOCAL_MODULE的定义规则，hello后面跟有default，
# hello.default能够保证我们的模块总能被硬象抽象层加载到
