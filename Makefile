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

all: run

run:
	make -C server run
	make -C clients/cli run

gui:
	make -C clients/gui run

bonus: re

clean:
	make -C clients/gui clean
	make -C clients/cli clean
	make -C server clean

fclean: clean
	make -C clients/gui fclean
	make -C clients/cli fclean
	make -C server fclean

re: fclean all