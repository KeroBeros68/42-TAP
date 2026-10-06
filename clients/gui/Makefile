
# Compilation
EXECUTABLE_NAME		:=	42TAP
BUILD_DIR			:=	build

all: $EXECUTABLE_NAME

$EXECUTABLE_NAME:
	cmake . -B ./$(BUILD_DIR)
	cmake --build ./build
	cp ./$(BUILD_DIR)/$(EXECUTABLE_NAME) ./$(EXECUTABLE_NAME)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(EXECUTABLE_NAME)

run: $EXECUTABLE_NAME
	QT_XCB_GL_INTEGRATION=none ./$(EXECUTABLE_NAME)

re: fclean install

valgrind: $EXECUTABLE_NAME
	QT_XCB_GL_INTEGRATION=none valgrind --leak-check=full ./$(EXECUTABLE_NAME)
