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

# sha256sum on Linux, shasum on macOS
SHA256 := $(shell command -v sha256sum >/dev/null 2>&1 && echo sha256sum || echo shasum -a 256)


# **************************************************************************** #
#									.PHONY									   #
# **************************************************************************** #

.PHONY: clean fclean re bonus run build client gui help install-deps

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

all: install-deps run

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

build:
	make -C server server
	make -C clients/cli clientCLI

# Start the server and the CLI client, each in its own terminal window
run: build
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
	$(ECHO) "$(CYAN)Lancement du serveur et du client ($$term)...$(RESET)"
	launch "TAP server" "make -C '$(CURDIR)/server' run"
	sleep 1
	launch "TAP client" "make -C '$(CURDIR)/clients/cli' run"

# Build and run the CLI client alone, in the current terminal
client:
	make -C clients/cli

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