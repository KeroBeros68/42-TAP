# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-19 (seconde version du jour) — régénéré après `INVENTORY` (`f7bac7b`), `WHO` (`b3a1e37`), la configuration par variables d'environnement / `.env` (`9cbf0ad`), le README v2 (`83b5c65`) et la réorganisation de `shared/` en `logging/` · `protocol/` · `system/` · `text/`. Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (`network/`, `protocol/`, `main.cpp`) + `game/` (`game.cpp`, `combat/`, `events/`, `world/`, `entities/`, `utils/lookup.hpp`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` + `.env.example` + `README.md` (comme documentation de conformité).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++20` sans aucun warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined`. Le port se règle désormais par `SERVER_PORT` (plus besoin de recompiler : 18080 en variable). Scénarios de cette mise à jour : `INVENTORY`/`WHO` seul et à deux joueurs, `TAKE`/`DROP` puis `INVENTORY`, `LOG_LEVEL=warn`, `.env` à la racine lancé depuis `server/`, valeurs invalides, client CLI avec `SERVER_PORT`. Ceux des mises à jour précédentes (combat, événements, respawn) n'ont pas été rejoués : le code correspondant n'a changé que par déplacement de fichiers et renommages (`take`→`takeItem`, `drop`→`dropItem`). Aucune erreur ASAN/UBSAN. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟢 État général : 11 commandes RFC sur 15, événements et combat tour par tour en place, configuration par `.env` — le travail restant est le chat, les groupes, les quêtes, et la fin du README

Mesuré en conditions réelles :

- ✅ **11 commandes RFC réelles** (+ `DEFEND` et `FLEE`, extensions) : `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, **`INVENTORY`**, **`WHO`**, `ATTACK`, `STATUS`. `INVENTORY` → `OK []` puis `OK ["item.herbs#3"]` après un `TAKE`, puis `OK []` après `DROP` ; autorisée en combat. `WHO` → `OK players=2` avec deux joueurs connectés (conforme à la RFC §5.2.2), insensible à la casse (`who`).
- ✅ **Configuration sans recompilation** — nouveau : `SERVER_PORT`, `SERVER_IP` (clients), `LOG_LEVEL` lus dans l'environnement ou dans `.env` (`shared/system/loadenv.*`, `.env.example` versionné, `.env` ignoré par git). Vérifié : `SERVER_PORT=18080` en variable ; `LOG_LEVEL=warn` supprime bien toutes les lignes INFO (0 sur le log) ; un `.env` à la racine (`SERVER_PORT = "18081"`, avec espaces, guillemets et commentaire) est trouvé depuis `server/` et appliqué ; une valeur invalide (`SERVER_PORT=abc`, `99999`, `LOG_LEVEL=zzz`) retombe silencieusement sur le défaut. La variable d'environnement passe avant le fichier.
- ✅ **Événements `EVT`, combat au tour par tour avec initiative, `DEFEND`/`FLEE`, combat multijoueur, respawn annoncé** (acquis de la mise à jour précédente, code inchangé hors renommages).
- ✅ **README v2** (460 lignes, +255) : sections Combat System (règles, formule, initiative, commandes, diffusion, justification), World Design (format, carte ASCII, rôles, items, validation), Configuration, tableau des 8 écarts à la RFC, tableau des événements. Il corrige tout ce que l'audit précédent relevait (casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411).
- 🔴 **4 stubs restent** : `CHAT`, `QUEST`, `QUESTS`, `GROUP` répondent `OK Message received`. Donc **aucun événement `CHAT`/`GROUP`**, pas de coopération par groupe, et le système de quêtes (3 quêtes en données, `quests.json` toujours non lu) n'existe pas.
- 🔴 **Le README est de nouveau en retard d'un commit** : il liste encore `INVENTORY` et `WHO` comme « not implemented yet » (statut, tableau des commandes, écart n°8 « 6 commandes »), et annonce « 15 NPC definitions » alors que `npc.json` en contient **16** (3 dialogue, 3 quêtes, 10 ennemis — vérifié par script).
- 🔴 **Six sections du README restent vides** : Instructions, Quest System, **Server Logging**, **Testing**, Group Contributions, Resources (et « Client interface choice » en `TODO`).

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 11/15 ; 4 stubs (`CHAT`, `QUEST`, `QUESTS`, `GROUP`) ; +`DEFEND`, `FLEE` |
| Événements `EVT` ROOM (PRESENCE) / STATS | ✅ |
| Événements `EVT` CHAT (ROOM / GLOBAL / GROUP) et GROUP | ❌ |
| Commandes insensibles à la casse (RFC §4.2) | ✅ |
| Chargement monde JSON + validation des références | ✅ ; ❌ `quests.json` jamais lu |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items, ≥ 2 quêtes | ✅ données (15 salles, 2 boucles, 16 PNJ, 8 items) — quêtes non exploitées |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots, inventaire visible | ✅ ; ⚠️ ids `#N` hors RFC (documenté) |
| Respawn PNJ / items | ✅ annoncé par `EVT` |
| Combat : 100 HP, dégâts, `STATUS`, respawn joueur, tour par tour, initiative, `DEFEND`/`FLEE`, diffusion | ✅ |
| Quêtes (conception, validation, récompenses) | ❌ |
| Déconnexion : état retiré **avant** le broadcast de départ | ✅ |
| Logging : connexions/IP, commandes, réponses, monde, combat, JSON, niveaux, timestamps | ✅ ; niveau configurable ✅ |
| Logging : fichier / sortie configurable, quêtes, détection d'abus | ❌ |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ; `lint`, `run-server`, `run-client`, `run-client-gui` absents |
| README (§VI) | ⚠️ 6 sections remplies sur 12, mais 2 affirmations périmées |

---

## `server/network/` — `server.cpp`, `connection.*`, `user.*`

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes**, une seule boucle dans `main` : réseau → temps de jeu → livraison des événements. Aucune course possible, aucun thread pour les timers de tour ni de respawn. Port d'écoute lu dans l'environnement (`getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT)`), validé dans [1, 65535].
2. **Ordre réponse → événements respecté** : l'auteur d'une commande reçoit sa réponse avant les `EVT` qu'elle déclenche.
3. **Hook de déconnexion appelé avant `close`/`erase`** (§IV) : retrait du joueur, abandon du combat, `LEAVE`, `STATS` avant la destruction de la session.
4. **Framing et constantes centralisés** (`shared/protocol/line.hpp`, `replyError`, `setNonBlocking`, `utf8SafeLength`), fichiers documentés (bandeaux, `@param/@return`).
5. **Journalisation de chaque étape réseau**, désormais filtrable par `LOG_LEVEL` (le `DEBUG` par `sendTo`/socket disparaît en `info`).

### ❌ Mauvais

1. **🔴 Aucune détection d'abus** (§V.1 « command flooding, rapid connections ») : les `WARN` existants ne comptent rien ; ni limite de débit, ni limite de connexions. `_out` reste non borné *(lecture de code)* ; un client qui ne lit pas fait grossir la mémoire, d'autant plus avec les `EVT` de combat.
2. **Exception dans une action = crash du serveur** *(lecture de code)* — toujours aucun `try/catch` autour de l'action ni de `game.tick()` / `takeEvents()` ; le `try` global de `main` arrête tout le serveur.
3. **`sendToArray` → `sendTo` → `flush` immédiat par destinataire et par événement** : N appels `send()` par ligne pour une salle de N joueurs en combat, au lieu d'un flush par tour de `poll`.
4. **`QUIT` avant `CONNECT` refusé (`ERR 400`) sans fermeture** — le client CLI l'envoie sur Ctrl+C et attend alors 1 s.
5. **Limite de 1024 octets appliquée seulement au reliquat sans `\n`** : une ligne complète plus longue est traitée *(lecture de code)*.

> Points mineurs : `handeLine` (faute de frappe) ; `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide ; ids de session recyclés ; `server.hpp` inclut ~13 en-têtes système.

---

## `server/protocol/` — `commands.cpp`, `serializer.cpp` (+ `main.cpp`)

### ✅ Bon

1. **Une fonction `cmdX` par commande, toutes documentées** ; `combatResponse()` factorise `ATTACK`/`DEFEND`/`FLEE`. Les deux nouvelles commandes (`cmdInventory`, `cmdWho`) suivent le même gabarit, et précisent « allowed during a fight ».
2. **`INVENTORY` renvoie un tableau JSON d'ids** (`serializeInventory`, via `nlohmann`, donc échappé et UTF-8 sûr) ; **`WHO` utilise `TapOk::PLAYERS`** (`OK players=<n>`), le format exact de la RFC §5.2.2.
3. **Codes d'erreur précis par cas** (`403`, `404`, `405`, `301`, `409`-`411`), vérifiés lors des mises à jour précédentes.
4. **Un seul endroit qui écrit le JSON de réponse**, champs en constantes `FIELD_*` ; `ATTACK`/`STATUS` étendent la RFC sans en retirer de champ (écart n°5 du README).
5. **`main.cpp`** : charge `.env`, règle le niveau de log, lit le port, `try/catch` global avec `LOG_ERROR("fatal error")` + code 1.

### ❌ Mauvais

1. **🔴 4 stubs `OK Message received`** : `CHAT`, `QUEST`, `QUESTS`, `GROUP`. Un client croit que l'action a réussi.
2. **🔴 `WHO` : le sujet et la RFC divergent, et le choix n'est pas documenté** — le sujet (§V.5) montre `OK { "room": ["alice","bob"], "server": 5 }`, la RFC (§5.2.2) montre `OK players=<count>`. Le serveur suit la RFC, ce qui est défendable (la RFC prime), mais un GUI qui suivrait l'exemple du sujet n'y trouverait ni la liste de la salle ni le total. À mentionner dans les écarts/choix du README.
3. **Ids d'instance `#N` sur le fil** (`item.herbs#3`) — documenté (écart n°6) mais hors RFC §5.4.1 ; `INVENTORY` et les `EVT … COMBAT` les propagent aussi.
4. **Format des `EVT … COMBAT ROUND` toujours ambigu** (`… npc.wolf#11 npc.wolf#11 1 attack cara miss` : acteur et ennemi répétés, dernier champ qui mélange nombre et mots) — le README donne maintenant le tableau, mais pas de grammaire ABNF.
5. **Validation des arguments inutiles absente** (`INVENTORY x` est accepté, `LOOK foo`, `QUIT bar`…) ; `MOVE` sans argument → `301` plutôt que `400`.

---

## `game/combat/` · `game/game.cpp` · `game/events/` · `game/world/` · `game/entities/` · `game/utils/lookup.hpp`

### ✅ Bon

1. **Combat : moteur pur (`combat.cpp`, `roll` injectable) / orchestration (`CombatManager`) / interface (`CombatHost`)** — testable sans réseau ni horloge. Rounds, initiative, timeout, multijoueur, `abandon` à la déconnexion, `dropFight` complet. Formules et décisions maintenant **justifiées dans le README**.
2. **`EventBus`** découplé du réseau (`send`, `toRoom`, `toOthers`, `take`), événements aux bons endroits (présence, `STATS`, combat, `SPAWN`) ; `RespawnQueue` dédiée.
3. **Validation fail-fast du monde** (sorties, directions, spawns, items, `count`, `respawn_seconds`, stats) ; je l'ai revérifiée sur les données : 15 salles, **toutes les sorties ont un retour**, 2 boucles (`square→village→plain→plain2→square` et `square→forest→forest_edge→plain2→square`), 0 référence cassée.
4. **Séparation définition / instance** (`Definition<T>`, `Instance<T>`, `Respawnable`), `Inventory` unique pour joueur et salle, `transfer()` atomique ; `Game::showInventories` expose l'inventaire sans exposer `Player`.
5. **`lookup::findIn`** (id d'instance, id de définition, nom, un mot du nom, casse ignorée) ; `lookup::sortedIds` pour des choix déterministes.

### ❌ Mauvais

1. **🔴 `quests.json` n'est toujours pas lu** (aucun `QuestDef`, aucune progression) ; `QUEST`/`QUESTS` stubs ; les 3 PNJ `quest_giver` répondent par un dialogue aléatoire comme les autres. Le README annonce honnêtement que les quêtes sont « described … but not used ».
2. **Toujours aucun soin ni régénération** — `Stats::heal()` n'est appelé nulle part ; `item.herbs` (objet de la quête « Medicine for the Chapel ») ne soigne pas.
3. **Un joueur AFK reste lié au combat indéfiniment** (le timeout joue un `DEFEND` à chaque tour) *(lecture de code)*, aucune règle de sortie après N timeouts ; **une fuite réussie téléporte vers une sortie au hasard**, possiblement dans le combat d'une autre salle.
4. **Items inépuisables** (réapparition dans la salle d'origine même si l'original est gardé) et **PNJ qui réapparaît dans une salle occupée** sans délai de grâce ; non documenté.
5. **Aucun test automatisé** malgré le design testable ; la section « Testing » du README est vide. `emitPlayerCount` envoie à tous les joueurs à chaque (dé)connexion (O(N)).

---

## `shared/` — `logging/`, `protocol/`, `system/`, `text/`, `defines.hpp`

### ✅ Bon

1. **Réorganisation en sous-dossiers par responsabilité** (`logging/logger.*`, `protocol/{errors,event,line,player_name,response,success}.hpp`, `system/{loadenv,random,signalFd}`, `text/utf8`) : un client sait ce qu'il peut inclure (`protocol/`, `system/signalFd`) sans tirer le logger du serveur. Les Makefiles (`vpath`, `INCLUDES` en `../shared/*/*.hpp`) ont suivi, build propre.
2. **`loadenv` soigné** : ignore commentaires et lignes vides, retire espaces et guillemets, n'accepte que des clés `[A-Za-z0-9_]`, **ne remplace jamais une variable déjà définie** (le shell prime), cherche `.env` dans le dossier courant, le parent et le grand-parent (le programme se lance depuis `server/` ou `clients/cli/`). `getEnvInt` borne et rejette les suites de caractères (`strtol` + `*end != '\0'`).
3. **`LOG_LEVEL` branché** via `logLevelFromName` (insensible à la casse, repli sur `Debug`) — corrige le grief « niveau non configurable » des audits précédents.
4. **Logging JSON structuré** complet (ts + fuseau, niveau, `src` dès WARN, échappement anti-injection, évaluation paresseuse) ; `.env.example` versionné et `.env` ignoré.
5. **`TAP_ERRORS(X)`, `eventLine()`, `shared/protocol/*`** : un seul endroit par famille de texte du protocole.

### ❌ Mauvais

1. **Sortie des logs toujours non configurable** — `Logger::openFile` et `setOutput` ne sont toujours appelés nulle part : pas de fichier de log, pas de redirection (`LOG_FILE` serait la suite logique de `LOG_LEVEL`). La section « Server Logging » du README (formats, niveaux, destinations, surveillance) est un titre vide.
2. **Valeurs invalides ignorées en silence** : `SERVER_PORT=abc` ou `99999` retombe sur 8080 sans le dire — l'utilisateur croit lancer un serveur sur son port et ne le voit qu'au démarrage (`"port":8080` dans le log INFO). Une ligne `WARN` à la lecture serait utile. De même `PORT=8080 # commentaire` (commentaire en fin de ligne) donne une valeur invalide ignorée.
3. **`.env` cherché par chemins relatifs** — depuis un autre dossier que `server/`, `clients/cli/`, la racine ou leurs parents, le fichier n'est pas trouvé et le défaut s'applique sans message ; `PROJECT_ROOT` (compilé) n'est pas réutilisé pour le localiser.
4. **`shared/defines.hpp` mélange toujours** système, protocole, noms de champs JSON, règles de jeu et configuration (`ENV_*`, `MIN_PORT`) ; `INSTANCE_SEPARATOR`, `MAX_NAME_LENGTH`, `PLAYER_INVENTORY_CAPACITY` n'ont rien à y faire pour les clients.
5. **Décalage horaire figé au démarrage** ; **erreurs `409`-`411` hors RFC** (documentées) proches de conventions HTTP sans rapport avec la plage 4xx de la RFC.

---

## `server/Makefile` · `Makefile` (racine) · `.env.example`

### ✅ Bon

1. **`install-deps`** : nlohmann/json avec SHA-256 épinglé, documenté.
2. **`SRCS`/`vpath`/`INCLUDES` à jour** après la réorganisation de `shared/` (`loadenv.cpp` ajouté) ; build propre, **zéro warning**, `-Wall -Wextra -Werror -std=c++20`.
3. **`INCLUDES` par `wildcard`** (y compris `../shared/*/*.hpp`).
4. **Binaires et `.env` ignorés par git**, `.env.example` versionné ; `clean/fclean/re` propagés.
5. **README documente cibles et configuration** (tableau des variables, priorité shell > fichier).

### ❌ Mauvais

1. **Cibles §VII.1 manquantes : `lint`, `run-server`, `run-client`, `run-client-gui`** (le README le reconnaît en `TODO`).
2. **Pas de cible `test`** alors que le moteur de combat est conçu pour être testé.
3. **`all: $(NAME) run`** — `make` build **et lance** le serveur au premier plan ; incompatible avec une CI.
4. **Cible `help` toujours vide.**
5. **Téléchargement réseau au premier `make`**, `PROJECT_ROOT` absolu compilé, `.vscode/c_cpp_properties.json` versionné.

---

## `README.md` — v2 (460 lignes)

### ✅ Bon

1. **Sections remplies et de bonne qualité** : Description, Build (avec Configuration), Architecture (dispatcher, `poll`, couches), Protocol Implementation (commandes, événements, **8 écarts justifiés**), **Combat System** (règles, formule, initiative, `DEFEND`/`FLEE`, diffusion, justification), **World Design** (format JSON, carte ASCII, rôles, items, validation). Les affirmations vérifiées par l'audit (15 salles, retours de toutes les sorties, deux boucles, 7 items obtenables sur 8, 3 rôles) sont exactes.
2. **Rattrapage complet de l'état précédent** : casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411, extensions (`COMBAT`, `SPAWN`), tableau des événements — tout ce que l'audit du jour précédent listait comme périmé est corrigé.
3. **Honnêteté** (`Current status`, `TODO` explicites, « GUI not connected yet »).
4. **Justification de conception** pour le combat (ratio de défense, initiative par `flee`, minuteur de tour) — exactement ce que le §V.1 demande.
5. **Usage de l'IA** documenté pour `Room::findNpc`.

### ❌ Mauvais

1. **🔴 `INVENTORY` et `WHO` encore listés « not implemented yet »** (statut, tableau des commandes, écart n°8 « 6 commands ») alors qu'ils sont livrés ; le sujet exige un README fidèle au code.
2. **🔴 « 15 NPC definitions »** (World Design) : `npc.json` en contient **16** (3 `dialogue_npc`, 3 `quest_giver`, 10 `enemy`) ; le tableau de rôles du README en cite bien 16 noms.
3. **Sections vides exigées par le sujet** : **Server Logging**, **Testing**, **Quest System**, **Group Contributions**, **Resources**, **Instructions** ; « Client interface choice » (§V.3) en `TODO`. Les commentaires HTML du squelette restent dans le fichier livré.
4. **Choix `WHO` (RFC) vs exemple du sujet non expliqué** (voir `protocol/` ❌ 2) ; la grammaire des `EVT … COMBAT ROUND` n'est pas spécifiée.
5. **Section Group Contributions** : trois « Member » prévus alors que seuls deux logins sont cités en tête ; à aligner avec l'historique git avant le rendu.

---

## Corrigés depuis l'audit initial

Audits précédents (2026-10-02 → 2026-10-19) : `CONNECT`, hook de déconnexion, monde, `INCLUDES`, `install-deps`, injection JSON, logging, `QUIT`/`TALK`/`TAKE`/`DROP`/`ATTACK`/`STATUS`, instances, `locate()`, noms UTF-8, casse, respawn, README v1 et écarts, refactors, événements `EVT`, combat tour par tour, `DEFEND`/`FLEE`, diffusion du combat, raté visible, respawn annoncé, documentation du code.

Depuis la mise à jour du matin du 2026-10-19 :
- **`INVENTORY` et `WHO` stubs** → réels (`OK ["item.herbs#3"]`, `OK players=2`). Vérifiés, y compris la casse et l'usage en combat.
- **Port fixé à la compilation** → `SERVER_PORT` (env ou `.env`), bornes [1, 65535].
- **Niveau de log non configurable** → `LOG_LEVEL`. Vérifié : `warn` supprime les lignes INFO.
- **README en retard sur le code** (casse, `EVT`, combat) → README v2 avec Combat System, World Design, Configuration et 8 écarts.
- **`shared/` plat** → `logging/`, `protocol/`, `system/`, `text/`.
- **`Game::take`/`drop` ambigus** → `takeItem`/`dropItem`.
- *(Correction de l'audit précédent : il annonçait « 4 PNJ `quest_giver` » ; `npc.json` en contient 3.)*

## Priorités (ordre suggéré)

1. **README** : retirer `INVENTORY`/`WHO` des stubs, corriger « 15 NPC » → 16, écrire **Server Logging** (format JSON, niveaux, `LOG_LEVEL`, comment surveiller), **Testing**, **Group Contributions**, **Resources**, **Instructions**, trancher « Client interface choice » ; expliquer le choix de `WHO` (RFC) face à l'exemple du sujet.
2. **`CHAT`** (événements `EVT ROOM|GLOBAL|GROUP CHAT`), puis `GROUP` ; supprimer les stubs ou les faire répondre `ERR` en attendant.
3. **Quêtes** : parser `quests.json`, valider les références, `QUEST`/`QUESTS`, récompenses, lien avec les `quest_giver`.
4. **Soin** (`Stats::heal` avec `item.herbs`) et règle de sortie pour un joueur AFK.
5. **Anti-abus et robustesse** : compteurs par IP/joueur, plafonds de connexions et de `_out`, `try/catch` autour des actions et de `tick`, groupage des `send` d'événements.
6. **Logger** : `LOG_FILE`, avertir quand une variable d'environnement est invalide.
7. **Tests automatisés** du moteur de combat (`roll` injecté) et cibles Makefile §VII.1 (`lint`, `run-server`, `run-client`, `run-client-gui`, `test`).
