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

# Third-party dependency downloaded by "make install-deps" (not stored in git)
JSON_VERSION = 3.12.0
JSON_URL     = https://github.com/nlohmann/json/releases/download/v$(JSON_VERSION)/json.hpp
JSON_SHA256  = aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63
JSON_FILE    = third_party/json.hpp

# Sources checked by "make lint" (the Qt GUI needs the Qt headers, so it is not included)
LINT_SRCS  := $(shell find server game shared clients/client.cpp clients/cli -name '*.cpp' -not -path '*/Obj/*')
LINT_DIRS  = server game shared clients/client.cpp clients/client.hpp clients/cli
LINT_FLAGS = -std=c++20 -fsyntax-only -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wold-style-cast \
	-Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wnull-dereference -Wdouble-promotion -Wformat=2 \
	'-DPROJECT_ROOT="$(CURDIR)"'
JOBS ?= $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)

# sha256sum on Linux, shasum on macOS
SHA256 := $(shell command -v sha256sum >/dev/null 2>&1 && echo sha256sum || echo shasum -a 256)


# **************************************************************************** #
#									.PHONY									   #
# **************************************************************************** #

.PHONY: all clean fclean re bonus run build build-gui server cli client gui run-server run-client run-client-gui help install-deps lint
.PHONY: clean fclean re bonus run build client gui valgrind help install-deps

.DEFAULT_GOAL := all

# **************************************************************************** #
#									Help								  	   #
# **************************************************************************** #

# Show available make commands.
help:
	$(ECHO) "$(YELLOW)Available commands:$(RESET)"
	$(ECHO) ""
	$(ECHO) "  $(CYAN)make$(RESET) / $(CYAN)make all$(RESET)    install the dependencies and build the server and the CLI client (nothing is started)"
	$(ECHO) "  $(CYAN)make build$(RESET)          build the server and the CLI client"
	$(ECHO) "  $(CYAN)make build-gui$(RESET)      build the GUI client (needs CMake and Qt 5)"
	$(ECHO) "  $(CYAN)make server$(RESET)         build and start the server in a new terminal window (alias: run-server)"
	$(ECHO) "  $(CYAN)make cli$(RESET)            build and start the CLI client in a new terminal window (aliases: client, run-client)"
	$(ECHO) "  $(CYAN)make gui$(RESET)            build and start the GUI client                 (alias: run-client-gui)"
	$(ECHO) "  $(CYAN)make run$(RESET)            build, then start the server and the CLI client in two terminal windows"
	$(ECHO) "  $(CYAN)make lint$(RESET)           strict compilation of all the sources (warnings are errors), check of the whitespace, and cppcheck / clang-tidy if installed"
	$(ECHO) "  $(CYAN)make install-deps$(RESET)   download and verify the JSON library"
	$(ECHO) "  $(CYAN)make clean$(RESET) / $(CYAN)fclean$(RESET) / $(CYAN)re$(RESET)   remove the objects / also the executables / rebuild everything"

# **************************************************************************** #
#									Rules									   #
# **************************************************************************** #

# Static checks: the strict warnings of the compiler on every source (nothing is built), no whitespace at the end of a line,
# then cppcheck and clang-tidy when they are installed
lint: install-deps
	status=0
	$(ECHO) "$(CYAN)Lint : compilation stricte (les warnings sont des erreurs)...$(RESET)"
	if printf '%s\n' $(LINT_SRCS) | xargs -P $(JOBS) -I{} c++ $(LINT_FLAGS) {}; then
		$(ECHO) "$(GREEN)✓ $(words $(LINT_SRCS)) fichiers$(RESET)"
	else
		$(ECHO) "$(RED)✗ warnings du compilateur$(RESET)"
		status=1
	fi
	$(ECHO) "$(CYAN)Lint : espaces en fin de ligne...$(RESET)"
	if grep -rnE '[[:blank:]]+$$' --include='*.cpp' --include='*.hpp' $(LINT_DIRS); then
		$(ECHO) "$(RED)✗ espaces en fin de ligne$(RESET)"
		status=1
	else
		$(ECHO) "$(GREEN)✓$(RESET)"
	fi
	if command -v cppcheck >/dev/null 2>&1; then
		$(ECHO) "$(CYAN)Lint : cppcheck...$(RESET)"
		if cppcheck --enable=warning,performance,portability --error-exitcode=1 --inline-suppr --quiet -I shared -I game $(LINT_SRCS); then
			$(ECHO) "$(GREEN)✓$(RESET)"
		else
			status=1
		fi
	else
		$(ECHO) "$(YELLOW)- cppcheck non installé : ignoré$(RESET)"
	fi
	if command -v clang-tidy >/dev/null 2>&1; then
		$(ECHO) "$(CYAN)Lint : clang-tidy...$(RESET)"
		if clang-tidy --quiet --warnings-as-errors='*' $(LINT_SRCS) -- -std=c++20 '-DPROJECT_ROOT="$(CURDIR)"'; then
			$(ECHO) "$(GREEN)✓$(RESET)"
		else
			status=1
		fi
	else
		$(ECHO) "$(YELLOW)- clang-tidy non installé : ignoré$(RESET)"
	fi
	exit $$status

# Builds everything but does not start anything
all: install-deps build

# Download the dependencies (nlohmann/json), checked against a pinned SHA-256
install-deps: $(JSON_FILE)

$(JSON_FILE):
	$(ECHO) "$(CYAN)Téléchargement de nlohmann/json $(JSON_VERSION)...$(RESET)"
	mkdir -p $(dir $@)
	tmp="$@.tmp"
	trap 'rm -f "$$tmp"' EXIT
	if command -v curl >/dev/null 2>&1; then
		curl -fsSL -o "$$tmp" "$(JSON_URL)"
	elif command -v wget >/dev/null 2>&1; then
		wget -q -O "$$tmp" "$(JSON_URL)"
	else
		$(ECHO) "$(RED)✗ curl ou wget est requis$(RESET)"
		exit 1
	fi
	if ! echo "$(JSON_SHA256)  $$tmp" | $(SHA256) -c --status; then
		$(ECHO) "$(RED)✗ Somme de contrôle invalide : fichier rejeté$(RESET)"
		exit 1
	fi
	mv "$$tmp" "$@"
	$(ECHO) "$(GREEN)✓ $(JSON_FILE)$(RESET)"

# Build the server and the CLI client (the GUI needs Qt: see build-gui)
build: install-deps
	make -C server server
	make -C clients/cli clientCLI

# Build the GUI client
build-gui:
	if ! command -v cmake >/dev/null 2>&1; then
		$(ECHO) "$(RED)✗ cmake est requis pour le client GUI (et Qt 5).$(RESET)"
		exit 1
	fi
	make -C clients/gui all

# Shell code that finds a terminal emulator and defines launch "<title>" "<command>", which runs a command in a new window
define TERM_SETUP
term=""
for t in gnome-terminal konsole xfce4-terminal kitty alacritty xterm x-terminal-emulator; do
	if command -v $$t >/dev/null 2>&1; then term=$$t; break; fi
done
if [ -z "$$term" ]; then
	$(ECHO) "$(RED)✗ Aucun terminal trouvé (gnome-terminal, konsole, xfce4-terminal, kitty, alacritty, xterm).$(RESET)"
	exit 1
fi
launch() {
	title="$$1"; cmd="$$2"
	full="$$cmd; echo; read -rp 'Appuyez sur Entrée pour fermer...'"
	case "$$term" in
		gnome-terminal) gnome-terminal --title="$$title" -- bash -c "$$full" ;;
		konsole)        konsole -p tabtitle="$$title" -e bash -c "$$full" & ;;
		xfce4-terminal) xfce4-terminal --title="$$title" -e "bash -c \"$$full\"" & ;;
		*)              "$$term" -e bash -c "$$full" & ;;
	esac
}
endef

# Build the server, then start it in a new terminal window (it finds the .env file from server/)
run-server: install-deps
	make -C server server
	$(TERM_SETUP)
	launch "TAP server" "make -C '$(CURDIR)/server' run"

# Build the CLI client, then start it in a new terminal window (it finds the .env file from clients/cli/)
run-client: install-deps
	make -C clients/cli clientCLI
	$(TERM_SETUP)
	launch "TAP client" "make -C '$(CURDIR)/clients/cli' run"

# Build and start the GUI client
run-client-gui: build-gui
	cd clients/gui && ./42TAP

# Short names of the three targets above
server: run-server
cli client: run-client
gui: run-client-gui

# Start the server and the CLI client, each in its own terminal window
run: build
	$(TERM_SETUP)
	$(ECHO) "$(CYAN)Lancement du serveur et du client ($$term)...$(RESET)"
	launch "TAP server" "make -C '$(CURDIR)/server' run"
	sleep 1
	launch "TAP client" "make -C '$(CURDIR)/clients/cli' run"

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