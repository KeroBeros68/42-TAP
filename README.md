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

- 🤝 Connection handshake and player naming (`CONNECT`); command names are case-insensitive.
- 🧭 Exploration of a 15-room world containing a loop and optional branches (`LOOK`, `MOVE`).
- 🎒 Dynamic item system: every item is a unique instance, taking it removes it from the room, dropping it makes it available to others; items and NPCs can be designated by id, by name (multi-word names included), case-insensitive (`TAKE healing herbs`, `TALK hector`).
- 🧙 NPCs with dialogue (`TALK`) and enemies.
- ⚔️ Turn-based combat shared by several players: initiative order, `ATTACK`, `DEFEND`, `FLEE`, a time limit per turn, `STATUS`; a player reaching 0 HP respawns at the chapel with reduced health.
- 📣 Real-time events pushed to the players of a room: arrivals and departures, combat rounds, respawns, and the number of connected players.
- ♻️ Enemies and items that come back after a delay.
- 📝 Structured JSON logging with timestamps, levels, IP addresses, players and commands.
- 🚪 Clean disconnection (`QUIT`, signals) and graceful handling of malformed commands.

### 🚧 Current status and known limitations

The project is still in progress. At the time of writing:

- ✅ 9 of the 15 RFC commands are implemented: `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, `ATTACK`, `STATUS`; plus two commands of our own for the combat, `DEFEND` and `FLEE`.
- ✅ Events: `EVT ROOM PRESENCE`, `EVT STATS players=`, and our own `EVT ROOM COMBAT`, `EVT ROOM SPAWN` and `EVT ROOM EFFECT`.
- ✅ `CHAT`, `INVENTORY`, `WHO` and `GROUP` are implemented (`GROUP CREATE|INVITE|JOIN|LEAVE`: the leader invites, the invited player joins by the name of a member, 4 players at most, ids `group.<n>`).
- ❌ `QUEST` and `QUESTS` are not implemented yet.
- ⚠️ The quests are described in `game/ressources/quests.json` but are not used by the server yet.
- ⚠️ The GUI client is not connected to the server yet.

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

The server listens on TCP port **8080** by default (`SERVER_PORT` in `shared/defines.hpp`; see *Configuration* below to change it) and loads the world from `game/ressources/*.json`. It stops cleanly on `SIGINT`/`SIGTERM` (Ctrl+C). The JSON log is written while it runs (see [Server Logging](#-server-logging)).

If you move the project folder after a build, run `make -C server re`: the absolute path of the project is compiled into the server to find the world files.

### ⌨️ CLI client

```sh
make client           # build ./clients/cli/clientCLI and start it in the current terminal
```

The client connects to `127.0.0.1:8080` by default (`SERVER_IP` in `clients/defines.hpp`, `SERVER_PORT` in `shared/defines.hpp`). To use another address or port, see *Configuration* below. Start the server first.

### ⚙️ Configuration

The server and the CLI client read a few settings from the environment, with the values of `shared/defines.hpp` as defaults:

| Variable | Used by | Meaning | Default |
|---|---|---|---|
| `SERVER_PORT` | server, clients | TCP port, from 1 to 65535 | `8080` |
| `SERVER_IP` | clients | address of the server | `127.0.0.1` |
| `LOG_LEVEL` | server | lowest level of the logs: `debug`, `info`, `warn` or `error` | `debug` |
| `LOG_FILE` | server | file where the logs are also written, at its end (the console output stays) | none |
| `MAX_OUT_BUFFER` | server | bytes waiting to be sent to a client before he is disconnected (abuse limit; a value out of its bounds is ignored) | `65536` |
| `MAX_READ_PER_TURN` | server | bytes read from a client in one turn of the loop (abuse limit; a value out of its bounds is ignored) | `16384` |
| `COMMAND_WINDOW_MS` | server | length of the window in which the commands of a client are counted (abuse limit; a value out of its bounds is ignored) | `1000` |
| `MAX_COMMANDS_PER_WINDOW` | server | commands accepted from a client in one window (abuse limit; a value out of its bounds is ignored) | `20` |
| `MAX_FLOOD_WINDOWS` | server | windows in a row over the limit before the client is disconnected (abuse limit; a value out of its bounds is ignored) | `3` |
| `MAX_SESSIONS` | server | clients connected at the same time (abuse limit; a value out of its bounds is ignored) | `64` |
| `MAX_SESSIONS_PER_IP` | server | clients connected at the same time from one address (abuse limit; a value out of its bounds is ignored) | `5` |
| `CONNECT_WINDOW_MS` | server | length of the window in which the connections of an address are counted (abuse limit; a value out of its bounds is ignored) | `10000` |
| `MAX_CONNECTS_PER_WINDOW` | server | connections accepted from one address in one window (abuse limit; a value out of its bounds is ignored) | `10` |

Copy `.env.example` to `.env` at the root of the project to set them (the file is ignored by git). A variable that is already set in the shell wins over the file (`SERVER_PORT=9099 ./server/server`), and an invalid value (not a number, out of range, unknown level) is ignored and the default is used.

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
server/            the server program
  network/         sockets: Server (poll loop), Connection (buffers), User (name, state)
  protocol/        the protocol: command handlers (commands.cpp) and JSON serialization
game/              game logic, independent from the network
  world/           World, Room, JSON parser and validation, respawn queue
  entities/        Player, Npc, Item, Inventory, Stats
  combat/          rules of a fight (combat.cpp) and the manager of the fights in progress
  events/          EventBus: the events waiting to be sent
  ressources/      world data: room.json, npc.json, items.json, quests.json
  utils/lookup.hpp designation of an NPC / item by id or name
shared/            code used by the server and the clients
  defines.hpp      constants of the protocol and of the system
  protocol/        errors, success, response, event, line framing, player name rule
  logging/         logger
  text/            UTF-8 checks and JSON escaping
  system/          signalFd, random numbers
clients/           client.cpp: network engine shared by the clients
  cli/             command-line client
  gui/             Qt client
third_party/       nlohmann/json (downloaded by make install-deps)
tools/             mapviz: draws the world map as an SVG
audit/ docs/       internal audits, subject and RFC
```

The code is split in two layers that only meet in `server/protocol/commands.cpp`:

- the **network layer** (`Server`, `Connection`, `User`) knows sockets and protocol lines, but nothing about the game;
- the **game layer** (`Game`, `World`, `CombatManager`...) knows rooms, items and fights, but nothing about sockets or the protocol syntax. It never writes to a socket: it queues events, and the main loop sends them.

`commands.cpp` is the glue: it receives a command from the network layer, calls the `Game`, and turns the result into a `Response` (success or error code), which the server writes back as a protocol line.

### 🚦 Server design: dispatcher

We chose a **dispatcher** (a router) rather than inline handling. The server never interprets commands itself:

1. `Server::handeLine` splits a received line into the command name and its arguments.
2. It looks the name up in a table `command name -> handler` (`std::unordered_map<std::string, std::function<Response(User&, const std::string&)>>`).
3. The handler returns a `Response`, which is logged and sent to the client.

Handlers are registered at startup with `registerCommands` (`server.defineAction(CMD_LOOK, ...)`). Adding a command means writing one `cmdXxx` function and one registration line, without touching the network code. Commands that are not implemented yet are registered on a common stub handler, so the table already lists all 15 RFC commands.

The dispatcher also enforces the protocol state machine: until `CONNECT` has succeeded, any other command is refused with an error; an unknown command gets an error too, and the connection stays open.

Why a dispatcher: the commands share the same flow (parse, check state, act, reply, log), so this flow is written once; each handler stays short and can be read on its own; and the network layer can be tested and reused without the game.

### 🔄 Concurrency model

The server is **single-threaded and event-driven**, built on `poll(2)`. There is no thread, no mutex. The loop of `main` does three things, in this order, at each turn:

1. `server.updatePoll()`: waits for the network (at most 1 second), accepts the new clients, reads and dispatches the commands, flushes the replies.
2. `game.tick(now)`: lets the time pass in the game: the turns of a fight whose player was too slow are played, and the enemies and items due to come back are created.
3. The events queued by the game are sent to their recipients (`server.sendToArray`).

Because the replies are written during step 1 and the events during step 3, a player always receives the answer to his command **before** the events it caused.

- All sockets are **non-blocking**. Reading drains the socket into a per-client input buffer; complete lines (ending with `\n`, an optional `\r` is tolerated) are extracted from it and dispatched. Replies go to a per-client output buffer, flushed with `send(MSG_NOSIGNAL)`, so a client that disconnects in the middle of a send cannot kill the server or block the others.
- Signals (`SIGINT`, `SIGTERM`, `SIGQUIT`) are turned into a readable file descriptor with `signalfd` (`shared/system/signalFd.*`, also used by the CLI client), so the shutdown is handled in the same loop as the network events.
- A line longer than 1024 bytes (with or without newline) gets an error and the client is disconnected, which bounds the memory used per client.
- Abuse is logged (`WARN`) and limited, with the defaults in `server/defines.hpp` (each one can be set in the environment or in `.env`, see *Configuration*): more than 20 commands per second are answered `ERR 429 RATE_LIMITED` (3 such seconds in a row disconnect the client), a client whose unsent data exceeds 64 KB is disconnected, at most 16 KB are read per client and per loop turn, and a new connection is answered `ERR 503 SERVER_BUSY` and closed beyond 64 clients, 5 per address, or 10 per address in 10 seconds. The limits of commands, clients and connections are turned off with the value `0`. The per-address limits also apply to `127.0.0.1`, so a test with many local clients needs `MAX_SESSIONS_PER_IP` and `MAX_CONNECTS_PER_WINDOW` raised (or `0`). A setting that is not a number or is out of its bounds is ignored, with a `WARN` `invalid setting ignored, default used`.
- On disconnection (`QUIT`, closed socket, error), the player is removed from the game by the `onDisconnect` callback **before** the session is destroyed, as the subject requires, so that no leave event can reach a player who is already gone.

Why this model: since every command is handled one after the other in a single thread, the game state (rooms, items, NPCs, fights) is never accessed concurrently. Two players taking the same item at the same moment is solved for free: the first command processed wins and the second gets `ITEM_NOT_FOUND`. The timers of the fights and of the respawns do not need a thread either, they are checked at each turn of the loop. A MUD is I/O-bound and has few players, so one thread is largely enough, and there are no locks to get wrong.

### 🎲 Game layer

- `Game` owns the `World`, the players (`std::unordered_map<client id, Player>`), the `EventBus`, the `CombatManager` and the `RespawnQueue`. Its methods (`move`, `take`, `drop`, `talk`, `attack`, `defend`, `flee`, `status`, `tick`...) take a player id and return a plain result, not a protocol string.
- `World` loads and **validates** the JSON files at startup (exits, spawns, NPC and item references, start and respawn rooms) and refuses to start if a reference is broken. Each `Room` holds its exits, its item instances and its NPC instances.
- **Items and NPCs are instances**: the definition (from the JSON database) is separated from each living copy, which has its own id, so an item exists exactly once in the world and cannot be duplicated. `Inventory` is composed of items and used by `Player`.
- `lookup.hpp` finds an item or an NPC from what the player typed (instance id, definition id, name, or one word of the name, in any case).
- `EventBus` is a queue of `(recipients, line)`. The game and the combat manager push events into it (`toRoom`, `toOthers`, `send`) and the main loop empties it. This keeps the game independent from the sockets.
- `RespawnQueue` remembers when a dead enemy or a taken item must come back, and `tick` creates it again at the right time.
- `combat/` is split in two: `combat.cpp` holds the pure rules of a fight (damage, chances, initiative, turns) on `Stats` objects, without any network or time; `CombatManager` holds the fights in progress, the turn timers and the events. See [Combat System](#-combat-system).

### 🖱️ Clients

- `clients/client.cpp` is a small network engine shared by the clients: it connects to the server, reads the lines in a buffer and calls the handler registered for the first word of the line (`OK`, `ERR`, `EVT`), the same dispatcher idea as on the server.
- The **CLI client** waits with `poll` on the server socket, the keyboard and the signals, so it keeps displaying incoming messages and events while the user is typing. On Ctrl+C it sends `QUIT` to the server before leaving.
- The **GUI client** is built with Qt 5 (`clients/gui/`). It is organised in windows (connection, game, error) built from custom widgets (`TAPLabel`, `TAPLineEdit`, `TAPRoomView`, `TAPPlayerCountBar`...). Its network part is not connected yet.

### 💬 Client interface choice

<!-- TODO: to decide and document (subject V.3): the CLI either sends the RFC syntax as typed, or translates friendly commands into RFC packets. Today the CLI sends each typed line as is and prints the answers as `[OK]`, `[ERR]` and `[EVT]`. -->

## 📡 Protocol Implementation <!-- [S] -->

The server follows **RFC 42TAP** (`docs/protocol-rfc.html`, protocol version 1): UTF-8, TCP, one message per line terminated by `\n` (a `\r` before it is accepted and removed), greeting `OK hello proto=1` on connection, then `CONNECT <name>` before anything else. The protocol words (commands, fields, errors, events) are defined once in `shared/` (`defines.hpp`, `errors.hpp`, `success.hpp`, `event.hpp`) and are the only place where protocol text is written. Command names are case-insensitive, as the RFC asks (§4.2).

### ✅ Implemented commands

| Command | Reply | State |
| --- | --- | --- |
| `CONNECT <name>` | `OK connected` / `ERR 201 NAME_IN_USE` | done |
| `LOOK` | `OK {room, players, items, npcs}` | done |
| `MOVE <direction>` | `OK room=<id>` / `ERR 301 NO_EXIT` | done (refused during a fight) |
| `TAKE <item>` | `OK taken=<id>` / `ERR 404 ITEM_NOT_FOUND` | done (refused during a fight) |
| `DROP <item>` | `OK dropped=<id>` / `ERR 404 ITEM_NOT_IN_INVENTORY` | done |
| `TALK <npc>` | `OK <dialogue>` / `ERR 404 NPC_NOT_FOUND` | done (refused during a fight) |
| `ATTACK <npc>` | `OK {"attacker_hp","target_hp","damage","status","round","escaped","turns"}` / `ERR 404` / `ERR 405 NPC_NOT_HOSTILE` | done |
| `STATUS` | `OK {"hp","max_hp","status"}` plus the state of the fight when there is one | done |
| `QUIT` | `OK bye`, then the connection is closed | done |
| `DEFEND`, `FLEE` | same reply as `ATTACK` | done, **not in the RFC** (our combat commands) |
| `QUEST`, `QUESTS` | registered, but answer a placeholder `OK Message received` | **not implemented yet** |

### 📣 Implemented events

All events are sent to the players of the room concerned, except `STATS`.

| Event | Sent when |
| --- | --- |
| `EVT ROOM PRESENCE ENTER <player>` / `LEAVE <player>` | a player connects, moves, dies, flees, quits or disconnects |
| `EVT STATS players=<n>` | a player connects or disconnects (sent to everybody) |
| `EVT ROOM COMBAT START <player> <enemy>` | a fight begins |
| `EVT ROOM COMBAT JOIN <player> <enemy>` | another player joins a fight |
| `EVT ROOM COMBAT ROUND <actor> <enemy> <round> <action> <target> <result>` | a turn was played; the result is the damage, or `miss`, `failed` or `escaped` |
| `EVT ROOM COMBAT TURN <player> <enemy> <seconds>` | it is the turn of a player, who has this many seconds to play |
| `EVT ROOM COMBAT TIMEOUT <player> <enemy>` | a turn was played for a player who was too slow |
| `EVT ROOM COMBAT END <player> <enemy> <victory\|defeat\|escaped>` | a player leaves the fight |
| `EVT ROOM SPAWN <npc\|item> <id>` | an enemy or an item came back |
| `EVT ROOM EFFECT healing <hp>` | sent only to the player healed by his room, with the hit points he got (not sent at full health) |

The player who sent a command does not receive the events about his own action (he has the answer); the other players of the room do. `EVT GLOBAL|ROOM CHAT` and `EVT GROUP INVITE|JOIN|LEAVE|CHAT` are sent; chat messages are also sent back to the sender.

### ↔️ Deviations from RFC 42TAP

The subject asks us to document every deviation and to justify it.

| # | Deviation | Justification |
| --- | --- | --- |
| 1b | **Extra error `ERR 500 INTERNAL_ERROR`**, sent when the server fails while handling a command (the error is logged and the server keeps running). | The RFC defines no code for a server failure; 500 is the usual "server error" class. |
| 1c | **Extra errors `ERR 429 RATE_LIMITED`** (more than 20 commands per second) **and `ERR 503 SERVER_BUSY`** (connection refused, sent in place of the greeting `OK hello`, then the socket is closed). | The subject asks to monitor command flooding and rapid connections (§V.1) but the RFC has no code for them. Reusing `400` would hide the reason from the client. 429 and 503 are the usual HTTP classes for "slow down" and "server cannot accept you". |
| 1 | **Extra error `ERR 400 BAD_REQUEST`**, sent for an unknown command, a malformed command (for example a name with a space in `CONNECT`), any command before `CONNECT`, and a line over 1024 bytes (the client is then disconnected). | The RFC asks for "appropriate error responses" to malformed messages (§9.3) but defines no code for them. We use 400, the usual "bad request" class, so that a client can tell a protocol error from a game error (2xx-4xx codes of the RFC). |
| 2 | **Extra error `ERR 403 INVENTORY_FULL`** for `TAKE` when the inventory holds 10 items. | The RFC recommends limits on "resource acquisition per player" (§9.4) but defines no error for it. |
| 3 | **Extra errors for the combat**: `409 IN_COMBAT` (`MOVE`, `TAKE`, `TALK` during a fight, or `ATTACK` on another enemy), `410 NOT_IN_COMBAT` (`DEFEND` or `FLEE` with no fight), `411 NOT_YOUR_TURN`. | The turn-based combat requested by the subject needs them, and the RFC leaves the combat to the implementers (§6.1.1). The codes continue after the 40x family of the RFC. |
| 4 | **Extra commands `DEFEND` and `FLEE`**, and extra events `COMBAT` and `SPAWN`. | The subject asks for additional combat commands ("DEFEND, FLEE, etc."), and the combat is broadcast to the players concerned (§V.1). |
| 5 | **Richer replies for `ATTACK` and `STATUS`**: the fields of the RFC (`attacker_hp`, `target_hp`, `damage`, `status`; `hp`, `max_hp`, `status`) are always there and in the same place, with extra fields added (`round`, `escaped`, `turns`; `enemy`, `turn`, `your_turn`, `turn_seconds_left`). | A turn-based fight plays several turns for one command (yours, then the enemy's), and the client needs to know them. A client of another group that only knows the RFC fields still works, because extra JSON fields can be ignored. |
| 6 | **Instance ids are visible on the wire**: `LOOK` returns `"items":["item.herbs#3"]`, `"npcs":["npc.guard#10"]`, and `TAKE` answers `OK taken=item.herbs#3` (the RFC examples show `item.herbs`). | The subject requires items to be unique instances (no duplication). Two herbs in the same room need two different ids, so the instance counter is part of the id. The definition id (`item.herbs`), the name (`Healing Herbs`) and a word of the name are still accepted in commands, so a client that does not know about `#N` keeps working (`TAKE herbs`). A client must treat ids as opaque strings. |
| 7 | **Player names** are limited to 32 bytes, must be valid UTF-8 without control characters, and cannot contain a space. | The RFC only requires uniqueness (§3.3) and correct handling of Unicode (§9.2). Without a limit and without spaces, a name cannot break the parsing of the lines that contain it, such as `EVT ROOM PRESENCE ENTER <name>`. |
| 8 | **Placeholder answers** `OK Message received` for the 2 commands not yet implemented (`QUEST`, `QUESTS`). | Temporary state of the development, not a choice. These lines will disappear when the commands are done. |

### 🚨 Error handling

- Every error line is `ERR <code> <NAME>` and never contains free text, so it can be parsed by a client of another group.
- Errors never close the connection, except the line that is too long and `QUIT`. A client can keep playing after any other error.
- A client that disconnects in the middle of a send (broken pipe, reset) is detected on the next read or write and cleaned up; the other players are not affected (`MSG_NOSIGNAL`, non-blocking sockets).
- Messages split in several TCP packets are buffered until the `\n` arrives, and several commands in one packet are processed one by one (RFC §9.2).

## ⚔️ Combat System <!-- [S] -->

The combat is turn-based and can be shared: all the players of a room who attack the same enemy fight together. The rules are in `game/combat/combat.cpp` (without network or clock, so they can be tested alone); the fights in progress, the timers and the events are handled by `CombatManager`.

### ❤️ Rules (HP, respawn)

- A player has **100 HP** and these stats: attack 10, defense 10, hit 10, flee 10. An enemy has its own stats in `game/ressources/npc.json` (for example the wolf: 15 HP, attack 6, defense 2, hit 8, flee 10).
- `ATTACK <enemy>` starts a fight with an enemy of the room, or joins the fight already started against it. Only NPCs with the role `enemy` can be attacked (`ERR 405` otherwise).
- During a fight the player cannot `MOVE`, `TAKE` or `TALK` (`ERR 409 IN_COMBAT`), nor attack another enemy.
- A fight ends when the enemy dies, when all the players are dead or gone. A player at 0 HP loses the fight and **respawns in the chapel with 50 HP** (`respawn` in `room.json`). Enemies and items come back after `respawn_seconds` (60 for the enemies of the world), announced by `EVT ROOM SPAWN`.
- A room can have an `effect` in `room.json`, like `{ "type": "healing", "amount": 10, "interval_seconds": 5 }` (the chapel): every `interval_seconds`, at the rhythm of the room, each player in it gets `amount` hit points back, up to his maximum. A player in a fight is not healed.
- The `status` of a player is `healthy`, `wounded` (below his maximum), `combat` or `dead`.

### 🧮 Damage formula

- **Damage** of a strike that hits: `attack × 50 / (50 + defense)`, rounded down, at least 1. A defense of 50 halves the damage.
- **Chance to hit**: `hit / (hit + 0.2 × flee of the target)`. A fast target is harder to hit, but never impossible to hit.
- A defending player receives **half** of the damage (rounded up).
- **Chance to escape** with `FLEE`: `flee / (flee + hit of the enemy)`.

### 🎲 Initiative order

A fight is a succession of **rounds**. At the beginning of each round, every fighter (players and enemy) is sorted by **initiative**, the highest plays first: it is the `flee` stat, lowered by the wounds (100% of it at full HP, down to 50% with almost no HP). The order is computed again at each round, so a wounded fighter can lose his place. A player who joins a fight in progress plays at the end of the current round.

The enemy attacks a player chosen at random among the participants. The enemy turns are played immediately by the server, right after the turn of the player: the answer to the command of a player contains his turn and every turn that followed until the next player has to play (`turns` in the reply).

### 🕹️ Additional commands (DEFEND, FLEE)

| Command | Effect |
| --- | --- |
| `ATTACK` | Strike the enemy (the target can be omitted during a fight). |
| `DEFEND` | Take half damage until the next turn of the player. |
| `FLEE` | Try to run away: on success the player leaves the fight and is moved through a random exit of the room; on failure the turn is lost. |
| `STATUS` | HP, status, and during a fight: round, enemy, whose turn it is, and the seconds left. |

Playing out of turn gives `ERR 411 NOT_YOUR_TURN`; `DEFEND` and `FLEE` outside a fight give `ERR 410 NOT_IN_COMBAT`. A player who does not play within **20 seconds** has a `DEFEND` played for him (`EVT ROOM COMBAT TIMEOUT`), so one idle player cannot block the others.

### 📣 Broadcast of combat results

Every step of a fight is sent as an `EVT ROOM COMBAT ...` event to the other players of the room (see [Implemented events](#-implemented-events)) and written in the server log (`combat started`, `player joined a combat`, deaths, respawns). The player who played has all the turns in his reply.

### 💡 Design justification

- **Same formula for players and enemies**: one set of stats (`attack`, `defense`, `hit`, `flee`) describes everybody, so a new enemy only needs numbers in a JSON file.
- **Defense as a ratio** (`attack × 50 / (50 + defense)`) instead of a subtraction: damage never falls to zero and never goes negative, whatever the stats, and the progression stays smooth.
- **Initiative from `flee`, lowered by the wounds**: no extra stat, and being hurt has a visible cost.
- **Turn timer**: in a shared fight, a player who leaves his keyboard would freeze the others. A forced `DEFEND` is the least harmful action.
- **Rules separated from the manager**: the rules take a function to roll the dice, so a test can replace the randomness and check exact results.

## 📜 Quest System <!-- [S] -->

### Quest data format
### Progression mechanics
### Completion validation
### Rewards
### Quest list

## 🗺️ World Design <!-- [S] -->

The world is a small fantasy region around a village: a quiet central square, a village with a tavern and a chapel, a forest with a spider cave to the south, and a plain leading west to goblins and an orc lord. The safe places are in the north and east, the danger grows towards the south and the west.

### 📄 World data format

The world is described by four JSON files in `game/ressources/`, read by the server at startup. Definitions and places are separated: a room only *refers* to definitions by id.

| File | Content |
| --- | --- |
| `room.json` | the `start_room`, the `respawn` point (room and HP of a dead player), and the `rooms`: name, description, `exits` (`north`/`east`/`south`/`west` to a room id), `items` (item id + `count`) and `spawns` (NPC id + `count` + optional `respawn_seconds`). |
| `npc.json` | NPC definitions: name, description, `role` (`dialogue_npc`, `quest_giver`, `enemy`), `dialogue` (lines said in turn by `TALK`), and for the enemies `stats` (`hp`, `attack`, `defense`, `hit`, `flee`) and `loot`. |
| `items.json` | item definitions: name, description, `obtainable`. |
| `quests.json` | quest definitions (see [Quest System](#-quest-system)). |

Each spawn creates `count` **instances** of the definition (`npc.rats#14`, `npc.rats#15`...), each with its own id and life. We chose JSON because the subject allows it and because the same library (nlohmann/json) is used for the replies of the protocol.

### 🧭 Map and room connections

There are **15 rooms**. Every exit has a return exit in the opposite direction (checked on the current data), so a player can always come back.

```
                                                   [chapel]
                                                      |
 [orc boss]-[orc village]-[goblin camp]-[plain]---[village]---[tavern]
                                          |          |
                                       [plain2]----[square]---[circus]
                                          |          |
                                    [forest edge]-[forest]
                                                     |
                                                [deep forest]-[spider cave 1]-[spider cave 2]
```

The connections in words:

- **Hub**: `square` (start) is connected to `village` (north), `forest` (south), `circus` (east) and `plain2` (west).
- **Village branch**: `village` leads to the `chapel` (north, where dead players respawn), the `tavern` (east) and the `plain` (west).
- **Two loops** (the subject asks for at least one):
  - `square` → `village` → `plain` → `plain2` → `square`;
  - `square` → `forest` → `forest_edge` → `plain2` → `square`.
- **Dead ends** (optional branches): `circus`, `tavern`, `chapel`.
- **West branch** (goblins and orcs): `plain` → `goblin_camp` → `orc_village` → `orc_boss_house`.
- **South branch** (the cave): `forest` → `deep_forest` → `spider_cave01` → `spider_cave02`.

So a player can walk a full circuit without going back on his steps, and the map is not a simple line.

### 🧙 NPC roles

There are 15 NPC definitions, in three roles (the subject asks for at least three distinct ones):

| Role | NPCs | Behaviour |
| --- | --- | --- |
| **Dialogue** | Priestess Elara (chapel), Bard Lyric (tavern), Citizen (village) | `TALK` only: tells the player about the world. |
| **Quest giver** | Guard Hector (square), Tavern Keeper Bronn (tavern), the Dead Acrobat (circus) | `TALK` for the story; they give the three quests. |
| **Enemy** | Baby Wolf, Rats, Wolf, Great Deer, Spider, Goblin, Orc (small and medium) · Queen Spider, Orc Lord Gorak, Dark Lord Nub-Shegorath (bosses) | Can be attacked, fight back, come back after 60 seconds. |

Enemies are placed by **difficulty zone**, with growing stats (HP / attack): Baby Wolf 5/2 and Rats 10/4 near the village, Wolf 15/6 and Great Deer 20/8 in the forest, Goblins 20/8 and Orcs 30/10 on the plain, then the bosses: Queen Spider 50/15 at the end of the cave and Orc Lord 100/20 at the end of the west branch. A new player (100 HP, attack 10) can beat the first ones alone, and the bosses call for several players, which is the point of a shared world. The Dark Lord (200 HP) is defined but not placed in any room yet.

### 🎒 Items and distribution

There are 8 item definitions, 7 of them obtainable (the subject asks for at least 4, with at least 2 obtainable):

| Item | Where | Use |
| --- | --- | --- |
| Healing Herbs | `square` | object of the quest *Medicine for the Chapel* |
| Loaf of Bread | `tavern` | reward of a quest |
| Silver Bell | `goblin_camp` | object of the quest *The Silent Bell* |
| Blue Ribbon Charm | reward only | reward of a quest |
| Spider Silk, Great Deer Antler, Crown of Nails | loot of the Spider, the Great Deer and the Orc Lord | trophies |
| Stone Fountain | `square` | decoration, **not obtainable** (`TAKE` is refused) |

The objects lie on the floor of their room at the start, and the loot of an enemy falls on the floor of the room where it dies. A player carries at most **10** items. Taking an item removes it from the room and dropping it puts it back there, where another player can take it. A spawn can give an object a `respawn_seconds` so that it comes back after being taken; no item of the current world uses it yet.

### ✅ World validation

At startup, `game/world/parser.cpp` checks the files and **refuses to start** with a message naming the file and the room if something is wrong:

- the files exist and are valid JSON;
- every exit has a known direction and leads to an **existing room**;
- every spawn and every item refers to an **existing NPC or item** definition;
- `start_room` and the `respawn` room exist;
- enemy stats are valid (`hp` > 0, other values ≥ 0), `count` is between 1 and the maximum, `respawn_seconds` is not negative.

## 📝 Server Logging <!-- [S] -->

### Log format

One JSON object per line (`std::endl` flushes each line), built by hand in `shared/logging/logger.cpp`:

```json
{"ts":"2026-10-21T11:57:56.213+00:00","level":"INFO","msg":"world loaded","rooms":15}
{"ts":"...","level":"WARN","msg":"command flooding","src":"server/network/server.cpp:171","client":3,"ip":"127.0.0.1","windows":1}
```

- `ts`: local date and time with milliseconds and the UTC offset; `level`; `msg`: a short fixed sentence.
- `src` (file and line) is added from `WARN` upward.
- The other keys are the context of the event (`client` id, `player`, `ip`, `room`, ...). Texts are escaped, so a line is always valid JSON.

### Log levels and event types

| Level | Used for |
|-------|----------|
| `DEBUG` | details: sockets, actions defined, messages sent, chat text |
| `INFO` | normal life: connections, commands and responses, moves, fights, groups, respawns |
| `WARN` | abnormal but handled: unknown command, command before `CONNECT`, bad request, abuse, a client that does not read |
| `ERROR` | failures of the server: an exception in a command or in a turn of the loop, failed system calls, inconsistent data |

`LOG_LEVEL` sets the lowest level written (default `debug`).

### Output destinations

- `DEBUG` and `INFO` go to the standard output, `WARN` and `ERROR` to the standard error (red or yellow when it is a terminal; `NO_COLOR` disables the colours).
- `LOG_FILE` (in the environment or `.env`) also writes every line, without colours, at the end of a file. If the file cannot be opened, a `WARN` is logged and the server goes on with the console only.

### Monitoring the server

The logs are JSON lines, so they can be followed and filtered with ordinary tools:

```sh
LOG_FILE=server.log make -C server          # also write server.log
tail -f server.log | grep '"level":"WARN"'  # warnings only
grep '"msg":"player took an item"' server.log
```

Each connection has a `client` id, so all the lines of one player can be found with `grep '"client":3'`.

### Abuse detection (command flooding, rapid connections)

Abuse is logged at `WARN` level and limited (thresholds in `server/defines.hpp`):

| Message | Meaning |
|---------|---------|
| `command flooding` | a client sent more than 20 commands in one second (one line per second; the extra commands get `ERR 429 RATE_LIMITED`) |
| `flooding goes on, disconnecting` | 3 seconds in a row over the limit: the client is disconnected |
| `client does not read, disconnecting` | more than 64 KB waiting to be sent to a client |
| `line too long, disconnecting` | a line over 1024 bytes |
| `too many clients, refusing a connection` / `... for one address ...` (the client gets `ERR 503 SERVER_BUSY`) | 64 clients in all, or 5 from one address |
| `connections too fast, refusing a connection` (`ERR 503 SERVER_BUSY` too) | more than 10 connections from one address in 10 seconds |
| `command before CONNECT`, `unknown command` | commands that are not allowed or do not exist |

### Performance considerations

- The server is single-threaded (`poll`), so a log line is written in the loop: with `LOG_LEVEL=debug` every command and every message is logged, which is useful to develop but is more output than needed to run. `info` is a good level for a running server.
- The level is checked before the line is built, so a disabled level costs almost nothing.
- Every line is flushed at once, so no log is lost if the server stops.

## 🧪 Testing <!-- [S] -->

### Multiplayer
### Combat
### Quests
### Protocol compliance and edge cases
### 🩸 Leaks report <!-- [+] -->

The GUI client ships a memory-check target:

```sh
make -C clients/gui valgrind
```

It runs the executable under Valgrind with `clients/gui/valgrind-suppressions.txt`, which hides the allocations of the system and third-party libraries (Qt, glib/GTK platform theme, OpenGL/glvnd, glibc's dynamic loader, ...) — these are intentionally alive at exit or leaked inside the libraries, and are not our responsibility. Only leaks caused by our own code are reported, and the target fails (`--error-exitcode=1`) if any remain.

Current result on Linux/Qt 5.15: **0 errors — no definitely, indirectly or possibly lost bytes**. The only remaining memory is "still reachable" (globals and library caches, alive by design at exit), which Valgrind does not count as a leak.

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
