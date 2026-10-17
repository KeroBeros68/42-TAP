# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-17 — régénéré après le respawn des PNJ et des items, la mécanique `hit`/`flee`, le README v1, les commandes insensibles à la casse et la réorganisation `server/network` · `server/protocol` · `shared/` (commits `6cc82e9` → `118b360`). Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (`network/`, `protocol/`, `main.cpp`) + `game/` (`game.cpp`, `world/`, `entities/`, `combat/`, `utils/lookup.hpp`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` + `README.md` (comme documentation de conformité).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++20` sans aucun warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine) : commandes en minuscules, combat, mort/respawn du joueur, respawn d'un loup et d'un item (délais ramenés à 3 s dans la copie), `QUIT`, `SIGINT` côté client. Aucune erreur ASAN/UBSAN. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟠 État général : toujours 9 commandes sur 15, mais le monde est maintenant vivant (respawn, précision, commandes insensibles à la casse) et la documentation a enfin rattrapé le code

Mesuré en conditions réelles :

- ✅ **9 commandes réelles** : `CONNECT`, `LOOK`, `MOVE`, `QUIT`, `TALK`, `TAKE`, `DROP`, `ATTACK`, `STATUS`. Aucune nouvelle commande depuis le 2026-10-14.
- ✅ **Commandes insensibles à la casse (RFC §4.2)** — nouveau : `connect Zoe`, `look`, `Talk guard` fonctionnent (`handeLine` passe la commande en majuscules avant la recherche).
- ✅ **Respawn des PNJ et des items** — nouveau : un loup tué réapparaît au bout du délai (`npc.wolf#23`, log `npc respawned`), des herbes ramassées réapparaissent dans la salle d'origine (`item.herbs#5`, log `item respawned`). Les 10 entrées `respawn_seconds` de `room.json` sont enfin utilisées, via `Game::tick()` appelé à chaque tour de boucle.
- ✅ **Combat avec précision** — nouveau : `hit`/`flee` dans `npc.json` ; chance de toucher = `hit / (hit + 0.2 × flee)` (8 contre 10 → 80 %), jet injectable (`fight(..., roll)`) donc testable ; constantes nommées (`COMBAT_*`).
- ✅ **Documentation** : le README passe de 12 à **299 lignes** : première ligne italique conforme, sections Description, Building and Running, Architecture (dispatcher, mono-thread + `poll`, couches), **Protocol Implementation avec un tableau honnête de 7 écarts à la RFC**. C'est le plus gros progrès de la période.
- 🔴 **Le README est en partie périmé et en partie vide** : l'écart n°4 (« Command names are case-sensitive ») est **faux depuis le code actuel** ; les sections Combat System, Quest System, World Design, Server Logging, Testing, Group Contributions, Resources, Instructions et Client interface choice ne sont que des titres. Or le Combat (formule, `hit`/`flee`) et le Logging sont implémentés et exigés au §VI.
- 🔴 **6 stubs restent** (`CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO`, `GROUP`) : `OK Message received`. **Toujours aucun événement `EVT`.**
- 🔴 **Pas de commande de combat supplémentaire** : malgré le commit « Flee and Hits », il n'existe pas de `DEFEND`/`FLEE` — `flee` est une statistique d'esquive, pas une commande de fuite.

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 9/15 réelles ; 6 stubs `OK` |
| Événements `EVT` (ROOM / GLOBAL / GROUP / STATS) | ❌ aucun émetteur |
| Commandes insensibles à la casse (RFC §4.2) | ✅ corrigé |
| Chargement monde JSON + validation des références | ✅ salles, sorties, PNJ, items, spawns, start/respawn ; ❌ `quests.json` jamais lu |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items (≥ 2 obtenables), ≥ 2 quêtes | ✅ données — quêtes non exploitées |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots | ✅ ; ⚠️ ids `#N` hors RFC (documenté) |
| Respawn PNJ / items | ✅ |
| Combat : 100 HP, dégâts, contre-attaque, `STATUS`, respawn joueur | ✅ ; précision `hit`/`flee` ✅ |
| Combat : tour par tour, initiative, `DEFEND`/`FLEE`, diffusion aux joueurs | ❌ |
| Quêtes (conception, validation, récompenses) | ❌ |
| Déconnexion : état retiré avant le broadcast | ⚠️ retiré, aucun broadcast |
| Logging : connexions/IP, commandes, réponses, monde, JSON, niveaux, timestamps | ✅ |
| Logging : quêtes, détection d'abus (flood, connexions rapides) | ❌ |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ; `lint`, `run-server`, `run-client`, `run-client-gui` absents (`TODO` dans le README) |
| README (§VI) | ⚠️ 4 sections remplies sur 13 (Description, Building and Running, Architecture, Protocol Implementation) |

---

## `server/network/` — `server.cpp`, `connection.*`, `user.*` (+ `shared/signalFd.*`, `shared/line.hpp`)

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes** — l'état de jeu est modifié séquentiellement : la course « deux joueurs prennent le même objet » est impossible par construction (vérifié). Les timers de respawn tiennent dans cette même boucle (`game.tick()` après chaque `updatePoll()`), sans thread.
2. **Framing TCP centralisé** — `Connection::popLine` appelle `extractLine` (`shared/line.hpp`), partagé avec le client : une seule implémentation, écriture partielle gérée (`POLLOUT` seulement si `out` non vide).
3. **Hook de déconnexion appelé *avant* `close`/`erase`** — ordre exigé par §IV ; le joueur est retiré de sa salle avant toute suite.
4. **`replyError`, constantes `UNKNOWN_IP`/`LOG_RESPONSE_MAX`, `setNonBlocking`, `utf8SafeLength`** — les doublons de l'audit précédent (quatre `fcntl` copiés, troncature maison de la réponse) sont factorisés ; `User::authenticate()` n'est **plus une bascule**, `User::setName` délègue à `isValidPlayerName` (`shared/player_name.hpp`), réutilisable côté client.
5. **Journalisation de chaque étape réseau** — connexion + IP, commande (joueur, nom, arguments), réponse (tronquée sans couper un caractère UTF-8), ligne trop longue, commande avant `CONNECT`, commande inconnue, échec de `recv`/`send`.

### ❌ Mauvais

1. **🔴 Aucune détection d'abus** (§V.1 « command flooding, rapid connections ») — les `WARN` existants ne comptent rien : pas de compteur par IP/joueur, pas de limite de débit, pas de limite de connexions. Et `_out` reste non borné : un client qui ne lit pas fait grossir la mémoire du serveur *(lecture de code)*.
2. **🔴 Aucun `EVT` émis** alors que `sendTo` / `sendToArray` / `sendToAll` existent : `MOVE`, `QUIT`, déconnexion, mort, **respawn d'un PNJ ou d'un item** et combat ne préviennent personne. Deux joueurs dans la même pièce ne se voient qu'en rappelant `LOOK`.
3. **Exception dans une action = crash du serveur** *(lecture de code)* — toujours aucun `try/catch` autour de `it->second(s.user, args)` (aucun `try` dans `server/network/` ni `server/protocol/`). Les actions utilisent encore `_players.at()` (`Game::playerMap`).
4. **`QUIT` avant `CONNECT` refusé en `ERR 400` et ne ferme pas** (la garde exempte seulement `CONNECT`) — un client qui veut juste partir proprement avant de s'authentifier n'a que la coupure TCP ; le client CLI l'envoie sur Ctrl+C et attend alors 1 s pour rien.
5. **La limite de 1024 octets ne s'applique qu'au reliquat sans `\n`** : une ligne complète plus longue est traitée *(lecture de code)* ; `SIGINT` dans la boucle `poll` est lu une fois par tour.

> Points mineurs : `handeLine` (faute de frappe) ; `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide ; `server.hpp` inclut ~13 en-têtes système ; ids de session recyclés.

---

## `server/protocol/` — `commands.cpp`, `serializer.cpp` (+ `main.cpp`)

### ✅ Bon

1. **Séparation réseau / protocole claire** : `network/` ne connaît pas le jeu, `protocol/` fait le lien (une fonction `cmdX` par commande), `game/` ne connaît ni sockets ni lignes. Le dossier correspond au découpage décrit dans le README.
2. **Codes d'erreur corrects par cas**, vérifiés : `TAKE` → `404`/`403`, `DROP` → `404 ITEM_NOT_IN_INVENTORY`, `TALK` → `404 NPC_NOT_FOUND`, `ATTACK` → `404`/`405`, `MOVE` → `301`.
3. **Un seul endroit qui écrit le JSON de réponse** (`serializer.cpp`, noms de champs en constantes `FIELD_*`) via `nlohmann::ordered_json` avec remplacement des UTF-8 invalides.
4. **`TapOk::ROOM`, `TapOk::BYE` + `Response.close`** pour `MOVE`/`QUIT` : réponse puis fermeture, sans toucher au socket.
5. **`main.cpp`** : `try/catch` global, `LOG_ERROR("fatal error")` + code 1 si le monde est invalide ou le port pris ; `game.tick()` branché à la boucle.

### ❌ Mauvais

1. **🔴 6 stubs répondent `OK Message received`** — `INVENTORY` bloque `DROP` côté joueur (impossible de lister ce qu'on porte) et le GUI (§V.4) ; `WHO`/`CHAT` bloquent le multijoueur.
2. **🔴 Ids d'instance (`item.herbs#3`, `npc.guard#10`) envoyés au client** (`LOOK`/`TAKE`/`DROP`) — désormais **documenté** (README, écart n°3) et atténué (id de définition, nom et un mot du nom acceptés en entrée), mais un client tiers qui attend `item.herbs` (RFC §5.4.1 et exemple du sujet) ne retrouvera rien dans `LOOK`. Écart assumé, à défendre en soutenance.
3. **Les erreurs ajoutées (`400`, `403`) restent hors RFC** (documentées, c'est un progrès) ; `ATTACK` sur un PNJ mort → `ERR 400` ; `MOVE` sans argument → `301` plutôt que `400`.
4. **Une touche manquée est invisible dans la réponse** — `serializeAttack` n'expose ni `missed` ni `attacker_missed` : un coup raté s'affiche `"damage":0`, indiscernable d'un coup absorbé (`COMBAT_MIN_DAMAGE` garantit ≥ 1 sur un coup touché, donc `0` = raté, mais le client doit le deviner). La RFC montre `"damage"` sans préciser.
5. **Validation des arguments inutiles absente** (`LOOK foo`, `QUIT bar` acceptés) ; captures `[&game]` par référence (valable car `Game` survit au `Server`).

---

## `game/game.cpp` (+ `game.hpp`, `entities/player.*`, `entities/respawnable.hpp`, `world/spawn.hpp`)

### ✅ Bon

1. **Respawn piloté par une file ordonnée** (`std::multimap<time_point, Respawn>`) : `tick()` ne traite que les échéances dépassées, `O(échéances échues)` par tour, sans scruter les salles. Réutilise le même mécanisme pour PNJ et items via `Respawn::of` + `spawnInstance` (patron générique), loggué (`npc respawned`, `item respawned`) — vérifié en réel.
2. **Pas de duplication à la prise** : quand un item est ramassé et que son respawn est planifié, l'instance prise est détachée (`clearOrigin`), donc la déposer ailleurs ne la fera pas réapparaître une seconde fois.
3. **`locate()` centralise « joueur ∈ salle »** et la macro `LOG_HERE(here)` unifie le contexte des logs (client, joueur, salle) — les ~20 lignes de log dupliquées de l'audit précédent sont réduites.
4. **Respawn du joueur conforme au sujet** (« reduced health ») : `teleport` vers `respawn_room` puis `revive(respawn_life)` (50 PV, borné). Vérifié : mort contre le boss → chapelle, `STATUS` 50/100.
5. **Inventaire plafonné à 10 avec `transfer()` atomique** (destination vérifiée avant de retirer la source).

### ❌ Mauvais

1. **🔴 Combat toujours pas « turn-based »** — un coup, une contre-attaque immédiate ; pas d'initiative, pas de `DEFEND`/`FLEE`. `in_combat` ne retombe que si le joueur change de salle ou tue sa cible (vérifié précédemment : `STATUS` → `combat` indéfiniment). Aucun autre joueur n'est informé du combat (§V.1 « broadcast to relevant players »).
2. **🔴 Toujours aucune régénération ni soin** — `Stats::heal()` n'est appelé nulle part (`grep`), `item.herbs` ne sert à rien. Avec le respawn des PNJ, un PNJ tué revient à pleine vie (nouvelle instance), mais un PNJ blessé qui n'est pas tué ne récupère jamais ; un joueur à 5 PV n'a pas d'autre recours que mourir.
3. **Un item ramassé réapparaît dans sa salle d'origine même si on le garde** — par construction (`schedule` à la prise) ; les herbes sont donc **inépuisables** (une prise toutes les `respawn_seconds`), ce qui peut être voulu mais n'est pas documenté et vide l'intérêt de l'unicité des objets (§V.1 « no duplication » est respecté au sens des ids, pas au sens du stock).
4. **Un PNJ réapparu ne prévient personne** et peut réapparaître dans une salle où se trouve un joueur sans transition : combat injuste possible *(lecture de code)* ; aucune limite sur le nombre d'instances si le délai est court et la mort rapide.
5. **Formule de dégâts et statistiques du joueur codées en dur** (`player.cpp`: 100 PV, attack 10, defense 10, hit/flee) — à justifier dans la section « Combat System » du README, vide à ce jour.

---

## `game/world/` · `game/entities/` · `game/utils/lookup.hpp`

### ✅ Bon

1. **Validation fail-fast étendue** (sorties, directions, start/respawn, spawns de PNJ/items inconnus, `count` ∈ [1, 100], `respawn_seconds` ≥ 0, stats) ; sorties à sens unique en `WARN`. Le serveur refuse de démarrer sur un monde incohérent (§V.1).
2. **Refactor définition/instance** : `Definition<T>` / `Instance<T>` (`definition.hpp`, `instance.hpp`), `Respawnable` — `item.cpp` et `npc.cpp` perdent ~45 lignes chacun, plus de logique d'id d'instance recopiée.
3. **`lookup::findIn` générique** (id d'instance, id de définition, nom, un mot du nom, casse et espaces ignorés) ; résolution déterministe. Documenté dans le README (Architecture) comme proposé avec l'aide de l'IA et relu par l'équipe.
4. **`Inventory` unique pour joueur et salle**, `transfer()` partagé par `TAKE`/`DROP`/butin.
5. **Données conformes et enrichies** (`hit`/`flee` sur tous les ennemis) : 15 salles, cycle, 3 rôles de PNJ, 8 items (7 obtenables), 3 quêtes.

### ❌ Mauvais

1. **🔴 `quests.json` n'est toujours pas lu** — aucun `QuestDef`/`QuestDb`, aucune progression ; `QUEST`/`QUESTS` sont des stubs. Les `quest_giver` (4 PNJ) n'ont aucun comportement propre : `TALK` renvoie un dialogue aléatoire à tous.
2. **Correspondance « un mot du nom » trop permissive** — `TAKE of` ou `TAKE the` prend le premier objet dont le nom contient ce mot ; pas d'avertissement d'ambiguïté.
3. **`Npc::dropLoot` ignore l'échec de `floor.add`** (retour jamais testé) — sans conséquence tant que l'inventaire d'une salle est illimité.
4. **Ids d'instance par compteur statique global**, non réinitialisé et dépendant de l'ordre de parcours d'un `unordered_map` ; les ids changent d'une exécution à l'autre du JSON (acceptable côté serveur, gênant si un client mémorise un id).
5. **Aucun test automatisé du jeu** — la section « Testing » du README est vide ; `fight()` accepte pourtant un `roll` injectable précisément pour pouvoir tester de façon déterministe, mais aucun test ne l'utilise.

---

## `shared/logger.*` · `shared/utf8.*`

### ✅ Bon

1. **Logging structuré JSON** — `{"ts","level","msg",…}`, niveaux DEBUG/INFO/WARN/ERROR, `src` dès WARN ; toutes les lignes produites pendant l'audit (WARN/ERROR/INFO) sont du JSON valide (vérifié lors de l'audit précédent, 127/127).
2. **Échappement JSON robuste** (`jsonEscape`) : un joueur ne peut pas injecter de fausse ligne de log via un argument de commande (§V.1, RFC §9.2).
3. **Macros `LOG_*` qui n'évaluent pas leurs arguments si le niveau est désactivé** ; champs typés (`LogValue`).
4. **Sorties séparées** : INFO/DEBUG sur stdout, WARN/ERROR sur stderr, couleurs seulement si TTY et `NO_COLOR` absent (`LOG_NO_COLOR_ENV`).
5. **`validateUTF8`** (surlongues, surrogates, > U+10FFFF, contrôles) + `utf8SafeLength` pour tronquer sans couper un caractère.

### ❌ Mauvais

1. **Niveau de log non configurable, fichier jamais utilisé** — `setMinLevel`, `setOutput`, `openFile` ne sont appelés nulle part (`grep` : seules les définitions) : le serveur tourne toujours en `Debug` et ne journalise que sur la sortie standard. Le README (§ Server Logging) est un titre vide, donc la surveillance du serveur n'est documentée nulle part.
2. **Pas de journalisation des quêtes ni d'événement d'abus** (exigences §V.1).
3. **Décalage horaire calculé une seule fois au démarrage** ; la variable du timestamp s'appelle `utc` mais contient `localtime_r`.
4. **Le refactor des constantes est allé loin** (`LOG_MS_PER_SECOND`, `LOG_DATE_SIZE`, `LOG_OFFSET_HOURS_LENGTH`, clés JSON `LOG_KEY_*`…) : `logger.cpp` perd en lisibilité (`"{\"" LOG_KEY_TS "\":\"" + …`) pour un gain faible — des valeurs comme `1000` ms/s ou `"%z"` n'ont rien de « magique ».
5. **`std::endl` à chaque ligne** (flush) et `args` non tronqués (une ligne de 1024 octets est journalisée en entier) ; pas de rotation.

---

## `shared/errors.hpp` · `success.hpp` · `response.hpp` · `defines.hpp` · `random.hpp`

### ✅ Bon

1. **`TAP_ERRORS(X)` (X-macro)** : une seule liste `X(nom, code)` génère l'`enum`, `tapErrorCode`, `tapErrorName` — ajouter une erreur est une ligne, et les trois fonctions ne peuvent plus diverger. Les 9 codes de la RFC §8.2 sont exacts.
2. **Formatage des réponses centralisé** (`tapOkLine`, `tapErrorLine` avec `RESPONSE_OK`/`RESPONSE_ERR`).
3. **`Response::success/failure`** en fabriques statiques, `Response.close` pour `QUIT`.
4. **Vocabulaire partagé** (`CMD_*`, `DIR_*`, `STATUS_*`, `FIELD_*`, `PROTOCOL_VERSION`) : serveur et clients utilisent les mêmes chaînes.
5. **`random.hpp` : `mt19937` thread-local + `randomUnit()`** pour les jets de touche, injectable dans `fight`.

### ❌ Mauvais

1. **`ERR 400` et `ERR 403` hors RFC** — documentés dans le README (progrès), restent des écarts à justifier devant un évaluateur.
2. **Pas de support des événements** — aucun `evtLine()` pour `EVT ROOM|GLOBAL|GROUP …`, `EVT STATS players=`.
3. **Pas de chemin inverse code → `TapError`** (utile au client) ; les trois `404` ne se distinguent que par le nom.
4. **`TapOk::PLAIN`/`PLAYERS`/`GROUP` encore inutilisés** — reflet des commandes à venir.
5. **`defines.hpp` partagé devient un fourre-tout** (système, protocole, règles de jeu, tailles de tampon) : `INSTANCE_SEPARATOR` et `MAX_NAME_LENGTH` n'ont rien à y faire pour les clients.

---

## `server/Makefile` · `Makefile` (racine)

### ✅ Bon

1. **`install-deps`** : nlohmann/json avec SHA-256 épinglé, fichier temporaire nettoyé, `third_party/` ignoré ; documenté dans le README.
2. **`INCLUDES` par `wildcard` étendu à `network/` et `protocol/`**, `vpath` mis à jour — la réorganisation des dossiers n'a rien cassé, build propre.
3. **Flags stricts** `-Wall -Wextra -Werror`, **zéro warning**, `-std=c++20`.
4. **Binaires ignorés par git**, `clean/fclean/re` propagés.
5. **Documentation des cibles dans le README** (tableau, prérequis, `make -C server re` après déplacement du dossier).

### ❌ Mauvais

1. **Cibles §VII.1 manquantes : `lint`, `run-server`, `run-client`, `run-client-gui`** — le README le reconnaît en `TODO` ; aucun `clang-tidy`/`cppcheck` n'est installé sur la machine d'audit.
2. **`all: $(NAME) run`** — `make` build **et lance** le serveur au premier plan ; la racine ouvre en plus deux terminaux. Incompatible avec une CI.
3. **Cible `help` toujours vide.**
4. **Téléchargement réseau au premier `make`** — internet + `curl`/`wget` requis à l'évaluation (documenté).
5. **`PROJECT_ROOT` absolu compilé dans le binaire** (déplacement du dossier = `make re`, documenté) ; `.vscode/c_cpp_properties.json` versionné.

---

## `README.md`

### ✅ Bon

1. **Conforme sur la forme** : première ligne en italique (« *This project has been created as part of the 42 curriculum by kebertra and gtourdiat.* »), rédigé en anglais, plan en sections reprenant celles du §VI.
2. **Section Protocol Implementation exemplaire** : commandes implémentées vs stubs, événements (aucun), **7 écarts à la RFC numérotés avec justification** (400, 403, ids `#N`, casse, noms de joueurs, statut de `ATTACK`, stubs). Les écarts sont exactement ceux que l'audit relevait.
3. **Section Architecture solide** : dispatcher vs inline justifié, modèle de concurrence (`poll`, mono-thread, `signalfd`, tampons) justifié par le jeu (« le premier qui prend l'objet gagne »), couches `network` / `game` / `commands.cpp` expliquées.
4. **Honnêteté sur l'état d'avancement** (« Current status and known limitations », `TODO` visibles pour `lint`, choix d'interface) — conforme à la règle du sujet sur la documentation des écarts.
5. **Usage de l'IA documenté** pour `Room::findNpc` (la note est conservée pour la section Resources), conforme au chapitre I.

### ❌ Mauvais

1. **🔴 Écart n°4 périmé** — « Command names are case-sensitive … a gap that we have to fix » : c'est **corrigé** dans le code (`std::toupper` dans `handeLine`, vérifié). Un évaluateur qui lit le README puis teste `look` verra une contradiction.
2. **🔴 Sections vides exigées par le sujet** : **Combat System**, **Quest System**, **World Design**, **Server Logging**, **Testing**, **Group Contributions**, **Resources**, **Instructions**. Les deux premiers sont indispensables et implémentés (combat, logging) ; leur contenu existe dans le code mais pas dans le README.
3. **Le combat décrit dans « Features »** (« enemies counter-attack ») ne mentionne ni la formule, ni `hit`/`flee`, ni le respawn des PNJ/items — impossible de justifier « turn-based » sans cela.
4. **Section « Client interface choice » en `TODO`** — exigée par le §V.3 ; et les commentaires HTML `TODO` / « Legend » du squelette restent dans le fichier livré.
5. **« The GUI … Its network part is not connected yet »** — honnête, mais le tableau de contributions (3 membres prévus alors que 2 logins sont cités en tête) devra être aligné.

---

## Corrigés depuis l'audit initial

Audit du 2026-10-02 → 2026-10-05 : `CONNECT` impossible (garde), hook de déconnexion, chargement et validation du monde, `Connection::ip`, `INCLUDES` vide, `install-deps`, `testMain.cpp`/`TEXT`, injection JSON via `LOOK`.

Audit du 2026-10-05 → 2026-10-14 : logging absent (`shared/logger.*`), `QUIT`/`TALK`/`TAKE`/`DROP`/`ATTACK`/`STATUS` stubs, items sans instances, `items.json`/`spawns` ignorés, `Player` sans état, `.at()` dispersés, validation des noms, instances de PNJ.

Depuis le 2026-10-14 :
- **Commandes sensibles à la casse (RFC §4.2)** → `std::toupper` dans `handeLine`. Vérifié : `connect`, `look`, `Talk`.
- **`respawn_seconds` lu mais jamais utilisé, PNJ tués définitivement** → `Game::tick`, file de respawn, PNJ et items. Vérifié en réel.
- **Combat sans aléa ni précision** → `hit`/`flee`, chance de toucher, `roll` injectable.
- **README de 12 lignes** → 299 lignes dont Protocol Implementation (7 écarts) et Architecture.
- **Écarts à la RFC non documentés** (`400`, `403`, ids `#N`) → documentés.
- **`User::authenticate()` bascule** → `_authenticated = true`.
- **Doublons de code** (`fcntl`, troncature UTF-8, framing, erreurs) → `setNonBlocking`, `utf8SafeLength`, `shared/line.hpp`, X-macro `TAP_ERRORS`, `replyError`, `LOG_HERE`.
- **Réorganisation** → `server/network/`, `server/protocol/`, `shared/signalFd.*`.

## Priorités (ordre suggéré)

1. **Mettre le README à jour** : corriger l'écart n°4 ; remplir **Combat System** (formule `attack × 100/(100+défense)`, `hit/(hit+0.2·flee)`, respawn joueur/PNJ), **Server Logging**, **World Design**, **Testing** — le code existe déjà, c'est de la rédaction.
2. **`INVENTORY`, `WHO`, `CHAT`** (les plus simples), puis supprimer les stubs ou les faire répondre `ERR`.
3. **Événements `EVT`** (helper `evtLine` dans `shared/`) : `PRESENCE ENTER/LEAVE`, `CHAT`, `STATS`, annonce du combat, du respawn d'un PNJ et de la mort d'un joueur.
4. **Quêtes** : parser `quests.json`, valider les références, `QUEST`/`QUESTS`, récompenses ; relier aux `quest_giver`.
5. **Combat** : `DEFEND`/`FLEE` réels, fin de l'état `combat`, soin (utiliser `Stats::heal` avec `item.herbs`), exposer le raté dans la réponse.
6. **Anti-abus et robustesse** : compteurs par IP/joueur, plafonds de connexions et de `_out`, `try/catch` autour des actions, `QUIT` avant `CONNECT`.
7. **Logger** : niveau/fichier configurables (variable d'environnement) et tronquer `args`.
8. **Cibles Makefile §VII.1** (`lint`, `run-server`, `run-client`, `run-client-gui`), `all` non bloquant, tests automatisés (`fight` avec `roll` injecté).
