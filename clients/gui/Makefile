.SILENT:
.ONESHELL:
SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c

# **************************************************************************** #
#                                   COLORS                                     #
# **************************************************************************** #

GREEN   := \033[0;32m
RED     := \033[0;31m
YELLOW  := \033[0;33m
BLUE    := \033[0;34m
MAGENTA := \033[0;35m
CYAN    := \033[0;36m
RESET   := \033[0m

ECHO    := echo -e

# **************************************************************************** #
#                                  VARIABLES                                   #
# **************************************************************************** #

# Output Name
EXECUTABLE_NAME = 42TAP

# Build directory
BUILD_DIR = build

# Compilation
CMAKE = cmake

# **************************************************************************** #
#									.PHONY									   #
# **************************************************************************** #

.PHONY: all clean fclean re run valgrind help

.DEFAULT_GOAL := all

# **************************************************************************** #
#									Help								  	   #
# **************************************************************************** #

# Show available make commands.
help:
	$(ECHO) "$(YELLOW)Available commands:$(RESET)"
	$(ECHO) ""

# **************************************************************************** #
#									Rules									   #
# **************************************************************************** #

# cmake handles the incremental build: "make" always brings the executable up to date
all:
	$(ECHO) "$(CYAN)Configuration de $(EXECUTABLE_NAME)...$(RESET) "
	if $(CMAKE) . -B ./$(BUILD_DIR); then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		exit 1; \
	fi
	$(ECHO) "$(CYAN)Compilation de $(EXECUTABLE_NAME)...$(RESET) "
	if $(CMAKE) --build ./$(BUILD_DIR); then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		exit 1; \
	fi
	$(ECHO) "$(CYAN)Copie de l'exécutable...$(RESET) "
	if cp ./$(BUILD_DIR)/$(EXECUTABLE_NAME) ./$(EXECUTABLE_NAME); then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		exit 1; \
	fi

run: all
	$(ECHO) "$(CYAN)Lancement de $(EXECUTABLE_NAME)...$(RESET)"
	QT_XCB_GL_INTEGRATION=none ./$(EXECUTABLE_NAME)

valgrind: all
	$(ECHO) "$(CYAN)Analyse de $(EXECUTABLE_NAME) avec valgrind...$(RESET)"
	QT_XCB_GL_INTEGRATION=none valgrind --leak-check=full ./$(EXECUTABLE_NAME)

clean:
	$(ECHO) "$(CYAN)Suppression de $(BUILD_DIR)...$(RESET) "
	if [ -d "$(BUILD_DIR)" ]; then \
		$(RM) -rf $(BUILD_DIR) && \
		$(ECHO) "$(GREEN)✓ Répertoire de build supprimé$(RESET)"; \
	else \
		$(ECHO) "$(YELLOW)⚠ Rien à nettoyer$(RESET)"; \
	fi

fclean: clean
	$(ECHO) "$(CYAN)Suppression de $(EXECUTABLE_NAME)...$(RESET) "
	if rm $(EXECUTABLE_NAME) 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(YELLOW)⚠ Déjà supprimée$(RESET)"; \
	fi

re: fclean all
