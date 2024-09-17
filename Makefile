BUILD_DIR = ./build

.PHONY: clean configure build

configure:
	@if [ -d "${BUILD_DIR}" ]; then echo "Project already configured. Maybe you need to call 'make clean'?" && false ; fi
	@mkdir ${BUILD_DIR}
	@cmake -B ${BUILD_DIR} -S .

build:
	@make -j4 -C ${BUILD_DIR}

clean:
	@rm -rf ${BUILD_DIR}
	@echo "Build directory '${BUILD_DIR}' removed."