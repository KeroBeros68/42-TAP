# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-19 — régénéré après le combat au tour par tour (`24befcb`), le système d'événements `EVT` (`e786ce7`) et le refactor « commentaires, combat, event bus, file de respawn » (`8bdcc27`). Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `server/` (`network/`, `protocol/`, `main.cpp`) + `game/` (`game.cpp`, `combat/`, `events/`, `world/`, `entities/`, `utils/lookup.hpp`) + `shared/` + `Makefile` racine et `server/Makefile` + `game/ressources/*.json` + `README.md` (comme documentation de conformité).
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++20` sans aucun warning ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine). Dans la copie seulement : délai d'un tour de combat ramené de 20 s à 2 s et respawn de 60 s à 3 s. Scénarios : observateurs et événements, combat complet (`ATTACK`/`DEFEND`/`FLEE`), `MOVE`/`TALK`/`TAKE` refusés en combat, tour hors-délai, deuxième joueur qui rejoint un combat, mort contre le boss, client CLI qui reçoit des `EVT`. Aucune erreur ASAN/UBSAN. Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🟢 État général : le serveur devient un vrai jeu multijoueur — événements de présence, combat tour par tour à plusieurs, `DEFEND`/`FLEE` — mais 6 commandes RFC restent des stubs et le README est maintenant très en retard sur le code

Mesuré en conditions réelles :

- ✅ **Événements `EVT` — le plus gros manque des audits précédents est comblé** : `EVT ROOM PRESENCE ENTER|LEAVE <joueur>` (connexion, `MOVE`, mort/fuite, `QUIT`, déconnexion), `EVT STATS players=<n>` à chaque connexion/déconnexion, au format exact de la RFC §6.2. Vérifié avec un observateur resté dans la salle : il voit `ENTER bob`, `LEAVE bob`, `ENTER bob`, `LEAVE bob` et `players=1/2/1`. Vérifié aussi avec le **client CLI** : les `[EVT]` s'affichent pendant qu'il attend la saisie (critère central du §V.3).
- ✅ **Combat au tour par tour avec initiative** (§V.1 « turn-based mechanics, initiative order, additional commands ») : `ATTACK`, `DEFEND` (dégâts reçus ÷ 2), `FLEE` (jet de fuite, succès → téléportation vers une sortie au hasard), rounds numérotés, ordre d'initiative recalculé à chaque round (le loup a joué avant le joueur au round 3), chance de toucher `hit / (hit + 0.2 × flee)`. Vérifié : `{"round":3,"turns":[{"actor":"cara","action":"attack","missed":false,"damage":9}]}`, `FLEE` raté puis réussi (`"escaped":true`).
- ✅ **Combat multijoueur** : un second joueur qui attaque le même ennemi **rejoint** le combat (`EVT … COMBAT JOIN j2 npc.wolf#11` reçu par le premier) ; hors de son tour il reçoit `ERR 411 NOT_YOUR_TURN`. `STATUS` indique `your_turn`, `turn`, `turn_seconds_left`. Un joueur qui tarde (20 s en production) voit un `DEFEND` joué à sa place ; vérifié par des rounds auto-joués pendant l'attente (2 s dans la copie).
- ✅ **Combat diffusé** (§V.1 « logged **and broadcast** ») : l'observateur reçoit `COMBAT START`, chaque `ROUND` (acteur, action, cible, dégâts ou `miss`/`failed`/`escaped`), `TURN`, `END … escaped|victory|defeat`. Le raté est enfin visible (`"missed":true`).
- ✅ **États cohérents pendant un combat** : `MOVE`, `TALK`, `TAKE` → `ERR 409 IN_COMBAT` ; `DEFEND`/`FLEE` hors combat → `ERR 410 NOT_IN_COMBAT` ; la mort ou le départ d'un joueur (`removePlayer`) retire le combattant (`abandon`) sans bloquer le combat des autres.
- ✅ **Respawn des PNJ annoncé** : `EVT ROOM SPAWN npc npc.wolf#23` à la salle, après le délai.
- 🔴 **6 stubs restent** : `CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO`, `GROUP` répondent `OK Message received`. Donc toujours **9 commandes RFC sur 15** (+ `DEFEND`/`FLEE`, extensions) et **aucun événement `CHAT`/`GROUP`**. `WHO` et `INVENTORY` sont les plus proches : `Game` fournit déjà `playerCount()` / `Inventory::ids()`.
- 🔴 **Le README n'a pas suivi** (aucun commit depuis `6cc82e9`) et se contredit désormais avec le code sur 6 points (voir `README.md`).

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ⚠️ 9/15 ; 6 stubs `OK` ; +`DEFEND`, `FLEE` hors RFC |
| Événements `EVT` ROOM (PRESENCE) / STATS | ✅ |
| Événements `EVT` CHAT (ROOM / GLOBAL / GROUP) et GROUP | ❌ |
| Commandes insensibles à la casse (RFC §4.2) | ✅ |
| Chargement monde JSON + validation des références | ✅ ; ❌ `quests.json` jamais lu |
| Monde : ≥ 8 salles avec boucle, ≥ 3 rôles PNJ, ≥ 4 items, ≥ 2 quêtes | ✅ données — quêtes non exploitées |
| Items : instances uniques, prise/dépôt, id ou nom, multi-mots | ✅ ; ⚠️ ids `#N` hors RFC |
| Respawn PNJ / items | ✅ + annoncé par `EVT` |
| Combat : 100 HP, dégâts, contre-attaque, `STATUS`, respawn joueur | ✅ |
| Combat : tour par tour, initiative, `DEFEND`/`FLEE`, diffusion | ✅ |
| Quêtes (conception, validation, récompenses) | ❌ |
| Déconnexion : état retiré **avant** le broadcast de départ | ✅ (`LEAVE` émis après `removePlayer`) |
| Logging : connexions/IP, commandes, réponses, monde, combat, JSON, niveaux, timestamps | ✅ |
| Logging : quêtes, détection d'abus (flood, connexions rapides) | ❌ |
| Fragmentation / coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `install-deps` ; `lint`, `run-server`, `run-client`, `run-client-gui` absents |
| README (§VI) | ⚠️ 4 sections remplies sur 13 ET partiellement périmé |

---

## `server/network/` — `server.cpp`, `connection.*`, `user.*` (+ `shared/signalFd.*`, `shared/line.hpp`)

### ✅ Bon

1. **Boucle `poll()` mono-thread, sockets non bloquantes**, désormais cadencée par une seule boucle de `main` : `updatePoll()` → `game.tick()` → livraison des événements. Aucune course possible (vérifié : deux joueurs sur le même ennemi, même objet), aucun thread pour les timers de tour ni de respawn.
2. **Ordre réponse → événements respecté** : la réponse à une commande est mise en file par `handeLine`, les `EVT` sont émis ensuite par `main` ; l'auteur reçoit sa réponse avant les événements qu'il a déclenchés (vérifié : `OK room=loc.village` puis `EVT ROOM PRESENCE …`).
3. **Hook de déconnexion appelé avant `close`/`erase`** — le joueur est retiré (`removePlayer`, `abandon` du combat, `LEAVE`, `STATS`) avant la destruction de la session (§IV).
4. **Framing et constantes centralisés** (`shared/line.hpp`, `RECV_BUFFER_SIZE`, `replyError`, `setNonBlocking`, `utf8SafeLength`), et **documentation systématique** : chaque fichier a maintenant son bandeau de sections et ses commentaires de fonctions (Javadoc-like) — commit `8bdcc27`.
5. **Journalisation de chaque étape réseau** (connexion + IP, commande, réponse tronquée sans couper l'UTF-8, ligne trop longue, commande avant `CONNECT`, commande inconnue, échec de `recv`/`send`).

### ❌ Mauvais

1. **🔴 Aucune détection d'abus** (§V.1 « command flooding, rapid connections ») : les `WARN` existants ne comptent rien ; pas de limite de débit ni de connexions. `_out` reste non borné *(lecture de code)* — et avec les événements, un client qui ne lit pas (donc qui n'absorbe pas les `EVT ROOM COMBAT …`, une ligne par tour d'un ou plusieurs combattants) fait grossir plus vite la mémoire du serveur.
2. **Exception dans une action = crash du serveur** *(lecture de code)* — toujours aucun `try/catch` autour de `it->second(s.user, args)` ni autour de `game.tick()` / `takeEvents()` dans `main` (le `try` global de `main` arrête tout le serveur, pas un seul joueur).
3. **`sendToArray` appelle `sendTo` qui `flush` immédiatement** pour chaque événement et chaque destinataire (`main.cpp`) : une salle de N joueurs en combat provoque N appels `send()` par ligne d'événement, au lieu d'un remplissage de tampon flushé une fois par tour de `poll`. Sans conséquence à 3 joueurs, à surveiller (§V.1 « must not significantly impact performance »).
4. **`QUIT` avant `CONNECT` refusé en `ERR 400` sans fermeture** — un client qui veut partir proprement avant de s'authentifier n'a que la coupure TCP ; le client CLI l'envoie sur Ctrl+C et attend alors 1 s.
5. **Limite de 1024 octets appliquée seulement au reliquat sans `\n`** : une ligne complète plus longue est traitée *(lecture de code)*.

> Points mineurs : `handeLine` (faute de frappe) ; `accept` en échec répété (`EMFILE`) fait tourner la boucle à vide ; ids de session recyclés ; `server.hpp` inclut ~13 en-têtes système.

---

## `server/protocol/` — `commands.cpp`, `serializer.cpp` (+ `main.cpp`)

### ✅ Bon

1. **Une fonction `cmdX` par commande, toutes documentées** (`@param`/`@return`) ; `combatResponse()` factorise la traduction d'un `CombatReport` en réponse pour `ATTACK`/`DEFEND`/`FLEE`, donc les trois commandes ne peuvent pas diverger.
2. **Codes d'erreur précis par cas**, vérifiés : `409 IN_COMBAT`, `410 NOT_IN_COMBAT`, `411 NOT_YOUR_TURN`, `404`/`405`/`403`/`301`, plus ceux de la RFC ; `STATUS` enrichi (`round`, `enemy`, `turn`, `your_turn`, `turn_seconds_left`) seulement pendant un combat.
3. **Un seul endroit qui écrit le JSON de réponse**, noms de champs en constantes `FIELD_*`, UTF-8 invalide remplacé ; la réponse d'`ATTACK` reste compatible avec l'exemple de la RFC (`attacker_hp`, `target_hp`, `damage`, `status`) et l'étend (`round`, `escaped`, `turns[]`) — un client qui ignore les champs inconnus continue de fonctionner.
4. **`TapOk::ROOM` / `TapOk::BYE` + `Response.close`** pour `MOVE`/`QUIT`.
5. **`main.cpp` documenté** : séquence réseau → temps → événements, `LOG_ERROR("fatal error")` + code 1.

### ❌ Mauvais

1. **🔴 6 stubs `OK Message received`** (`CHAT`, `INVENTORY`, `QUEST`, `QUESTS`, `WHO`, `GROUP`). `INVENTORY` manque au joueur (impossible de lister ce qu'il porte) et au GUI (§V.4) ; `CHAT`/`WHO`/`GROUP` bloquent la coopération.
2. **🔴 Ids d'instance `#N` sur le fil** (`item.herbs#3`, `npc.wolf#11`) — documenté (README, écart n°3) mais toujours hors RFC §5.4.1 ; les nouveaux `EVT … COMBAT` les propagent (`npc.wolf#11` comme cible, sans nom lisible), ce qui oblige un client à les afficher tels quels.
3. **Extensions à la RFC non documentées** : commandes `DEFEND`/`FLEE`, erreurs `409`/`410`/`411`, événements `EVT ROOM COMBAT …` (START/JOIN/ROUND/TURN/TIMEOUT/END) et `EVT ROOM SPAWN …`, champs étendus d'`ATTACK`/`STATUS`. Le sujet impose de documenter « toute déviation » (§IV) : rien de cela n'est dans le README. Point positif : elles sont additives et ne cassent pas un client strict, sauf si celui-ci rejette un `EVT` de type inconnu.
4. **Format des `EVT COMBAT ROUND` ambigu** : `EVT ROOM COMBAT ROUND npc.wolf#11 npc.wolf#11 1 attack cara miss` — l'acteur et l'ennemi sont répétés, et le dernier champ mélange un nombre (dégâts) et trois mots (`miss`, `failed`, `escaped`). Un client doit deviner la grammaire ; à spécifier en ABNF dans le README.
5. **Validation des arguments inutiles absente** (`LOOK foo`, `QUIT bar`, `FLEE x` acceptés) ; `DROP` reste permis pendant un combat alors que `TAKE` est refusé (incohérent) ; `MOVE` sans argument → `301` plutôt que `400`.

---

## `game/combat/` — `combat.cpp`, `combat_manager.cpp` (+ `combat_host.hpp`)

### ✅ Bon

1. **Moteur pur séparé de l'orchestration** : `combat.cpp` (`startEncounter`, `join`, `advance`, `act`, `currentActor`) ne connaît que des `Stats` et des ids ; `CombatManager` fait le lien avec les salles, les joueurs et les événements. Le moteur reçoit un `roll` injectable → déterministe, testable (aucun test écrit à ce jour, voir ci-dessous).
2. **`CombatHost` (interface)** : le gestionnaire n'a pas de dépendance directe sur `Game`, ce qui évite la dépendance circulaire et permet de le tester avec un faux hôte.
3. **Initiative par round**, timeout de tour (20 s, `COMBAT_TURN_SECONDS`) qui joue un `DEFEND` à la place du joueur absent, `abandon()` à la déconnexion, `dropFight` qui libère tous les participants et l'ennemi — vérifié sans fuite ni crash (ASAN) dans les scénarios de départ en plein combat.
4. **Un seul combat par ennemi, plusieurs participants** (`_fights` par id d'ennemi, `_fighting` par joueur) : refuse d'attaquer un autre ennemi pendant un combat (`ANOTHER_ENEMY` → `409`).
5. **Toutes les constantes nommées** (`COMBAT_DEFENSE_SCALE`, `COMBAT_DEFEND_DIVISOR`, `COMBAT_FLEE_WEIGHT`, `COMBAT_MIN_DAMAGE`) et documentées ; formule de dégâts `attack × 50/(50+défense)`, minimum 1.

### ❌ Mauvais

1. **🔴 Formule, initiative, `DEFEND`/`FLEE`, timeout non décrits dans le README** (section Combat System vide) : toutes les justifications que le sujet demande (§V.1 « Document and justify your choices ») sont dans les commentaires du code mais pas dans le livrable.
2. **Toujours aucun soin ni régénération** — `Stats::heal()` n'est appelé nulle part (`grep`), `item.herbs` ne sert à rien ; avec `DEFEND` ÷ 2 et `FLEE`, un joueur peut survivre un peu plus longtemps mais pas récupérer. La fuite est de plus aléatoire et les PNJ blessés qui ne meurent pas ne guérissent pas.
3. **Un joueur AFK reste lié au combat indéfiniment** (le timeout joue `DEFEND` à chaque tour) *(lecture de code + rounds auto-joués observés)* : il ne se retire jamais, subit des dégâts réduits à chaque round et empêche les autres de partir librement de la salle tant que le PNJ vit ; aucune règle de sortie après N timeouts.
4. **Fuite réussie = téléportation aléatoire vers une sortie** (`playerFled`), sans l'annoncer à la salle d'arrivée au-delà du `PRESENCE ENTER`, et sans interdire de fuir vers la salle d'un autre ennemi : un joueur blessé peut se retrouver dans un combat voisin.
5. **Aucun test automatisé** malgré le design testable (`roll` injectable, `CombatHost` virtuel) ; `Makefile` sans cible `test`.

---

## `game/game.cpp` (+ `events/event_bus.*`, `world/presence.hpp`, `entities/player.*`)

### ✅ Bon

1. **`EventBus` : file d'événements découplée du réseau** — `send`, `toRoom`, `toOthers`, `take()` ; le jeu ne sait pas ce qu'est un socket, `main` délivre. Aucun événement n'est émis s'il n'y a pas de destinataire.
2. **Événements émis aux bons endroits** : `ENTER` à la connexion et à l'arrivée, `LEAVE` au départ (après retrait de la salle), `STATS` à chaque variation du nombre de joueurs, `SPAWN` au respawn d'un PNJ ou d'un item. `teleport` (mort/fuite) émet les deux présences.
3. **`Presence` / `locate()`** centralisent « joueur ∈ salle » ; `LOG_HERE` unifie le contexte des logs (client, joueur, salle).
4. **File de respawn** (`RespawnQueue`) en structure dédiée, planifiant PNJ et items, avec `update(now, world, callback)` qui annonce chaque retour via `EVT`.
5. **Respawn du joueur conforme au sujet** (mort → chapelle, 50 PV) et inventaire plafonné à 10 avec `transfer()` atomique.

### ❌ Mauvais

1. **`emitPlayerCount` envoie à tous les joueurs à chaque connexion/déconnexion** (`_events.send(playerIds(), …)`) : O(N) par événement, O(N²) lors d'un pic de connexions. Acceptable à l'échelle d'un projet 42.
2. **Aucun événement d'inventaire** : `TAKE`/`DROP` ne préviennent pas les autres joueurs de la salle, qui ne voient la salle changer qu'au `LOOK` suivant (la RFC ne définit pas d'événement pour cela, mais une extension `EVT ROOM ITEM …` serait cohérente avec `SPAWN`).
3. **Items inépuisables** (`item.herbs` réapparaît dans la salle d'origine même si le joueur garde l'original) — vérifié précédemment ; non documenté.
4. **Un PNJ réapparu peut surgir dans une salle occupée** (annoncé par `EVT … SPAWN`) sans règle de proximité ni délai de grâce ; aucune borne sur le nombre d'instances vivantes.
5. **Statistiques du joueur codées en dur** (`player.cpp` : 100 PV, attaque 10, défense 10, `hit`/`flee`), pas de progression ni d'équipement ; à justifier dans le README.

---

## `game/world/` · `game/entities/` · `game/utils/lookup.hpp`

### ✅ Bon

1. **Validation fail-fast étendue** : sorties, directions, start/respawn, spawns de PNJ/items inconnus, `count` ∈ [1, 100], `respawn_seconds` ≥ 0, stats et `hit`/`flee`, sorties à sens unique en `WARN`. Le serveur refuse de démarrer sur un monde incohérent.
2. **Séparation définition / instance** (`Definition<T>`, `Instance<T>`, `Respawnable`) avec ids d'instance uniques ; items et PNJ partagent la même mécanique.
3. **`lookup::findIn` générique** (id d'instance, id de définition, nom, un mot du nom, casse et espaces ignorés) ; `lookup::sortedIds` rend déterministe le choix d'une sortie de fuite.
4. **`Inventory` unique** pour joueur et salle ; `transfer()` partagé par `TAKE`/`DROP`/butin.
5. **Données conformes** : 15 salles avec cycle, 16 PNJ en 3 rôles, 8 items (7 obtenables), 3 quêtes, `hit`/`flee` sur tous les ennemis.

### ❌ Mauvais

1. **🔴 `quests.json` n'est toujours pas lu** (`grep` : aucune référence dans `game/` ni `server/`) — pas de `QuestDef`, pas de progression ; `QUEST`/`QUESTS` stubs ; les 4 `quest_giver` répondent par un dialogue aléatoire comme les autres.
2. **Correspondance « un mot du nom » trop permissive** — `TAKE of` ou `TAKE the` prend le premier objet dont le nom contient ce mot ; aucun avertissement d'ambiguïté.
3. **`Npc::dropLoot` ignore l'échec de `add`** (retour jamais testé) ; sans conséquence tant que l'inventaire d'une salle est illimité.
4. **Ids d'instance par compteur statique global**, dépendant de l'ordre de parcours d'un `unordered_map` ; les ids changent d'une exécution à l'autre.
5. **Pas d'événement « PNJ mort »** distinct hors combat et pas de butin annoncé : `COMBAT END … victory` ne précise pas le loot déposé (seulement journalisé).

---

## `shared/` — `logger.*`, `utf8.*`, `errors.hpp`, `event.hpp`, `defines.hpp`, `response.hpp`, `success.hpp`, `random.hpp`

### ✅ Bon

1. **Logging structuré JSON** (ts + fuseau, niveau, `src` dès WARN, champs typés), échappement robuste (pas d'injection de fausse ligne de log), macros `LOG_*` à évaluation paresseuse, couleurs seulement sur TTY. Les événements de combat ajoutent `combat started`, `player joined a combat`, `combat turn`, `combat turn timeout`, `player fled`, `player died`/`respawned` : toute la partie « combat results » du §V.1 est tracée.
2. **`shared/event.hpp`** : toutes les chaînes d'événements en constantes (`EVT_PRESENCE_ENTER`, `EVT_COMBAT_ROUND`, `EVT_MISS`…) et `eventLine()` — un seul endroit qui construit `EVT <catégorie> <données>`, comme `tapErrorLine` pour les erreurs.
3. **`TAP_ERRORS(X)` (X-macro)** : une liste génère l'`enum`, les codes et les noms ; ajouter `IN_COMBAT`/`NOT_IN_COMBAT`/`NOT_YOUR_TURN` a coûté trois lignes.
4. **`validateUTF8` / `jsonEscape` / `utf8SafeLength`** réutilisés par les noms, les logs et la troncature.
5. **`random.hpp`** : générateur thread-local + `randomUnit()` injectable, utilisé par le combat et le choix de sortie de fuite.

### ❌ Mauvais

1. **Niveau de log non configurable, fichier jamais utilisé** — `setMinLevel` / `setOutput` / `openFile` ne sont appelés nulle part : le serveur tourne toujours en `Debug` (et avec le combat, une ligne INFO par tour s'ajoute aux 2 lignes par commande).
2. **Pas de journalisation des quêtes ni d'événement d'abus** (§V.1).
3. **Décalage horaire calculé une seule fois au démarrage** ; le timestamp s'appelle `utc` mais utilise `localtime_r`.
4. **`shared/defines.hpp` est devenu un fourre-tout de ~140 lignes** (système, protocole, noms de champs JSON, règles de jeu, tailles de tampon) ; `INSTANCE_SEPARATOR`, `MAX_NAME_LENGTH`, `PLAYER_INVENTORY_CAPACITY` n'ont rien à y faire pour les clients. Les noms `FIELD_*` sont utiles aux clients mais mélangés aux autres.
5. **Erreurs ajoutées hors RFC (`400`, `403`, `409`, `410`, `411`)** : `400`/`403` documentés, `409`-`411` non ; le préfixe 4xx de la RFC est réservé aux erreurs de groupe/ressource/quête, `409`-`411` sont proches de conventions HTTP sans rapport.

---

## `server/Makefile` · `Makefile` (racine)

### ✅ Bon

1. **`install-deps`** : nlohmann/json avec SHA-256 épinglé, documenté dans le README.
2. **`SRCS` et `vpath` étendus** (`combat_manager.cpp`, `event_bus.cpp`, `respawn_queue.cpp`, `../game/events`) ; build propre, **zéro warning**, `-Wall -Wextra -Werror -std=c++20`.
3. **`INCLUDES` par `wildcard`** : tout en-tête modifié (y compris `game/*/*.hpp`) recompile.
4. **Binaires ignorés par git**, `clean/fclean/re` propagés.
5. **README documente les cibles** (tableau, prérequis).

### ❌ Mauvais

1. **Cibles §VII.1 manquantes : `lint`, `run-server`, `run-client`, `run-client-gui`** (le README le reconnaît en `TODO`) ; aucun outil d'analyse statique installé sur la machine d'audit.
2. **Pas de cible `test`** alors que le moteur de combat est conçu pour être testé.
3. **`all: $(NAME) run`** — `make` build **et lance** le serveur au premier plan ; incompatible avec une CI.
4. **Cible `help` toujours vide.**
5. **Téléchargement réseau au premier `make`** et `PROJECT_ROOT` absolu compilé dans le binaire (documentés) ; `.vscode/c_cpp_properties.json` versionné.

---

## `README.md` — inchangé depuis `6cc82e9`

### ✅ Bon

1. **Forme conforme** : première ligne italique correcte, anglais, plan reprenant les sections du §VI.
2. **Section Protocol Implementation** (écarts n°1-7 avec justification) et **Architecture** (dispatcher, `poll`, mono-thread, couches) toujours de bonne qualité.
3. **Honnêteté sur l'avancement** (`Current status`, `TODO`).
4. **Usage de l'IA** documenté pour `Room::findNpc`.
5. **Instructions de build détaillées** (prérequis, `install-deps`, cibles).

### ❌ Mauvais

1. **🔴 Six affirmations fausses ou périmées** : « 9 of the 15 RFC commands » (+`DEFEND`/`FLEE` existent) ; « *The server does not push EVT events yet* » (présence, stats, combat et spawn sont émis) ; écart n°4 « *Command names are case-sensitive* » (corrigé) ; « Implemented events: None yet » ; écart n°6 (« `ATTACK` returns the status of the attacker … `ERR 400` when the target is already dead » : la réponse a changé, `ERR 400` n'est plus renvoyé pour un PNJ mort) ; « `combat` … runs one round of fight on two `Stats` objects » (c'est maintenant un combat à plusieurs, par rounds).
2. **🔴 Les sections que le code remplit déjà restent vides** : **Combat System** (formule, initiative, `DEFEND`, `FLEE`, délai, multijoueur), **Server Logging**, **World Design**, **Testing**.
3. **Extensions à la RFC non documentées** (commandes `DEFEND`/`FLEE`, événements `COMBAT`/`SPAWN`, erreurs 409-411).
4. **Sections vides inchangées** : Quest System, Group Contributions, Resources, Instructions, Client interface choice ; commentaires HTML du squelette (`<!-- … -->`) toujours dans le fichier livré.
5. **Pas de description du protocole d'événements** : aucun tableau des `EVT` émis ni de grammaire.

---

## Corrigés depuis l'audit initial

Audits précédents (2026-10-02 → 2026-10-17) : `CONNECT`, hook de déconnexion, chargement et validation du monde, `Connection::ip`, `INCLUDES`, `install-deps`, injection JSON, logging, `QUIT`/`TALK`/`TAKE`/`DROP`/`ATTACK`/`STATUS`, instances d'items et de PNJ, `locate()`, noms UTF-8, casse des commandes, respawn PNJ/items, README v1 et écarts documentés, `authenticate()`, refactors (`network/`, `protocol/`, X-macro des erreurs, `LOG_HERE`).

Depuis le 2026-10-17 :
- **Aucun événement `EVT`** → `EventBus`, `shared/event.hpp`, présence, `STATS`, combat, `SPAWN`. Vérifié avec un observateur et avec le client CLI.
- **Combat non « turn-based »** (un coup, une riposte) → `CombatManager` : rounds, initiative par round, `DEFEND`, `FLEE`, timeout, multijoueur (`JOIN`).
- **Pas de diffusion du combat aux autres joueurs** → `EVT ROOM COMBAT …`.
- **Pas de commandes de combat supplémentaires** (`DEFEND`/`FLEE`) → ajoutées.
- **`in_combat` permanent** → fin de combat explicite (`END`, `abandon`, `dropFight`).
- **Coup raté invisible** → `"missed"` dans `turns[]`, `miss`/`failed` dans les événements.
- **Respawn non annoncé** → `EVT ROOM SPAWN`.
- **Fonctions non documentées** → bandeaux et commentaires `@param/@return` sur tous les fichiers.

## Priorités (ordre suggéré)

1. **README** : corriger les six affirmations périmées ; écrire **Combat System** (formule `attack × 50/(50+défense)`, `hit/(hit+0.2·flee)`, initiative par round, `DEFEND` ÷ 2, `FLEE`, timeout de 20 s, multijoueur), **Server Logging**, **World Design**, **Testing** ; documenter les extensions (`DEFEND`/`FLEE`, `409`-`411`, `EVT COMBAT`/`SPAWN`) avec leur grammaire.
2. **`WHO`, `INVENTORY`, `CHAT`** (événements `EVT ROOM|GLOBAL|GROUP CHAT` inclus), puis `GROUP` ; supprimer les stubs ou les faire répondre `ERR`.
3. **Quêtes** : parser `quests.json`, valider les références, `QUEST`/`QUESTS`, récompenses ; relier aux `quest_giver`.
4. **Soin** : utiliser `Stats::heal` (herbes, repos) ; règle de sortie pour un joueur AFK.
5. **Anti-abus et robustesse** : compteurs par IP/joueur, plafonds de connexions et de `_out`, `try/catch` autour des actions et de `tick`, `QUIT` avant `CONNECT`, groupage des `send` d'événements.
6. **Logger** : niveau/fichier configurables (variable d'environnement).
7. **Tests automatisés** du moteur de combat (`roll` injecté) et cibles Makefile §VII.1 (`lint`, `run-server`, `run-client`, `run-client-gui`, `test`).
