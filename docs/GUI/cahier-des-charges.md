# Cahier des charges — Client GUI (42 TAP)

**Sources :**

- `docs/TAP-subject-1.4.md` — chapitre IV (Global rules) et § V.4 (GUI Client)
- `docs/protocol-rfc.html` — RFC 42TAP (commandes, événements, codes d'erreur)

**Objet :** lister ce que le client graphique doit implémenter, fenêtre par fenêtre, puis
l'état d'avancement **au 29 octobre 2026** (branche `feat/gui`, commit `c2cc020`).

**Légende :** ✅ fait et branché sur le serveur · ⚠️ fait mais incomplet (le détail dit quoi) ·
❌ à faire

> Ce document est un **cahier des charges vivant** : il a été réécrit après la mise en
> service de la couche `clients/state/`. Les marques ✅ ne veulent plus dire « widget
> posé » mais « l'écran affiche réellement ce que le serveur envoie ».

---

## 1. Contraintes générales

Ces exigences viennent du sujet et s'appliquent au client GUI dans son ensemble, pas à une
fenêtre en particulier.

- ✅ **Toolkit :** Qt 5 Widgets (`find_package(Qt5 COMPONENTS Widgets REQUIRED)`) — *curses est
  explicitement refusé par le sujet*.
- ✅ **Réactivité :** aucun appel réseau bloquant dans le thread graphique. La socket est
  non bloquante et lue depuis la boucle Qt par un `QTimer` de 50 ms qui appelle
  `Client::update(0)` (`clients/gui/main.cpp:67-74`). Les événements s'affichent pendant que
  le joueur tape dans un champ.
- ✅ **Framing :** TCP, UTF-8, un message par ligne terminée par `\n`. Le buffer est dans
  `Client` (`clients/client.cpp`) : il accumule ce qui arrive et ne rend que les lignes
  complètes, donc les messages fragmentés comme les messages collés sont gérés.
- ✅ **Reprise sur erreur :** les 4xx sont des erreurs non fatales, journalisées et sans
  interruption de la partie. Les erreurs de connexion (`connect()` qui échoue, socket perdue)
  sont remontées à `TAPErrorWindow` et ramènent à l'écran de connexion.
- ⚠️ **Ressources par ID ou par nom d'affichage :** le catalogue (`Catalog`) traduit un id en
  nom lisible pour l'affichage (`item.herbs` → « Healing herbs »), et le GUI renvoie toujours
  l'**id** au serveur. Il n'y a **pas de champ libre** acceptant un nom : le joueur choisit
  une entrée dans une liste. L'exigence de la RFC (le serveur accepte les deux formes) est
  donc satisfaite côté serveur, mais le GUI n'offre que la forme id.
- ✅ **Interopérabilité :** le GUI n'utilise que ce que la RFC définit (`OK`, `ERR`, `EVT`) et
  les trois commandes maison (`DEFEND`, `FLEE`, `USE`) sont seulement *émises* par le GUI,
  jamais supposées obligatoires.

---

## 2. Fenêtres

### 2.1 Fenêtre de connexion — `TAPConnectionWindow`

Première fenêtre affichée au lancement.

| État | Possibilité | Détail |
| --- | --- | --- |
| ✅ | Saisie du **nom d'utilisateur** | Pré-rempli avec le login système (`whoami`), modifiable |
| ✅ | Saisie de l'**adresse serveur** | Défaut `127.0.0.1` (`_remote_address`) |
| ✅ | Saisie du **port** | Défaut `8080`, la valeur par défaut du serveur (`_remote_port`) |
| ✅ | Bouton **Connect** | `connectToServer()` : `Client::setServer`, `connect()`, `Session::restart()`, puis `CONNECT <username>` |
| ✅ | Gestion du **déjà pris** | `ERR 201 NAME_IN_USE` → la fenêtre de connexion traduit le message (« This player name is already in use. »), ferme la socket et laisse le joueur réessayer |
| ✅ | Gestion du **serveur injoignable** | Exception de `connect()` → `TAPErrorWindow` avec le message d'erreur système |
| ✅ | Transition | Sur `OK connected` (état `AUTHENTICATED`), la fenêtre de jeu s'ouvre et la fenêtre de connexion se cache |
| ⚠️ | Champs vides | Un `CONNECT` sans nom est refusé par le serveur (`ERR 400`) et affiché dans la fenêtre d'erreur ; il n'y a pas de validation locale avant l'envoi |

Le parcours imposé par la RFC est respecté dans cet ordre : greeting `OK hello proto=1`
(consommé par `Session`), `CONNECT <username>`, `OK connected`, puis entrée en jeu.

### 2.2 Fenêtre d'erreur — `TAPErrorWindow`

Fenêtre de message simple (540×360, un label, `setErrorMessage()` / `getErrorMessage()`).

- ✅ **Définir le message** — utilisé par la fenêtre de connexion.
- ✅ **Erreur bloquante** de connexion : message explicite et bouton de fermeture, le joueur
  peut corriger ses champs et recliquer sur *Connect*.
- ❌ **Erreur non bloquante** (4xx) : elle n'ouvre pas cette modale, ce qui est le bon
  comportement, mais elle n'est visible que dans la **vue de log**. Un bandeau transitoire
  serait plus lisible.
- ❌ **Action associée** au message (Réessayer / Fermer) : il n'y a que la fermeture de fenêtre.

### 2.3 Fenêtre de jeu — `TAPGameWindow`

Fenêtre principale, affichée après authentification réussie. Elle ne lit jamais la socket :
`update_gui(Section)` est appelé par le **seul** slot `Session::onChanged` et ne redessine que
la section qui a changé.

#### 2.3.1 Barre de compteurs joueurs

- ✅ Nombre de **joueurs dans la pièce courante** — `players.size()` de l'état, alimenté par
  `LOOK` et entretenu par `EVT ROOM PRESENCE ENTER/LEAVE`.
- ✅ Nombre de **joueurs sur le serveur** — `server_players`, alimenté par `WHO` et
  `EVT STATS players=N`.
- ✅ Sources : `LOOK`, `WHO`, `EVT ROOM PRESENCE`, `EVT STATS`.

#### 2.3.2 Vue de la pièce

- ✅ **Nom** et **description** de la pièce, depuis le JSON de `LOOK`.
- ✅ **Sorties** — `TAPExitView` + `TAPExitButtons`, un bouton par direction ; le clic envoie
  `MOVE <direction>`, la réponse déclenche un `LOOK` de resynchronisation.
- ✅ **Items présents au sol** — `TAPGroundItemsView` (liste + sélection + bouton), envoie
  `TAKE <id>`.
- ✅ **NPCs présents** — `TAPNPCTalkView` (liste + sélection + boutons TALK / QUEST / ATTACK /
  DEFEND / FLEE).
- ✅ **Autres joueurs présents** dans la pièce — nommés un par un dans `TAPPlayersView`
  (« PLAYERS HERE »), alimenté par le tableau `players` de `LOOK` puis entretenu par
  `EVT ROOM PRESENCE ENTER/LEAVE`.
- ✅ Mise à jour **temps réel** sur réception des événements (`EVT ROOM PRESENCE`,
  `EVT ROOM ITEM TAKEN|DROPPED`, `EVT ROOM SPAWN`, `EVT ROOM COMBAT *`).
- ✅ **Rafraîchissement automatique après chaque `TAKE`/`DROP`** : `applyTake` / `applyDrop`
  mettent à jour l'état de façon optimiste puis renvoient `LOOK` + `INVENTORY`.

#### 2.3.3 Inventaire

- ✅ Liste des items portés, alimentée par `INVENTORY` et par `EVT INVENTORY CHANGED`.
- ✅ Boutons **DROP** et **USE** par item sélectionné (`TAPInventoryView`).
- ✅ Bouton **TAKE** sur la vue des items au sol.
- ✅ Le déplacement d'un item entre la pièce et l'inventaire se reflète dans les deux vues
  après le `LOOK` + `INVENTORY` de resynchronisation.

#### 2.3.4 Vue de chat — **séparée** de la vue de log

Trois onglets distincts dans `TAPChatTabs`, chacun avec sa zone de saisie et son bouton
*Send*, et un historique propre dans `GameState`.

| État | Onglet | Commande d'envoi | Événement d'arrivée |
| --- | --- | --- | --- |
| ✅ | **Global** | `CHAT GLOBAL <message>` | `EVT GLOBAL CHAT <joueur> <message>` |
| ✅ | **Room** | `CHAT ROOM <message>` | `EVT ROOM CHAT <joueur> <message>` |
| ✅ | **Group** | `CHAT GROUP <message>` | `EVT GROUP CHAT <joueur> <message>` |

La séparation avec la vue de log est respectée : `Section::Chat` et `Section::Log` sont deux
sections indépendantes.

#### 2.3.5 Vue de log

Zone défilante en lecture seule (`TAPLogView`), alimentée par `GameState::log`.

- ✅ Les **réponses** du serveur à nos propres commandes.
- ✅ Les **erreurs** (`ERR <code> <nom>`), avec le code conservé dans `last_error`.
- ✅ Les **événements d'ambiance** : entrées/sorties de joueurs, combats, progression de
  quête, groupe, spawn d'objets.
- ✅ `std::cout` du client reste redirigé dans cette vue (`TAPLogStreamBuf`).
- ❌ Horodatage des lignes : la vue affiche le texte brut, sans heure locale.
- ⚠️ Ce que dit un **NPC** (`TALK`) arrive dans le log et non dans une zone de dialogue
  dédiée — voir 2.3.6.

#### 2.3.6 Interaction NPC — dialogue et quêtes

- ✅ Chaque NPC de la pièce est sélectionnable (`TAPNPCTalkView`).
- ✅ `TALK <npc>` — la réplique est écrite dans le journal.
- ⚠️ **Affichage du dialogue** : le sujet § V.4 demande un affichage du dialogue auprès du
  NPC. Aujourd'hui la réponse `OK <texte>` n'apparaît que dans le journal technique
  (`Session::applyTalk` → `log()`). Il manque un panneau de dialogue à côté de la liste NPC.
- ✅ `QUEST <npc>` — la quête proposée ou rendue est mise à jour dans le journal de quêtes
  (id, nom, statut, progression) et son texte dans le log.
- ✅ Erreurs associées : `404 NPC_NOT_FOUND`, `406 NO_QUEST_AVAILABLE`, `405 NPC_NOT_HOSTILE`
  sont journalisées sans casser l'interface.
- ✅ Boutons **ATTACK**, **DEFEND** et **FLEE** disponibles sur le NPC sélectionné ; ils ne
  partent que si un NPC est sélectionné.

#### 2.3.7 Panneau de combat

- ✅ Barre de vie du joueur (`hp` / `max_hp`) et statut (`healthy` / `wounded` / `combat` /
  `dead`), dans `TAPPlayerInfoBar`, alimentée par `STATUS` — seule source des PV. Aucune
  valeur n'est calculée localement ; avant la première réponse elle affiche `HP : ?`.
- ✅ Déclenchement via `ATTACK <npc>`, et tours suivants via `ATTACK` / `DEFEND` / `FLEE`.
- ✅ Exploitation de la réponse :
  `OK {"attacker_hp","target_hp","damage","status","round","escaped","turns"}` →
  `GameState::Combat` mis à jour, événement de la section `Combat`, journal alimenté. Le
  rapport de combat demande ensuite un `STATUS` pour rester aligné sur le serveur.
- ✅ Les commandes de combat propres au projet (`DEFEND`, `FLEE`) sont déjà dans la même zone
  d'actions que `ATTACK` : l'extensibilité demandée est acquise.
- ✅ Les événements de combat des autres joueurs de la salle (`EVT ROOM COMBAT START|ROUND|
  END|TURN|RESOLVED`, `EVT ROOM DEATH`) sont journalisés et rafraîchissent la pièce.

#### 2.3.8 Journal de quêtes

- ✅ **`TAPQuestsView`** — une ligne par quête (`nom [statut] progression`), placeholder
  « (no active quests) » quand la liste est vide.
- ✅ Alimenté par `QUESTS` (`OK [{"quest_id","name","status","progress"}]`) au moment du
  `CONNECT` et après chaque `QUEST`.
- ✅ Distinction des statuts : le statut du serveur (`available`, `active`, `completed`) est
  affiché tel quel entre crochets.
- ⚠️ La RFC ne définit **aucun événement de progression de quête** : le journal se rafraîchit
  uniquement sur action du joueur (ou sur `EVT INVENTORY CHANGED quest`), pas en continu.

#### 2.3.9 Panneau de groupe

- ✅ `GROUP CREATE` → `OK group=<id>`, l'id est affiché.
- ✅ `GROUP JOIN <membre>` → champ pré-rempli avec le nom de celui qui invite, modifiable.
- ✅ `GROUP LEAVE`.
- ✅ `GROUP INVITE <joueur>` → champ de saisie + bouton.
- ✅ `EVT GROUP INVITE <joueur>` → l'invitation est affichée dans le panneau, le champ de
  jointure est pré-rempli.
- ✅ `EVT GROUP JOIN <joueur>` / `EVT GROUP LEAVE <joueur>` → mise à jour de l'affichage.
- ✅ Erreurs `401 NOT_IN_GROUP` et `402 ALREADY_IN_GROUP` journalisées.

#### 2.3.10 Barre d'actions

Le sujet exige des boutons pour **toutes** les actions suivantes :

| État | Action | Où |
| --- | --- | --- |
| ✅ | `LOOK` | `TAPActionButtons` |
| ✅ | `MOVE` | boutons de sortie (`TAPExitButtons`) |
| ✅ | `TAKE` | `TAPGroundItemsView` |
| ✅ | `DROP` | `TAPInventoryView` (+ `USE`) |
| ✅ | `TALK` | `TAPNPCTalkView` |
| ✅ | `ATTACK` | `TAPNPCTalkView` (+ `DEFEND`, `FLEE`) |
| ✅ | `STATUS` | `TAPActionButtons` |
| ✅ | `QUEST` | `TAPNPCTalkView` |
| ✅ | `QUESTS` | `TAPActionButtons` |
| ✅ | `WHO` | `TAPActionButtons` |
| ✅ | `GROUP` | `TAPGroupView` (CREATE / INVITE / JOIN / LEAVE) |
| ✅ | `QUIT` | `TAPActionButtons` |

`TAPActionButtons` porte les **six actions sans cible** — `LOOK`, `INVENTORY`, `STATUS`,
`QUESTS`, `WHO`, `QUIT` — et rien d'autre. Toutes les actions qui demandent une cible sont
portées par l'élément qui la possède (sortie, item, NPC, panneau de groupe), ce qui est
l'ergonomie visée : le bouton générique ne sert qu'aux actions sans argument.

- ⚠️ `QUIT` envoie bien la commande au serveur et reçoit `OK bye`. Le serveur ferme alors la
  socket, `Client::update()` renvoie faux, donc `Session::handleDisconnect()` passe la phase à
  `TERMINATED` et écrit « connection lost » dans le journal — le cas n'est pas particulier à
  `QUIT`, il est commun avec une coupure réseau. Il manque seulement le **retour à l'écran de
  connexion** : la fenêtre de jeu reste affichée. Voir § 4.8 et § 5.2.

---

## 3. Couverture du protocole

### 3.1 Commandes

Toutes les commandes du protocole sont **émises** par le GUI et leurs réponses sont routées
par `Session::handleOk` (sur `pending.command`, la file `_pending` garantissant l'ordre).

| État | Commande | Usage GUI |
| --- | --- | --- |
| ✅ | `CONNECT <username>` | Fenêtre de connexion |
| ✅ | `LOOK` | Vue de la pièce ; renvoyé après `MOVE`, `TAKE`, `DROP`, `USE` |
| ✅ | `MOVE <direction>` | Boutons de sortie |
| ✅ | `CHAT <scope> <message>` | Vue de chat (3 onglets) |
| ✅ | `WHO` | Compteur serveur |
| ✅ | `GROUP CREATE` | Panneau groupe |
| ✅ | `GROUP INVITE <joueur>` | Panneau groupe |
| ✅ | `GROUP JOIN <membre>` | Panneau groupe |
| ✅ | `GROUP LEAVE` | Panneau groupe |
| ✅ | `TAKE <item>` | Vue des items au sol |
| ✅ | `DROP <item>` | Vue d'inventaire |
| ✅ | `INVENTORY` | Vue d'inventaire |
| ✅ | `TALK <npc>` | Dialogue NPC (affiché dans le journal) |
| ✅ | `ATTACK <npc>` | Panneau de combat |
| ✅ | `STATUS` | Barre de vie |
| ✅ | `QUEST <npc>` | Journal de quêtes |
| ✅ | `QUESTS` | Journal de quêtes |
| ✅ | `QUIT` | Envoyé, mais le client ne réagit pas à la réponse |
| ✅ | `DEFEND` / `FLEE` / `USE` *hors RFC* | Panneau de combat / inventaire |

### 3.2 Événements traités

`Session::handleEvent` route tous les `EVT` vers l'état, puis notifie la section concernée.

| État | Événement | Impact GUI |
| --- | --- | --- |
| ✅ | `EVT ROOM PRESENCE ENTER <joueur>` | +1 joueur dans la pièce, ligne de log |
| ✅ | `EVT ROOM PRESENCE LEAVE <joueur>` | −1 joueur dans la pièce, ligne de log |
| ✅ | `EVT ROOM CHAT <joueur> <msg>` | Onglet **Room** |
| ✅ | `EVT GLOBAL CHAT <joueur> <msg>` | Onglet **Global** |
| ✅ | `EVT GROUP CHAT <joueur> <msg>` | Onglet **Group** |
| ✅ | `EVT GROUP INVITE <joueur>` | Invitation affichée, champ JOIN pré-rempli |
| ✅ | `EVT GROUP JOIN <joueur>` | Panneau groupe + log |
| ✅ | `EVT GROUP LEAVE <joueur>` | Panneau groupe + log |
| ✅ | `EVT STATS players=<n>` | Compteur joueurs serveur |
| ✅ | `EVT ROOM ITEM TAKEN|DROPPED` *hors RFC* | Vue pièce + inventaire + log |
| ✅ | `EVT ROOM SPAWN` *hors RFC* | Vue pièce + log |
| ✅ | `EVT ROOM COMBAT START|ROUND|END|TURN|RESOLVED` *hors RFC* | Panneau de combat + log |
| ✅ | `EVT ROOM DEATH <salle>` *hors RFC* | Barre de vie + log |
| ✅ | `EVT ROOM EFFECT` *hors RFC* | Log |
| ✅ | `EVT INVENTORY CHANGED <raison>` *hors RFC* | Inventaire + quêtes |

### 3.3 Codes d'erreur gérés

Toutes les erreurs sont journalisées par `Session::handleErr`, qui enregistre aussi
`last_error` pour que la fenêtre concernée puisse réagir.

| État | Code | Nom | Comportement |
| --- | --- | --- | --- |
| ✅ | 201 | `NAME_IN_USE` | Fenêtre de connexion : message dédié, on ne rentre pas en jeu |
| ✅ | 301 | `NO_EXIT` | Log ; la pièce n'a pas changé |
| ✅ | 400 | (requête invalide) | Log |
| ✅ | 401 | `NOT_IN_GROUP` | Log + panneau groupe |
| ✅ | 402 | `ALREADY_IN_GROUP` | Log + panneau groupe |
| ✅ | 403 | `INVENTORY_FULL` | Log, rien n'est volé au joueur |
| ✅ | 404 | `ITEM_NOT_FOUND` | Log + `LOOK` de resynchronisation (l'item a bougé) |
| ✅ | 404 | `ITEM_NOT_IN_INVENTORY` | Log + `INVENTORY` |
| ✅ | 404 | `NPC_NOT_FOUND` | Log + `LOOK` |
| ✅ | 405 | `NPC_NOT_HOSTILE` | Log, pas de panneau de combat |
| ✅ | 406 | `NO_QUEST_AVAILABLE` | Log |
| ✅ | 407 | `ITEM_NOT_USABLE` | Log |
| ✅ | 408 | `ITEM_NOT_TAKEABLE` | Log + `LOOK` |
| ✅ | 410 | `NOT_IN_COMBAT` | Log |
| ✅ | 429 | `RATE_LIMITED` | Log |
| ✅ | 900 | `CONNECTION_FAILED` | Exception de `connect()` → fenêtre d'erreur, retour à la connexion |
| ⚠️ | — | *socket perdue en cours de partie* | `Session::handleDisconnect()` passe en `TERMINATED` et notifie la section `Connection`, mais **la fenêtre de connexion n'écoute pas ce cas** : la fenêtre de jeu reste affichée avec des données périmées. Le sujet n'exige rien ici (sa règle de déconnexion du § IV vise le **serveur**) : c'est une finition. |

---

## 4. Points de vigilance

1. ✅ **`WHO` — divergence sujet / RFC.** La RFC § 5.2.2 spécifie `OK players=<n>`, l'exemple du
   sujet § V.5 montre un JSON `{"room": [...], "server": n}`. **Tranché : `OK players=<n>`**,
   conforme à la RFC, et la liste des joueurs de la salle vient du tableau `players` de
   `LOOK`. C'est la bonne décision, elle est argumentée dans le README.
2. ✅ **`TALK` — même divergence.** La RFC § 5.4.4 : `OK <texte du dialogue>` (texte brut).
   C'est cette forme qui est utilisée ; le JSON du sujet n'est pas reconnu. À garder.
3. ✅ **Aucun événement pour les PV.** Le projet a ajouté ses propres événements
   (`EVT ROOM EFFECT`, `EVT INVENTORY CHANGED`, `EVT ROOM COMBAT *`) et le GUI redemande
   `STATUS` après chaque action de combat : les PV ne peuvent pas rester faux.
4. ✅ **Liste des joueurs de la pièce.** Reconstruite depuis `LOOK` puis entretenue par
   `PRESENCE ENTER/LEAVE` ; un `LOOK` est renvoyé après chaque `MOVE` et chaque échec de
   ciblage, ce qui resynchronise un joueur arrivé entre-temps.
5. ✅ **Séparation chat / log** : trois onglets de chat et une vue de log distincte
   (`Section::Chat` vs `Section::Log`).
6. ✅ **Port par défaut** : `8080`, la valeur par défaut du serveur. *(L'ancienne version de ce
   document affirmait `4224` ; c'est faux, le sujet ne fixe aucun port et le serveur écoute sur
   8080.)*
7. ✅ **Interaction clavier** : les trois onglets de chat, le champ d'invitation et le champ de
   jointure sont des zones de texte ; les actions ciblées restent à la souris.
8. ❌ **Connexion perdue** : aucun retour à l'écran de connexion, aucun message. **Non exigé
   par le sujet** — le § IV ne demande la gestion des déconnexions que pour le serveur — mais
   c'est la finition la plus visible en démonstration.

---

## 5. État d'avancement

### 5.1 Architecture

Le GUI ne parse pas le protocole. La chaîne est :

```
Client (socket)  →  Session (file _pending + GameState)  →  onChanged(Section)  →  widgets
```

- `Client` (`clients/client.cpp`) — connexion, buffer de lignes, dispatch `OK` / `ERR` / `EVT`.
- `Session` (`clients/state/session.{hpp,cpp}`) — reçoit la socket par référence
  (`Client &_client`), empile chaque commande émise dans `std::deque<Pending> _pending`,
  recolle chaque réponse à sa commande grâce à l'ordre garanti du serveur, et met à jour un
  `GameState`. Aucune dépendance Qt, aucune dépendance à la socket : la couche est
  testable seule. À la connexion réussie elle envoie `LOOK`, `INVENTORY`, `STATUS`, `WHO`,
  `QUESTS`.
- `GameState` (`clients/state/`) — la copie locale du jeu : pièce, joueurs, items, NPCs,
  inventaire, PV, combat, quêtes, groupe, chat par portée, journal, phase.
- `TAPGameWindow::update_gui(Section)` — un `switch` sur la section ; chaque branche
  redessine ses widgets depuis l'état.

### 5.2 Ce qui reste à faire

| Priorité | Point | Fichier |
| --- | --- | --- |
| **Haute** | Réagir à la perte de connexion et au `QUIT` : sur `Phase::TERMINATED`, fermer la fenêtre de jeu, rouvrir la fenêtre de connexion, afficher un message. Aujourd'hui la fenêtre de jeu reste figée. *Non exigé par le sujet (§ IV : serveur uniquement), mais c'est le défaut le plus visible en démonstration* | `tap_connection_window.cpp` / `session.cpp` |
| Moyenne | Afficher le dialogue `TALK` dans un panneau dédié, à côté de la liste des NPC, et non seulement dans le journal | `tap_npc_talk.cpp` / `session.cpp:applyTalk` |
| Faible | Champ libre acceptant un nom d'affichage pour `TAKE` / `TALK` | `tap_ground_items_view`, `tap_npc_talk` |
| Faible | Horodatage des lignes de la vue de log | `tap_log_view.cpp` |
| Faible | `TAPBaseWindow` sans destructeur virtuel, alors qu'elle est destinée à l'héritage | `tap_base_window.hpp` |

### 5.3 Composants

| État | Composant | Fichier | Remarque |
| --- | --- | --- | --- |
| ✅ | Fenêtre de base | `src/windows/base_window/` | `QWidget` + feuille de style sombre |
| ❌ | — destructeur virtuel | `src/windows/base_window/` | Classe destinée à l'héritage, sans `virtual ~` |
| ✅ | Fenêtre de connexion | `src/windows/connection_window/` | UI + socket + `CONNECT` + transition + `NAME_IN_USE` |
| ✅ | Fenêtre d'erreur | `src/windows/error_window/` | Affichée par la fenêtre de connexion en cas d'échec |
| ✅ | Fenêtre de jeu — squelette | `src/windows/game_window/` | Barres en haut, colonne de contenu, colonne actions + log |
| ✅ | Fenêtre de jeu — données | `src/windows/game_window/` | Tout passe par `update_gui(Section)` |
| ✅ | Barre de compteurs | `.../widgets/tap_player_count_bar/` | Salle et serveur, depuis `LOOK` / `WHO` / `EVT STATS` |
| ✅ | Barre d'infos joueur | `.../widgets/tap_player_info_bar/` | Nom, statut, barre `hp / max_hp`, depuis `STATUS` |
| ✅ | Vue de la pièce | `.../widgets/tap_room_view/` | Nom + description depuis `LOOK` |
| ✅ | Vue des sorties | `.../widgets/tap_exit_view/` | Boutons par direction, envoie `MOVE` |
| ✅ | Vue des items au sol | `.../widgets/tap_ground_items_view/` | Liste + sélection, envoie `TAKE` |
| ✅ | Vue d'inventaire | `.../widgets/tap_inventory_view/` | Liste + sélection, envoie `DROP` et `USE` |
| ✅ | Vue des NPCs | `.../widgets/tap_ncp_talk/` | Sélection + `TALK` / `QUEST` / `ATTACK` / `DEFEND` / `FLEE` |
| ✅ | Vue des joueurs de la pièce | `.../widgets/tap_players_view/` | « PLAYERS HERE », un nom par ligne, depuis `LOOK` et `EVT ROOM PRESENCE` |
| ✅ | Journal de quêtes | `.../widgets/tap_quests_view/` | Alimenté par `QUESTS`, pas par des données de test |
| ✅ | Panneau de groupe | `.../widgets/tap_group_view/` | CREATE / INVITE / JOIN / LEAVE + invitation reçue |
| ✅ | Vue de chat | `.../widgets/tap_chat_tabs/` | Onglets Global / Room / Group, saisie + envoi |
| ✅ | Barre d'actions | `.../widgets/tap_action_buttons/` | Les six actions sans cible : `LOOK`, `INVENTORY`, `STATUS`, `QUESTS`, `WHO`, `QUIT` |
| ✅ | Vue de logs | `.../widgets/tap_log_view/` | Réponses, erreurs et événements serveur |
| ✅ | Éléments UI custom | `src/custom_ui_elements/` | `TAPLabel`, `TAPLineEdit`, `TAPGameSectionWidget`, `TAPRoomNameLabel` |
| ✅ | Styles et constantes | `globals.hpp` | Couleurs, dimensions, textes par défaut |
| ✅ | Build | `CMakeLists.txt` + `Makefile` | Compile `client.cpp`, `shared/system/*.cpp` et `clients/state/*.cpp` ; `all`, `run`, `valgrind`, `clean`, `fclean`, `re`, `help` |
| ⚠️ | `ressources.qrc` | `CMakeLists.txt` | Toujours commenté : l'icône est chargée par `app.setWindowIcon(QIcon(":/src/icon.ico"))` mais la ressource n'est pas compilée, donc l'icône est vide |

### 5.4 Build et fuites mémoire

- `CMAKE_CXX_STANDARD 17` côté GUI (le serveur et le CLI compilent en C++20).
- `CATALOG_DEFAULT_DIR` pointe vers `game/ressources/`, ce qui permet au GUI de traduire les
  ids en noms sans serveur.
- `make -C clients/gui valgrind` exécute le binaire sous Valgrind avec
  `clients/gui/valgrind-suppressions.txt` (allocations de Qt, glib/GTK, OpenGL/glvnd, loader
  glibc). Résultat sur Linux / Qt 5.15 : **0 erreur, aucun octet perdu** ; seul du « still
  reachable » (globals et caches des bibliothèques) demeure, ce que Valgrind ne compte pas
  comme fuite.
