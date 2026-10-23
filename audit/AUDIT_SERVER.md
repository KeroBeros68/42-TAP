# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-23 — régénéré après le travail **non commité** sur la configuration du serveur (`server/config.{hpp,cpp}`, `ServConfig`, `.env.example`, README : état de l'arbre de travail au moment de l'audit — il compile et passe les tests ci-dessous). Les commits n'ont pas bougé depuis `740e0d4` (2026-10-21). Mises à jour précédentes : 2026-10-21 (anti-abus commité, `LOG_FILE`, soin de salle), 2026-10-20, 2026-10-19. Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (`network/`, `protocol/`, `main.cpp`) + `game/` (`game.cpp`, `combat/`, `events/`, `world/`, `entities/`, `utils/lookup.hpp`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` + `.env.example` + `README.md` (comme documentation de conformité).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -Werror -std=c++20` sans warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` (`SERVER_PORT=18080`). Scénarios de cette mise à jour : 7 clients simultanés avec les défauts puis avec `MAX_SESSIONS_PER_IP=8`, `MAX_COMMANDS_PER_WINDOW=5`, valeurs hors bornes (`0`, `-3`), `COMMAND_WINDOW_MS=100` + `MAX_FLOOD_WINDOWS=1`. Le reste du serveur n'a pas changé depuis l'audit du 2026-10-21 et n'a pas été rejoué. Aucune erreur ASAN/UBSAN. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟢 État général : 13 commandes RFC sur 15, tous les événements de la RFC émis, anti-abus, exceptions, logs en fichier et soin de salle — il ne reste que les quêtes (`QUEST`/`QUESTS`), les tests et la fin du README

Mesuré en conditions réelles :

- ✅ **`CHAT` — nouveau, vérifié** (`3159d1b`) : `CHAT GLOBAL` atteint tous les joueurs, `CHAT ROOM` seulement ceux de la salle (le joueur resté au village ne reçoit pas le message de la place), l'émetteur reçoit aussi son propre `EVT` (conforme à l'exemple du sujet §V.5 : `OK` puis `EVT GLOBAL CHAT alice Hello everyone`). Sont refusés en `ERR 400` : scope inconnu, message vide, caractère de contrôle (`\x01`), commande sans argument ; `CHAT GROUP` sans groupe → `ERR 401 NOT_IN_GROUP`. Limite de 256 octets (`MAX_CHAT_LENGTH`), UTF-8 valide, scope insensible à la casse, autorisé en combat.
- ✅ **Détection et limitation des abus — commité (`c1d92ba`), vérifié** (§V.1 « command flooding, rapid connections », le dernier grand manque du logging) : plus de **20 commandes par seconde** d'un client → les suivantes reçoivent `ERR 400` (vérifié : rafale de 100 → exactement 19 réponses normales + `CONNECT`, 81 `ERR 400`), `WARN command flooding` à chaque fenêtre fautive, et **déconnexion à la 3ᵉ fenêtre consécutive** (`flooding goes on, disconnecting`, vérifié sur un flood de 4 s : coupé après 90 réponses). **64 clients** au plus, **5 par adresse**, **10 connexions par adresse en 10 s** (vérifié : sur 7 connexions simultanées depuis 127.0.0.1, les 5 premières reçoivent `OK hello`, les 2 dernières sont fermées avec `WARN too many clients for one address`). Lecture de **16 Ko max par client et par tour** de boucle (un flood ne monopolise plus le `poll`) et déconnexion d'un client dont les données non envoyées dépassent **64 Ko** *(lecture de code : non atteignable sur loopback, le tampon noyau absorbe plusieurs Mo)*. Seuils en constantes (`server/defines.hpp`), décrits dans le README.
- ✅ **Seuils anti-abus configurables — nouveau, non commité, vérifié** : une structure `ServConfig` (`server/config.*`) lit port, niveau et fichier de log **et les 9 limites** (`MAX_OUT_BUFFER`, `MAX_READ_PER_TURN`, `COMMAND_WINDOW_MS`, `MAX_COMMANDS_PER_WINDOW`, `MAX_FLOOD_WINDOWS`, `MAX_SESSIONS`, `MAX_SESSIONS_PER_IP`, `CONNECT_WINDOW_MS`, `MAX_CONNECTS_PER_WINDOW`) dans l'environnement ou `.env`, chacune bornée (`MIN_*`/`MAX_*` dans `defines.hpp`). Vérifié : défauts → 5 clients locaux acceptés sur 7 ; `MAX_SESSIONS_PER_IP=8` → 7 acceptés ; `MAX_COMMANDS_PER_WINDOW=5` → `CONNECT` + 4 `WHO` passent, 8 `ERR 400` ; `0` ou `-3` → **retour au défaut** (20 commandes/s : 19 réponses + 11 `ERR`) ; `COMMAND_WINDOW_MS=100` + `MAX_FLOOD_WINDOWS=1` → coupure après une fenêtre. Le problème des évaluateurs qui testent depuis une seule machine (limite par adresse, 127.0.0.1 compris) a donc une réponse : relever la valeur dans `.env`. README (tableau des variables) et `.env.example` mis à jour.
- ✅ **Soin par les salles — (`740e0d4`), vérifié** : une salle peut porter un `effect` dans `room.json` (`{"type":"healing","amount":10,"interval_seconds":5}`, aujourd'hui la chapelle). Vérifié : joueur blessé à 95 PV par le loup, marche jusqu'à la chapelle, reçoit `EVT ROOM EFFECT healing 5` (les PV réellement gagnés, plafonnés au maximum) puis `STATUS` → 100/100 ; rien n'est envoyé à pleine santé ; le soin est refusé en combat (`in_combat`). L'effet est validé au chargement (type inconnu, montant ou intervalle ≤ 0 → le serveur refuse de démarrer) et sa description est dans le README. C'est le premier usage de `Stats::heal()`.
- ✅ **Logs dans un fichier — nouveau, vérifié** : `LOG_FILE` (env ou `.env`, `.env.example` mis à jour) écrit chaque ligne JSON, sans couleurs, à la fin du fichier en plus de la console (46 lignes identiques dans les deux sorties) ; un fichier inutilisable (`/nonexistent/x.log`) donne `WARN cannot open the log file` et le serveur continue. **README « Server Logging » rédigé** : format, niveaux, destinations, surveillance (`tail -f … | grep`), détection d'abus.
- ✅ **Exceptions : plus de crash serveur** (`f9ecfb1`, commité) : `runAction` encadre chaque commande (`catch (const std::exception&)` et `catch (...)`) et répond `ERR 500 INTERNAL_ERROR` (documenté README écart 1b) ; la boucle de `main` encadre séparément le tour réseau (`network turn failed`) et le tour de jeu (`game turn failed`) ; le rappel de déconnexion est lui aussi protégé. *(Lecture de code : aucune action ne lève aujourd'hui, donc le chemin n'a pas pu être déclenché en réel.)*
- ✅ **Corrections de protocole** : `QUIT` avant `CONNECT` est accepté (`OK bye`, vérifié) ; la limite de 1024 octets s'applique à **chaque ligne**, avec ou sans `\n` (vérifié : ligne de 2000 octets → `ERR 400` puis déconnexion, la commande suivante n'est pas exécutée) ; les événements sont mis en file puis envoyés **une fois par client et par tour** (`flushAll`).
- ✅ **`GROUP` — vérifié, commité (`17d05bc`)** : `CREATE` → `OK group=group.1` (un second → `402 ALREADY_IN_GROUP`) ; `INVITE <joueur>` réservé au chef (4 joueurs maximum, `MAX_GROUP_SIZE`), l'invité reçoit `EVT GROUP INVITE <chef>` ; `JOIN <nom d'un membre>` → `OK group=group.1` et les membres reçoivent `EVT GROUP JOIN <joueur>` ; `LEAVE` → `EVT GROUP LEAVE <joueur>` aux restants ; `CHAT GROUP` n'arrive qu'aux membres (`EVT GROUP CHAT g1 salut`). Quitter la partie (`QUIT`/coupure) fait quitter le groupe (`removePlayer` → `leaveGroup`). Syntaxe conforme à la RFC §5.3 (`GROUP JOIN <leader-name>`).
- ✅ **11 autres commandes RFC réelles** (+ `DEFEND` et `FLEE`, extensions) : `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, **`INVENTORY`**, **`WHO`**, `ATTACK`, `STATUS`. `INVENTORY` → `OK []` puis `OK ["item.herbs#3"]` après un `TAKE`, puis `OK []` après `DROP` ; autorisée en combat. `WHO` → `OK players=2` avec deux joueurs connectés (conforme à la RFC §5.2.2), insensible à la casse (`who`).
- ✅ **Configuration sans recompilation** — nouveau : `SERVER_PORT`, `SERVER_IP` (clients), `LOG_LEVEL` lus dans l'environnement ou dans `.env` (`shared/system/loadenv.*`, `.env.example` versionné, `.env` ignoré par git). Vérifié : `SERVER_PORT=18080` en variable ; `LOG_LEVEL=warn` supprime bien toutes les lignes INFO (0 sur le log) ; un `.env` à la racine (`SERVER_PORT = "18081"`, avec espaces, guillemets et commentaire) est trouvé depuis `server/` et appliqué ; une valeur invalide (`SERVER_PORT=abc`, `99999`, `LOG_LEVEL=zzz`) retombe silencieusement sur le défaut. La variable d'environnement passe avant le fichier.
- ✅ **Événements `EVT`, combat au tour par tour avec initiative, `DEFEND`/`FLEE`, combat multijoueur, respawn annoncé** (acquis de la mise à jour précédente, code inchangé hors renommages).
- ✅ **README v2** (460 lignes, +255) : sections Combat System (règles, formule, initiative, commandes, diffusion, justification), World Design (format, carte ASCII, rôles, items, validation), Configuration, tableau des 8 écarts à la RFC, tableau des événements. Il corrige tout ce que l'audit précédent relevait (casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411).
- 🔴 **2 stubs restent** : `QUEST` et `QUESTS` répondent `OK Message received` ; le système de quêtes (3 quêtes en données, `quests.json` toujours non lu) n'existe pas. C'est désormais le seul grand manque fonctionnel du §V.1.
- 🔴 **README** : à jour pour `CHAT`/`GROUP`/`INVENTORY`/`WHO`/`ERR 500`, les limites anti-abus, `LOG_FILE`, `EVT ROOM EFFECT` et Server Logging, mais il annonce toujours « 15 NPC definitions » alors que `npc.json` en contient **16** (3 dialogue, 3 quêtes, 10 ennemis — vérifié par script).
- 🔴 **Cinq sections du README restent vides** : Instructions, Quest System, **Testing**, Group Contributions, Resources (et « Client interface choice » en `TODO`). « Server Logging » est désormais rempli.
- 🔴 **`log.txt` (131 lignes d'une session de développement, port 4563) est versionné** par le commit `740e0d4` et non ignoré par `.gitignore` : fichier généré à ne pas livrer (et à ajouter à l'ignore, ou à remplacer par `LOG_FILE=server.log` ignoré).

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 13/15 réelles ; 2 stubs (`QUEST`, `QUESTS`) ; +`DEFEND`, `FLEE` |
| Événements `EVT` ROOM (PRESENCE) / STATS | ✅ |
| Événements `EVT` CHAT (ROOM / GLOBAL / GROUP) et GROUP (INVITE / JOIN / LEAVE) | ✅ |
| Commandes insensibles à la casse (RFC §4.2) | ✅ |
| Chargement monde JSON + validation des références | ✅ ; ❌ `quests.json` jamais lu |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items, ≥ 2 quêtes | ✅ données (15 salles, 2 boucles, 16 PNJ, 8 items) — quêtes non exploitées |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots, inventaire visible | ✅ ; ⚠️ ids `#N` hors RFC (documenté) |
| Respawn PNJ / items | ✅ annoncé par `EVT` |
| Combat : 100 HP, dégâts, `STATUS`, respawn joueur, tour par tour, initiative, `DEFEND`/`FLEE`, diffusion | ✅ |
| Quêtes (conception, validation, récompenses) | ❌ |
| Déconnexion : état retiré **avant** le broadcast de départ | ✅ |
| Logging : connexions/IP, commandes, réponses, monde, combat, JSON, niveaux, timestamps | ✅ ; niveau configurable ✅ |
| Logging : détection d'abus (flood, connexions rapides) | ✅ |
| Logging : fichier de sortie configurable (`LOG_FILE`) | ✅ |
| Logging : événements de quêtes | ❌ (pas de quêtes) |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ; `lint`, `run-server`, `run-client`, `run-client-gui` absents |
| README (§VI) | ⚠️ 6 sections remplies sur 12 ; 1 affirmation périmée (15 PNJ) |

---

## `server/network/` — `server.cpp`, `connection.*`, `user.*`

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes**, une seule boucle dans `main` : réseau → temps de jeu → livraison des événements. Aucune course possible, aucun thread pour les timers de tour ni de respawn. Port d'écoute lu dans l'environnement (`getEnvInt(ENV_PORT, SERVER_PORT, MIN_PORT, MAX_PORT)`), validé dans [1, 65535].
2. **Ordre réponse → événements respecté** : l'auteur d'une commande reçoit sa réponse avant les `EVT` qu'elle déclenche.
3. **Hook de déconnexion appelé avant `close`/`erase`** (§IV) : retrait du joueur, abandon du combat, `LEAVE`, `STATS` avant la destruction de la session.
4. **Framing et constantes centralisés** (`shared/protocol/line.hpp`, `replyError`, `setNonBlocking`, `utf8SafeLength`), fichiers documentés (bandeaux, `@param/@return`).
5. **Journalisation de chaque étape réseau**, désormais filtrable par `LOG_LEVEL` (le `DEBUG` par `sendTo`/socket disparaît en `info`).

### ❌ Mauvais

1. **Refus de connexion silencieux** : un client refusé (trop de clients, adresse saturée, reconnexions trop rapides) est fermé **sans `OK hello` ni `ERR`** — il ne peut pas distinguer la protection d'une panne réseau. De même, un flood est répondu par `ERR 400 BAD_REQUEST`, le même code qu'une erreur de syntaxe : aucun code ni message « slow down » (le README le décrit mais ne le justifie pas parmi ses écarts).
2. **Valeurs de configuration invalides ignorées en silence** : `MAX_COMMANDS_PER_WINDOW=0` ou `abc` retombe sur 20 sans un seul `WARN` (vérifié : 0 ligne de log), comme `SERVER_PORT=abc` ; l'opérateur croit avoir désactivé ou durci une limite alors que le défaut s'applique. Les bornes sont larges (`MAX_COUNT` = 10 000, `MAX_OUT_BUFFER` jusqu'à 16 Mo) : `MAX_COMMANDS_PER_WINDOW=10000` désactive de fait la protection, et aucune valeur ne la désactive explicitement. La limite par adresse s'applique toujours aussi à `127.0.0.1` (réglable, mais par défaut 5).
3. **Compteur de flood par session, remis à zéro à la reconnexion** : un client coupé pour flood peut revenir tout de suite (dans la limite de 10 connexions / 10 s) ; aucune sanction par adresse. Les 20 commandes/s comptent aussi les commandes invalides et `CONNECT`.
4. **Coût des contrôles** *(lecture de code)* : `allowConnection` parcourt toutes les sessions pour compter l'adresse et purge toute la table `_recent_connects` à **chaque** `accept` — O(N + M) par connexion, sans conséquence à 64 clients.
5. **Quelques arêtes restantes** : `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide ; ids de session recyclés ; la limite `MAX_OUT_BUFFER` n'est vérifiée qu'à chaque tour (un gros `LOOK` peut la dépasser d'un coup) ; `server.hpp` inclut ~15 en-têtes.

> Point mineur : `handeLine` (faute de frappe).

---

## `server/protocol/` — `commands.cpp`, `serializer.cpp` (+ `main.cpp`)

### ✅ Bon

1. **Une fonction `cmdX` par commande, toutes documentées** ; `combatResponse()` factorise `ATTACK`/`DEFEND`/`FLEE`. Les deux nouvelles commandes (`cmdInventory`, `cmdWho`) suivent le même gabarit, et précisent « allowed during a fight ».
2. **`INVENTORY` renvoie un tableau JSON d'ids** (`serializeInventory`, via `nlohmann`, donc échappé et UTF-8 sûr) ; **`WHO` utilise `TapOk::PLAYERS`** (`OK players=<n>`), le format exact de la RFC §5.2.2.
3. **Codes d'erreur précis par cas** (`403`, `404`, `405`, `301`, `409`-`411`), vérifiés lors des mises à jour précédentes.
4. **Un seul endroit qui écrit le JSON de réponse**, champs en constantes `FIELD_*` ; `ATTACK`/`STATUS` étendent la RFC sans en retirer de champ (écart n°5 du README).
5. **`main.cpp`** : construit un `ServConfig` (port, log, limites) lu en un seul endroit puis le passe au `Server` (`Server(const ServConfig&)`) — plus de `getEnv` dispersés ; `try/catch` global (`fatal error`, code 1) **et** par tour (`network turn failed`, `game turn failed`) — une erreur de jeu ne tue plus le serveur.

### ❌ Mauvais

1. **🔴 2 stubs `OK Message received`** : `QUEST`, `QUESTS`. Un client croit que l'action a réussi.
2. **`GROUP` : les erreurs de groupe sont écrasées en `ERR 400`** (`cmdGroup`, `default:`) — non-chef qui invite (`NOT_LEADER`), groupe plein, invitation inexistante, invitation à soi-même, joueur inconnu : tout donne `400 BAD_REQUEST`, indiscernable d'une faute de syntaxe. La RFC n'a que `401`/`402` pour les groupes, mais un client ne sait pas pourquoi `GROUP JOIN` échoue (vérifié : `JOIN` sans invitation → `400`). À documenter comme écart, ou ajouter des codes dédiés.
3. **`CHAT` : le texte est journalisé en `DEBUG`** (`chat text`), donc visible dans les logs de développement ; et le premier espace seul sépare le scope du message (`chat room  é ü` envoie `" é ü"` avec un espace en tête).
4. **🔴 `WHO` : le sujet et la RFC divergent, et le choix n'est pas documenté** — le sujet (§V.5) montre `OK { "room": ["alice","bob"], "server": 5 }`, la RFC (§5.2.2) montre `OK players=<count>`. Le serveur suit la RFC, ce qui est défendable (la RFC prime), mais un GUI qui suivrait l'exemple du sujet n'y trouverait ni la liste de la salle ni le total. À mentionner dans les écarts/choix du README.
5. **Ids d'instance `#N` sur le fil et grammaire des `EVT … COMBAT ROUND` ambiguë** (`item.herbs#3`) — documenté (écart n°6) mais hors RFC §5.4.1 ; `INVENTORY` et les `EVT … COMBAT` les propagent aussi. Le format `… npc.wolf#11 npc.wolf#11 1 attack cara miss` répète acteur et ennemi et mélange nombre et mots dans le dernier champ ; le README donne le tableau, pas de grammaire ABNF.

> Point mineur : aucune validation des arguments inutiles (`INVENTORY x`, `LOOK foo`, `QUIT bar` acceptés) ; `MOVE` sans argument → `301` plutôt que `400`.

---

## `game/group/` — `group_manager.*` · `shared/protocol/chat.hpp`

### ✅ Bon

1. **`GroupManager` pur et sans réseau** : travaille sur des ids de joueurs, retourne un `GroupResult` (statut, id du groupe, autres membres à prévenir) ; `Game` en tire les `EVT`. Même schéma que le combat, testable seul.
2. **Modèle d'invitation cohérent** : l'invité ne rejoint que si une invitation de ce groupe existe (`NO_INVITATION` sinon), le chef est le premier de la liste et la tête passe automatiquement au membre suivant s'il part ; les invitations d'un chef parti ou d'un groupe vidé sont annulées ; un groupe vide est supprimé.
3. **Bornes vérifiées** : `MAX_GROUP_SIZE` = 4 à l'invitation **et** à l'entrée (deux invités simultanés ne dépassent pas la limite), pas d'auto-invitation, pas de double appartenance (`402`).
4. **Nettoyage à la déconnexion** (`removePlayer` → `leaveGroup` → `EVT GROUP LEAVE`) : vérifié, les membres restants sont prévenus.
5. **`chat.hpp` : `isValidChatMessage`** (non vide, ≤ 256 octets, UTF-8 sans contrôle) partagé avec les futurs clients ; ids de groupes `group.<n>` lisibles.

### ❌ Mauvais

1. **Erreurs de groupe indistinctes côté client** (voir `protocol/` ❌ 2) : `NOT_LEADER`, `GROUP_FULL`, `NO_INVITATION`, `SELF` → `ERR 400`.
2. **Invitations sans expiration** *(lecture de code)* : elles restent valables jusqu'à usage ou départ du chef ; une seconde invitation d'un autre groupe écrase la première sans prévenir l'invité.
3. **Recherche du joueur par nom dupliquée** dans `Game::groupInvite` et `Game::groupJoin` (même boucle O(N) recopiée) ; `GroupReply.unknown_player` est un drapeau à côté d'un statut, au lieu d'un statut `UNKNOWN_PLAYER`.
4. **Un groupe d'un seul membre persiste** (chef seul après départ des autres) : pas gênant, mais `GROUP CREATE` de ce chef donne `402`, il doit d'abord faire `LEAVE`.
5. **Aucun test automatisé** du `GroupManager`, alors que son interface (ids, statuts) s'y prête.

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
2. **Soin limité à un lieu** : seule la chapelle soigne (`effect` dans `room.json`) ; `item.herbs` (objet de la quête « Medicine for the Chapel ») ne soigne toujours pas, il n'existe pas de commande d'utilisation d'objet, et un joueur à 5 PV loin de la chapelle ne peut que s'y rendre — or la chapelle est aussi le point de respawn. Le soin n'a lieu qu'entre deux combats et un PNJ blessé qui n'est pas tué ne guérit pas.
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

1. **`LOG_FILE` : pas de rotation ni de limite de taille**, `std::endl` (flush) à chaque ligne dans la console et dans le fichier, `args` des commandes non tronqués (une ligne de 1024 octets est journalisée en entier) ; le fichier est ouvert en ajout, donc il grossit indéfiniment. Les chemins relatifs (`LOG_FILE=server.log`) dépendent du dossier courant.
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

## `README.md` — v2 (520 lignes)

### ✅ Bon

1. **Sections remplies et de bonne qualité** : Description, Build (avec Configuration), Architecture (dispatcher, `poll`, couches), Protocol Implementation (commandes, événements, **8 écarts justifiés**), **Combat System** (règles, formule, initiative, `DEFEND`/`FLEE`, diffusion, justification), **World Design** (format JSON, carte ASCII, rôles, items, validation). Les affirmations vérifiées par l'audit (15 salles, retours de toutes les sorties, deux boucles, 7 items obtenables sur 8, 3 rôles) sont exactes.
2. **Rattrapage complet de l'état précédent** : casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411, extensions (`COMBAT`, `SPAWN`), tableau des événements — tout ce que l'audit du jour précédent listait comme périmé est corrigé.
3. **Honnêteté** (`Current status`, `TODO` explicites, « GUI not connected yet »).
4. **Justification de conception** pour le combat (ratio de défense, initiative par `flee`, minuteur de tour) — exactement ce que le §V.1 demande.
5. **Usage de l'IA** documenté pour `Room::findNpc`.

### ❌ Mauvais

1. **L'arbre de travail a rattrapé le code** : `INVENTORY`, `WHO`, `CHAT` et `GROUP` sont déclarés implémentés (statut, tableau des commandes, événements `GLOBAL|ROOM CHAT` / `GROUP INVITE|JOIN|LEAVE|CHAT`, écart n°8 réduit à `QUEST`/`QUESTS`). Mais ces modifications du README ne sont **pas commitées** non plus, et il manque la description de la syntaxe `GROUP` dans le tableau des commandes.
2. **🔴 « 15 NPC definitions » (toujours là)** (World Design) : `npc.json` en contient **16** (3 `dialogue_npc`, 3 `quest_giver`, 10 `enemy`) ; le tableau de rôles du README en cite bien 16 noms.
3. **Sections vides exigées par le sujet** : **Testing**, **Quest System**, **Group Contributions**, **Resources**, **Instructions** ; « Client interface choice » (§V.3) en `TODO`. Les commentaires HTML du squelette restent dans le fichier livré. (« Server Logging » est maintenant complet : format, niveaux, destinations, surveillance, abus.)
4. **Choix `WHO` (RFC) vs exemple du sujet non expliqué** (voir `protocol/` ❌ 2) ; la grammaire des `EVT … COMBAT ROUND` n'est pas spécifiée.
5. **Section Group Contributions** : trois « Member » prévus alors que seuls deux logins sont cités en tête ; à aligner avec l'historique git avant le rendu.

---

## Corrigés depuis l'audit initial

Audits précédents (2026-10-02 → 2026-10-19) : `CONNECT`, hook de déconnexion, monde, `INCLUDES`, `install-deps`, injection JSON, logging, `QUIT`/`TALK`/`TAKE`/`DROP`/`ATTACK`/`STATUS`, instances, `locate()`, noms UTF-8, casse, respawn, README v1 et écarts, refactors, événements `EVT`, combat tour par tour, `DEFEND`/`FLEE`, diffusion du combat, raté visible, respawn annoncé, documentation du code.

Depuis le 2026-10-21 :
- **Seuils anti-abus figés dans le code** → `ServConfig` + variables d'environnement bornées (non commité). Vérifié.

Depuis le 2026-10-20 (soir) :
- **Anti-abus non commité** → commité (`c1d92ba`). **Sortie des logs non configurable** → `LOG_FILE`, vérifié (y compris le cas d'un fichier inutilisable). **Section README « Server Logging » vide** → rédigée.
- **Aucun soin** (`Stats::heal` jamais appelé) → effet de salle `healing` (`740e0d4`), vérifié : `EVT ROOM EFFECT healing 5`.

Depuis la mise à jour du 2026-10-20 (chat) :
- **Aucune détection d'abus** → limite de débit, de clients, d'adresses, de reconnexions, de lecture et de tampon de sortie ; avertissements et déconnexion (non commité). Vérifié.
- **Exception dans une action = crash** → `runAction`, `ERR 500 INTERNAL_ERROR`, `try/catch` par tour (`f9ecfb1`).
- **`sendToArray` flush par destinataire** → `flushAll` une fois par tour.
- **`QUIT` avant `CONNECT` refusé** → accepté. **Ligne longue acceptée si elle finit par `\n`** → refusée et déconnexion.
- **`GROUP`** commité (`17d05bc`).

Depuis la mise à jour du matin du 2026-10-19 :
- **`CHAT` stub** → `CHAT GLOBAL|ROOM` réels, avec validation du message (`chat.hpp`) et `EVT GLOBAL|ROOM CHAT` ; **`GROUP` stub** → `CREATE|INVITE|JOIN|LEAVE` + `EVT GROUP …` + `CHAT GROUP` (non commité). Vérifiés.
- **`INVENTORY` et `WHO` stubs** → réels (`OK ["item.herbs#3"]`, `OK players=2`). Vérifiés, y compris la casse et l'usage en combat.
- **Port fixé à la compilation** → `SERVER_PORT` (env ou `.env`), bornes [1, 65535].
- **Niveau de log non configurable** → `LOG_LEVEL`. Vérifié : `warn` supprime les lignes INFO.
- **README en retard sur le code** (casse, `EVT`, combat) → README v2 avec Combat System, World Design, Configuration et 8 écarts.
- **`shared/` plat** → `logging/`, `protocol/`, `system/`, `text/`.
- **`Game::take`/`drop` ambigus** → `takeItem`/`dropItem`.
- *(Correction de l'audit précédent : il annonçait « 4 PNJ `quest_giver` » ; `npc.json` en contient 3.)*

## Priorités (ordre suggéré)

1. **README** : corriger « 15 NPC » → 16  écrire **Testing**, **Group Contributions**, **Resources**, **Instructions**, trancher « Client interface choice » ; expliquer le choix de `WHO` (RFC) face à l'exemple du sujet.
2. **Commiter `ServConfig`**, avertir quand une valeur de configuration est rejetée (port, niveau, limites) et répondre explicitement à un client refusé ; **distinguer les erreurs de `GROUP`** (`NOT_LEADER`, `GROUP_FULL`, `NO_INVITATION`) plutôt que de tout renvoyer en `400` ; supprimer les stubs `QUEST`/`QUESTS` ou les faire répondre `ERR` en attendant.
3. **Quêtes** : parser `quests.json`, valider les références, `QUEST`/`QUESTS`, récompenses, lien avec les `quest_giver`.
4. **Usage d'objets** (`item.herbs` soigne) et règle de sortie pour un joueur AFK ; **retirer `log.txt` du dépôt** et l'ajouter au `.gitignore`.
5. **Robustesse restante** : tests de charge réels (plafond de `_out`, `MAX_READ_PER_TURN`), `EMFILE`, cohérence des codes d'erreur (flood, refus).
6. **Logger** : avertir quand une variable d'environnement est invalide ; rotation ou taille maximale du fichier.
7. **Tests automatisés** du moteur de combat (`roll` injecté) et cibles Makefile §VII.1 (`lint`, `run-server`, `run-client`, `run-client-gui`, `test`).
