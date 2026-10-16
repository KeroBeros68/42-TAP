# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-14 — régénéré après le logger JSON, `QUIT`, `TALK`, `TAKE`/`DROP`, les instances d'items et de PNJ, `ATTACK`/`STATUS` et la réorganisation `game/` (commits `5d05671` → `80632a8`). Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` + `game/` (`game.cpp`, `world/`, `entities/`, `combat/`, `utils/lookup.hpp`) + `shared/` (dont `logger`, `utf8`) + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` (consommation par le serveur).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++20` sans aucun warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine) : exploration, `TAKE`/`DROP`, combats jusqu'à la mort et au respawn, deux joueurs concurrents, `QUIT`. Aucune erreur ASAN/UBSAN. Les 127 lignes de log produites sont toutes du JSON valide. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟠 État général : 9 commandes sur 15 sont réelles, le jeu est jouable en solo — mais le multijoueur n'est pas encore visible (aucun `EVT`, ni `CHAT`/`WHO`/`GROUP`)

Mesuré en conditions réelles :

- ✅ **9 commandes implémentées** : `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, `ATTACK`, `STATUS`. `QUIT` répond `OK bye` puis ferme la connexion (vérifié aussi avec le client CLI : sortie propre).
- ✅ **Items uniques, sans duplication** (§V.1) : deux joueurs envoient `TAKE bread` presque simultanément → l'un reçoit `OK taken=item.bread#1`, l'autre `ERR 404 ITEM_NOT_FOUND`. `TAKE herbs` deux fois de suite → la seconde échoue. `DROP` remet l'objet dans la salle ; un objet non obtenable (`fountain`) refuse `TAKE`.
- ✅ **Désignation par id ou par nom, noms à plusieurs mots** (§V.1) : `TALK guard`, `TALK Guard Hector`, `TAKE healing herbs`, `DROP item.herbs`, casse et espaces ignorés.
- ✅ **Combat de base fonctionnel** : `ATTACK wolf` → `{"attacker_hp":95,"target_hp":6,"damage":9,"status":"combat"}`, le PNJ meurt au 2ᵉ coup, un joueur qui tombe à 0 HP est téléporté à la chapelle avec 50 HP (`respawn` de `room.json`), `STATUS` conforme à la RFC.
- ✅ **Logging JSON** : timestamps ms avec fuseau, niveaux DEBUG/INFO/WARN/ERROR, IP, joueur, commande + arguments, réponse, événements du monde (déplacement, prise, mort de PNJ, respawn). Voir `shared/logger.*`.
- 🔴 **6 stubs restants répondent encore `OK Message received`** (`commands.cpp:115-118`) : `CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO`, `GROUP`. Un client croit que l'action a réussi.
- 🔴 **Toujours aucun événement `EVT`** — ni `PRESENCE ENTER/LEAVE`, ni `CHAT`, ni `STATS`. Deux joueurs dans la même salle ne se voient que par `LOOK`, et le combat n'est annoncé à personne (§V.1 : « results must be logged **and broadcast** »).
- 🔴 **Ids d'instance `#N` exposés sur le fil** : `LOOK` renvoie `"items":["item.fountain#4","item.herbs#3"]`, `"npcs":["npc.guard#10"]`, `TAKE` → `taken=item.herbs#3`. La RFC (§5.4.1, §V.5 du sujet) montre `item.herbs`. Conséquence directe sur l'interchangeabilité entre groupes (§II) : un client tiers qui compare avec `item.herbs` ne retrouve rien. Écart à documenter ou à supprimer.
- 🔴 **README** : 12 lignes (palette + une note sur l'usage de l'IA). Aucune des 9 sections du §VI, ni la première ligne italique obligatoire.

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 9/15 réelles ; 6 stubs `OK` (`CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO`, `GROUP`) |
| Événements `EVT` (ROOM / GLOBAL / GROUP / STATS) | ❌ aucun émetteur |
| Chargement monde JSON + validation des références | ✅ salles, sorties, PNJ, items, spawns, start/respawn ; ❌ `quests.json` jamais lu |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items (≥ 2 obtenables), ≥ 2 quêtes | ✅ données (15 salles, cycle, 3 rôles, 8 items, 3 quêtes) — quêtes non exploitées |
| Erreur conforme sur commande mal formée | ✅ `ERR 400` (hors RFC) |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots | ✅ vérifié ; ⚠️ ids `#N` hors RFC |
| Combat : 100 HP, dégâts, contre-attaque, `STATUS`, respawn | ✅ de base |
| Combat : tour par tour, initiative, `DEFEND`/`FLEE`, diffusion aux joueurs | ❌ |
| Quêtes (conception, validation, récompenses) | ❌ |
| Déconnexion : état retiré avant le broadcast | ⚠️ retiré, aucun broadcast |
| Logging : connexions/IP, commandes, réponses, monde, JSON, niveaux, timestamps | ✅ |
| Logging : quêtes, détection d'abus (flood, connexions rapides) | ❌ |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ; `lint` absent |
| README (§VI) | ❌ |

---

## `server/server.cpp` (+ `server.hpp`)

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes** — l'état de jeu est modifié séquentiellement : la course sur « qui prend l'objet » est impossible par construction (vérifié avec deux joueurs concurrents).
2. **Framing TCP correct dans les deux sens** (RFC §9.2) et écriture partielle gérée (`POLLOUT` demandé seulement si `out` non vide).
3. **Hook de déconnexion appelé *avant* `close`/`erase`** (`server.cpp:123-125`) — ordre exigé par §IV ; le joueur est retiré de sa salle avant toute suite.
4. **`MSG_NOSIGNAL` + `signalfd` (`SIGINT`/`SIGTERM`/`SIGQUIT`)** — pas de `SIGPIPE` fatal, arrêt propre vérifié.
5. **Journalisation de chaque étape réseau** — connexion + IP, commande reçue (joueur, commande, arguments), réponse envoyée (tronquée à 100 octets sans couper un caractère UTF-8, `shorten`), ligne trop longue, commande avant `CONNECT`, commande inconnue, échec de `recv`/`send`.

### ❌ Mauvais

1. **🔴 Aucune détection d'abus** (§V.1 « command flooding, rapid connections ») — les `WARN` existants (commande inconnue, avant `CONNECT`, ligne trop longue) ne comptent rien : pas de compteur par IP/joueur, pas de limite de débit, pas de limite de connexions. Une rafale de 5 000 commandes est absorbée et journalisée en INFO sans alerte. Et `_out` reste non borné : un client qui ne lit pas fait grossir la mémoire du serveur *(lecture de code)*.
2. **🔴 Aucun `EVT` émis** alors que `sendTo` / `sendToArray` / `sendToAll` existent : `MOVE`, `QUIT`, la déconnexion, le combat et la mort ne préviennent personne.
3. **Exception dans une action = crash du serveur** *(lecture de code)* — toujours pas de `try/catch` autour de `it->second(s.user, args)` (`server.cpp:278`). Les actions utilisent encore `_players.at()` (`Game::playerMap`, `game.cpp:64`) qui lève `std::out_of_range` si le joueur a disparu.
4. **Commandes sensibles à la casse** — vérifié : `look` → `ERR 400` (RFC §4.2 : insensibles). La limite de 1024 octets ne s'applique qu'au reliquat sans `\n` : une ligne complète plus longue est traitée *(lecture de code)*.
5. **Tampon de log et de réponse au coût non maîtrisé** — chaque `sendTo` journalise en DEBUG, chaque commande produit 2 lignes INFO avec `std::endl` (flush) ; acceptable à 2–3 joueurs, à surveiller sous charge (§V.1 « must not significantly impact performance »).

> Points mineurs : `handeLine` (faute de frappe) ; `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide ; `server.hpp` inclut ~13 en-têtes système pour tous ses clients ; les ids de session sont recyclés (`_available_id`).

---

## `server/commands.cpp` (+ `serializer.cpp`, `main.cpp`)

### ✅ Bon

1. **9 commandes réelles, une fonction `static Response cmdX(Game&, User&, …)` par commande** — le câblage (`registerCommands`) reste lisible ; chaque action retourne une `Response` donc une ligne valide.
2. **Codes d'erreur corrects par cas** : `TAKE` → `404 ITEM_NOT_FOUND` ou `403 INVENTORY_FULL`, `DROP` → `404 ITEM_NOT_IN_INVENTORY`, `TALK` → `404 NPC_NOT_FOUND`, `ATTACK` → `404 NPC_NOT_FOUND` / `405 NPC_NOT_HOSTILE`, `MOVE` → `301 NO_EXIT`. Vérifié pour chacun.
3. **`serializer.cpp` : seul endroit qui écrit le JSON de réponse** (`LOOK`, `STATUS`, `ATTACK`) avec les noms de champs de la RFC (`hp`, `max_hp`, `status`, `attacker_hp`, `target_hp`, `damage`) via `nlohmann::ordered_json` + `error_handler_t::replace` — pas d'injection par nom de joueur, UTF-8 invalide remplacé.
4. **`MOVE` utilise enfin `TapOk::ROOM`** ; `QUIT` utilise `Response.close` + `TapOk::BYE` — réponse puis fermeture, sans toucher au socket.
5. **`main.cpp` borne la vie du jeu** : `try/catch` global, `LOG_ERROR("fatal error")` + code 1 si le monde est invalide ou le port pris ; `LOG_INFO("server stopped")` en sortie normale.

### ❌ Mauvais

1. **🔴 6 stubs répondent `OK Message received`** — `INVENTORY` en particulier bloque `DROP` côté joueur (on ne peut pas lister ce qu'on porte) et le GUI (§V.4) ; `WHO`/`CHAT` bloquent tout le côté multijoueur.
2. **🔴 Ids d'instance (`item.herbs#3`, `npc.guard#10`) renvoyés au client** (`TAKE`/`DROP`/`LOOK`) — voir l'état général. Pistes : renvoyer l'id de définition (`item.herbs`) et résoudre l'instance côté serveur, ou documenter l'écart au §VI « Protocol Implementation ».
3. **`ERR 403 INVENTORY_FULL` hors RFC** (commenté dans `errors.hpp`, absent du README) ; `ATTACK` sur un PNJ déjà mort → `ERR 400` *(lecture de code ; inatteignable aujourd'hui car le PNJ est retiré de la salle à sa mort)*.
4. **Aucune validation des arguments inutiles** — `LOOK foo`, `QUIT bar`, `STATUS x` sont acceptés ; `MOVE` sans argument → `301` (plutôt un `400`).
5. **Captures `[&game]` par référence** — valable tant que `Game` survit au `Server` (déclaré après dans `main`), invariant implicite non documenté.

---

## `game/game.cpp` (+ `game.hpp`, `entities/player.*`, `server/user.*`, `connection.*`)

### ✅ Bon

1. **`locate()` centralise la vérification « joueur ∈ salle »** (version `const` + non-`const` sans duplication) : chaque action obtient `Presence{player, room}` ou un échec journalisé, au lieu de multiplier les `find`/`at`. Réduit fortement le risque de crash de l'audit initial.
2. **Journalisation métier complète** : `player entered/left room`, `player moved`, `player took/dropped an item`, `inventory full`, `player attacked an npc`, `npc died` (avec butin), `player died`, `player respawned` — exactement les « world state changes / combat results » demandés (§V.1).
3. **Respawn conforme au sujet** (« respawn at a safe location with reduced health ») : `teleport` vers `respawn_room` puis `revive(respawn_life)` (50 HP, borné à `[1, max_hp]`). Vérifié : mort contre le boss orc → chapelle, `STATUS` = 50/100.
4. **Inventaire du joueur plafonné** (`PLAYER_INVENTORY_CAPACITY` = 10) avec transfert atomique `transfer()` : vérifie la destination avant de retirer la source — aucun objet perdu si l'inventaire est plein.
5. **`User::setName` renforcé** : UTF-8 valide (`validateUTF8`, refuse contrôles C0/C1, `U+2028/2029`), ≤ 32 octets, sans espace ; nom pris → `ERR 201`.

### ❌ Mauvais

1. **🔴 Combat non conforme au « turn-based » du sujet** — `fight()` = un coup du joueur puis une contre-attaque immédiate, pas d'initiative, pas de `DEFEND`/`FLEE`, pas d'état de combat durable : `in_combat` passe à `true` au premier coup et **ne retombe que si le joueur change de salle ou tue sa cible** (vérifié : après avoir survécu à un coup, `STATUS` → `"combat"` indéfiniment). Aucun autre joueur n'est informé du combat (§V.1 « broadcast to relevant players »).
2. **🔴 Pas de régénération ni de soin** — `Stats::heal()` n'est appelé nulle part, `item.herbs` ne sert à rien, les HP d'un PNJ blessé restent bas (le boss est resté à 46 HP après la mort du joueur). Un joueur à 5 HP n'a pas d'autre recours que mourir.
3. **🔴 `respawn_seconds` est lu et validé mais jamais utilisé** (10 entrées de `room.json`) — un PNJ tué disparaît définitivement (`removeNpc`), le monde se vide au fil des parties. Aucun timer n'existe dans la boucle (le `poll` à 1 s en offre pourtant un).
4. **Formule de dégâts figée** — `attack × 100/(100+défense)` toujours déterministe, joueur à `attack=10/defense=10` codé en dur (`player.cpp:5`), aucune variance ni critique ni équipement : à justifier et documenter dans la section « Combat System » du README.
5. **`User::authenticate()` est une bascule** (`_authenticated = _authenticated ? false : true`, `user.cpp:31`) — un second appel désauthentifie. Inoffensif aujourd'hui (`setName` refuse déjà un joueur authentifié, donc `cmdConnect` sort avant), mais piège pour la prochaine personne qui l'appellera. `Game::addPlayer` écrase aussi silencieusement un joueur de même id (`insert_or_assign`).

---

## `game/world/` (`world.cpp`, `parser.cpp`, `room.cpp`) · `game/entities/` (`npc`, `item`, `inventory`, `stats`) · `game/utils/lookup.hpp`

### ✅ Bon

1. **Validation fail-fast étendue** : sorties vers salle inconnue, direction inconnue, `start_room`/`respawn_room` existantes, spawn de PNJ/item inconnu, `count` ∈ [1, 100], `respawn_seconds` ≥ 0, stats d'ennemi valides ; sorties à sens unique signalées en `WARN`. Le serveur refuse de démarrer sur un monde incohérent (§V.1 « Validates that all exits and references are correct »).
2. **Séparation définition / instance** (`ItemDef`→`Item`, `NpcDef`→`Npc`, `instantiate`) — c'est le modèle demandé par « items exist as unique instances » : chaque `spawn` crée `count` objets distincts avec un id d'instance. Le butin d'un PNJ est instancié à sa mort et déposé dans la salle.
3. **`lookup::findIn` générique** (PNJ et items, id d'instance, id de définition avec/sans préfixe, nom complet, un mot du nom, casse et espaces ignorés) — résolution déterministe (premier par id d'instance) ; testé sur `guard`, `Guard Hector`, `hector`, `healing herbs`, `item.herbs`. Documenté dans le README comme proposé avec l'aide de l'IA et relu par l'équipe — conforme à l'esprit du §I.
4. **`Inventory` unique pour joueur et salle** (capacité 0 = illimité pour une salle) + `transfer()` — `TAKE`/`DROP`/butin partagent le même code, donc la même garantie d'unicité.
5. **Données conformes** : 15 salles, 16 liens (cycle), 16 PNJ répartis en `quest_giver` / `dialogue_npc` / `enemy`, 8 items (7 obtenables), 3 quêtes ; toutes les références du JSON existent (vérifié par script).

### ❌ Mauvais

1. **🔴 `quests.json` n'est jamais lu** — aucun `QuestDef`/`QuestDb`, aucune progression, aucune validation que les quêtes référencent des items/PNJ existants. `QUEST`/`QUESTS` sont des stubs. Le système de quêtes (§V.1 « Design Choice ») est entièrement à concevoir.
2. **Pas de quête → pas d'usage des rôles `quest_giver`** ; `TALK` renvoie un dialogue aléatoire pour tous (y compris les `quest_giver`), sans lien avec une quête.
3. **Correspondance « un mot du nom » trop permissive** — `designates` accepte n'importe quel mot du nom : `TAKE of` ou `TAKE the` prend le premier objet dont le nom contient ce mot ; pas d'avertissement d'ambiguïté (la première instance par id l'emporte). À restreindre ou à signaler.
4. **`Npc::dropLoot` ignore l'échec de `floor.add`** (retour de `add` jamais testé) — sans conséquence tant que l'inventaire de salle est illimité, mais perd silencieusement le butin si on lui met une capacité.
5. **Ids d'instance par compteur statique global** (`static inline int _current_id`, un par classe) — non réinitialisé, dépend de l'ordre de parcours d'un `unordered_map` de salles ; les ids changent d'une version à l'autre du JSON. Acceptable côté serveur, gênant si un client mémorise un id (voir `commands.cpp` ❌ 2).

---

## `shared/logger.*` · `shared/utf8.*`

### ✅ Bon

1. **Logging structuré JSON, une ligne par événement** — `{"ts","level","msg",…champs}` ; 127 lignes produites pendant l'audit, toutes parsables. Niveaux DEBUG/INFO/WARN/ERROR ; `src` (`fichier:ligne`) ajouté dès WARN.
2. **Échappement JSON robuste** (`jsonEscape`) — guillemets, contrôles, UTF-8 invalide remplacé par `�`, `U+2028/2029` et C1 échappés : un joueur ne peut pas injecter de fausse ligne de log via un argument de commande (§V.1 + RFC §9.2).
3. **Macros `LOG_*` qui n'évaluent pas leurs arguments si le niveau est désactivé** (`if (Logger::instance().enabled(level))`) ; champs typés (`LogValue` : nombres, bool, chaînes) sans `printf` fragile.
4. **Sortie adaptée** : INFO/DEBUG sur stdout, WARN/ERROR sur stderr, couleurs seulement si TTY et `NO_COLOR` absent ; mutex pour un futur multithread.
5. **`validateUTF8`** (rejette séquences invalides, surlongues, surrogates, > U+10FFFF, contrôles) réutilisée pour les noms de joueur — conforme à RFC §9.2 « Unicode characters in usernames ».

### ❌ Mauvais

1. **Niveau de log non configurable et fichier jamais utilisé** — `setMinLevel`, `setOutput` et `openFile` ne sont appelés nulle part (`grep` : uniquement leur définition) : le serveur tourne toujours en `Debug` (une ligne par `sendTo`, par socket…) et ne journalise que sur la sortie standard. Aucune variable d'environnement ni option. Le §V.1 demande des « appropriate output streams » et le README doit documenter comment surveiller le serveur.
2. **Pas de journalisation des quêtes** (exigence §V.1) puisqu'elles n'existent pas, ni d'événement d'abus (voir `server.cpp` ❌ 1).
3. **Décalage horaire calculé une seule fois au démarrage** (`_formattedUtcOffset`) — après un changement d'heure d'été le suffixe est faux ; la variable locale du timestamp s'appelle `utc` mais contient `localtime_r`.
4. **`std::endl` sur chaque ligne** (flush) et `_file` flushé à chaque écriture — correct pour ne pas perdre de logs, mais coûteux ; pas de limite de taille / rotation.
5. **Entrées utilisateur non tronquées dans `args`** — une ligne de 1024 octets est journalisée en entier à chaque commande (seule la réponse `OK` est tronquée à 100) ; amplifie le volume de logs en cas de flood.

---

## `server/signalFd.cpp` (+ `.hpp`)

### ✅ Bon

1. `signalfd` + `sigprocmask` : arrêt intégré à `poll` ; `SIGQUIT` ajouté ; vérifié (`SIGTERM`).
2. `SFD_NONBLOCK | SFD_CLOEXEC`.
3. Classe non copiable.
4. RAII.
5. `read()` renvoie `SIGNAL_NONE` sur lecture incomplète.

### ❌ Mauvais

1. **`open()` retourne `_fd` dans une fonction `bool`** si déjà ouvert (`signalFd.cpp:16`) — `true` par accident ; écrire `return true;`.
2. **`SIGPIPE` non masqué** — protégé seulement par `MSG_NOSIGNAL`.
3. **`SIGINT` ignoré quand le process est lancé en arrière-plan par un shell non interactif** (artefact du shell, `SIGTERM` OK) — à connaître en soutenance.
4. **Un seul signal lu par tour de `poll`**.
5. **Aucun signal de rechargement de log** (`SIGHUP`/`SIGUSR1`) maintenant que le logger a un fichier prévu.

---

## `shared/errors.hpp` · `success.hpp` · `response.hpp` · `defines.hpp` · `random.hpp`

### ✅ Bon

1. **Codes d'erreur de la RFC §8.2 exacts** ; `tapErrorLine()` produit `ERR <code> <NAME>`, `switch` sans `default`.
2. **Un seul endroit pour formater les réponses** ; toutes les formes `OK …` utilisées (`OK bye`, `OK room=`, `OK taken=`, `OK dropped=`, `OK connected`).
3. **`Response::success/failure` en fabriques statiques**, `Response.close` pour `QUIT`.
4. **Vocabulaire partagé serveur/clients** (`CMD_*`, `DIR_*`, `MAX_NAME_LENGTH`, `PLAYER_INVENTORY_CAPACITY`, `INSTANCE_SEPARATOR`).
5. **`random.hpp`** : `mt19937` thread-local, prêt pour des jets de dégâts variables.

### ❌ Mauvais

1. **`ERR 400` et `ERR 403` hors RFC, non documentés dans un README** (§VI « Protocol Implementation »).
2. **Pas de support des événements** — aucun `evtLine()` pour `EVT ROOM|GLOBAL|GROUP …`, `EVT STATS players=`.
3. **Pas de chemin inverse code → `TapError`** (utile au client).
4. **`TapOk::PLAIN`/`PLAYERS`/`GROUP` encore inutilisés** — signe des commandes à venir (`CHAT`, `WHO`, `GROUP`).
5. **`INSTANCE_SEPARATOR "#"` partagé dans `shared/`** alors que l'id d'instance est un détail interne du serveur : le placer dans `shared/` invite les clients à en dépendre.

---

## `server/Makefile` · `Makefile` (racine)

### ✅ Bon

1. **`install-deps`** : nlohmann/json 3.12.0 téléchargé avec SHA-256 épinglé, fichier temporaire nettoyé, `third_party/` ignoré par git ; `serializer.o` et `parser.o` en dépendent.
2. **`INCLUDES` par `wildcard` étendu aux sous-dossiers** (`../game/*/*.hpp`), `vpath` étendu aux nouveaux dossiers — la réorganisation `game/` n'a rien cassé, build propre.
3. **Flags stricts** `-Wall -Wextra -Werror`, **zéro warning**, `-std=c++20` (abaissé depuis c++23 pour le compilateur des machines de l'école).
4. **Binaires ignorés par git**, `clean/fclean/re` propagés.
5. **`PROJECT_ROOT` absolu** : le serveur trouve ses données quel que soit le répertoire courant.

### ❌ Mauvais

1. **Toujours pas de cible `lint`** (exigée §VII.1) ; aucun `clang-tidy`/`cppcheck`/`clang-format` installé sur la machine d'audit.
2. **`all: $(NAME) run`** — `make` build **et lance** le serveur au premier plan ; la racine ouvre en plus deux terminaux. Incompatible avec une CI.
3. **Cible `help` toujours vide.**
4. **Téléchargement réseau au premier `make`** — internet + `curl`/`wget` requis à l'évaluation.
5. **`.vscode/c_cpp_properties.json` versionné** et modifié à chaque réorganisation (bruit de diff) ; `PROJECT_ROOT` figé dans le binaire (`make re` après déplacement du dossier).

---

## Corrigés depuis l'audit initial

Audit du 2026-10-02 (corrigés avant le 2026-10-05, voir historique) :
- `CONNECT` impossible → exempté de la garde. Hook de déconnexion → `setOnDisconnect`. Chargement et validation du monde. `Connection::ip` renseignée. `INCLUDES` vide. `install-deps`. `testMain.cpp`/`TEXT` supprimés. Injection JSON via `LOOK`.

Depuis le 2026-10-05 :
- **Logging absent** → `shared/logger.*` JSON (timestamps, niveaux, IP, joueur, commandes, réponses, événements du monde). Vérifié : 127/127 lignes JSON valides.
- **`QUIT` stub** → réel (`OK bye` + fermeture), vérifié aussi côté client CLI.
- **`TALK`, `TAKE`, `DROP`, `ATTACK`, `STATUS` stubs** → réels, avec les bons codes d'erreur.
- **Items sans instances** (`vector<string>` d'ids) → `ItemDef`/`Item`, `Inventory`, `transfer()` ; unicité vérifiée avec deux joueurs concurrents.
- **`items.json` et `spawns` ignorés, `"npcs":[]` partout** → items et PNJ instanciés, spawns validés, PNJ visibles dans `LOOK`.
- **`Player` sans état de jeu** → inventaire (10), stats (100 HP), salle courante, respawn.
- **`.at()` dispersés pouvant lever** → `Game::locate()` centralise la vérification (il reste `playerMap`).
- **`MOVE` via `DATA` + `"room="` à la main** → `TapOk::ROOM`.
- **Validation des noms** (octets ≥ 0x80 seulement vérifiés) → décodeur UTF-8 complet.
- **Pas de `INSTANCE` de PNJ pour `ATTACK`** → PNJ vivants par salle, butin déposé à la mort.

## Priorités (ordre suggéré)

1. **Décider du sort des ids `#N`** (RFC `item.herbs`) avant qu'un client GUI ne s'y adosse ; documenter l'écart sinon.
2. **`INVENTORY`, `WHO`, `CHAT`** (les plus simples) puis supprimer les stubs ou les faire répondre `ERR` en attendant.
3. **Événements `EVT`** (helper `evtLine` dans `shared/`) : `PRESENCE ENTER/LEAVE` sur `MOVE`/déconnexion/mort, `CHAT`, `STATS`, et diffusion du combat à la salle.
4. **Quêtes** : parser `quests.json`, valider les références, `QUEST`/`QUESTS`, récompenses ; relier aux `quest_giver`.
5. **Combat** : `respawn_seconds` (timer sur la boucle `poll`), régénération / usage des herbes, `DEFEND`/`FLEE`, fin de l'état `combat`, justification de la formule dans le README.
6. **Anti-abus + robustesse** : compteurs par IP/joueur, plafonds de connexions et de `_out`, `try/catch` autour des actions, commandes insensibles à la casse.
7. **Logger** : niveau/fichier configurables (variable d'environnement), tronquer `args`.
8. **Cible `lint`, `all` non bloquant, README §VI** (sections Architecture, Protocol Implementation, Combat, Quest, World, Logging, Contributions, Testing).
