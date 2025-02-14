# Copyright 2021 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

# Generic Makefile target for ARM Cortex M builds.
# For more info see: tensorflow/lite/micro/cortex_m_generic/README.md

FLOAT := soft
GCC_TARGET_ARCH := $(TARGET_ARCH)

# Explicitly set this to true to include the kissfft symbols.
INCLUDE_MICRO_SPEECH := false

ifeq ($(TARGET_ARCH), cortex-m0)
  CORE=M0
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M0


else ifeq ($(TARGET_ARCH), cortex-m0plus)
  CORE=M0plus
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M0plus


else ifeq ($(TARGET_ARCH), cortex-m3)
  CORE=M3
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M3

else ifeq ($(TARGET_ARCH), cortex-m33)
  CORE=M33
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M33
  TARGET_SPECIFIC_FLAGS += -D__DSP_PRESENT=1 -D__FPU_PRESENT=1 -D__VTOR_PRESENT=1 -D__FPU_USED=1
  FLOAT=hard

else ifeq ($(TARGET_ARCH), cortex-m33+nodsp)
  CORE=M33
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M33.no_dsp.no_fp

else ifeq ($(TARGET_ARCH), cortex-m4)
  CORE=M4
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M4.no_fp
  GCC_TARGET_ARCH := cortex-m4+nofp

else ifeq ($(TARGET_ARCH), cortex-m4+fp)
  CORE=M4
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M4
  TARGET_SPECIFIC_FLAGS += -D__FPU_PRESENT=1
  FLOAT=hard
  GCC_TARGET_ARCH := cortex-m4

else ifeq ($(TARGET_ARCH), cortex-m4+sfp)
  CORE=M4
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M4
  TARGET_SPECIFIC_FLAGS += -D__FPU_PRESENT=1
  FLOAT=softfp
  GCC_TARGET_ARCH := cortex-m4

else ifeq ($(TARGET_ARCH), cortex-m55)
  CORE=M55
  ARM_LDFLAGS := -Wl,--cpu=8.1-M.Main.mve.fp
  TARGET_SPECIFIC_FLAGS += -D__DSP_PRESENT=1 -D__FPU_PRESENT=1
  FLOAT=hard

else ifeq ($(TARGET_ARCH), cortex-m55+nodsp+nofp)
  CORE=M55
  ARM_LDFLAGS := -Wl,--cpu=8.1-M.Main.mve.no_dsp.no_fp

else ifeq ($(TARGET_ARCH), cortex-m55+nofp)
  CORE=M55
  ARM_LDFLAGS := -Wl,--cpu=8.1-M.Main.mve.no_fp
  TARGET_SPECIFIC_FLAGS += -D__DSP_PRESENT=1

else ifeq ($(TARGET_ARCH), cortex-m7)
  CORE=M7
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M7.no_fp
  GCC_TARGET_ARCH := cortex-m7+nofp

else ifeq ($(TARGET_ARCH), cortex-m7+fp)
  CORE=M7
  ARM_LDFLAGS := -Wl,--cpu=Cortex-M7
  FLOAT=hard
  GCC_TARGET_ARCH := cortex-m7

else ifeq ($(TARGET_ARCH), project_generation)
 # None of the parameters matter since we are not going to build any code.
else
  $(error "TARGET_ARCH=$(TARGET_ARCH) is not supported")
endif

ifneq ($(filter cortex-m55%,$(TARGET_ARCH)),)
  # soft-abi=soft disables MVE - use softfp instead for M55.
  ifeq ($(FLOAT),soft)
    FLOAT=softfp
  endif
endif

# Toolchain specfic flags
ifeq ($(TOOLCHAIN), armclang)
  CXX_TOOL  := armclang
  CC_TOOL   := armclang
  AR_TOOL   := armar
  LD        := armlink

  FLAGS_ARMC = \
    --target=arm-arm-none-eabi \
    -mcpu=$(TARGET_ARCH)

  CXXFLAGS += $(FLAGS_ARMC)
  CCFLAGS += $(FLAGS_ARMC)
  LDFLAGS += $(ARM_LDFLAGS)

  # Arm Compiler will not link the Math library (see below), therefore we're filtering it out.
  # See Fatal error: L6450U: Cannot find library m:
  # "Arm Compiler is designed to run in a bare metal environment,
  # and automatically includes implementations of these functions,
  # and so no such flag is necessary."
  # https://developer.arm.com/documentation/100891/0611/troubleshooting/general-troubleshooting-advice
  MICROLITE_LIBS := $(filter-out -lm,$(MICROLITE_LIBS))

else ifeq ($(TOOLCHAIN), gcc)
  TARGET_DEFAULT_TOOLCHAIN_ROOT := ${TENSORFLOW_ROOT}$(MAKEFILE_DIR)/downloads/gcc_embedded/bin/
  TARGET_TOOLCHAIN_ROOT := $(TARGET_DEFAULT_TOOLCHAIN_ROOT)
  ifeq ($(TARGET_TOOLCHAIN_ROOT), $(TARGET_DEFAULT_TOOLCHAIN_ROOT))
    DOWNLOAD_RESULT := $(shell $(MAKEFILE_DIR)/arm_gcc_download.sh ${MAKEFILE_DIR}/downloads)
    ifneq ($(DOWNLOAD_RESULT), SUCCESS)
      $(error Something went wrong with the GCC download: $(DOWNLOAD_RESULT))
    endif
  endif

  TARGET_TOOLCHAIN_PREFIX := arm-none-eabi-

  FLAGS_GCC = -mcpu=$(GCC_TARGET_ARCH) -mfpu=auto
  CXXFLAGS += $(FLAGS_GCC)
  CCFLAGS += $(FLAGS_GCC)

else
  $(error "TOOLCHAIN=$(TOOLCHAIN) is not supported.")
endif

# TODO(#47718): resolve warnings.
OMIT_ERRORS = \
  -Wno-implicit-fallthrough \
  -Wno-unused-variable

PLATFORM_FLAGS = \
  -DTF_LITE_MCU_DEBUG_LOG \
  -mthumb \
  -mfloat-abi=$(FLOAT) \
  -funsigned-char \
  -mlittle-endian \
   ${OMIT_ERRORS} \
  -Wno-type-limits \
  -Wno-unused-private-field \
  -fomit-frame-pointer \
  -MD \
  -DCPU_$(CORE)=1 \
  $(TARGET_SPECIFIC_FLAGS)

# Common + C/C++ flags
CXXFLAGS += $(PLATFORM_FLAGS)
CCFLAGS += $(PLATFORM_FLAGS)

# Needed for the project generation interface.
MICROLITE_CC_HDRS += \
  tensorflow/lite/micro/cortex_m_generic/debug_log_callback.h

# We only include micro_speech for project generation to allow for all the files
# to be downloaded. We do not include it for an actual build with the
# cortex_m_generic target to prevent kissfft symbols from getting included in
# libtensorflow-microlite.a which can result in symbol collision.
ifneq ($(TARGET_ARCH), project_generation)
  EXCLUDED_EXAMPLE_TESTS := \
    tensorflow/lite/micro/examples/micro_speech/Makefile.inc
  MICRO_LITE_EXAMPLE_TESTS := $(filter-out $(EXCLUDED_EXAMPLE_TESTS), $(MICRO_LITE_EXAMPLE_TESTS))
endif
