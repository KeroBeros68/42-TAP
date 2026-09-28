
# Compilation
EXECUTABLE_NAME		:=	42TAP
BUILD_DIR			:=	build

install: $EXECUTABLE_NAME

$EXECUTABLE_NAME:
	cmake . -B ./$(BUILD_DIR)
	cmake --build ./build
	cp ./$(BUILD_DIR)/$(EXECUTABLE_NAME) ./$(EXECUTABLE_NAME)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(EXECUTABLE_NAME)

run: $EXECUTABLE_NAME
	./$(EXECUTABLE_NAME)

re: fclean install