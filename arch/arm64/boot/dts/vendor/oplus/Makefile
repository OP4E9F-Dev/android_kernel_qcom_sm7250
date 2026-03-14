ifeq ($(CONFIG_BUILD_ARM64_DT_OVERLAY),y)
    dtbo-$(CONFIG_ARCH_LITO) += ciri-20127_t0-mtp-overlay.dtbo \
    
    ciri-20127_t0-mtp-overlay.dtbo-base := lito-ciri.dtb lito-v2.dtb lito.dtb
else
    dtb-$(CONFIG_ARCH_LITO) += lito-mtp.dtb
endif

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files    := *.dtb *.dtbo
