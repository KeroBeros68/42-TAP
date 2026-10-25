# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-25 — régénéré après `0363f87` (avertissements de configuration, `ERR 429`/`503`, `0` pour désactiver une limite : commités) et les commits `9e462bf` / `664b515` (**système de quêtes** et commande `USE`). L'arbre de travail est propre. Mises à jour précédentes : 2026-10-23 (deux), 2026-10-21, 2026-10-20, 2026-10-19. Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (`network/`, `protocol/`, `main.cpp`) + `game/` (`game.cpp`, `combat/`, `events/`, `world/`, `entities/`, `utils/lookup.hpp`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` + `.env.example` + `README.md` (comme documentation de conformité).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -Werror -std=c++20` sans warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` (`SERVER_PORT=18080`). Scénarios de cette mise à jour : quête de livraison complète (`QUEST` → `TAKE` → `QUESTS` → rendu à la prêtresse), quête `defeat` (3 rats, rendu au tavernier), relance d'une quête terminée, `USE` (charme, objet absent, sans argument, pain à pleine santé), `QUEST` sans cible / PNJ inconnu. Le reste du serveur n'a pas changé de comportement depuis le 2026-10-23 et n'a pas été rejoué. Aucune erreur ASAN/UBSAN. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟢 État général : **les 15 commandes de la RFC sont implémentées** (+ `DEFEND`, `FLEE`, `USE`), tous les événements, trois quêtes jouables, anti-abus configurable — il reste les tests automatisés et la fin du README

Mesuré en conditions réelles :

- ✅ **`CHAT` — nouveau, vérifié** (`3159d1b`) : `CHAT GLOBAL` atteint tous les joueurs, `CHAT ROOM` seulement ceux de la salle (le joueur resté au village ne reçoit pas le message de la place), l'émetteur reçoit aussi son propre `EVT` (conforme à l'exemple du sujet §V.5 : `OK` puis `EVT GLOBAL CHAT alice Hello everyone`). Sont refusés en `ERR 400` : scope inconnu, message vide, caractère de contrôle (`\x01`), commande sans argument ; `CHAT GROUP` sans groupe → `ERR 401 NOT_IN_GROUP`. Limite de 256 octets (`MAX_CHAT_LENGTH`), UTF-8 valide, scope insensible à la casse, autorisé en combat.
- ✅ **Détection et limitation des abus — commité (`c1d92ba`), vérifié** (§V.1 « command flooding, rapid connections », le dernier grand manque du logging) : plus de **20 commandes par seconde** d'un client → les suivantes reçoivent `ERR 400` (vérifié : rafale de 100 → exactement 19 réponses normales + `CONNECT`, 81 `ERR 400`), `WARN command flooding` à chaque fenêtre fautive, et **déconnexion à la 3ᵉ fenêtre consécutive** (`flooding goes on, disconnecting`, vérifié sur un flood de 4 s : coupé après 90 réponses). **64 clients** au plus, **5 par adresse**, **10 connexions par adresse en 10 s** (vérifié : sur 7 connexions simultanées depuis 127.0.0.1, les 5 premières reçoivent `OK hello`, les 2 dernières sont fermées avec `WARN too many clients for one address`). Lecture de **16 Ko max par client et par tour** de boucle (un flood ne monopolise plus le `poll`) et déconnexion d'un client dont les données non envoyées dépassent **64 Ko** *(lecture de code : non atteignable sur loopback, le tampon noyau absorbe plusieurs Mo)*. Seuils en constantes (`server/defines.hpp`), décrits dans le README.
- ✅ **Seuils anti-abus configurables (`26d9ae5`) et rendus sûrs — vérifié** : `ServConfig` (`server/config.*`) lit port, niveau et fichier de log **et les 9 limites** dans l'environnement ou `.env`, bornées. **Nouveau (non commité)** : (a) un réglage **non numérique ou hors bornes est ignoré avec un `WARN invalid setting ignored, default used`** qui nomme la variable et la valeur (vérifié : `MAX_SESSIONS=abc`, `MAX_FLOOD_WINDOWS=0`, `COMMAND_WINDOW_MS=5`, `MAX_OUT_BUFFER=0` tous signalés) ; (b) **`0` désactive** la limite de commandes, de clients ou de connexions (vérifié : `MAX_COMMANDS_PER_WINDOW=0` → 200 `WHO` sur 200 répondus ; `MAX_SESSIONS_PER_IP=0` → 8 clients locaux acceptés), sauf la limite en octets, qui ne peut pas être coupée ; (c) **un client refusé reçoit `ERR 503 SERVER_BUSY`** à la place du greeting avant la fermeture (vérifié sur les clients 5 et 7 d'un test à 7 connexions), et un flood reçoit **`ERR 429 RATE_LIMITED`** au lieu de `400` (vérifié : 11 réponses `429` sur une rafale de 30). Le tout est documenté dans le README (écart n°1c, tableau des messages de log) et `.env.example`.
- ✅ **Soin par les salles — (`740e0d4`), vérifié** : une salle peut porter un `effect` dans `room.json` (`{"type":"healing","amount":10,"interval_seconds":5}`, aujourd'hui la chapelle). Vérifié : joueur blessé à 95 PV par le loup, marche jusqu'à la chapelle, reçoit `EVT ROOM EFFECT healing 5` (les PV réellement gagnés, plafonnés au maximum) puis `STATUS` → 100/100 ; rien n'est envoyé à pleine santé ; le soin est refusé en combat (`in_combat`). L'effet est validé au chargement (type inconnu, montant ou intervalle ≤ 0 → le serveur refuse de démarrer) et sa description est dans le README. C'est le premier usage de `Stats::heal()`.
- ✅ **Logs dans un fichier — nouveau, vérifié** : `LOG_FILE` (env ou `.env`, `.env.example` mis à jour) écrit chaque ligne JSON, sans couleurs, à la fin du fichier en plus de la console (46 lignes identiques dans les deux sorties) ; un fichier inutilisable (`/nonexistent/x.log`) donne `WARN cannot open the log file` et le serveur continue. **README « Server Logging » rédigé** : format, niveaux, destinations, surveillance (`tail -f … | grep`), détection d'abus.
- ✅ **Exceptions : plus de crash serveur** (`f9ecfb1`, commité) : `runAction` encadre chaque commande (`catch (const std::exception&)` et `catch (...)`) et répond `ERR 500 INTERNAL_ERROR` (documenté README écart 1b) ; la boucle de `main` encadre séparément le tour réseau (`network turn failed`) et le tour de jeu (`game turn failed`) ; le rappel de déconnexion est lui aussi protégé. *(Lecture de code : aucune action ne lève aujourd'hui, donc le chemin n'a pas pu être déclenché en réel.)*
- ✅ **Corrections de protocole** : `QUIT` avant `CONNECT` est accepté (`OK bye`, vérifié) ; la limite de 1024 octets s'applique à **chaque ligne**, avec ou sans `\n` (vérifié : ligne de 2000 octets → `ERR 400` puis déconnexion, la commande suivante n'est pas exécutée) ; les événements sont mis en file puis envoyés **une fois par client et par tour** (`flushAll`).
- ✅ **`GROUP` — vérifié, commité (`17d05bc`)** : `CREATE` → `OK group=group.1` (un second → `402 ALREADY_IN_GROUP`) ; `INVITE <joueur>` réservé au chef (4 joueurs maximum, `MAX_GROUP_SIZE`), l'invité reçoit `EVT GROUP INVITE <chef>` ; `JOIN <nom d'un membre>` → `OK group=group.1` et les membres reçoivent `EVT GROUP JOIN <joueur>` ; `LEAVE` → `EVT GROUP LEAVE <joueur>` aux restants ; `CHAT GROUP` n'arrive qu'aux membres (`EVT GROUP CHAT g1 salut`). Quitter la partie (`QUIT`/coupure) fait quitter le groupe (`removePlayer` → `leaveGroup`). Syntaxe conforme à la RFC §5.3 (`GROUP JOIN <leader-name>`).
- ✅ **11 autres commandes RFC réelles** (+ `DEFEND` et `FLEE`, extensions) : `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, **`INVENTORY`**, **`WHO`**, `ATTACK`, `STATUS`. `INVENTORY` → `OK []` puis `OK ["item.herbs#3"]` après un `TAKE`, puis `OK []` après `DROP` ; autorisée en combat. `WHO` → `OK players=2` avec deux joueurs connectés (conforme à la RFC §5.2.2), insensible à la casse (`who`).
- ✅ **Configuration sans recompilation** — nouveau : `SERVER_PORT`, `SERVER_IP` (clients), `LOG_LEVEL` lus dans l'environnement ou dans `.env` (`shared/system/loadenv.*`, `.env.example` versionné, `.env` ignoré par git). Vérifié : `SERVER_PORT=18080` en variable ; `LOG_LEVEL=warn` supprime bien toutes les lignes INFO (0 sur le log) ; un `.env` à la racine (`SERVER_PORT = "18081"`, avec espaces, guillemets et commentaire) est trouvé depuis `server/` et appliqué ; une valeur invalide (`SERVER_PORT=abc`, `99999`, `LOG_LEVEL=zzz`) retombe silencieusement sur le défaut. La variable d'environnement passe avant le fichier.
- ✅ **Événements `EVT`, combat au tour par tour avec initiative, `DEFEND`/`FLEE`, combat multijoueur, respawn annoncé** (acquis de la mise à jour précédente, code inchangé hors renommages).
- ✅ **README v2** (460 lignes, +255) : sections Combat System (règles, formule, initiative, commandes, diffusion, justification), World Design (format, carte ASCII, rôles, items, validation), Configuration, tableau des 8 écarts à la RFC, tableau des événements. Il corrige tout ce que l'audit précédent relevait (casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411).
- ✅ **Système de quêtes — nouveau (`9e462bf`, `664b515`), vérifié** (§V.1 « design quest progression, completion validation, reward »). `QUEST <pnj>` donne la quête que ce PNJ propose (`status: active`) ou, à son PNJ de rendu, la valide ; `QUESTS` liste les quêtes avec leur progression. Trois types : `deliver` (apporter un objet à un autre PNJ), `fetch` (rapporter un objet), `defeat` (tuer N ennemis). Vérifié : *Medicine for the Chapel* — `QUEST guard` (`active`), seconde demande → `ERR 406 NO_QUEST_AVAILABLE`, `QUESTS` passe de `0/1` à `1/1` après `TAKE herbs`, `QUEST priestess` → `completed` + `text_done`, l'herbe est retirée de l'inventaire et `item.charm#4` y apparaît, relancer → `406` ; *Cellar Pests* — `defeat` de 3 rats, `QUESTS` indique `3/3`, rendu au tavernier → `completed`, récompense (pain + 20 PV). Validation au démarrage (PNJ, cible, récompenses existent, `count ≥ 1`, `heal ≥ 0`), refus de rendre une quête si la récompense ne rentre pas dans l'inventaire (`403`, rien n'est pris), une quête ne se fait qu'une fois, journalisation `quest started/progress/completed`. **README « Quest System » rédigé** (format, progression, validation, récompenses, liste) et écart n°8 documenté (réponse de `QUEST` avec `reward` en objet et `progress`).
- ✅ **`USE <objet>` — nouveau, hors RFC, vérifié** : consomme un objet doté d'un `effect` (`healing`, pain 15 PV, herbes 30 PV) et répond `OK healed=<PV>` ; `USE charm` → `ERR 407 ITEM_NOT_USABLE`, objet absent → `404 ITEM_NOT_IN_INVENTORY`, sans argument → `400`, refusé en combat (`409`). Les herbes soignent enfin.
- ✅ **Il ne reste plus aucun stub** (`cmdStub` supprimé).
- 🔴 **README** : à jour pour `CHAT`/`GROUP`/`INVENTORY`/`WHO`/`ERR 500`, les limites anti-abus, `LOG_FILE`, `EVT ROOM EFFECT` et Server Logging, mais il annonce toujours « 15 NPC definitions » alors que `npc.json` en contient **16** (3 dialogue, 3 quêtes, 10 ennemis — vérifié par script).
- 🔴 **Cinq sections du README restent vides** : Instructions, Quest System, **Testing**, Group Contributions, Resources (et « Client interface choice » en `TODO`). « Server Logging » est désormais rempli.
- 🔴 **`log.txt` (131 lignes d'une session de développement, port 4563) est versionné** par le commit `740e0d4` et non ignoré par `.gitignore` : fichier généré à ne pas livrer (et à ajouter à l'ignore, ou à remplacer par `LOG_FILE=server.log` ignoré).

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ✅ 15/15 ; +`DEFEND`, `FLEE`, `USE` hors RFC |
| Événements `EVT` ROOM (PRESENCE) / STATS | ✅ |
| Événements `EVT` CHAT (ROOM / GLOBAL / GROUP) et GROUP (INVITE / JOIN / LEAVE) | ✅ |
| Commandes insensibles à la casse (RFC §4.2) | ✅ |
| Chargement monde JSON + validation des références | ✅ (salles, PNJ, items, spawns, effets, quêtes) |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items, ≥ 2 quêtes | ✅ données (15 salles, 2 boucles, 16 PNJ, 8 items) — quêtes non exploitées |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots, inventaire visible | ✅ ; ⚠️ ids `#N` hors RFC (documenté) |
| Respawn PNJ / items | ✅ annoncé par `EVT` |
| Combat : 100 HP, dégâts, `STATUS`, respawn joueur, tour par tour, initiative, `DEFEND`/`FLEE`, diffusion | ✅ |
| Quêtes (conception, validation, récompenses) | ✅ 3 quêtes jouables |
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

1. **Les nouveaux codes `429` et `503` s'ajoutent aux extensions hors RFC** (documentés, écart n°1c), et le refus de connexion est envoyé avec `MSG_DONTWAIT` sans vérifier l'écriture *(lecture de code)* : sur une socket saturée le message peut être perdu, ce qui retombe sur l'ancien comportement (fermeture sans explication).
2. **Bornes encore larges** : `MAX_COUNT` = 10 000 et `MAX_OUT_BUFFER` jusqu'à 16 Mo ; `MAX_FLOOD_WINDOWS` ≥ 1 mais sans moyen de couper le flood tout de suite, et `0` désactive aussi bien la limite par adresse sur `127.0.0.1` que sur une adresse publique (il n'existe pas de réglage « loopback uniquement »).
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

1. **Extensions hors RFC en plus** : `USE`, `ERR 407 ITEM_NOT_USABLE`, `OK healed=<n>` ; la réponse de `QUEST` diffère de l'exemple de la RFC (`reward` est un objet `{items, heal}` et non une chaîne, `status` vaut `active` et non `available`, champs `progress` et `text_done`) — l'écart n°8 du README le documente, mais un client strict qui lit `reward` comme une chaîne échouera.
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

## `game/quests/` — `quest.*`, `quest_manager.*` · `server/protocol/` (QUEST, QUESTS, USE)

### ✅ Bon

1. **Même architecture que les PNJ et les items** : `QuestDef` hérite de `Definition` et vit dans une `Database` ; `QuestProgress` (état + compteur de morts) par joueur dans un `QuestManager` sans réseau, que `Game` interroge. Le serveur refuse de démarrer sur une quête invalide.
2. **Les trois types demandés par le sujet** (« fetch item, defeat NPC, deliver item ») avec un seul modèle de données JSON ; texte de début et de fin dans le fichier, récompense en objets et/ou PV.
3. **Transaction sûre au rendu** : la quête n'est validée que si l'objectif est atteint **et** si la récompense tient dans l'inventaire (10 objets) ; sinon `ERR 403 INVENTORY_FULL` et **rien n'est pris ni donné**. Les objets de livraison sont retirés puis la récompense créée.
4. **Crédit de `defeat` pour tous les participants du combat** (`participants` passé à `enemyDied`) : un combat partagé fait avancer la quête de chaque joueur qui l'a en cours — adapté au multijoueur du combat.
5. **Un seul verbe pour donner et rendre** (`QUEST <pnj>`) et `QUESTS` autorisé en combat ; codes d'erreur précis (`404`, `406`, `403`, `409`), journal `quest started/progress/completed`.

### ❌ Mauvais

1. **Aucune vérification d'origine de l'objet** : `deliver`/`fetch` comptent les objets portés au moment du rendu, pas ceux ramassés pendant la quête ; un objet reçu d'un autre joueur (ou une herbe de la salle qui réapparaît) fait l'affaire, et un objet déposé fait repasser la progression à `0/1`.
2. **Pas de prérequis ni de chaîne de quêtes** (la RFC §6.1.2 cite « Quest dependencies ») et `offeredBy` donne la première quête non commencée par ordre d'identifiant : si un PNJ en propose plusieurs, l'ordre dépend de l'alphabet des ids.
3. **Récompense limitée** : objets et PV uniquement (pas d'or, d'équipement ni de statistiques) ; `heal: 0` est un champ par défaut explicite dans le JSON mais un simple `0` ignoré.
4. **Désignation du PNJ par nom partiel impossible** : `QUEST tavern keeper` → `NPC_NOT_FOUND` (vérifié : le nom complet « Tavern Keeper Bronn », un seul mot du nom comme `bronn`, ou l'id sont acceptés, mais pas une partie de plusieurs mots) — piège pour un utilisateur du CLI, qui n'a aucune aide.
5. **Aucun événement ni notification de quête** : le joueur n'apprend l'avancement d'un `defeat` qu'en appelant `QUESTS` ; aucun `EVT` « quest progress » ; pas de test automatisé du `QuestManager`.

---

## `game/combat/` · `game/game.cpp` · `game/events/` · `game/world/` · `game/entities/` · `game/utils/lookup.hpp`

### ✅ Bon

1. **Combat : moteur pur (`combat.cpp`, `roll` injectable) / orchestration (`CombatManager`) / interface (`CombatHost`)** — testable sans réseau ni horloge. Rounds, initiative, timeout, multijoueur, `abandon` à la déconnexion, `dropFight` complet. Formules et décisions maintenant **justifiées dans le README**.
2. **`EventBus`** découplé du réseau (`send`, `toRoom`, `toOthers`, `take`), événements aux bons endroits (présence, `STATS`, combat, `SPAWN`) ; `RespawnQueue` dédiée.
3. **Validation fail-fast du monde** (sorties, directions, spawns, items, `count`, `respawn_seconds`, stats) ; je l'ai revérifiée sur les données : 15 salles, **toutes les sorties ont un retour**, 2 boucles (`square→village→plain→plain2→square` et `square→forest→forest_edge→plain2→square`), 0 référence cassée.
4. **Séparation définition / instance** (`Definition<T>`, `Instance<T>`, `Respawnable`), `Inventory` unique pour joueur et salle, `transfer()` atomique ; `Game::showInventories` expose l'inventaire sans exposer `Player`.
5. **`lookup::findIn`** (id d'instance, id de définition, nom, un mot du nom, casse ignorée) ; `lookup::sortedIds` pour des choix déterministes.

### ❌ Mauvais

1. **Quêtes simples, à compléter** : les quêtes `deliver`/`fetch` ne comptent que les objets **portés** au moment du rendu (rien n'est enregistré pendant le jeu) ; un joueur peut donc faire la quête *Medicine* en reprenant l'herbe qui réapparaît chaque minute à la place (aucune vérification que l'objet vient de la salle de départ) ; la progression `defeat` n'est comptée qu'après le début de la quête, ce qui est annoncé. Les quêtes sont **oubliées à la déconnexion** (pas de persistance, conforme au sujet) et un joueur ne peut pas abandonner une quête (pas de `ABANDON_QUEST`, que la RFC cite comme exemple).
2. **`USE` gaspille l'objet à pleine santé** : vérifié, `USE bread` à 100/100 PV répond `OK healed=0` et **consomme quand même le pain** (aucune confirmation ni refus) ; `USE` n'a pas de fichier d'effets autre que `healing`. Le soin par salle reste limité à la chapelle, qui est aussi le point de respawn.
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
2. **`.env` et variables d'environnement : un `SERVER_PORT` invalide est signalé mais le serveur démarre quand même** sur le défaut (8080) — avec un avertissement dans le log INFO/WARN, pas de refus de démarrer ; une faute de frappe sur le port passe donc inaperçue si l'on ne lit pas les logs. De même `PORT=8080 # commentaire` (commentaire en fin de ligne) rend la valeur invalide.
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

## `README.md` — v2 (572 lignes)

### ✅ Bon

1. **Sections remplies et de bonne qualité** : Description, Build (avec Configuration), Architecture (dispatcher, `poll`, couches), Protocol Implementation (commandes, événements, **8 écarts justifiés**), **Combat System** (règles, formule, initiative, `DEFEND`/`FLEE`, diffusion, justification), **World Design** (format JSON, carte ASCII, rôles, items, validation). Les affirmations vérifiées par l'audit (15 salles, retours de toutes les sorties, deux boucles, 7 items obtenables sur 8, 3 rôles) sont exactes.
2. **Rattrapage complet de l'état précédent** : casse, `EVT`, `DEFEND`/`FLEE`, erreurs 409-411, extensions (`COMBAT`, `SPAWN`), tableau des événements — tout ce que l'audit du jour précédent listait comme périmé est corrigé.
3. **Honnêteté** (`Current status`, `TODO` explicites, « GUI not connected yet »).
4. **Justification de conception** pour le combat (ratio de défense, initiative par `flee`, minuteur de tour) — exactement ce que le §V.1 demande.
5. **Usage de l'IA** documenté pour `Room::findNpc`.

### ❌ Mauvais

1. **README à jour sur les quêtes, `USE` et la configuration** : le statut déclare `QUEST`/`QUESTS` implémentés, la section « Quest System » est complète (format JSON, progression, validation, récompenses, tableau des 3 quêtes) et l'écart n°8 documente la réponse de `QUEST`.
2. **🔴 « 15 NPC definitions » (toujours là)** (World Design) : `npc.json` en contient **16** (3 `dialogue_npc`, 3 `quest_giver`, 10 `enemy`) ; le tableau de rôles du README en cite bien 16 noms.
3. **Sections vides exigées par le sujet** : **Testing** (Multiplayer, Combat, Quests, Protocol compliance), **Group Contributions**, **Resources**, **Instructions** ; « Client interface choice » (§V.3) en `TODO`. Les commentaires HTML du squelette restent dans le fichier livré. Aucun des 3 jeux d'essai possibles n'y est décrit alors que le serveur peut maintenant être testé de bout en bout.
4. **Choix `WHO` (RFC) vs exemple du sujet non expliqué** (voir `protocol/` ❌ 2) ; la grammaire des `EVT … COMBAT ROUND` n'est pas spécifiée.
5. **Section Group Contributions** : trois « Member » prévus alors que seuls deux logins sont cités en tête ; à aligner avec l'historique git avant le rendu.

---

## Corrigés depuis l'audit initial

Audits précédents (2026-10-02 → 2026-10-19) : `CONNECT`, hook de déconnexion, monde, `INCLUDES`, `install-deps`, injection JSON, logging, `QUIT`/`TALK`/`TAKE`/`DROP`/`ATTACK`/`STATUS`, instances, `locate()`, noms UTF-8, casse, respawn, README v1 et écarts, refactors, événements `EVT`, combat tour par tour, `DEFEND`/`FLEE`, diffusion du combat, raté visible, respawn annoncé, documentation du code.

Depuis le 2026-10-23 (soir) :
- **`QUEST`/`QUESTS` stubs, `quests.json` non lu** → système de quêtes complet (3 types, validation, récompenses, README). Vérifié : livraison et défaite de bout en bout.
- **Herbes sans effet, aucun usage d'objet** → `USE` (soin par objet), `ERR 407`.
- **Avertissements de configuration, `ERR 429`/`503`, `0` pour désactiver** → commités (`0363f87`).
- **Plus aucun stub** : les 15 commandes de la RFC répondent.

Depuis le 2026-10-23 (matin) :
- **Valeurs de configuration rejetées en silence** → `WARN invalid setting ignored`. **Impossible de désactiver une limite** (y compris pour `127.0.0.1`) → `0`. **Client refusé sans explication** → `ERR 503 SERVER_BUSY`. **Flood répondu en `ERR 400`** → `ERR 429 RATE_LIMITED`. Vérifiés (non commités).
- **`ServConfig`** commité (`26d9ae5`).

Depuis le 2026-10-21 :
- **Seuils anti-abus figés dans le code** → `ServConfig` + variables d'environnement bornées (commité `26d9ae5`). Vérifié.

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
2. **Distinguer les erreurs de `GROUP`** (`NOT_LEADER`, `GROUP_FULL`, `NO_INVITATION`) plutôt que de tout renvoyer en `400` ; supprimer les stubs `QUEST`/`QUESTS` ou les faire répondre `ERR` en attendant.
3. **Quêtes** : événements d'avancement, prérequis, enregistrement des objets ramassés ; documenter `USE` et la réponse de `QUEST` dans la section Protocol (déjà fait pour `QUEST`).
4. **Règle de sortie pour un joueur AFK**, refus de `USE` à pleine santé ; **retirer `log.txt` du dépôt** et l'ajouter au `.gitignore`.
5. **Robustesse restante** : tests de charge réels (plafond de `_out`, `MAX_READ_PER_TURN`), `EMFILE`, cohérence des codes d'erreur (flood, refus).
6. **Logger** : avertir quand une variable d'environnement est invalide ; rotation ou taille maximale du fichier.
7. **Tests automatisés** du moteur de combat (`roll` injecté) et cibles Makefile §VII.1 (`lint`, `run-server`, `run-client`, `run-client-gui`, `test`).
