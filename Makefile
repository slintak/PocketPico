BUILD_DIR = ./build

.PHONY: configure build clean fullclean

configure:
	@if [ -d "${BUILD_DIR}" ]; then echo "Project already configured. Maybe you need to call 'make clean'?" && false ; fi
	@mkdir ${BUILD_DIR}
	@cmake -B ${BUILD_DIR} -S .

build:
	@make -j4 -C ${BUILD_DIR}

clean:
	@make -C ${BUILD_DIR} clean

fullclean:
	@rm -rf ${BUILD_DIR}
	@echo "Build directory '${BUILD_DIR}' removed."