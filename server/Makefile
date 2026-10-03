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
NAME = server

# Directories
OBJ_DIR = Obj

SRCS =	server.cpp \
	signalFd.cpp \
	user.cpp \
	connection.cpp \
	commands.cpp \
	main.cpp

INCLUDES = $(wildcard *.hpp) $(wildcard ../shared/*.hpp)

# Compilation
CC = c++
CFLAGS = -Wall -Werror -Wextra -std=c++23 -pthread

# Object name
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

#
TOTAL := $(words $(OBJS))
CURRENT := 0

# **************************************************************************** #
#									.PHONY									   #
# **************************************************************************** #

.PHONY: clean fclean re bonus run help

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

all: $(NAME) run

$(NAME): $(OBJS)
	$(ECHO) "$(CYAN)Création de l'exécutable $(NAME)...$(RESET) "
	if $(CC) $(CFLAGS) -o $@ $(OBJS) 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		$(CC) $(CFLAGS) -o $@ $(OBJS); \
	fi

$(OBJ_DIR)/%.o: %.cpp $(INCLUDES) | $(OBJ_DIR)
	$(eval CURRENT=$(shell $(ECHO) $$(($(CURRENT)+1))))
	printf "$(CYAN)[%2d/%2d]$(RESET) %-30s " $(CURRENT) $(TOTAL) $(notdir $<)
	if $(CC) $(CFLAGS) -c $< -o $@ 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		$(CC) $(CFLAGS) -c $< -o $@; \
	fi

$(OBJ_DIR):
	$(ECHO) "$(CYAN)Création du répertoire $(OBJ_DIR)...$(RESET) "
	if mkdir -p $(OBJ_DIR) 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		mkdir -p $(OBJ_DIR); \
	fi

run:
	if [ -f "$(NAME)" ]; then \
		./$(NAME); \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		$(ECHO) "$(YELLOW)⚠ L'exécutable $(NAME) n'existe pas. Veuillez exécuter 'make' avant de lancer le programme.$(RESET)"; \
	fi

bonus: re

clean:
	$(ECHO) "$(CYAN)Suppression de $(OBJ_DIR)...$(RESET) "
	if [ -d "$(OBJ_DIR)" ]; then \
		$(RM) -rf $(OBJ_DIR) && \
		$(ECHO) "$(GREEN)✓ Fichiers objets supprimés$(RESET)"; \
	else \
		$(ECHO) "$(YELLOW)⚠ Rien à nettoyer$(RESET)"; \
	fi

fclean: clean
	$(ECHO) "$(CYAN)Suppression de $(NAME)...$(RESET) "
	if rm $(NAME) 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(YELLOW)⚠ Déjà supprimée$(RESET)"; \
	fi

re: fclean all