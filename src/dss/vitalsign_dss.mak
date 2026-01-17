###################################################################################
# Vital Signs DSS Makefile
###################################################################################

# This makefile is included by the main mmw_dss.mak when VITAL_SIGNS=1

###################################################################################
# Local Library Configuration
###################################################################################
# USE_LOCAL_MATHLIB:   Use project-integrated mathlib headers (default: 1)
# USE_LOCAL_DSPLIB:    Use project-integrated dsplib source (default: 0)
# USE_LOCAL_MATHUTILS: Use project-integrated mathutils source (default: 1)
#
# Local mathlib is header-only and provides full visibility into math functions.
# Local dsplib compiles C source instead of assembly - slower but visible.
# Local mathutils provides log2, power-of-2, and window generation utilities.
#
# Override on command line: make VITAL_SIGNS=1 USE_LOCAL_DSPLIB=1
###################################################################################
USE_LOCAL_MATHLIB ?= 1
USE_LOCAL_DSPLIB ?= 0
USE_LOCAL_MATHUTILS ?= 1

###################################################################################
# Vital Signs DSS source files
###################################################################################
VITALSIGNS_DSS_SOURCES = vitalsign_dsp.c

# Add DSPlib source files when using local implementation (FFT + IFFT)
ifeq ($(USE_LOCAL_DSPLIB),1)
VITALSIGNS_DSS_SOURCES += fft_sp.c ifft_sp.c fft_twiddle.c
endif

# Add mathutils source when using local implementation
ifeq ($(USE_LOCAL_MATHUTILS),1)
VITALSIGNS_DSS_SOURCES += mathutils.c
endif

###################################################################################
# Vital Signs DSS include paths
###################################################################################
VITALSIGNS_DSS_INC = -i$(MMWAVE_SDK_INSTALL_PATH)/../src/common \
                     -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss

# Local mathlib include path (header-only)
ifeq ($(USE_LOCAL_MATHLIB),1)
VITALSIGNS_DSS_INC += -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathlib
endif

# Local dsplib include path (compiled source)
ifeq ($(USE_LOCAL_DSPLIB),1)
VITALSIGNS_DSS_INC += -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/dsplib
endif

# Local mathutils include path (compiled source)
ifeq ($(USE_LOCAL_MATHUTILS),1)
VITALSIGNS_DSS_INC += -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathutils
endif

###################################################################################
# Vital Signs DSS compiler flags
###################################################################################
VITALSIGNS_DSS_CFLAGS = -DVITAL_SIGNS_ENABLED

# Define USE_LOCAL_MATHLIB for conditional includes in source
ifeq ($(USE_LOCAL_MATHLIB),1)
VITALSIGNS_DSS_CFLAGS += -DUSE_LOCAL_MATHLIB
endif

# Define USE_LOCAL_DSPLIB for conditional includes in source
ifeq ($(USE_LOCAL_DSPLIB),1)
VITALSIGNS_DSS_CFLAGS += -DUSE_LOCAL_DSPLIB
endif

# Define USE_LOCAL_MATHUTILS for conditional includes in source
ifeq ($(USE_LOCAL_MATHUTILS),1)
VITALSIGNS_DSS_CFLAGS += -DUSE_LOCAL_MATHUTILS
endif

###################################################################################
# Vital Signs DSS vpath for source files
###################################################################################
VITALSIGNS_DSS_VPATH = $(MMWAVE_SDK_INSTALL_PATH)/../src/dss

ifeq ($(USE_LOCAL_DSPLIB),1)
VITALSIGNS_DSS_VPATH += $(MMWAVE_SDK_INSTALL_PATH)/../src/dss/dsplib
endif

ifeq ($(USE_LOCAL_MATHUTILS),1)
VITALSIGNS_DSS_VPATH += $(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathutils
endif

###################################################################################
# Export for use by parent makefile
###################################################################################
# These are appended to the main build variables:
# C674_CFLAGS += $(VITALSIGNS_DSS_CFLAGS) $(VITALSIGNS_DSS_INC)
# DSS_MMW_DEMO_SOURCES += $(VITALSIGNS_DSS_SOURCES)
# vpath %.c $(VITALSIGNS_DSS_VPATH)
#
# Conditional library linking (in parent makefile):
# ifneq ($(USE_LOCAL_DSPLIB),1)
#   DSS_MMW_DEMO_STD_LIBS += -ldsplib.ae64P
# endif
# ifneq ($(USE_LOCAL_MATHLIB),1)
#   DSS_MMW_DEMO_STD_LIBS += -lmathlib.$(C674_LIB_EXT)
# endif
# ifneq ($(USE_LOCAL_MATHUTILS),1)
#   DSS_MMW_DEMO_STD_LIBS += -llibmathutils.$(C674_LIB_EXT)
# endif
