# Plan — État local du jeu côté client (GUI)

| Champ | Valeur |
| --- | --- |
| Pour | Gaspard |
| Date de rédaction | 2026-10-20 |
| **Mise à jour** | **2026-10-28 — le plan est réalisé, voir §0.** |
| Références | `docs/TAP-subject-1.4.md` (§II, §V.4), `docs/protocol-rfc.html`, `docs/GUI/cahier-des-charges.md`, `README.md` (« Protocol Implementation ») |
| Hors périmètre | Le client CLI : on n'y touche pas. |

---

## 0. État au 29 octobre 2026

**Le plan est implémenté.** `clients/state/` existe et est déployé : `message.cpp`,
`catalog.cpp`, `game_state.hpp` et `session.cpp` sont compilés par le CMake du GUI et branchés
sur la fenêtre de jeu (`TAPGameWindow::update_gui(Section)`). Les écarts entre le plan rédigé
et le code livré sont signalés par **➜ Réalisé** dans les sections ci-dessous.

Changements par rapport au plan initial :

| Plan § | Prévu | Livré |
| --- | --- | --- |
| 4 | `Session` reçoit un `std::function<bool(const std::string&)>` | `Session(Client &)` : la session possède une référence à `Client` et appelle `_client.send()` / `_client.isConnected()` |
| 7 | `enum class Section { …, Quests … }` à 9 valeurs | 10 valeurs, `Section::Quests` incluse |
| 8.1 | `CONNECT` → `LOOK`, `INVENTORY`, `STATUS`, `WHO` | `LOOK`, `INVENTORY`, `STATUS`, `WHO`, **`QUESTS`** |
| 8.1 | `QUEST` / `QUESTS` : « pas encore implémentés côté serveur » | **Implémentés** des deux côtés ; `QUEST` renvoie un objet, `QUESTS` un tableau |
| 8.1 | `QUIT` → phase `TERMINATED` | **Sans handler dédié, mais le résultat est là** : le serveur ferme la socket après `OK bye`, `Client::update()` renvoie faux, donc `handleDisconnect()` passe la phase à `TERMINATED`. Il manque seulement le retour visuel à l'écran de connexion |
| 8.1 | `TALK` tolère `OK {"npc","dialogue"}` (plan) et `WHO` tolère le JSON | **Fait**, `applyTalk` et `applyWho` acceptent les deux formes |
| 9 | `QSocketNotifier` | **`QTimer` de 50 ms** appelant `Client::update(0)` — plus simple et suffisant |
| 9 | Coupure → retour à l'écran de connexion | `Session::handleDisconnect()` passe en `TERMINATED`, mais la fenêtre de connexion ne réagit pas encore. **Non exigé par le sujet** (§IV : gestion des déconnexions côté serveur seulement) |
| 10 | Étapes 0 à 6 | 0 à 5 faites ; 6 (test contre un autre groupe) non faite |

---

## 1. Objectif

Aujourd'hui, le moteur réseau (`clients/client.cpp`) aiguille chaque ligne reçue selon son préfixe (`OK`, `ERR`, `EVT`), **sans l'interpréter**. Le GUI n'a que des widgets statiques et n'est pas relié au réseau.

Le but est d'ajouter entre les deux une **copie locale de l'état du jeu**, mise à jour par les réponses aux commandes et par les événements. Le GUI ne parse plus le protocole : il **lit l'état** et se redessine quand celui-ci change.

```
socket ──► Client (existant) ──► Session ──► GameState ──► widgets Qt
                                   ▲  │
              boutons du GUI ──────┘  └──► Client::send()
```

## 2. Décisions déjà prises

| Sujet | Décision |
| --- | --- |
| Noms des objets et des PNJ | **Catalogue local optionnel**, avec **un libellé dérivé de l'id en secours** (voir §5). |
| Client CLI | Inchangé pour l'instant. Il pourra réutiliser `state/` plus tard. |
| Langage | `clients/state/` est en **C++17 pur**, sans Qt ni socket. Les headers de `shared/`, `third_party/json.hpp` et `clients/client.cpp` compilent déjà en C++17 (vérifié avec `g++ -std=c++17 -Wall -Wextra`). |
| Corrélation des réponses | **File FIFO des commandes envoyées** (voir §6). |

## 3. Les deux règles à ne jamais casser

1. **Le client doit marcher avec le serveur d'un autre groupe** (sujet §II : « CLI and GUI clients must be interchangeable »). Il ne doit donc **dépendre** d'aucune donnée propre à notre serveur :
   - les ids sont **opaques** : on les affiche via un libellé, mais on renvoie toujours **l'id exact** dans `TAKE`, `DROP`, `TALK`, `ATTACK` et `QUEST` (notre serveur envoie `item.herbs#3`, un autre peut envoyer `item.herbs`) ;
   - le catalogue est un **bonus** : sans lui, ou avec un id inconnu, le client affiche le libellé dérivé de l'id ;
   - nos extensions (`DEFEND`, `FLEE`, `EVT ROOM COMBAT …`, `EVT ROOM SPAWN …`, champs JSON en plus) sont **utilisées si présentes, jamais exigées** ;
   - un `EVT` inconnu, un code `ERR` inconnu ou un champ JSON inconnu va dans le journal, sans erreur.
2. **Le serveur est la source de vérité.** Une mise à jour locale (par exemple déplacer un objet après `OK taken=…`) est toujours suivie d'une resynchronisation (`LOOK` / `INVENTORY` / `STATUS`).

## 4. Arborescence

```
clients/
├── client.{hpp,cpp}        réseau — existant, inchangé
├── state/                  NOUVEAU — C++17, sans Qt, sans socket
│   ├── message.{hpp,cpp}   une ligne du serveur → ServerMessage
│   ├── catalog.{hpp,cpp}   id → nom / description (catalogue + secours)
│   ├── game_state.hpp      l'état local
│   └── session.{hpp,cpp}   envoi des commandes, FIFO, application des réponses et des EVT
├── cli/                    inchangé
└── gui/                    branchement Qt (étape 5)
```

**➜ Réalisé autrement :** `state/` connaît `Client`, mais seulement par référence et par ses
trois méthodes publiques (`send`, `isConnected`, `update`). La `Session` est construite par
`Session(Client &client)` et appelle `_client.send(line)`. Le découplage visé est conservé :
`state/` n'ouvre aucune socket et ne dépend pas de Qt.

## 5. Le catalogue

### 5.1 Source

Il reprend les fichiers du monde, au format déjà utilisé par le serveur :

```json
// items.json
{ "items": { "item.herbs": { "name": "Healing Herbs", "description": "…" } } }
// npc.json
{ "npcs":  { "npc.guard":  { "name": "Guard Hector",  "description": "…" } } }
```

- Le dossier est `game/ressources/` par défaut. Il peut être remplacé par une variable d'environnement, à ajouter dans `clients/defines.hpp` et à lire comme `SERVER_IP` avec `getEnv` (proposition : `ENV_CATALOG` = `"TAP_CATALOG_DIR"`).
- **Un fichier absent ou invalide n'est pas une erreur** : le catalogue est vide, une ligne est écrite dans le journal, et le client continue avec le secours.
- Seuls `name` et `description` sont lus. Tout autre champ (`role`, `dialogue`, `stats`…) est ignoré : le client ne doit rien déduire du jeu à partir de ces fichiers.

### 5.2 Recherche

```cpp
class Catalog {
	public:
		bool load(const std::string& dir);               // false si rien n'a pu être chargé (pas bloquant)
		std::string name(const std::string& id) const;   // jamais vide
		std::optional<std::string> description(const std::string& id) const;
};
```

1. On retire le suffixe d'instance : tout ce qui suit le dernier `#` (`item.herbs#3` → `item.herbs`). Utiliser `INSTANCE_SEPARATOR` de `shared/defines.hpp`.
2. Si cette clé est dans le catalogue, on renvoie son `name`.
3. **Sinon, secours** : on dérive un libellé de l'id.

### 5.3 Libellé de secours

| Id reçu | Libellé |
| --- | --- |
| `item.healing_herbs#3` | `Healing Herbs` |
| `npc.guard` | `Guard` |
| `loc.square` | `Square` |
| `herbs` | `Herbs` |

Règle : retirer `#N`, retirer le préfixe jusqu'au premier `.`, remplacer `_` par une espace, et mettre une majuscule au début de chaque mot. Les noms de joueurs ne passent jamais par le catalogue : ils sont affichés tels quels.

## 6. Messages et corrélation

### 6.1 `message` : découper une ligne

```cpp
struct ServerMessage {
	enum class Kind { OK, ERR, EVT, UNKNOWN } kind;
	int code = 0;            // ERR uniquement (404…)
	std::string name;        // ERR : "ITEM_NOT_FOUND"
	std::string category;    // EVT : "ROOM", "STATS", "GLOBAL", "GROUP"
	std::string data;        // reste de la ligne, brut (texte, clé=valeur ou JSON)
};
ServerMessage parseLine(const std::string& line);
```

Pour les données, un utilitaire `std::optional<nlohmann::json> asJson(const std::string&)` ne parse que si la donnée commence par `{` ou `[`. Il ne lève jamais d'exception : en cas d'échec, `std::nullopt` et une ligne dans le journal.

### 6.2 La file FIFO

Le protocole n'a pas d'identifiant de corrélation : une réponse `OK {…}` peut venir de `LOOK`, de `STATUS` ou d'`ATTACK`. Mais côté serveur, **chaque commande reçoit exactement une ligne `OK` ou `ERR`, dans l'ordre d'arrivée**, et un `EVT` ne commence jamais par `OK` ni `ERR`. D'où :

```cpp
struct Pending { std::string command; std::string args; };   // "TAKE", "item.herbs#3"
std::deque<Pending> _pending;
```

- `Session::send("TAKE", id)` envoie la ligne et ajoute `{TAKE, id}` en fin de file.
- Chaque `OK` ou `ERR` reçu retire l'élément de tête et passe dans le handler de **cette** commande.
- Chaque `EVT` passe dans le handler d'événements, sans toucher à la file.
- La file démarre avec `{HELLO}` : le message d'accueil `OK hello proto=1` arrive sans commande.
- `OK` ou `ERR` avec une file vide : anomalie, à écrire dans le journal sans planter.

Plusieurs commandes peuvent donc être en vol en même temps (`LOOK`, puis `INVENTORY`, puis `STATUS` juste après la connexion).

## 7. `GameState`

```cpp
struct ChatLine { std::string from, text; };

struct GameState {
	enum class Phase { DISCONNECTED, CONNECTED, AUTHENTICATED, TERMINATED };   // RFC §2.2
	Phase phase = Phase::DISCONNECTED;
	std::string me;

	struct Room {
		std::string id, name, description;
		std::map<std::string, std::string> exits;   // direction → id de salle
	} room;
	std::vector<std::string> players, items, npcs;  // ids, tels que reçus
	std::vector<std::string> inventory;

	int hp = 0, max_hp = 0;
	std::string status;                             // healthy, wounded, combat, dead
	int server_players = 0;

	struct Combat {
		bool active = false;
		std::string enemy;
		int round = 0;
		bool my_turn = false;
		int seconds_left = 0;
	} combat;

	std::optional<std::string> group;               // id du groupe
	std::optional<std::string> pending_invite;      // chef qui nous a invité
	std::vector<ChatLine> chat_global, chat_room, chat_group;
	std::deque<std::string> log;                    // journal technique, borné (500 lignes)
};
```

Le compteur « joueurs dans la salle » n'est pas stocké : c'est `players.size()`.

Pour que le GUI sache quoi redessiner, la `Session` appelle un callback avec la partie modifiée :

```cpp
enum class Section { Connection, Room, Inventory, Status, Combat, Counters, Chat, Group, Quests, Log };
void onChanged(std::function<void(Section)> callback);
```

## 8. Mises à jour de l'état

### 8.1 Réponses aux commandes

| Commande en tête de file | Réponse | Mise à jour | Envoyé ensuite |
| --- | --- | --- | --- |
| `HELLO` | `OK hello proto=1` | phase `CONNECTED` | `CONNECT <nom>` |
| `CONNECT` | `OK connected` | phase `AUTHENTICATED`, `me` | `LOOK`, `INVENTORY`, `STATUS`, `WHO`, `QUESTS` |
| `CONNECT` | `ERR 201 NAME_IN_USE` | reste en `CONNECTED`, erreur affichée sur l'écran de connexion | — |
| `LOOK` | `OK {"room":…,"players":…,"items":…,"npcs":…}` | `room`, `players`, `items`, `npcs` | — |
| `MOVE` | `OK room=<id>` | `room.id` | `LOOK` (on ne reçoit pas son propre `PRESENCE`) |
| `MOVE` | `ERR 301 NO_EXIT` | rien | — |
| `TAKE` | `OK taken=<id>` | id retiré de `items`, ajouté à `inventory` | `LOOK`, `INVENTORY` (sujet §V.4) |
| `TAKE` | `ERR 404 ITEM_NOT_FOUND` | rien | `LOOK` (un autre joueur l'a peut-être pris) |
| `DROP` | `OK dropped=<id>` | id retiré d'`inventory`, ajouté à `items` | `LOOK`, `INVENTORY` |
| `INVENTORY` | `OK [..]` | `inventory` | — |
| `TALK` | `OK <dialogue>` | dialogue affiché (journal ou zone dédiée) | — |
| `ATTACK`, `DEFEND`, `FLEE` | `OK {"attacker_hp",…}` | `hp`, `combat`, les `turns` dans le journal | `STATUS`, et `LOOK` si `escaped` est vrai (changement de salle) |
| `STATUS` | `OK {"hp","max_hp","status",…}` | `hp`, `max_hp`, `status`, et `combat` si les champs de combat sont présents | — |
| `WHO` | `OK players=<n>` | `server_players` | — |
| `CHAT` | `OK` | **rien** : le message s'affiche à réception de l'`EVT`, que le serveur renvoie aussi à l'auteur | — |
| `GROUP CREATE` / `JOIN` | `OK group=<id>` | `group`, `pending_invite` remis à vide | — |
| `GROUP LEAVE` | `OK` | `group` remis à vide | — |
| `QUEST` | `OK {"quest_id","name","status","description","progress","reward",…}` | ligne dans le journal ; si `completed`, `INVENTORY` ; puis toujours `QUESTS` | `INVENTORY`, `QUESTS` |
| `QUESTS` | `OK [{"quest_id","name","status","progress"}]` | `quests` (liste complète remplacée) | — |
| `QUIT` | `OK bye` | **⚠️ non implémenté** : pas de handler, la phase ne passe jamais à `TERMINATED` | — |
| n'importe laquelle | autre `ERR <code> <NOM>` | journal + message non bloquant (code **et** nom : 404 a trois sens) | — |

Tolérance, pour les serveurs des autres groupes :
- `TALK` : accepter aussi `OK {"npc":…,"dialogue":…}` (forme montrée par l'exemple du sujet §V.5).
- `WHO` : accepter aussi `OK {"room":[…],"server":<n>}` (même raison).

### 8.2 Événements

| Événement | Mise à jour |
| --- | --- |
| `EVT ROOM PRESENCE ENTER <p>` | ajoute `p` à `players` (sans doublon) |
| `EVT ROOM PRESENCE LEAVE <p>` | retire `p` de `players` |
| `EVT STATS players=<n>` | `server_players` |
| `EVT GLOBAL CHAT <p> <msg>` | `chat_global` |
| `EVT ROOM CHAT <p> <msg>` | `chat_room` |
| `EVT GROUP CHAT <p> <msg>` | `chat_group` |
| `EVT GROUP INVITE <chef>` | `pending_invite` (affichage visible, avec un bouton qui envoie `GROUP JOIN <chef>`) |
| `EVT GROUP JOIN <p>` / `LEAVE <p>` | journal, liste des membres si vous l'affichez |
| `EVT ROOM COMBAT START/JOIN/ROUND/TURN/TIMEOUT/END …` | `combat` (`TURN` avec notre nom → `my_turn`, `seconds_left`), et le détail dans le journal |
| `EVT ROOM COMBAT RESOLVED` | `combat` remis à zéro, puis envoie `LOOK` et `STATUS` : l'ennemi a quitté la salle et son butin est au sol |
| `EVT ROOM DEATH <salle>` | `combat` remis à zéro, salle remplacée par celle du respawn (`room.id` + contenu vidé), puis envoie `LOOK`, `STATUS` et `INVENTORY`. C'est le seul signal de mort : le `COMBAT END ... defeat` qui précède ne nous est pas adressé |
| `EVT ROOM SPAWN <npc\|item> <id>` | envoie `LOOK` |
| `EVT ROOM ITEM TAKEN <joueur> <id>` / `DROPPED <joueur> <id>` | envoie `LOOK`, et une ligne dans le journal |
| tout autre `EVT` | journal uniquement |

Les messages de chat ne vont **jamais** dans le journal technique (sujet §V.4 : « Separates the chat view … from the log view »).

Un `TAKE` ou un `DROP` d'un **autre** joueur est annoncé par `EVT ROOM ITEM`. Le joueur qui agit ne le reçoit pas : son client applique le `OK taken=` / `OK dropped=` de façon optimiste, puis resynchronise par `LOOK` + `INVENTORY`. La liste `items` reste donc corrigée par le `LOOK` qui suit l'événement, comme pour `SPAWN`.

Les formats complets de nos événements de combat sont dans `README.md`, section « Implemented events ».

## 9. Branchement Qt

- **Lecture réseau — ➜ Réalisé avec un `QTimer`** : `main.cpp` démarre un `QTimer` de 50 ms
  dont le slot teste `client.isConnected()` puis appelle `client.update(0)` ; un `false`
  signifie que la connexion ouverte vient d'être perdue et déclenche
  `session.handleDisconnect()`. Le timer tourne en permanence, donc une reconnexion ultérieure
  est servie par le même. Le `QSocketNotifier` du plan initial est inutilement compliqué
  pour un trafic de cette taille. Il ne faut **jamais** appeler la boucle
  `while (client.update())` du CLI : elle bloquerait `app.exec()`.
- **Pas de `enableKeyboard()` ni de `enableSignals()`** dans le GUI : ce sont des options du CLI.
- **Connexion** : `Client::connect()` utilise un `::connect()` bloquant. Avec un serveur local
  ou injoignable, il échoue vite. Avec une adresse qui ne répond pas, la fenêtre peut se figer
  quelques secondes. Documenté dans le README.
- **Coupure — ➜ partiellement fait** : `Session::handleDisconnect()` vide la file, passe la
  phase à `TERMINATED`, journalise « connection lost » et notifie `Section::Connection`. Il
  **reste** à faire réagir `TAPConnectionWindow` à `TERMINATED` : fermer la fenêtre de jeu,
  rouvrir l'écran de connexion, afficher un message.
- **Fermeture de la fenêtre — ➜ non fait** : `QUIT` est envoyé par la barre d'actions, mais
  rien ne ferme la fenêtre sur `OK bye`.
- **Widgets** : chacun s'abonne à sa `Section` et lit `GameState` ; aucun widget ne lit de ligne du protocole.
- **Libellés** : chaque id affiché passe par `catalog.name(id)`, et la description (si elle existe) peut aller dans une info-bulle.

## 10. Étapes

**➜ Bilan : les étapes 0 à 5 sont terminées, l'étape 6 reste à faire.**

| # | Travail | Fini quand |
| --- | --- | --- |
| 0 | **Build du GUI** : dans `clients/gui/CMakeLists.txt`, ajouter `../client.cpp`, `../../shared/system/loadenv.cpp` et `../../shared/system/signalFd.cpp` (et `../state/*.cpp` plus tard), plus les dossiers d'include. Dans le `Makefile` : `$(EXECUTABLE_NAME)` avec parenthèses (aujourd'hui `$EXECUTABLE_NAME` vaut `XECUTABLE_NAME`), `re: fclean all`, et `cmake_minimum_required` à une version réaliste (3.16). | `make` dans `clients/gui` produit `42TAP` sur une machine propre. |
| 1 | **`message`** : `parseLine` et `asJson`. | Un petit exécutable de test passe sur chaque ligne des tableaux du §8, plus des lignes vides ou malformées. |
| 2 | **`catalog`** : chargement, recherche, secours. | Test : `item.herbs#3` → `Healing Herbs` ; id inconnu → libellé de secours ; dossier absent → pas d'erreur. |
| 3 | **`GameState` + `Session`** : FIFO et handlers du §8.1. | **Rejeu scripté sans serveur** : on injecte `OK hello proto=1`, `OK connected`, la réponse d'un `LOOK`, etc., et on vérifie l'état après chaque ligne. |
| 4 | **Événements** : handlers du §8.2. | Même rejeu, scénario à deux joueurs (`PRESENCE`, `STATS`, `CHAT`, `GROUP INVITE`). |
| 5 | **Branchement du GUI** : écran de connexion avec le champ nom, `QTimer` de 50 ms, widgets abonnés, 3 onglets de chat, journal séparé, les boutons du sujet §V.4, compteurs salle et serveur. **Fait**, sauf deux boutons : `STATUS` et `QUESTS` manquent dans `TAPActionButtons`. | Plan de recette du cahier des charges contre notre serveur. |
| 6 | **Interchangeabilité** : tester avec un catalogue absent, puis si possible contre le serveur d'un autre groupe. | Le client reste utilisable ; les écarts trouvés sont notés dans le README. |

Les étapes 1 à 4 se font **sans Qt et sans serveur**, et se testent seules.

## 11. Points de vigilance

- **Ne jamais deviner les PV** localement : ils viennent de `STATUS` ou de la réponse de combat.
- **Pas d'envoi avec un argument vide** : chaque bouton qui demande une cible (`MOVE`, `TAKE`, `DROP`, `TALK`, `ATTACK`, `QUEST`, `GROUP`) doit en avoir une sélectionnée.
- **Noms en plusieurs mots** : l'argument est envoyé en entier (`TAKE Frothy Ale`). Mais quand le client connaît l'id, il envoie l'id.
- **UTF-8** : noms et messages transmis sans altération ; caractères de contrôle neutralisés à l'affichage.
- **Lignes trop longues** : le serveur coupe la connexion au-delà de 1024 octets (`MAX_LINE_LENGTH`). Il faut limiter la saisie du chat en conséquence.
- **README** : documenter le catalogue (où il est lu, ce qui se passe sans lui) dans « Architecture », et toute tolérance ajoutée dans « Protocol Implementation ».
