###################################################################################
# Vital Signs MSS Makefile
###################################################################################

# This makefile is included by the main mmw_mss.mak when VITAL_SIGNS=1

###################################################################################
# Vital Signs MSS source files
###################################################################################
VITALSIGNS_MSS_SOURCES = vitalsign_cli.c

###################################################################################
# Vital Signs MSS include paths
###################################################################################
VITALSIGNS_MSS_INC = -i$(MMWAVE_SDK_INSTALL_PATH)/../src/common \
                     -i$(MMWAVE_SDK_INSTALL_PATH)/../src/mss

###################################################################################
# Vital Signs MSS compiler flags
###################################################################################
VITALSIGNS_MSS_CFLAGS = -DVITAL_SIGNS_ENABLED

###################################################################################
# Export for use by parent makefile
###################################################################################
# These are appended to the main build variables:
# R4F_CFLAGS += $(VITALSIGNS_MSS_CFLAGS) $(VITALSIGNS_MSS_INC)
# MSS_MMW_DEMO_SOURCES += $(VITALSIGNS_MSS_SOURCES)
# vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/mss
