*This project has been created as part of the 42 curriculum by kebertra and gtourdiat.*

# 🗡️ 42-TAP

<!-- Section legend: [S] = required by the subject (chapter VI) or by the RFC rules
     (chapter IV), [+] = addition of the team. -->

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

### 🚧 Current status

The project is feature-complete for the mandatory part of the subject:

- ✅ **All 15 RFC commands** are implemented: `CONNECT`, `LOOK`, `MOVE`, `CHAT`, `TAKE`, `DROP`, `INVENTORY`, `TALK`, `ATTACK`, `STATUS`, `QUEST`, `QUESTS`, `WHO`, `GROUP`, `QUIT`; plus three commands of our own: `DEFEND`, `FLEE` (combat) and `USE` (consuming an item).
- ✅ Every RFC event is sent: `EVT ROOM PRESENCE ENTER|LEAVE`, `EVT ROOM|GLOBAL|GROUP CHAT`, `EVT GROUP INVITE|JOIN|LEAVE`, `EVT STATS players=`; plus our own `EVT ROOM COMBAT …`, `EVT ROOM ITEM TAKEN|DROPPED`, `EVT ROOM SPAWN`, `EVT ROOM EFFECT`, `EVT ROOM DEATH`, `EVT INVENTORY CHANGED`.
- ✅ `GROUP CREATE|INVITE|JOIN|LEAVE`: the leader invites, the invited player joins by the name of a member, 4 players at most, ids `group.<n>`.
- ✅ The three quests of `game/ressources/quests.json` can be played (see [Quest System](#-quest-system)).
- ✅ **Both clients are connected to the server.** The GUI reads a local copy of the state (`clients/state/`) filled by the answers and the events, and redraws only what changed.

Known limitations at this time (none of them breaks a mandatory requirement):

- The GUI has no dedicated dialogue area: what an NPC says through `TALK` is written in the log view, not next to the NPC list.
- The GUI asks for objects and NPCs by selecting an entry in a list; it has no free-text field accepting a display name (the server accepts both).
- The GUI does not react to a lost connection yet: the game window stays open as it is, without a message.

## 🧭 Instructions <!-- [S] -->

### Requirements

- A C++20 compiler (`c++`, g++ or clang++), GNU `make`, and `curl` or `wget`.
- For the GUI only: **CMake** and **Qt 5 Widgets**.
- Linux or macOS. The server and the clients use `poll(2)` and `signalfd(2)`, so they do not build on Windows.

### Quick start

```sh
make install-deps     # download and verify nlohmann/json
make build            # build the server and the CLI client
make run-server       # start the server in a new terminal window (port 8080)
make run-client       # start the CLI client in another terminal window
make run-client-gui   # build and start the Qt client (needs CMake + Qt 5)
```

Then type `CONNECT <name>` in the CLI client, or fill the connection window in the GUI. The full build matrix, the configuration variables and the troubleshooting notes are in [Building and Running](#-building-and-running).

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
make run-server       # build ./server/server, then start it in a new terminal window (alias: make server)
make -C server        # build it and start it in the current terminal
./server/server       # start it again later
```

The server listens on TCP port **8080** by default (`SERVER_PORT` in `shared/defines.hpp`; see *Configuration* below to change it) and loads the world from `game/ressources/*.json`. It stops cleanly on `SIGINT`/`SIGTERM` (Ctrl+C). The JSON log is written while it runs (see [Server Logging](#-server-logging)).

If you move the project folder after a build, run `make -C server re`: the absolute path of the project is compiled into the server to find the world files.

### ⌨️ CLI client

```sh
make run-client       # build ./clients/cli/clientCLI and start it in a new terminal window (alias: make cli)
make -C clients/cli   # build it and start it in the current terminal
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
make run-client-gui   # configure with CMake, build with Qt 5, start ./clients/gui/42TAP (alias: make gui)
```

The address and the port of the server are typed in the connection window.

### 🚀 Everything at once

```sh
make                  # install the dependencies and build the server and the CLI client (nothing is started)
make run              # build, then open the server and the CLI client, each one in its own terminal window
```

### 🧹 Other targets

| Target | Effect |
| --- | --- |
| `make install-deps` | Download and verify the third-party JSON library |
| `make build` | Build the server and the CLI client without starting them |
| `make build-gui` | Build the GUI client (needs CMake and Qt 5) |
| `make run` | Build, then start the server and the CLI client in two terminal windows |
| `make run-server`, `make run-client`, `make run-client-gui` | Build and start one component (the first two in a new terminal window); the aliases are `server`, `cli` / `client` and `gui` |
| `make lint` | Static checks, see below |
| `make clean` | Remove object files and the GUI `build/` folder |
| `make fclean` | `clean`, plus remove the executables |
| `make re` | `fclean`, then `all` |

**Lint.** `make lint` needs nothing but the compiler. It checks, without building anything:

1. every source of the server, the shared code, the game and the CLI client (the Qt GUI is left out, it needs the Qt headers) with strict warnings, all treated as errors: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wcast-align -Wnull-dereference -Wdouble-promotion -Wformat=2`. They are stricter than the build flags and catch hidden conversions and shadowed names;
2. that no line of the C++ sources ends with a space or a tab;
3. `cppcheck` and `clang-tidy`, only if they are installed (otherwise they are skipped with a message).

The command stops with an error status if one check fails.

**Speed.** The server and the CLI client compile their files in parallel (one job per core; `make JOBS=1` to see the errors one by one) and only recompile what depends on a header that changed (the compiler writes the dependencies, `-MMD -MP`).

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
  state/           Session + GameState: the local copy of the game, without Qt or sockets
third_party/       nlohmann/json (downloaded by make install-deps)
audit/ docs/       internal audits, subject, RFC and design notes
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
- The **GUI client** is built with Qt 5 (`clients/gui/`). It is organised in windows (connection, game, error) built from custom widgets (`TAPLabel`, `TAPLineEdit`, `TAPRoomView`, `TAPPlayerCountBar`...). It does not parse protocol lines: `clients/state/Session` turns the answers and the events into a `GameState`, and the widgets redraw the sections that changed (`Session::onChanged(Section)`). The socket is polled from the Qt event loop by a 50 ms `QTimer` calling `Client::update(0)`, so the window never blocks and the events are displayed while the player is typing.

### 💬 Client interface choice

The subject (V.3) lets us choose between sending the RFC syntax as typed and translating friendly commands into RFC packets. **We chose the first approach: the CLI client sends each line exactly as the player typed it**, and prints the answers as `[OK] …`, `[ERR] …` and `[EVT] …`.

Why:

- The clients must be interchangeable between groups, and the protocol is the only contract. A client that translates commands hides the actual protocol traffic, which makes it impossible to check against the RFC during a peer review, and it silently breaks as soon as another group's server differs on a detail.
- The server already accepts case-insensitive command names, the definition id, the instance id, the display name and any single word of a multi-word name (`TAKE herbs`, `TAKE healing herbs`, `TAKE item.herbs`). A translation layer would duplicate a tolerance the server already provides.
- The GUI covers the "friendly interface" need: it builds the protocol lines itself from the widgets, so nothing has to be typed there.

The GUI, for its part, does translate: a click on an exit button becomes `MOVE north`, a selection becomes `TAKE <id>`. This is the same split the subject describes — the CLI exposes the protocol, the GUI hides it.

## 📡 Protocol Implementation <!-- [S] -->

The server follows **RFC 42TAP** (`docs/protocol-rfc.html`, protocol version 1): UTF-8, TCP, one message per line terminated by `\n` (a `\r` before it is accepted and removed), greeting `OK hello proto=1` on connection, then `CONNECT <name>` before anything else. The protocol words (commands, fields, errors, events) are defined once in `shared/` (`defines.hpp`, `errors.hpp`, `success.hpp`, `event.hpp`) and are the only place where protocol text is written. Command names are case-insensitive, as the RFC asks (§4.2).

### ✅ Implemented commands

| Command | Reply | State |
| --- | --- | --- |
| `CONNECT <name>` | `OK connected` / `ERR 201 NAME_IN_USE` | done |
| `LOOK` | `OK {room, players, items, npcs}` | done |
| `MOVE <direction>` | `OK room=<id>` / `ERR 301 NO_EXIT` | done (refused during a fight) |
| `TAKE <item>` | `OK taken=<id>` / `ERR 404 ITEM_NOT_FOUND` / `ERR 408 ITEM_NOT_TAKEABLE` | done (refused during a fight) |
| `DROP <item>` | `OK dropped=<id>` / `ERR 404 ITEM_NOT_IN_INVENTORY` | done |
| `USE <item>` | `OK healed=<hit points>` / `ERR 404 ITEM_NOT_IN_INVENTORY` / `ERR 407 ITEM_NOT_USABLE` | done (refused during a fight), **not in the RFC** |
| `TALK <npc>` | `OK <dialogue>` / `ERR 404 NPC_NOT_FOUND` | done (refused during a fight) |
| `ATTACK <npc>` | `OK {"attacker_hp","target_hp","damage","status","round","escaped","turns"}` / `ERR 404` / `ERR 405 NPC_NOT_HOSTILE` | done |
| `STATUS` | `OK {"hp","max_hp","status"}` plus the state of the fight when there is one | done |
| `QUIT` | `OK bye`, then the connection is closed | done |
| `DEFEND`, `FLEE` | same reply as `ATTACK` | done, **not in the RFC** (our combat commands) |
| `QUEST <npc>` | `OK {"quest_id","name","status","description","progress","reward","reward_items","reward_heal"}` plus `"text_done"` when the quest is handed in / `ERR 404 NPC_NOT_FOUND` / `ERR 406 NO_QUEST_AVAILABLE` / `ERR 403 INVENTORY_FULL` | done (refused during a fight) |
| `QUESTS` | `OK [{"quest_id","name","status","progress"}]` | done |

### 📣 Implemented events

All events are sent to the players of the room concerned, except `STATS`.

| Event | Sent when |
| --- | --- |
| `EVT ROOM PRESENCE ENTER <player>` / `LEAVE <player>` | a player connects, moves, dies, flees, quits or disconnects |
| `EVT ROOM DEATH <respawn room id>` | a player died and respawned in that room. Sent **after** the respawn, to **every** player of the room he woke up in, the dead player included: it is the only event that tells a player he died (the `COMBAT END ... defeat` that precedes it skips him). The client asks `LOOK`, `STATUS` and `INVENTORY` again |
| `EVT STATS players=<n>` | a player connects or disconnects (sent to everybody) |
| `EVT ROOM COMBAT START <player> <enemy>` | a fight begins |
| `EVT ROOM COMBAT JOIN <player> <enemy>` | another player joins a fight |
| `EVT ROOM COMBAT ROUND <actor> <enemy> <round> <action> <target> <result>` | a turn was played; the result is the damage, or `miss`, `failed` or `escaped` |
| `EVT ROOM COMBAT TURN <player> <enemy> <seconds>` | it is the turn of a player, who has this many seconds to play |
| `EVT ROOM COMBAT TIMEOUT <player> <enemy>` | a turn was played for a player who was too slow |
| `EVT ROOM COMBAT END <player> <enemy> <victory\|defeat\|escaped>` | a player leaves the fight |
| `EVT ROOM COMBAT RESOLVED` | the fight is settled for the whole room: the enemy is gone (killed or nobody left to fight). Sent to **every** player of the room, the one who played included, so each client asks `LOOK` again. It names no player and no enemy: it only says the room changed, and it carries no answer to any command |
| `EVT ROOM SPAWN <npc\|item> <definition id>` | an enemy or an item came back |
| `EVT ROOM ITEM TAKEN <player> <definition id>` | a player of the room took an item from the floor |
| `EVT ROOM ITEM DROPPED <player> <definition id>` | a player of the room dropped an item on the floor |
| `EVT ROOM EFFECT healing <hp>` | sent only to the player healed by his room, with the hit points he got (not sent at full health) |
| `EVT INVENTORY CHANGED <quest\|use>` | sent only to the player whose inventory changed without a `TAKE` or `DROP`: a quest he handed in took or gave items, or `USE` consumed one. The client should send `INVENTORY` again (a quest that gives hit points also sends `EVT ROOM EFFECT healing <hp>`) |

The player who sent a command does not receive the events about his own action (he has the answer); the other players of the room do. `EVT GLOBAL|ROOM CHAT` and `EVT GROUP INVITE|JOIN|LEAVE|CHAT` are sent; chat messages are also sent back to the sender.

`EVT ROOM COMBAT RESOLVED` and `EVT ROOM DEATH` are the exceptions: they answer no command and describe no action of one player, so they go to the whole room. Without `RESOLVED` the player who killed an enemy would never learn that the enemy left the room and dropped its loot; without `DEATH` a player would never learn that he died, because the `END ... defeat` skips him and his `ATTACK` answer says nothing about the respawn. Both are sent after the state they describe is final (loot on the floor, respawn done) so that the `LOOK` they trigger finds the room already settled.

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
| 6 | **Instance ids only in the combat events.** Every other message uses the **definition id**, as the RFC examples do: `LOOK` returns `"items":["item.herbs"]` and `"npcs":["npc.guard"]` (an id appears twice if two such objects are there), `TAKE` answers `OK taken=item.herbs`, `INVENTORY` and `SPAWN` use `item.herbs` too. The `COMBAT` events and `STATUS` name the enemy by its instance id (`npc.rats#14`). | The subject requires objects to be unique instances, so the server keeps an instance id (`item.herbs#3`) for each of them, but a client of another group expects `item.herbs`. Items have no state of their own, so two herbs are interchangeable and the definition id is enough. Two rats in a room are two different fights, so the combat events keep the instance id (the RFC defines no combat event). The definition id, the instance id, the name (`Healing Herbs`) and a word of the name are all accepted in commands (`TAKE herbs`, `ATTACK npc.rats`). |
| 7 | **Player names** are limited to 32 bytes, must be valid UTF-8 without control characters, and cannot contain a space. | The RFC only requires uniqueness (§3.3) and correct handling of Unicode (§9.2). Without a limit and without spaces, a name cannot break the parsing of the lines that contain it, such as `EVT ROOM PRESENCE ENTER <name>`. |
| 8 | **`QUEST` is also the way to hand a quest in.** Its answer keeps the fields of the RFC (`quest_id`, `description`, `reward` as a text such as `"item.bread,heal=20"`, `status`) and adds `name`, `progress`, `reward_items`, `reward_heal` and, once the quest is handed in, `text_done`. The status is `active` at once: `QUEST` gives the quest (the RFC example shows `available`). | The RFC leaves the quest design to us (§6.1.2). We keep the type of every field of its example so that a client of another group can read the answer; the structured reward and the extra fields can be ignored by a client that does not know them. The quest is accepted by the first `QUEST` to avoid a second command. |
| 9 | **`WHO` follows the RFC**: `OK players=<count>` of the players of the server. | The subject (page 12) shows `OK { "room": [...], "server": 5 }`, which contradicts RFC §5.2.2 (`OK players=<n>`, "Query total player count"). The clients must follow the RFC, and the same `players=` is in the `STATS` event. The names of the players of the room are in `LOOK`. |
| 10 | **Extra errors for `GROUP`**: `404 PLAYER_NOT_FOUND` (nobody has this name; same code as `NPC_NOT_FOUND`), `412 NO_INVITATION` (`JOIN` without an invitation), `413 NOT_GROUP_LEADER` (`INVITE` by a member who is not the leader), `414 GROUP_FULL` (4 players at most). Inviting yourself answers `402 ALREADY_IN_GROUP`; `400` is kept for an unknown sub-command or a missing name. | The RFC only has `401` and `402` for groups. A client has to tell these cases apart, and `4xx` is the class of the errors caused by the player. `412` to `414` follow the combat errors (`409` to `411`), the other extra codes of the 4xx class. |
| 11 | **Extra command `USE <item>`** and extra error `ERR 407 ITEM_NOT_USABLE`. An item with an `effect` in `items.json` (healing herbs, bread) is consumed and gives its hit points back, up to the maximum; the answer tells how many were really given. | The RFC has no way to use an item, but items with an effect would be useless without it. The subject allows additional commands. |
| 12 | **Extra error `ERR 408 ITEM_NOT_TAKEABLE`**: `TAKE` on an object listed by `LOOK` whose definition has `"obtainable": false` (the Stone Fountain of the square). It stays in `items`, because it is there and the room description mentions it, but it can never enter an inventory. | `LOOK` and `TAKE` must not contradict each other. Without this code, a takeable-looking item answered `404 ITEM_NOT_FOUND`, which in the RFC means "the item is not in this room" — false, and a client following the usual rule (`404` on `TAKE` → `LOOK` again) would refresh in a loop. The code is distinct from `404`, so the client logs it and leaves the room as it is. |

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

The quests are described in `game/ressources/quests.json` and checked at startup (the server refuses to start if they are not valid):

```json
"quest.cellar_pests": {
    "name": "Cellar Pests",
    "type": "defeat",
    "giver": "npc.tavern_keeper",
    "turn_in": "npc.tavern_keeper",
    "target": "npc.rats",
    "count": 3,
    "reward": { "items": [ "item.bread" ], "heal": 20 },
    "text_start": "...",
    "text_done": "..."
}
```

`giver` gives the quest and `turn_in` takes the result (they can be the same npc). `type` is `deliver`, `fetch` or `defeat`; `target` is an item that can be taken (`deliver`, `fetch`) or an enemy (`defeat`), and `count` how many. The server checks that the npcs, the target and the reward items exist, that `count` is at least 1 and that `heal` is not negative.

In the code, a quest is a `QuestDef` (a `Definition`, like npcs and items, stored in a `Database`), and what a player did is a `QuestProgress`, kept by the `QuestManager` of the `Game`.

### Progression mechanics

- `QUEST <npc>` is the only command needed. Said to a `giver` who has a quest the player never had, it gives the quest (`status: active`). Said to the `turn_in` of a quest in progress, it hands it in.
- `deliver` and `fetch` work the same way: bring `count` items `target` to the `turn_in` npc. Nothing is recorded while the player plays: the progress is the number of such items he **carries** when he asks (`QUESTS` shows `1/3`). If he drops them, they do not count any more.
- `defeat`: every enemy of the definition `target` that dies counts for **all the players of the fight** who have the quest in progress. Only the enemies defeated after the quest was given count.
- A quest can be done once; asking again gives `ERR 406 NO_QUEST_AVAILABLE`. The quests of a player are forgotten when he disconnects (nothing is saved).
- `QUEST` is refused during a fight (`ERR 409`); `QUESTS` is allowed.

### Completion validation

The hand-in is **manual**: the player goes to the `turn_in` npc and says `QUEST <npc>`. If the objective is not reached, the answer is the same quest with its progress, and nothing changes. Otherwise the server checks that the reward fits in the inventory (10 items at most, after the items to bring are taken); if it does not, the answer is `ERR 403 INVENTORY_FULL` and **nothing is taken or given**, so the player can free some room and ask again.

### Rewards

When a quest is handed in, the items to bring are taken (`deliver`, `fetch`), the reward items are created in the inventory of the player, and `heal` hit points are given (up to his maximum). The quest is then `completed`. Quest events are logged (`quest started`, `quest progress`, `quest completed`).

### Quest list

| Quest | Type | Given by | Handed in to | Objective | Reward |
|-------|------|----------|--------------|-----------|--------|
| Medicine for the Chapel | `deliver` | Guard Hector (square) | the priestess (chapel) | bring 1 Healing Herbs, found in the square | Blue Ribbon Charm |
| Cellar Pests | `defeat` | Tavern Keeper Bronn (tavern) | Tavern Keeper Bronn | defeat 3 rats, in the plain | Loaf of Bread, 20 HP |
| The Silent Bell | `fetch` | the Dead Acrobat (circus) | the Dead Acrobat | bring 1 Silver Bell, found in the goblin camp | 50 HP |

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

There are 16 NPC definitions, in three roles (the subject asks for at least three distinct ones):

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
| Healing Herbs | `square` (comes back after 60 s) | object of the quest *Medicine for the Chapel*, with a healing effect (30 HP) |
| Loaf of Bread | reward only | reward of the quest *Cellar Pests*, with a healing effect (15 HP) |
| Silver Bell | `goblin_camp` (comes back after 60 s) | object of the quest *The Silent Bell* |
| Blue Ribbon Charm | reward only | reward of a quest |
| Spider Silk, Great Deer Antler, Crown of Nails | loot of the Spider, the Great Deer and the Orc Lord | trophies |
| Stone Fountain | `square` | decoration, **not obtainable** (`TAKE` is refused) |

The objects lie on the floor of their room at the start, and the loot of an enemy falls on the floor of the room where it dies. A player carries at most **10** items. Taking an item removes it from the room and dropping it puts it back there, where another player can take it. A spawn can give an object a `respawn_seconds` so that it comes back after being taken. The healing herbs of the square and the silver bell of the goblin camp both use it (60 seconds): once taken, they reappear in their room and the players there receive `EVT ROOM SPAWN`.

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

There is no automated test suite in the repository: everything below was checked by hand, over real TCP sessions, from a clean build (`make fclean && make build && make build-gui`). A raw `nc` (or `telnet`) connection is enough to reproduce any of it, since the protocol is one line of text per message.

```sh
./server/server &                       # listening on 8080 by default
nc 127.0.0.1 8080                       # a player
```

Each scenario below lists what to type and what to expect.

### Multiplayer

Open two `nc` connections and `CONNECT alice` / `CONNECT bob`.

- The second player to arrive makes both receive `EVT STATS players=2`; the first sees `EVT ROOM PRESENCE ENTER bob` if they are in the same room.
- Move one of them away (`MOVE north`): the other sees `EVT ROOM PRESENCE LEAVE alice` and the counter drops back to `players=1`.
- `CHAT room hello`: every player of the room, the sender included, receives `EVT ROOM CHAT alice hello`, and nobody outside the room does. `CHAT global hi` reaches every connected player.
- Items: alice takes the herbs in the square (`TAKE herbs`), moves away, drops them (`DROP herbs`) in the chapel. Bob, who followed her, receives `EVT ROOM ITEM DROPPED alice item.herbs` and his next `LOOK` lists `item.herbs` among the items of the room. An item instance lives in exactly one place: it can never be in two rooms or in two inventories at once.
- Groups (§`GROUP CREATE|INVITE|JOIN|LEAVE`): alice creates one (`OK group=group.1`), invites bob (`GROUP INVITE bob`), bob receives `EVT GROUP INVITE alice` and joins with `GROUP JOIN alice` (the name of **any member**, not only the leader) → alice receives `EVT GROUP JOIN bob`. `CHAT group hello` reaches the members only. On `QUIT`, the others receive `EVT GROUP LEAVE alice` before `EVT ROOM PRESENCE LEAVE alice`.
- A second `CONNECT alice` from another socket answers `ERR 201 NAME_IN_USE`.

### Combat

Walk to a room that holds an enemy (`west`, `north`, `west`, `west` reaches `loc.orc_village` from the square; `north` from the square reaches the forest wolf).

- `ATTACK orc` returns `OK {"attacker_hp":…,"target_hp":…,"damage":…,"status":"combat","round":1,"escaped":false,"turns":[…]}`: the player attacks, then the enemy answers **in the same reply**, inside `turns[]`. A missed attack appears with `"missed":true` and `"damage":0`.
- Every other player in the room receives, in order, `EVT ROOM COMBAT START alice npc.orc#17`, one `EVT ROOM COMBAT ROUND …` per turn, and `EVT ROOM COMBAT END alice npc.orc#17 victory` when the enemy falls. The attacker also receives `EVT ROOM COMBAT TURN alice npc.orc#17 20`, the turn timer of 20 seconds.
- `ATTACK` or `DEFEND` on an enemy that is not hostile returns `ERR 405 NPC_NOT_HOSTILE`; attacking again after the fight ended returns `ERR 410 NOT_IN_COMBAT`; `MOVE`, `TAKE` and `TALK` are refused while a fight is running.
- Killing the enemy gives its loot to the room; the `LOOK` of the winners lists the dropped item.
- **Death and respawn**: keep `DEFEND`ing an orc without attacking back for around twenty turns until `STATUS` reports `"hp":0,"status":"dead"`. The room receives `EVT ROOM DEATH loc.chapel` and the player is moved to the chapel with 50 hit points (`STATUS` → `{"hp":50,"max_hp":100,"status":"wounded"}`).

### Quests

There are three quests of the three data types (`deliver`, `fetch`, `defeat`); `QUESTS` lists them with their progress, `QUEST <npc>` talks to a giver or turns a quest in.

- **`deliver`** — *Medicine for the Chapel*: `TALK guard` accepts the quest, `TAKE herbs` in the square, walk to the chapel and `TALK priest` → the herbs leave the inventory, `EVT INVENTORY CHANGED quest` is pushed, the charm is added and `QUEST priest` shows it as completed.
- **`fetch`** — *The Silent Bell*: `TALK clown`, take `item.silver_bell` from the goblin camp, come back and `TALK clown` → the bell is consumed and the player is healed by 50.
- **`defeat`** — *Cellar Pests*: `TALK tavern_keeper`, kill three rats on the plain, `QUEST tavern_keeper` → the progress goes from `0/3` to `3/3` on each kill and the quest is handed in at the giver.
- **Refusals**: turning a quest in without the required item does not consume anything and keeps the progress; a reward that would not fit in the inventory is refused with `ERR 403 INVENTORY_FULL` **without stealing anything from the player**; `QUEST guard` on an NPC that has nothing left to say gives `ERR 406 NO_QUEST_AVAILABLE`.

### Protocol compliance and edge cases

Each case was checked against the RFC and against the error table of [Protocol Implementation](#-protocol-implementation).

| Case | Command | Expected |
| --- | --- | --- |
| Command before connecting | `LOOK` | `ERR 400` (unknown/unauthorised before `CONNECT`) |
| Unknown command | `FLY` | `ERR 400` |
| Bad direction | `MOVE up` | `ERR 301 NO_EXIT` |
| Unknown exit in the data | — | the server **refuses to start** and names the file and the room |
| Unknown item | `TAKE unicorn` | `ERR 404 ITEM_NOT_FOUND` |
| Non-takeable item | `TAKE fountain` | `ERR 408 ITEM_NOT_TAKEABLE` (formerly a silent success) |
| Take/drop an item during a fight | `TAKE herbs` | refused |
| Name resolution | `TAKE herbs`, `TAKE healing herbs`, `TAKE item.herbs`, `TALK Guard Hector`, `TALK hector` | all accepted |
| Case of the command | `look`, `Look`, `LOOK` | accepted (§4.2) |
| Line too long | 2000 bytes on one line | `ERR 400`, then the connection is closed |
| `\r\n` line ending | any command | the `\r` is removed and the command works |
| Broken client during a broadcast | kill one `nc` while the other is in a room | the server logs `send failed` as a **WARN** and keeps serving; the survivors are unaffected |
| Command flooding | 120 `WHO` in a burst | about 19 `OK` then `ERR 429 RATE_LIMITED` for the rest, with `WARN command flooding` in the log |
| Too many sessions | opening more clients than `MAX_SESSIONS` | the extra connections are refused |
| Disconnect | Ctrl+D on `nc` | `client disconnected` is logged, the state is removed **before** the other players receive `EVT ROOM PRESENCE LEAVE` |
| Item respawn | take the herbs and wait 60 s in the room | the herbs come back and `EVT ROOM SPAWN` is sent to the room |
| Enemy respawn | kill an enemy and wait | it comes back in its room, at full hit points |

The log of a session can be replayed line by line against these expectations: `LOG_LEVEL=debug LOG_FILE=server.log ./server/server` writes one JSON object per event, and every command, response and error code is in it.

### 🩸 Leaks report <!-- [+] -->

The GUI client ships a memory-check target:

```sh
make -C clients/gui valgrind
```

It runs the executable under Valgrind with `clients/gui/valgrind-suppressions.txt`, which hides the allocations of the system and third-party libraries (Qt, glib/GTK platform theme, OpenGL/glvnd, glibc's dynamic loader, ...) — these are intentionally alive at exit or leaked inside the libraries, and are not our responsibility. Only leaks caused by our own code are reported, and the target fails (`--error-exitcode=1`) if any remain.

Current result on Linux/Qt 5.15: **0 errors — no definitely, indirectly or possibly lost bytes**. The only remaining memory is "still reachable" (globals and library caches, alive by design at exit), which Valgrind does not count as a leak.

## 👥 Group Contributions <!-- [S] -->

The project was carried by two learners. The split below follows the code: each area is owned by the member who wrote it and still answers questions about it during a review.

| Area | kebertra | gtourdiat |
| --- | --- | --- |
| Server network layer (`server/network/`) | **Owner** | — |
| Server command dispatcher (`server/protocol/`) | **Owner** | — |
| Game layer (`game/`) — world, entities, combat, quests, groups | **Owner** | — |
| `shared/` — protocol words, errors, logging, UTF-8 | **Owner** | Review |
| World data (`game/ressources/*.json`) | **Owner** | Review |
| CLI client (`clients/cli/`) | **Owner** | — |
| GUI client (`clients/gui/`) | — | **Owner** |
| Client state layer (`clients/state/`) — `Session`, `GameState` | Review | **Owner** |
| Root `Makefile`, CMake build, lint target | **Owner** | Contributor |
| Internal audits (`audit/`) | **Owner** | — |
| Design docs (`docs/`) | Contributor | **Owner** |
| README | Contributor | Contributor |

### kebertra — Kevin Bertrand

Responsible for the **server and the CLI client**: the `poll(2)` network loop, the dispatcher table that maps a command word to its handler, the two-layer split between sockets and game state, the whole game layer (world loading and validation, item and NPC instances, inventories, the turn-based combat engine, the quest system, the group system, the event bus and the respawn queue), the shared protocol and logging code, the world data itself, and the CLI client. Wrote the internal audits. Set up the root `Makefile`, the parallel build and the `lint` target, and kept the server side of the README up to date.

Commits: `feat: Quest system`, `refactor: turn based fight motor`, `refactor: World instanciation`, `refactor: inventory system with class composition`, `refactor: signalFD for server and client`, `refactor: share defines, errors, success and response between server and client`, `fix: error code, ids instance`.

### gtourdiat — Gaspard TOURDIAT

Responsible for the **GUI client and the client state layer**: the Qt 5 application (connection window, game window, error window, and the custom widgets for the room, the exits, the ground items, the inventory, the NPCs, the quests, the group, the chat tabs, the player counters and the log), the `clients/state/` layer (`Session` + `GameState`) that turns the protocol answers and events into a local copy of the game without any Qt or socket dependency, the CMake build of the GUI, and its Valgrind target with the suppressions file. Wrote most of the design documentation (`docs/`) and contributed to the README, the root `Makefile` and the CMake standard setting.

Commits: `feat: added base gui window`, `feat(CMakeLists): added comments to describe different rules`, `feat: updated GUI structure, added custom UI components`, `feat: separated connection GUI from main()`, `feat: added TAPErrorWindow to display error messages`, `fix(leaks) fixed false alert leaks`.

> Git note: Gaspard's commits also appear under the identities `sousampere` and `gtourdia`, all using the same email address. All of them belong to the same member.

## 📚 Resources <!-- [S] -->

### Documentation and references

- **RFC 42TAP** — the protocol specification shipped with the subject (`docs/protocol-rfc.html`): message format, command and event grammar, error codes, resource management and security rules. All protocol strings in the code come from this document.
- **Subject v1.4** — `docs/TAP-subject-1.4.md` / `.pdf`.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) — `poll(2)`, non-blocking sockets and partial reads/writes.
- [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html), [`signalfd(2)`](https://man7.org/linux/man-pages/man2/signalfd.2.html), [`send(2)`](https://man7.org/linux/man-pages/man2/send.2.html) — Linux man pages.
- [RFC 3629 — UTF-8](https://datatracker.ietf.org/doc/html/rfc3629) — used to validate and truncate incoming text on a character boundary.
- [nlohmann/json](https://github.com/nlohmann/json) — the single third-party dependency, used to read the world files and to build the JSON replies.
- [Qt 5 Widgets documentation](https://doc.qt.io/qt-5/qtwidgets-index.html) — the GUI client, and [`QTimer`](https://doc.qt.io/qt-5/qtimer.html) for polling the socket from the event loop.
- [Valgrind manual](https://valgrind.org/docs/manual/manual.html) and the LeakSanitizer suppressions mechanism — the `make -C clients/gui valgrind` target.
- **C++ references** — [cppreference.com](https://en.cppreference.com/) for the C++20 standard library.
- **Color palette** of the GUI — https://coolors.co/palette/606c38-283618-fefae0-dda15e-bc6c25

### Use of AI

AI was used as a review and drafting aid, never as a substitute for understanding: every suggestion was read, tested against the running server and either kept, rewritten or dropped by the team. In particular:

- **NPC lookup** (`Room::findNpc` in `game/world/room.cpp`): the team first wrote a simple version where the player had to give the exact instance id of the NPC (for example `npc.guard#10`). The version in the code now was proposed and improved with the help of **Claude** (an AI assistant): a player can also designate an NPC by the id of its definition, its short id, its name, or one word of its name, in any case (`TALK guard`, `TALK Guard Hector`, `TALK hector`). The team reviewed it and kept it.
- **README and design documents**: the structure of this README, the protocol deviation table, the combat and quest write-ups, and the internal audits in `audit/` were drafted with Claude and then corrected against the code and against real sessions.
- **Debugging**: Claude was used to read compiler and sanitizer output (the Valgrind suppressions of the Qt client, the `-Wgnu-zero-variadic-macro-arguments` warnings on the logging macros) and to propose fixes that the team applied.
- **Not AI-written**: the game design, the world data, the combat and quest rules, the network architecture and all the C++ code were written and are maintained by the team.
