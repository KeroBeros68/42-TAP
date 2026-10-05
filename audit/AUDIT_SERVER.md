# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-05 — régénéré après `CONNECT`, chargement du monde, `LOOK`, `MOVE`, hook de déconnexion (commits `b7f2318` → `ba889f3`). Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (tous fichiers) + `game/` (`world`, `room`, `npc`, `item`, `parser`, `stats`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` (consommation par le serveur).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++23` propre ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine). Aucune erreur ASAN/UBSAN/fuite sur l'ensemble des scénarios ci-dessous. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟠 État général : le serveur est désormais jouable pour 3 commandes sur 15 ; les 12 autres répondent « OK » sans rien faire

Le blocage majeur de l'audit initial est levé : `CONNECT` fonctionne, le monde est chargé et validé au démarrage, `LOOK` et `MOVE` sont réels, et la déconnexion nettoie l'état joueur. Mesuré en conditions réelles :

- ✅ `CONNECT alice` → `OK connected` ; second `CONNECT carol` depuis un autre client → `ERR 201 NAME_IN_USE` ; nom vide / avec espace / > 32 octets → `ERR 400` ; commande avant `CONNECT` → `ERR 400`.
- ✅ `LOOK` renvoie un JSON conforme à la RFC §5.1.2 (`room{id,name,description,exits}`, `players`, `items`, `npcs`), `players` liste bien tous les joueurs de la salle (`["g1","g2"]`).
- ✅ `MOVE north` → `OK room=loc.village` ; direction inconnue → `ERR 301 NO_EXIT`.
- ✅ Nettoyage à la déconnexion : après le départ de `g1` et `g2`, un nouveau joueur voit `players:["g3"]` seul.
- 🔴 **12 commandes sont des stubs qui répondent `OK Message received`** (`commands.cpp:48-56`) : `CHAT`, `TAKE`, `DROP`, `INVENTORY`, `TALK`, `ATTACK`, `STATUS`, `QUEST`, `QUESTS`, `WHO`, `GROUP`, `QUIT`. C'est plus trompeur qu'une erreur : un client croit que l'action a réussi (vérifié : `QUIT` → `OK Message received`, la connexion reste ouverte ; `TAKE herbs` → `OK` sans rien prendre).
- 🔴 **Toujours aucun événement `EVT`** — ni `PRESENCE ENTER/LEAVE` sur `MOVE`/déconnexion, ni `CHAT`, ni `STATS`. Le hook de déconnexion existe mais n'émet rien.
- 🔴 **Monde chargé à moitié** : `npc.json` et `room.json` sont lus ; `items.json` et `quests.json` ne le sont **pas** (`ItemDb` n'est jamais remplie), et les `spawns` de `room.json` sont ignorés — d'où `"npcs":[]` dans tous les `LOOK`. Les items d'une salle apparaissent sous forme d'id bruts, sans instance unique (§V.1 « Dynamic Item Management »).
- 🔴 **Logging toujours non conforme** (`std::cout`, sans timestamp ni niveau ni JSON). L'IP du client est maintenant journalisée à la connexion, c'est le seul progrès.
- 🔴 **README** : toujours 8 lignes (palette de couleurs), aucune des sections du §VI.

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 3/15 réelles (`CONNECT`, `LOOK`, `MOVE`), 12 stubs `OK` |
| Événements `EVT` (ROOM / GLOBAL / GROUP / STATS) | ❌ aucun émetteur |
| Chargement monde JSON + validation des références | ⚠️ salles/PNJ oui (sorties validées) ; items, quêtes, spawns non |
| Monde : ≥ 8 salles avec boucle | ✅ 15 salles, 16 liens → au moins un cycle |
| Monde : ≥ 3 rôles de PNJ, ≥ 4 items dont 2 obtenables, ≥ 2 quêtes | ✅ en données (3 rôles, 8 items / 7 obtenables, 3 quêtes) — ❌ pas exploités par le serveur |
| Erreur conforme sur commande mal formée | ✅ `ERR 400` (code hors RFC) |
| Broadcast sans interruption si déconnexion en cours d'envoi | ✅ `MSG_NOSIGNAL` + `closing` différé |
| État joueur retiré **avant** le broadcast de départ | ⚠️ retiré (hook) mais aucun broadcast |
| Items uniques / combat / quêtes | ❌ |
| Logging structuré (JSON, niveaux, timestamps, anti-abus) | ❌ |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ajouté ; `lint` absent |
| README (§VI) | ❌ |

---

## `server/server.cpp` (+ `server.hpp`)

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes** — l'état de jeu est modifié séquentiellement, donc pas de course sur « qui prend l'objet / qui prend le nom ». Vérifié (ASAN) sur connexions multiples, doublons de nom simultanés et rafale de 5 000 `LOOK` : aucune erreur mémoire.
2. **Framing TCP correct dans les deux sens** (RFC §9.2) — accumulation, découpe sur `\n`, retrait du `\r`, plusieurs lignes par `recv` ; sortie en file avec `POLLOUT` demandé seulement si nécessaire.
3. **Hook `setOnDisconnect` appelé *avant* `close` et `erase`** (`server.cpp:120-125`) — c'est exactement l'ordre exigé par le sujet §IV (« remove player state before broadcasting leave events »). Vérifié : un joueur parti disparaît de `players` dans les `LOOK` suivants.
4. **`MSG_NOSIGNAL` + `signalfd`** — pas de `SIGPIPE` fatal, arrêt propre sur `SIGTERM` (sockets fermées, sortie 0) intégré à la boucle `poll`.
5. **Garde d'authentification corrigée** — `cmd != CMD_CONNECT` est exempté (`server.cpp:246`), l'IP du client est lue via `inet_ntop` et stockée dans `Connection`. Les 12 constantes `CMD_*` de `shared/defines.hpp` évitent les chaînes magiques dans le dispatcher.

### ❌ Mauvais

1. **🔴 Logging absent** (§V.1, 10 exigences) — `std::cout` brut, aucun timestamp, aucun niveau (le seul `[WARN]` est écrit à la main dans `game.cpp`/`world.cpp`), commandes reçues et réponses envoyées non journalisées (seul `cmdConnect` écrit une ligne), aucune détection de flood / connexions rapides. Chaque `sendTo` écrit encore une ligne de debug.
2. **🔴 Aucun événement émis alors que l'infrastructure est prête** — `sendTo` / `sendToArray` / `sendToAll` existent mais aucune commande ne les appelle ; `MOVE` ne prévient pas l'ancienne et la nouvelle salle, le hook de déconnexion ne prévient personne, `EVT STATS players=` n'existe pas.
3. **Aucune protection contre un client lent / un flood** *(lecture de code)* — `_out` croît sans limite (`appendOut`), pas de limite de connexions ni de débit de commandes (RFC §9.4). Une rafale de 5 000 `LOOK` d'un seul client a été absorbée sans erreur, mais rien ne la limite ni ne la journalise.
4. **Exception dans une action = crash du serveur** *(lecture de code)* — toujours pas de `try/catch` autour de `it->second(s.user, args)` ; le code de jeu utilise `_players.at()` / `_world.rooms.at()` (`game.cpp:61,68,73`), qui lèvent `std::out_of_range` si l'invariant « joueur ∈ salle » est un jour rompu (par exemple `addPlayer` qui n'a pas trouvé sa salle de départ, `game.cpp:14-17`). À encadrer et à convertir en `ERR`.
5. **Commandes sensibles à la casse** — vérifié : `connect bob` → `ERR 400`, alors que la RFC §4.2 les veut insensibles. La limite de 1024 octets n'est appliquée qu'au reliquat sans `\n` : une ligne complète plus longue passe au dispatcher *(lecture de code)*.

> Points mineurs : `handeLine` (faute de frappe) ; `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide avec spam `stderr` ; `server.hpp` inclut ~12 en-têtes système pour tous ses clients.

---

## `server/commands.cpp` (+ `commands.hpp`, `main.cpp`)

### ✅ Bon

1. **`main.cpp` est un vrai point d'entrée** (remplace `testMain.cpp`) : `registerCommands` → `setOnDisconnect` → `start` → boucle, le tout sous un `try/catch` qui retourne `1` avec message si le monde est invalide ou le port pris.
2. **`registerCommands(Server&, Game&)` isole le câblage** — chaque commande est une fonction `static Response cmdX(Game&, User&, args)` ; le `Server` ne connaît pas le jeu, le `Game` ne connaît pas les sockets. Chaque action retourne une `Response` (succès ou erreur) donc une ligne protocolaire valide.
3. **`cmdConnect` enchaîne les bons contrôles** — nom valide (`setName`) → unicité (`isNameTaken` → `ERR 201`) → `authenticate` → `addPlayer`. Un second `CONNECT` sur la même session est refusé (`ERR 400`, vérifié : `CONNECT eve` après `CONNECT dave`).
4. **`cmdLook` refuse un joueur absent du jeu** (`hasPlayer`) au lieu de laisser l'`at()` lever.
5. **Tableau `stubs[]` explicite** (« replace an entry by a real handler once implemented ») — la liste des commandes manquantes est visible en un coup d'œil, ce qui facilite le suivi d'avancement.

### ❌ Mauvais

1. **🔴 Les stubs répondent `OK Message received`** — voir l'état général. Un `ERR 400` (ou un `ERR` dédié) serait honnête tant qu'ils ne sont pas implémentés ; en l'état, `QUIT` ne ferme pas la connexion et `TAKE` ne prend rien.
2. **`cmdMove` : `MOVE` sans argument → `ERR 301 NO_EXIT`** (vérifié) — un argument manquant est plutôt un `ERR 400` ; `MOVE` répond via `TapOk::DATA` + `"room="` à la main alors que `TapOk::ROOM` existe (résultat identique sur le fil, enum jamais utilisé).
3. **Aucune validation des arguments au-delà du nom** — `LOOK foo`, `QUIT bar`, etc. sont acceptés silencieusement.
4. **`std::cout` dans les actions** (`cmdConnect`, `cmdStub`) en guise de logging, avec le contenu brut du message client — pas de journal structuré, et le contenu du chat (contrôles ANSI inclus) sera écrit tel quel dans le terminal de l'opérateur.
5. **Captures `[&game]` par référence** — correct tant que `Game` survit au `Server` (c'est le cas dans `main`, déclaré après), mais l'ordre de destruction est un invariant implicite non documenté.

---

## `server/game.cpp` (+ `game.hpp`, `user.*`, `connection.*`)

### ✅ Bon

1. **Séparation réseau / jeu bien tenue** — `Connection` (fd, ip, tampons) et `User` (id, nom, état d'auth) sont maintenant des classes avec accesseurs ; `Game` ne manipule que des `long long id` et des noms. Un test de gameplay n'a besoin d'aucun socket.
2. **`User::setName` valide sérieusement** — non vide, ≤ 32 octets (`MAX_NAME_LENGTH`), aucun caractère ≤ espace ni `0x7F` ; refuse de renommer une fois authentifié. Les noms UTF-8 passent (octets ≥ 0x80). Conforme à RFC §9.3.
3. **`Game::move` vérifie sortie *et* salle de destination** avant de modifier quoi que ce soit, puis met à jour `Room` et `Player` ensemble — pas d'état à moitié déplacé si la destination n'existe pas.
4. **`Room::addPlayer` idempotent, `removePlayer` via erase-remove** — pas de doublon de joueur dans une salle.
5. **`Game::look` résout les ids en noms** et laisse la sérialisation à `serializeLook` — une seule source du format JSON.

### ❌ Mauvais

1. **`Player` n'a que `name` et `current_map`** (`game.hpp:14-18`, commentaire « later: hp, inventory, group, quests… ») — tout l'état requis par §V.1 (100 HP, respawn, inventaire, quêtes, groupe) reste à créer.
2. **Items sans instances** — `Room::_items` est un `vector<string>` d'ids copié depuis le JSON ; rien ne permet d'en retirer un à `TAKE` ni d'en ajouter un à `DROP`. §V.1 exige des instances uniques sans duplication ; ce modèle devra changer avant d'implémenter `TAKE`/`DROP`.
3. **`Game::addPlayer` écrase silencieusement un joueur existant du même id** (`_players[user.id()] = p`) et, si la salle de départ est introuvable, laisse le joueur enregistré sans salle (`game.cpp:13-17`) — le prochain `LOOK` ferait lever `at()`. L'invariant est protégé par la validation au démarrage, pas par ce code.
4. **`isNameTaken` est sensible à la casse et linéaire** — `Alice` et `alice` coexistent (confusion en chat) ; sans conséquence de perf à cette taille.
5. **`Connection::_in` / `_out` toujours non bornés** (voir `server.cpp` ❌ 3) ; **`User::authenticate()` ne signale pas l'échec** (nom vide → no-op silencieux).

---

## `game/` — `world.cpp`, `parser.cpp`, `room.cpp`, `npc.cpp`, `item.cpp`

### ✅ Bon

1. **Validation au démarrage, fail-fast** (§V.1 « Validates that all exits and references are correct ») — sortie vers une salle inexistante → exception avec le nom de la salle et de la direction ; `start_room` et `respawn_room` doivent exister ; direction inconnue / destination vide refusées dans `parser.cpp`. Le serveur refuse de démarrer sur un monde invalide (message + code 1).
2. **Détection des sorties à sens unique** (`[WARN] … is a one-way exit`) via `oppositeDirection` — attrape l'erreur de conception classique sans bloquer un éventuel one-way voulu.
3. **nlohmann/json pour la sérialisation de `LOOK`** (`ordered_json`, `error_handler_t::replace`) — échappement correct des guillemets/contrôles, UTF-8 invalide remplacé plutôt que de casser la ligne. Ferme le risque d'injection par nom de joueur identifié à l'audit initial pour `LOOK` (RFC §9.3).
4. **Chemin des données robuste** — `PROJECT_ROOT` absolu injecté par le Makefile : le serveur trouve `game/ressources/` quel que soit le répertoire courant.
5. **Données conformes au sujet** — 15 salles, 16 liens (cycle), 3 rôles de PNJ (`quest_giver`, `dialogue_npc`, `enemy`) pour 16 PNJ, 8 items (7 obtenables), 3 quêtes ; toutes les références d'items et de loot du JSON existent (vérifié par script). Stats d'ennemis validées (`hp > 0`, `attack/defense ≥ 0`).

### ❌ Mauvais

1. **🔴 `items.json` et `quests.json` ne sont jamais lus** — `World::loadWorld` ne parse que `npc.json` et `room.json` ; `item_database` reste vide, donc les ids d'items des salles ne sont validés contre rien et la description / l'obtenabilité des items est inaccessible.
2. **🔴 Les `spawns` des salles sont ignorés** (clé présente dans `room.json`, absente de `Room`) — `World::look` passe `{}` en dur pour les PNJ, d'où `"npcs":[]` partout. Aucun PNJ n'existe donc dans le monde vivant : `TALK`, `ATTACK`, `QUEST` n'ont rien sur quoi agir.
3. **Définitions seulement (`NpcDef`, `ItemDef`)** — aucune notion d'instance de PNJ (HP courants, mort, respawn) ni d'item (propriétaire / salle). Voir aussi `game.cpp` ❌ 2.
4. **`parser.cpp` : `readRoom` ignore un monde sans `"rooms"`** (pas de `else`, contrairement à `readNpc` qui avertit) — un fichier vide ne lève qu'à `start_room`, avec un message moins clair.
5. **`NpcDef::dialogue()` renvoie une référence vers un élément choisi au hasard** — OK, mais `randomIndex(size_t)` avec `size == 0` est protégé seulement par le test amont ; `shared/random.hpp` n'est pas testé (le `mt19937` thread-local est correct, mais le générateur n'est pas seedable pour des tests déterministes).

---

## `server/signalFd.cpp` (+ `.hpp`) — inchangé

### ✅ Bon

1. **`signalfd` + `sigprocmask(SIG_BLOCK)`** — arrêt intégré à `poll` sans handler asynchrone ; vérifié : `SIGTERM` arrête proprement.
2. **`SFD_NONBLOCK | SFD_CLOEXEC`**.
3. **Classe non copiable**.
4. **RAII** — le destructeur ferme le fd.
5. **`read()` retourne `SIGNAL_NONE` sur lecture incomplète**.

### ❌ Mauvais

1. **`open()` retourne `_fd` dans une fonction `bool`** quand déjà ouvert (`signalFd.cpp:16`) — `true` par accident ; écrire `return true;`.
2. **`SIGPIPE` non masqué** — le serveur ne repose que sur `MSG_NOSIGNAL`.
3. **`SIGINT` ignoré quand le process est lancé en arrière-plan par un shell non interactif** (observé : `kill -INT` sans effet, `SIGTERM` OK) — artefact du shell, à connaître pour la soutenance.
4. **Un seul signal lu par tour de `poll`**.
5. **Aucun signal de rechargement/rotation de log** (`SIGHUP`/`SIGUSR1`) prévu pour le futur logging.

---

## `shared/errors.hpp` · `success.hpp` · `response.hpp` · `defines.hpp` · `random.hpp`

### ✅ Bon

1. **Les 9 codes d'erreur de la RFC §8.2 sont exacts** ; `tapErrorLine()` produit `ERR <code> <NAME>`. `enum class` + `switch` sans `default` : le compilateur signale un oubli.
2. **Un seul endroit pour formater les réponses** (`tapOkLine` / `Response::line()`), toutes les formes `OK …` de la RFC couvertes.
3. **`Response::success/failure` en fabriques statiques** + `Response.close` pour que `QUIT` demande la fermeture sans toucher au socket.
4. **Vocabulaire partagé serveur/clients** — `CMD_*`, `DIR_*`, `MAX_NAME_LENGTH`, `LINE_END` dans `shared/defines.hpp` : interchangeabilité inter-groupes (§II).
5. **`random.hpp` : `mt19937` thread-local seedé par `random_device`** — utilisable par le combat (jets de dégâts) sans état global partagé.

### ❌ Mauvais

1. **`BAD_REQUEST` = `ERR 400` hors RFC** — l'écart est commenté dans le code mais **toujours pas documenté dans un README** (§VI « Protocol Implementation »).
2. **Pas de support des événements** — aucun `evtLine()` pour `EVT ROOM|GLOBAL|GROUP …`, `EVT STATS players=` ; les formater à la main dans chaque action rouvrirait le risque d'écart de format.
3. **Pas de chemin inverse code → `TapError`** (utile au client).
4. **`TapOk::ROOM` défini mais inutilisé** (`MOVE` passe par `DATA`) ; **`MAX_LINE_LENGTH` ne précise pas si le `\n` compte**.
5. **Pas d'erreur dédiée « argument manquant / non authentifié »** — tout retombe sur 400.

---

## `server/Makefile` · `Makefile` (racine)

### ✅ Bon

1. **Cible `install-deps`** (§VII.1 « install dependencies ») — télécharge `nlohmann/json 3.12.0` via `curl` ou `wget`, **vérifie un SHA-256 épinglé** avant d'installer, écrit dans un fichier temporaire nettoyé par `trap`. `third_party/` est ignoré par git. Vérifié : le fichier est présent et le build passe.
2. **`INCLUDES` calculé par `wildcard`** (`*.hpp`, `../shared`, `../game`) — modifier un en-tête recompile ; corrige le défaut de l'audit initial.
3. **`vpath %.cpp ../game` + `-DPROJECT_ROOT` limité à `world.o`** — les sources du jeu sont compilées dans `Obj/` sans polluer les autres objets ; commentaire sur « `make re` après déplacement du dossier ».
4. **Flags stricts** `-Wall -Wextra -Werror -std=c++23`, build propre ; `clean/fclean/re` propagés.
5. **Binaires désormais ignorés par git** (`/server/server`, `/clients/cli/clientCLI`).

### ❌ Mauvais

1. **Toujours pas de cible `lint`** (exigée §VII.1) ; aucun `clang-tidy` / `cppcheck` / `clang-format` installé sur la machine d'audit.
2. **`all: $(NAME) run` dans `server/Makefile`** — `make` build **et lance** le serveur au premier plan ; la racine (`all: install-deps run`) ouvre en plus deux terminaux. Incompatible avec une CI.
3. **Cible `help` toujours vide**.
4. **Téléchargement réseau au premier `make`** — nécessite internet et `curl`/`wget` à l'évaluation ; `third_party/json.hpp` n'est pas versionné. À savoir (ou à vendoriser).
5. **`PROJECT_ROOT` absolu compilé en dur** — le binaire cesse de trouver `game/ressources` si le dossier est déplacé sans `make re` ; échec propre (`cannot open …`), mais piège en soutenance.

---

## Corrigés depuis l'audit initial (2026-10-02)

- **`CONNECT` impossible** (garde d'authentification qui rejetait tout) → exemptée pour `CONNECT`, `authenticated` posé par l'action (`b7f2318`). Vérifié en réel.
- **Aucun hook de déconnexion** → `setOnDisconnect`, appelé avant `close`/`erase` (entre `b7f2318` et `ba889f3`). Vérifié : l'état joueur est retiré.
- **Aucun chargement du monde / validation des sorties** → `World` + `parser` + `validateExits` (`fc88dfa`, `8b1ad94`). Vérifié sur les données réelles.
- **`Connection::ip` jamais renseignée** → `inet_ntop` à l'`accept`, journalisée à la connexion.
- **`User`/`Connection` structs à champs publics** → classes avec accesseurs ; `User::setName` valide le nom.
- **`INCLUDES` vide** dans `server/Makefile` → `wildcard`.
- **Pas de cible d'installation** → `install-deps` avec somme de contrôle.
- **`testMain.cpp` / action `TEXT` hors RFC** → supprimés, `main.cpp` réel.
- **Injection JSON possible via nom de joueur (`LOOK`)** → sérialisation par `nlohmann::json`.

## Priorités (ordre suggéré)

1. Remplacer les 12 stubs : au minimum `QUIT` (ferme), `WHO`, `CHAT` ; en attendant, les faire répondre `ERR` plutôt que `OK`.
2. Parser `items.json` / `quests.json` / `spawns`, valider les références, et passer à un modèle d'**instances** (items, PNJ) avant `TAKE`/`DROP`/`ATTACK`.
3. Émettre les `EVT` (helper `evtLine` dans `shared/`) : `PRESENCE ENTER/LEAVE` sur `MOVE` et déconnexion, `STATS`, `CHAT`.
4. Logger JSON (timestamp, niveau, IP, joueur, commande, code de réponse) + détection flood ; plafonner `_out`, connexions et débit.
5. `try/catch` autour des actions ; commandes insensibles à la casse ; `ERR 400` pour argument manquant.
6. Cible `lint`, `all` non bloquant, `help`, README §VI.
