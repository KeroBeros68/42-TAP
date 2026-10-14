*This project has been created as part of the 42 curriculum by kebertra and gtourdiat.*

# 🗡️ 42-TAP

<!--
  Skeleton of the README. Every section below is REQUIRED by the subject (chap. VI)
  or by the RFC rules (chap. IV). Fill the sections in place, keep the titles unchanged.
  The README must be written in English.
  Legend: [S] = required by the subject, [+] = optional addition of the team.
-->

## 📖 Description <!-- [S] -->

### 🎯 Goal

42-TAP is our implementation of **TAP — The Answer Protocol**, a shared-world retro text adventure in the spirit of the old MUDs (Multi-User Dungeons). Several players connect to the same server, explore a common map, talk, pick up and drop items, meet NPCs and fight enemies, all through a simple line-based TCP protocol specified by RFC 42TAP (`docs/protocol-rfc.html`).

The goal is to build the server and two interchangeable clients that strictly follow this protocol, and to make our own design decisions where the RFC deliberately leaves them open (combat, quests, NPC behaviour).

### 🔭 Overview

The project is made of three programs, all written in C++:

- 🖥️ **Server** (`server/`, `game/`, `shared/`): a TCP server speaking the RFC 42TAP protocol (UTF-8, one `\n`-terminated message per line). It loads the world from JSON files (`game/ressources/`), validates its references (exits, NPCs, items, spawns), and handles players, items, NPCs and combat. Every connection, command, response and world change is written to a structured JSON log.
- ⌨️ **CLI client** (`clients/cli/`): a terminal client that displays server messages in real time while waiting for user input.
- 🪟 **GUI client** (`clients/gui/`): a graphical client built with Qt, showing the room, its players, items and NPCs, and the player count.

Because the protocol is the only contract between server and clients, our clients are meant to work with the server of another group, and the other way around.

### ✨ Features

- 🤝 Connection handshake and player naming (`CONNECT`).
- 🧭 Exploration of a 15-room world containing a loop and optional branches (`LOOK`, `MOVE`).
- 🎒 Dynamic item system: every item is a unique instance, taking it removes it from the room, dropping it makes it available to others; items and NPCs can be designated by id, by name (multi-word names included), case-insensitive (`TAKE healing herbs`, `TALK hector`).
- 🧙 NPCs with dialogue (`TALK`) and enemies (`ATTACK`, `STATUS`): players start with 100 HP, enemies counter-attack, and a player reaching 0 HP respawns at a safe location with reduced health.
- 📝 Structured JSON logging with timestamps, levels, IP addresses, players and commands.
- 🚪 Clean disconnection (`QUIT`, signals) and graceful handling of malformed commands.

### 🚧 Current status and known limitations

The project is still in progress. At the time of writing:

- ✅ 9 of the 15 RFC commands are implemented: `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, `ATTACK`, `STATUS`.
- ❌ `CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO` and `GROUP` are not implemented yet.
- ❌ The server does not push `EVT` events yet (presence, chat, stats).
- ⚠️ The quests are described in `game/ressources/quests.json` but are not used by the server yet.

This section will be updated as the missing parts are completed.

## 🧭 Instructions <!-- [S] -->

### Requirements
### Quick start

## 🔨 Building and Running <!-- [S] -->

### 🛠️ Build system

The project is built with **GNU Make** (C++20, `c++ -Wall -Wextra -Werror`) for the server and the CLI client, and with **CMake** for the Qt GUI client. There is one Makefile at the root, which drives the three others (`server/Makefile`, `clients/cli/Makefile`, `clients/gui/Makefile`).

Requirements:

- a C++20 compiler (`c++`, g++ or clang++), `make`
- `curl` or `wget` (to download the JSON library)
- for the GUI client only: **CMake** and **Qt 5** (Widgets module). `clients/gui/CMakeLists.txt` asks for CMake 4.2 or newer.
- a graphical terminal emulator (gnome-terminal, konsole, xfce4-terminal, kitty, alacritty or xterm) for `make run` only

### 📦 Installing dependencies

```sh
make install-deps
```

The only third-party library is [nlohmann/json](https://github.com/nlohmann/json) 3.12.0, used to read the world files and to build the JSON replies. It is not stored in git: the target downloads `third_party/json.hpp` and checks it against a pinned SHA-256 checksum (the file is rejected if it does not match). The server Makefile calls this target by itself if the file is missing.

### 🖥️ Server

```sh
make -C server        # build ./server/server, then start it
./server/server       # start it again later
```

The server listens on TCP port **8080** (`SERVER_PORT` in `shared/defines.hpp`) and loads the world from `game/ressources/*.json`. It stops cleanly on `SIGINT`/`SIGTERM` (Ctrl+C). The JSON log is written while it runs (see [Server Logging](#-server-logging)).

If you move the project folder after a build, run `make -C server re`: the absolute path of the project is compiled into the server to find the world files.

### ⌨️ CLI client

```sh
make client           # build ./clients/cli/clientCLI and start it in the current terminal
```

The client connects to `127.0.0.1:8080` (`SERVER_IP` in `clients/defines.hpp` and `SERVER_PORT` in `shared/defines.hpp`). To use another address, change these values and rebuild. Start the server first.

### 🪟 GUI client

```sh
make gui              # configure with CMake, build with Qt 5, start ./clients/gui/42TAP
```

The address and the port of the server are typed in the connection window.

### 🚀 Everything at once

```sh
make                  # same as "make run" after "make install-deps"
make run              # build the server and the CLI client, open each one in its own terminal window
```

### 🧹 Other targets

| Target | Effect |
| --- | --- |
| `make install-deps` | Download and verify the third-party JSON library |
| `make build` | Build the server and the CLI client without starting them |
| `make run` | Build, then start the server and the CLI client in two terminal windows |
| `make client` | Build and start the CLI client in the current terminal |
| `make gui` | Build and start the GUI client |
| `make clean` | Remove object files and the GUI `build/` folder |
| `make fclean` | `clean`, plus remove the executables |
| `make re` | `fclean`, then `all` |

<!-- TODO (subject VII.1): the build tool must also provide the targets run-server, run-client, run-client-gui and lint. Missing today; add them to the root Makefile, then update this table. -->

## 🏗️ Architecture <!-- [S] -->

### 🗂️ Project layout

```
server/            network layer: sockets, sessions, command registration, JSON serialization
game/              game logic, independent from the network
  world/           World, Room, JSON parser and validation of the world files
  entities/        Player, Npc, Item, Inventory, Stats
  combat/          damage formula and fight round
  ressources/      world data: room.json, npc.json, items.json, quests.json
  utils/lookup.hpp designation of an NPC / item by id or name
shared/            code used by the server and the clients: protocol constants, errors, logger, UTF-8
clients/           client.cpp: network engine shared by the clients
  cli/             command-line client
  gui/             Qt client
third_party/       nlohmann/json (downloaded by make install-deps)
tools/             mapviz: draws the world map as an SVG
audit/ docs/       internal audits, subject and RFC
```

The code is split in two layers that only meet in `server/commands.cpp`:

- the **network layer** (`Server`, `Connection`, `User`) knows sockets and protocol lines, but nothing about the game;
- the **game layer** (`Game`, `World`, `Room`, `Player`...) knows rooms, items and fights, but nothing about sockets or the protocol syntax.

`commands.cpp` is the glue: it receives a command from the network layer, calls the `Game`, and turns the result into a `Response` (success or error code), which the server writes back as a protocol line.

### 🚦 Server design: dispatcher

We chose a **dispatcher** (a router) rather than inline handling. The server never interprets commands itself:

1. `Server::handeLine` splits a received line into the command name and its arguments.
2. It looks the name up in a table `command name -> handler` (`std::unordered_map<std::string, std::function<Response(User&, const std::string&)>>`).
3. The handler returns a `Response`, which is logged and sent to the client.

Handlers are registered at startup with `registerCommands` (`server.defineAction(CMD_LOOK, ...)`). Adding a command means writing one `cmdXxx` function and one registration line, without touching the network code. Commands that are not implemented yet are registered on a common stub handler, so the table already lists all 15 RFC commands.

The dispatcher also enforces the protocol state machine: until `CONNECT` has succeeded, any other command is refused with an error; an unknown command gets an error too, and the connection stays open.

Why a dispatcher: the 15 commands share the same flow (parse, check state, act, reply, log), so this flow is written once; each handler stays short and can be read on its own; and the network layer can be tested and reused without the game.

### 🔄 Concurrency model

The server is **single-threaded and event-driven**, built on `poll(2)`. There is no thread, no mutex.

- One loop (`main`) calls `Server::updatePoll()`. Each turn builds the list of file descriptors to watch: the listening socket, a `signalfd`, and one socket per client (watched for output too when data is waiting to be sent).
- All sockets are **non-blocking**. Reading drains the socket into a per-client input buffer; complete lines (ending with `\n`, an optional `\r` is tolerated) are extracted from it and dispatched. Replies go to a per-client output buffer, flushed with `send(MSG_NOSIGNAL)`, so a client that disconnects in the middle of a send cannot kill the server or block the others.
- Signals (`SIGINT`, `SIGTERM`, `SIGQUIT`) are turned into a readable file descriptor with `signalfd`, so the shutdown is handled in the same loop as the network events.
- A line longer than 1024 bytes without newline gets an error and the client is disconnected, which bounds the memory used per client.
- On disconnection (`QUIT`, closed socket, error), the player is removed from the game by the `onDisconnect` callback **before** the session is destroyed, as the subject requires, so that no leave broadcast can reach a player who is already gone.

Why this model: since every command is handled one after the other in a single thread, the game state (rooms, items, NPCs) is never accessed concurrently. Two players taking the same item at the same moment is solved for free: the first command processed wins and the second gets `ITEM_NOT_FOUND`. A MUD is I/O-bound and has few players, so one thread is largely enough, and there are no locks to get wrong.

### 🎲 Game layer

- `Game` owns the `World` and the players (`std::unordered_map<client id, Player>`). Its methods (`move`, `take`, `drop`, `talk`, `attack`, `status`, ...) take a player id and return a plain result, not a protocol string.
- `World` loads and **validates** the JSON files at startup (exits, spawns, NPC and item references, start and respawn rooms) and refuses to start if a reference is broken. Each `Room` holds its exits, its item instances and its NPC instances.
- **Items and NPCs are instances**: the definition (from the JSON database) is separated from each living copy, which has its own id, so an item exists exactly once in the world and cannot be duplicated. `Inventory` is composed of items and used by `Player`.
- `lookup.hpp` finds an item or an NPC from what the player typed (instance id, definition id, name, or one word of the name, in any case).
- `combat` computes the damage and runs one round of fight on two `Stats` objects. See [Combat System](#-combat-system).

### 🖱️ Clients

- `clients/client.cpp` is a small network engine shared by the clients: it connects to the server, reads the lines in a buffer and calls the handler registered for the first word of the line (`OK`, `ERR`, `EVT`), the same dispatcher idea as on the server.
- The **CLI client** waits with `poll` on both the server socket and the keyboard, so it keeps displaying incoming messages while the user is typing.
- The **GUI client** is built with Qt 5 (`clients/gui/`). It is organised in windows (connection, game, error) built from custom widgets (`TAPLabel`, `TAPLineEdit`, `TAPRoomView`, `TAPPlayerCountBar`...). Its network part is not connected yet.

### 💬 Client interface choice

<!-- TODO: to decide and document (subject V.3): the CLI either sends the RFC syntax as typed, or translates friendly commands into RFC packets. Today the CLI sends each typed line as is. -->

## 📡 Protocol Implementation <!-- [S] -->

The server follows **RFC 42TAP** (`docs/protocol-rfc.html`, protocol version 1): UTF-8, TCP, one message per line terminated by `\n` (a `\r` before it is accepted and removed), greeting `OK hello proto=1` on connection, then `CONNECT <name>` before anything else. The protocol constants and the success/error line builders are in `shared/` (`defines.hpp`, `errors.hpp`, `success.hpp`) and are the only place where protocol text is written.

### ✅ Implemented commands

| Command | Reply | State |
| --- | --- | --- |
| `CONNECT <name>` | `OK connected` / `ERR 201 NAME_IN_USE` | done |
| `LOOK` | `OK {room, players, items, npcs}` | done |
| `MOVE <direction>` | `OK room=<id>` / `ERR 301 NO_EXIT` | done |
| `TAKE <item>` | `OK taken=<id>` / `ERR 404 ITEM_NOT_FOUND` | done |
| `DROP <item>` | `OK dropped=<id>` / `ERR 404 ITEM_NOT_IN_INVENTORY` | done |
| `TALK <npc>` | `OK <dialogue>` / `ERR 404 NPC_NOT_FOUND` | done |
| `ATTACK <npc>` | `OK {"attacker_hp","target_hp","damage","status"}` / `ERR 404` / `ERR 405 NPC_NOT_HOSTILE` | done |
| `STATUS` | `OK {"hp","max_hp","status"}` | done |
| `QUIT` | `OK bye`, then the connection is closed | done |
| `INVENTORY`, `CHAT`, `WHO`, `GROUP`, `QUEST`, `QUESTS` | registered, but answer a placeholder `OK Message received` | **not implemented yet** |

### 📣 Implemented events

None yet. The `EVT` messages of the RFC (`ROOM PRESENCE ENTER/LEAVE`, `ROOM/GLOBAL/GROUP CHAT`, `GROUP INVITE/JOIN/LEAVE`, `STATS players=`) are not sent for the moment.

### ↔️ Deviations from RFC 42TAP

The subject asks us to document every deviation and to justify it.

| # | Deviation | Justification |
| --- | --- | --- |
| 1 | **Extra error `ERR 400 BAD_REQUEST`**, sent for an unknown command, a malformed command (for example a name with a space in `CONNECT`), any command before `CONNECT`, and a line over 1024 bytes (the client is then disconnected). | The RFC asks for "appropriate error responses" to malformed messages (§9.3) but defines no code for them. We use 400, the usual "bad request" class, so that a client can tell a protocol error from a game error (2xx-4xx codes of the RFC). |
| 2 | **Extra error `ERR 403 INVENTORY_FULL`** for `TAKE` when the inventory holds 10 items. | The RFC recommends limits on "resource acquisition per player" (§9.4) but defines no error for it. 403 is free between 402 and 404. |
| 3 | **Instance ids are visible on the wire**: `LOOK` returns `"items":["item.herbs#3"]`, `"npcs":["npc.guard#10"]`, and `TAKE` answers `OK taken=item.herbs#3` (the RFC examples show `item.herbs`). | The subject requires items to be unique instances (no duplication). Two herbs in the same room need two different ids, so the instance counter is part of the id. The definition id (`item.herbs`), the name (`Healing Herbs`) and a word of the name are still accepted in commands, so a client that does not know about `#N` keeps working (`TAKE herbs`). A client must treat ids as opaque strings. |
| 4 | **Command names are case-sensitive** (`LOOK` works, `look` gets `ERR 400`). The RFC (§4.2) says they are case-insensitive. | Not a choice: this is a gap that we have to fix. Item and NPC names in the arguments are case-insensitive. |
| 5 | **Player names** are limited to 32 bytes, must be valid UTF-8 and cannot contain a space. | The RFC only requires uniqueness (§3.3) and correct handling of Unicode (§9.2). Without a limit and without spaces, a name cannot break the parsing of the lines that contain it, such as `EVT ROOM PRESENCE ENTER <name>`. |
| 6 | **`ATTACK` returns the status of the attacker** in the `status` field (`healthy`, `combat`...), and `ERR 400` when the target is already dead. | The RFC shows the field but does not say whose status it is. See [Combat System](#-combat-system). |
| 7 | **Placeholder answers** `OK Message received` for the 6 commands not yet implemented, and no `EVT` at all. | Temporary state of the development, not a choice. These lines will disappear when the commands are done. |

### 🚨 Error handling

- Every error line is `ERR <code> <NAME>` and never contains free text, so it can be parsed by a client of another group.
- Errors never close the connection, except the line that is too long and `QUIT`. A client can keep playing after any other error.
- A client that disconnects in the middle of a send (broken pipe, reset) is detected on the next read or write and cleaned up; the other players are not affected (`MSG_NOSIGNAL`, non-blocking sockets).
- Messages split in several TCP packets are buffered until the `\n` arrives, and several commands in one packet are processed one by one (RFC §9.2).

## ⚔️ Combat System <!-- [S] -->

### Rules (HP, respawn)
### Damage formula
### Initiative order
### Additional commands (DEFEND, FLEE, ...)
### Broadcast of combat results
### Design justification

## 📜 Quest System <!-- [S] -->

### Quest data format
### Progression mechanics
### Completion validation
### Rewards
### Quest list

## 🗺️ World Design <!-- [S] -->

### World data format
### Map and room connections
### NPC roles
### Items and distribution
### World validation

## 📝 Server Logging <!-- [S] -->

### Log format
### Log levels and event types
### Output destinations
### Monitoring the server
### Abuse detection (command flooding, rapid connections)
### Performance considerations

## 🧪 Testing <!-- [S] -->

### Multiplayer
### Combat
### Quests
### Protocol compliance and edge cases

## 👥 Group Contributions <!-- [S] -->

### Member 1
### Member 2
### Member 3

## 📚 Resources <!-- [S] -->

### Documentation and references
### Use of AI

<!-- Kept from the previous draft, to be integrated in "Use of AI":

- NPC lookup (`Room::findNpc` in `game/world/room.cpp`): the team first wrote a simple version, where the player had to give the exact instance id of the NPC (for example `npc.guard#10`). The version in the code now was proposed and improved with the help of Claude (an AI assistant): a player can also designate an NPC by the id of its definition, its short id, its name, or one word of its name, in any case (`TALK guard`, `TALK Guard Hector`, `TALK hector`). The team reviewed and kept it.

Color palette: https://coolors.co/palette/606c38-283618-fefae0-dda15e-bc6c25
#606c38 #283618 #fefae0 #dda15e #bc6c25
-->
