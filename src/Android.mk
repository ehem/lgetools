LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := mergegpt

LOCAL_SRC_FILES := mergegpt.c gpt.c forcecrc32.c

LOCAL_STATIC_LIBRARIES := android_support

GPTLIBS := z

LOCAL_LDLIBS := $(GPTLIBS:%=-l%)

LOCAL_CFLAGS := -Wall -std=c11

include $(BUILD_EXECUTABLE)



include $(CLEAR_VARS)

LOCAL_MODULE := gpt

LOCAL_SRC_FILES := gpt.c display_gpt.c

LOCAL_STATIC_LIBRARIES := android_support

GPTLIBS := z

LOCAL_LDLIBS := $(GPTLIBS:%=-l%)

LOCAL_CFLAGS := -Wall -std=c11

include $(BUILD_EXECUTABLE)



$(call import-module,android/support)

