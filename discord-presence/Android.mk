# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)
TARGET_ARCH_ABI := $(APP_ABI)

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

# Build the modloader shared library
include $(CLEAR_VARS)
LOCAL_MODULE := modloader
LOCAL_EXPORT_C_INCLUDES := extern/modloader
LOCAL_SRC_FILES := extern/libmodloader.so
include $(PREBUILT_SHARED_LIBRARY)

# Build the beatsaber-hook shared library, SPECIFICALLY VERSIONED!
include $(CLEAR_VARS)
LOCAL_MODULE := beatsaber-hook_0_8_4
LOCAL_EXPORT_C_INCLUDES := extern/beatsaber-hook
LOCAL_SRC_FILES := extern/libbeatsaber-hook_0_8_4.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: codegen - version: 0.4.0
include $(CLEAR_VARS)
LOCAL_MODULE := codegen_0_4_0
LOCAL_EXPORT_C_INCLUDES := extern/codegen
LOCAL_SRC_FILES := extern/libcodegen_0_4_0.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := discord-presence
LOCAL_SRC_FILES += $(call rwildcard,src/,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook,*.c)
LOCAL_SHARED_LIBRARIES += beatsaber-hook_0_8_4
LOCAL_SHARED_LIBRARIES += modloader
LOCAL_SHARED_LIBRARIES += codegen_0_4_0
LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -I'extern/libil2cpp/il2cpp/libil2cpp' -I'extern/codegen/include' -DID='"discord-presence"' -DVERSION='"0.2.0"' -I'./shared' -I'./extern'
LOCAL_CPPFLAGS += -std=c++2a
LOCAL_C_INCLUDES += ./include ./src
include $(BUILD_SHARED_LIBRARY)
