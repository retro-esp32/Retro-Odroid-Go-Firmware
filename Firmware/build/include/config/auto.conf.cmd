deps_config := \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/app_trace/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/aws_iot/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/bt/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/driver/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esp32/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esp_event/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esp_http_client/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esp_http_server/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/ethernet/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/fatfs/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/freemodbus/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/freertos/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/heap/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/libsodium/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/log/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/lwip/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/mbedtls/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/mdns/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/mqtt/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/nvs_flash/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/openssl/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/pthread/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/spi_flash/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/spiffs/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/vfs/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/wear_levelling/Kconfig \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/eugene/Desktop/github/RetroESP32/Public/Firmware/Firmware/components/odroid/Kconfig.projbuild \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/eugene/Desktop/github/RetroESP32/Public/Software/Tools/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
