###################################################################################
# Millimeter Wave Demo
###################################################################################
.PHONY: dssDemo dssDemoClean

###################################################################################
# Setup the VPATH:
###################################################################################
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/ti/demo/utils \
          $(MMWAVE_SDK_INSTALL_PATH)/ti/datapath/dpc/objectdetection/objdetdsp/src \
	      $(C64Px_DSPLIB_INSTALL_PATH)/packages/ti/dsplib/src/DSP_fft16x16_imre/c64P \
	      $(C64Px_DSPLIB_INSTALL_PATH)/packages/ti/dsplib/src/DSP_fft32x32/c64P \
	      ./dss

###################################################################################
# Local Library Configuration (for Vital Signs integration)
###################################################################################
USE_LOCAL_MATHLIB ?= 1
USE_LOCAL_DSPLIB ?= 0
USE_LOCAL_MATHUTILS ?= 1

###################################################################################
# Additional libraries which are required to build the DEMO:
###################################################################################
DSS_MMW_DEMO_STD_LIBS = $(C674_COMMON_STD_LIB)						\
		   	-llibcrc_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)		\
		   	-llibmailbox_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)		\
			-llibmmwavealg_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)  	\
			-llibedma_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)		\
			-llibdpm_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT) 		\
			-llibosal_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)		\
			-llibcfarcaproc_dsp_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)    \
			-llibdopplerproc_dsp_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)   \
			-llibaoaproc_dsp_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)   \
			-llibdpedma_hwa_$(MMWAVE_SDK_DEVICE_TYPE).$(C674_LIB_EXT)

# Conditional DSPlib linking (vendor library when not using local source)
ifneq ($(USE_LOCAL_DSPLIB),1)
DSS_MMW_DEMO_STD_LIBS += -ldsplib.ae64P
endif

# Conditional Mathlib linking (vendor library when not using local headers)
ifneq ($(USE_LOCAL_MATHLIB),1)
DSS_MMW_DEMO_STD_LIBS += -lmathlib.$(C674_LIB_EXT)
endif

# Conditional Mathutils linking (vendor library when not using local source)
ifneq ($(USE_LOCAL_MATHUTILS),1)
DSS_MMW_DEMO_STD_LIBS += -llibmathutils.$(C674_LIB_EXT)
endif

DSS_MMW_DEMO_LOC_LIBS = $(C674_COMMON_LOC_LIB)						\
   			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/drivers/crc/lib			\
   			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/drivers/mailbox/lib	    	\
   			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/drivers/edma/lib		\
            -i$(MMWAVE_SDK_INSTALL_PATH)/ti/alg/mmwavelib/lib 		\
	        -i$(MMWAVE_SDK_INSTALL_PATH)/ti/control/dpm/lib         	\
        	-i$(MMWAVE_SDK_INSTALL_PATH)/ti/utils/mathutils/lib     	\
			-i$(C674x_MATHLIB_INSTALL_PATH)/packages/ti/mathlib/lib 	\
            -i$(C64Px_DSPLIB_INSTALL_PATH)/packages/ti/dsplib/lib \
			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/datapath/dpc/dpu/dopplerproc/lib    \
			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/datapath/dpc/dpu/cfarcaproc/lib     \
			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/datapath/dpc/dpu/aoaproc/lib        \
			-i$(MMWAVE_SDK_INSTALL_PATH)/ti/datapath/dpedma/lib                 \

###################################################################################
# Millimeter Wave Demo
###################################################################################
DSS_MMW_CFG_PREFIX       = mmw_dss
DSS_MMW_DEMO_CFG         = $(DSS_MMW_CFG_PREFIX).cfg
DSS_MMW_DEMO_ROV_XS      = $(DSS_MMW_CFG_PREFIX)_$(C674_XS_SUFFIX).rov.xs
DSS_MMW_DEMO_CONFIGPKG   = mmw_configPkg_dss_$(MMWAVE_SDK_DEVICE_TYPE)
DSS_MMW_DEMO_MAP         = $(MMWAVE_SDK_DEVICE_TYPE)_mmw_demo_dss.map
DSS_MMW_DEMO_OUT         = $(MMWAVE_SDK_DEVICE_TYPE)_mmw_demo_dss.$(C674_EXE_EXT)
DSS_MMW_DEMO_METAIMG_BIN = $(MMWAVE_SDK_DEVICE_TYPE)_mmw_demo.bin
DSS_MMW_DEMO_CMD         = dss/mmw_dss_linker.cmd
DSS_MMW_DEMO_SOURCES     = objectdetection.c \
                           dss_main.c

# Vital Signs DSS sources (conditional)
ifeq ($(VITAL_SIGNS),1)
DSS_MMW_DEMO_SOURCES += vitalsign_dsp.c
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/dss

# Add local DSPlib sources when enabled (FFT + IFFT)
ifeq ($(USE_LOCAL_DSPLIB),1)
DSS_MMW_DEMO_SOURCES += fft_sp.c ifft_sp.c fft_twiddle.c
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/dss/dsplib
endif

# Add local mathutils sources when enabled
ifeq ($(USE_LOCAL_MATHUTILS),1)
DSS_MMW_DEMO_SOURCES += mathutils.c
vpath %.c $(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathutils
endif
endif

DSS_MMW_DEMO_DEPENDS   = $(addprefix $(PLATFORM_OBJDIR)/, $(DSS_MMW_DEMO_SOURCES:.c=.$(C674_DEP_EXT)))
DSS_MMW_DEMO_OBJECTS   = $(addprefix $(PLATFORM_OBJDIR)/, $(DSS_MMW_DEMO_SOURCES:.c=.$(C674_OBJ_EXT)))

###################################################################################
# RTSC Configuration:
###################################################################################
mmwDssRTSC:
	@echo 'Configuring RTSC packages...'
	$(XS) --xdcpath="$(XDCPATH)" xdc.tools.configuro $(C674_XSFLAGS) -o $(DSS_MMW_DEMO_CONFIGPKG) dss/$(DSS_MMW_DEMO_CFG)
	@echo 'Finished configuring packages'
	@echo ' '

###################################################################################
# Build the Millimeter Wave Demo
###################################################################################
dssDemo: BUILD_CONFIGPKG=$(DSS_MMW_DEMO_CONFIGPKG)
dssDemo: C674_CFLAGS += --cmd_file=$(BUILD_CONFIGPKG)/compiler.opt \
   	                    --define=OBJDET_NO_RANGE \
                        --define=APP_RESOURCE_FILE="<ti/demo/$(MMWAVE_SDK_DEVICE_TYPE)/mmw/mmw_res.h>" \
                        -i$(C674x_MATHLIB_INSTALL_PATH)/packages \
			            -i$(C64Px_DSPLIB_INSTALL_PATH)/packages/ti/dsplib/src/DSP_fft16x16_imre/c64P	\
			            -i$(C64Px_DSPLIB_INSTALL_PATH)/packages/ti/dsplib/src/DSP_fft32x32/c64P	\
			            --define=DebugP_LOG_ENABLED

# Vital Signs compiler flags (conditional)
ifeq ($(VITAL_SIGNS),1)
dssDemo: C674_CFLAGS += --define=VITAL_SIGNS_ENABLED \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/common \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss

# Local mathlib include path and define
ifeq ($(USE_LOCAL_MATHLIB),1)
dssDemo: C674_CFLAGS += --define=USE_LOCAL_MATHLIB \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathlib
endif

# Local dsplib include path and define
ifeq ($(USE_LOCAL_DSPLIB),1)
dssDemo: C674_CFLAGS += --define=USE_LOCAL_DSPLIB \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/dsplib
endif

# Local mathutils include path and define
ifeq ($(USE_LOCAL_MATHUTILS),1)
dssDemo: C674_CFLAGS += --define=USE_LOCAL_MATHUTILS \
                        -i$(MMWAVE_SDK_INSTALL_PATH)/../src/dss/mathutils
endif
endif

dssDemo: buildDirectories mmwDssRTSC $(DSS_MMW_DEMO_OBJECTS)
	$(C674_LD) $(C674_LDFLAGS) $(DSS_MMW_DEMO_LOC_LIBS) $(DSS_MMW_DEMO_STD_LIBS) 					\
	-l$(DSS_MMW_DEMO_CONFIGPKG)/linker.cmd --map_file=$(DSS_MMW_DEMO_MAP) $(DSS_MMW_DEMO_OBJECTS) 	\
	$(PLATFORM_C674X_LINK_CMD) $(DSS_MMW_DEMO_CMD) $(C674_LD_RTS_FLAGS) -o $(DSS_MMW_DEMO_OUT)
	$(COPY_CMD) $(DSS_MMW_DEMO_CONFIGPKG)/package/cfg/$(DSS_MMW_DEMO_ROV_XS) $(DSS_MMW_DEMO_ROV_XS)
	@echo '******************************************************************************'
	@echo 'Built the DSS for Millimeter Wave Demo'
	@echo '******************************************************************************'

###################################################################################
# Cleanup the Millimeter Wave Demo
###################################################################################
dssDemoClean:
	@echo 'Cleaning the Millimeter Wave Demo DSS Objects'
	@rm -f $(DSS_MMW_DEMO_OBJECTS) $(DSS_MMW_DEMO_MAP) $(DSS_MMW_DEMO_OUT) $(DSS_MMW_DEMO_METAIMG_BIN) $(DSS_MMW_DEMO_DEPENDS) $(DSS_MMW_DEMO_ROV_XS)
	@echo 'Cleaning the Millimeter Wave Demo DSS RTSC package'
	@$(DEL) $(DSS_MMW_DEMO_CONFIGPKG)
	@$(DEL) $(PLATFORM_OBJDIR)

###################################################################################
# Dependency handling
###################################################################################
-include $(DSS_MMW_DEMO_DEPENDS)

