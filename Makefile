PROGRAM=my_program
FLASH_SIZE=32
PROGRAM_SRC_DIR=./user
EXTRA_COMPONENTS=extras/rboot-ota extras/dhcpserver extras/http-parser extras/stdin_uart_interrupt extras/rboot-ota

version:
	@echo "#define BUILD_DATE __DATE__" > $(PROGRAM_SRC_DIR)/version.h
	@echo "#define BUILD_TIME __TIME__" >> $(PROGRAM_SRC_DIR)/version.h
	@echo "#define GIT_VERSION \"$(shell git describe --always)\"" >> $(PROGRAM_SRC_DIR)/version.h

include /home/mehdi/Mehdi/perso/esp-open-rtos/common.mk