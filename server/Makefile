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
	game.cpp \
	event_bus.cpp \
	loadenv.cpp \
	respawn_queue.cpp \
	despawn_queue.cpp \
	main.cpp \
	item.cpp \
	npc.cpp \
	room.cpp \
	inventory.cpp \
	player.cpp \
	parser.cpp \
	world.cpp \
	logger.cpp \
	utf8.cpp \
	combat.cpp \
	combat_manager.cpp \
	group_manager.cpp \
	quest.cpp \
	quest_manager.cpp \
	serializer.cpp \
	config.cpp

# The game sources live in ../game
vpath %.cpp network:protocol:../game:../game/world:../game/entities:../shared/logging:../shared/system:../shared/text:../game/combat:../game/events:../game/group:../game/quests

# nlohmann/json is not stored in git: it is downloaded by "make install-deps" (root Makefile)
JSON = ../third_party/json.hpp


# Compilation
CC = c++
CFLAGS = -Wall -Werror -Wextra -std=c++20 -pthread -MMD -MP

# The objects are built in parallel (make JOBS=1 to see the errors one by one)
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
MAKEFLAGS += -j$(JOBS)

# Object name
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(SRCS:.cpp=.o)))

# Dependencies on the headers, written by the compiler (-MMD)
DEPS = $(OBJS:.o=.d)

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

all: $(NAME)
	$(MAKE) --no-print-directory run

$(NAME): $(OBJS)
	$(ECHO) "$(CYAN)Création de l'exécutable $(NAME)...$(RESET) "
	if $(CC) $(CFLAGS) -o $@ $(OBJS) 2>/dev/null; then \
		$(ECHO) "$(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(RED)✗$(RESET)"; \
		$(CC) $(CFLAGS) -o $@ $(OBJS); \
	fi

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	if $(CC) $(CFLAGS) -c $< -o $@ 2>/dev/null; then \
		$(ECHO) "$(CYAN)Compilation$(RESET) $(notdir $<) $(GREEN)✓$(RESET)"; \
	else \
		$(ECHO) "$(CYAN)Compilation$(RESET) $(notdir $<) $(RED)✗$(RESET)"; \
		$(CC) $(CFLAGS) -c $< -o $@; \
	fi

# Only parser.cpp uses the JSON library, only world.cpp needs the absolute path of the project
# (after moving the project folder, run "make re")
$(OBJ_DIR)/parser.o: $(JSON)
$(OBJ_DIR)/serializer.o: $(JSON)
$(OBJ_DIR)/world.o: CFLAGS += '-DPROJECT_ROOT="$(abspath ..)"'

$(JSON):
	$(MAKE) -C .. install-deps

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

re:
	$(MAKE) --no-print-directory fclean
	$(MAKE) --no-print-directory all

-include $(DEPS)
