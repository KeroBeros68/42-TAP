
# Compilation
EXECUTABLE_NAME		:=	TAP_Client
BUILD_DIR			:=	build

install:
	cmake . -B ./$(BUILD_DIR)
	cmake --build ./build
	mv ./$(BUILD_DIR)/$(EXECUTABLE_NAME) ./

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -rf $(EXECUTABLE_NAME)

run:
	./$(EXECUTABLE_NAME)

re: fclean install