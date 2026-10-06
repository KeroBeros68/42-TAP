# Cahier des charges — Client GUI (42 TAP)

**Sources :**

- `docs/TAP-subject-1.4.md` — chapitre IV (Global rules) et § V.4 (GUI Client)
- `docs/protocol-rfc.html` — RFC 42TAP (commandes, événements, codes d'erreur)

**Objet :** lister ce que le client graphique doit implémenter, fenêtre par fenêtre, puis
l'état d'avancement à la date du 6 octobre 2026.

**Légende :** ✅ fait et validé · 🟠 en cours d'implémentation · ❌ à faire

---

## 1. Contraintes générales

Ces exigences viennent du sujet et s'appliquent au client GUI dans son ensemble, pas à une
fenêtre en particulier.

- ✅ **Toolkit :** Qt (Qt5 Widgets) — *curses est explicitement refusé par le sujet*.
- ❌ **Réactivité :** le client doit rester utilisable en permanence pendant la réception
  d'événements asynchrones (sujet § IV). Une boucle bloquante dans le thread graphique est
  donc interdite.
- ❌ **Framing :** TCP, UTF-8, un message par ligne terminée par `\n` (0x0A).
  - ❌ Les messages peuvent être **fragmentés** sur plusieurs paquets TCP → il faut bufferiser
    jusqu'au `\n`.
  - ❌ Plusieurs messages peuvent arriver **collés** dans un même paquet → il faut découper
    ligne par ligne.
  - ❌ Limite recommandée par la RFC : 1024 octets par ligne.
- ❌ **Reprise sur erreur :** les erreurs 4xx sont non fatales (on continue), les erreurs 9xx
  (`900 CONNECTION_FAILED`, `901 SEND_FAILED`) imposent de repasser par l'écran de connexion.
- ❌ **Ressources par ID ou par nom d'affichage :** le client doit accepter les deux formes
  (`item.herbs` comme `Herbs`), y compris les **noms multi-mots** (`Frothy Ale`).
- ❌ **Interopérabilité :** les clients CLI et GUI doivent être interchangeables entre groupes.
  Le GUI ne doit donc rien supposer qui ne soit pas dans la RFC.

---

## 2. Fenêtres à implémenter

### 2.1 Fenêtre de connexion — `TAPConnectionWindow`

Première fenêtre affichée au lancement.

| État | Possibilité | Détail |
| --- | --- | --- |
| ✅ | Saisie du **nom d'utilisateur** | Pré-rempli avec le login système (`whoami`) |
| ✅ | Saisie de l'**adresse serveur** | Défaut `127.0.0.1` |
| ✅ | Saisie du **port** | Défaut `4224` |
| 🟠 | Bouton **Connect** | Handler branché, mais n'ouvre pas encore le socket : doit attendre `OK hello proto=1`, envoyer `CONNECT <username>`, attendre `OK connected` |
| ❌ | Gestion du **déjà pris** | `ERR 201 NAME_IN_USE` → message d'erreur, on reste sur la fenêtre |
| ❌ | Gestion du **serveur injoignable** | `ERR 900 CONNECTION_FAILED` / échec de `connect()` → message d'erreur |
| ❌ | Transition | Succès → ferme la fenêtre de connexion, ouvre la fenêtre de jeu |
| ❌ | Champs vides | Validation locale avant envoi (username non vide, port numérique) |

Le parcours imposé par la RFC est strict : `greeting → CONNECT → OK connected`, dans cet
ordre. Toute commande envoyée avant `CONNECT` est hors protocole.

### 2.2 Fenêtre d'erreur — `TAPErrorWindow`

Fenêtre générique de message, à utiliser de deux façons :

- ✅ **Définir le message** — `setErrorMessage()` / `getErrorMessage()` existent.
- ❌ **Erreur bloquante** (connexion impossible, 900/901) : modale, avec un bouton qui ramène
  à la fenêtre de connexion.
- ❌ **Erreur non bloquante** (4xx) : bandeau ou notification transitoire, sans interrompre la
  partie. L'usage en modale pour un `ERR 404 ITEM_NOT_FOUND` serait une mauvaise expérience.
- ❌ **Action associée** au message (Réessayer / Fermer).

### 2.3 Fenêtre de jeu — `TAPGameWindow`

Fenêtre principale, affichée après authentification réussie. Elle se décompose en sections :

#### 2.3.1 Barre de compteurs joueurs

- 🟠 Nombre de **joueurs dans la pièce courante**.
- 🟠 Nombre de **joueurs sur le serveur**.
- ❌ Sources : tableau `players` du JSON de `LOOK`, événements `EVT ROOM PRESENCE ENTER/LEAVE`,
  événements `EVT STATS players=N`, commande `WHO`.

#### 2.3.2 Vue de la pièce

- ✅ **Nom** de la pièce (widget présent, non alimenté par le serveur).
- ✅ **Description** (widget présent, non alimenté par le serveur).
- 🟠 **Sorties** disponibles → un contrôle cliquable par direction, qui envoie `MOVE <direction>`.
- ❌ **Items présents au sol** → liste cliquable, chaque item pouvant être pris (`TAKE`).
- ❌ **NPCs présents** → liste cliquable (voir 2.3.6).
- ❌ **Autres joueurs présents** dans la pièce.
- ❌ Mise à jour **temps réel** sur réception des événements.
- ❌ **Rafraîchissement automatique après chaque `TAKE`/`DROP`** (exigence explicite du sujet
  § V.4).

#### 2.3.3 Inventaire

- ❌ Liste des items portés, alimentée par `INVENTORY` (réponse JSON array).
- ❌ Un bouton **DROP** par item.
- ❌ Un bouton **TAKE** par item présent dans la pièce.
- ❌ Le déplacement d'un item entre la pièce et l'inventaire doit se refléter immédiatement
  dans les deux vues.

#### 2.3.4 Vue de chat — **séparée** de la vue de log

Le sujet impose une séparation nette entre le chat et le log. Trois portées distinctes :

| État | Onglet / zone | Commande d'envoi | Événement d'arrivée |
| --- | --- | --- | --- |
| ❌ | **Global** | `CHAT GLOBAL <message>` | `EVT GLOBAL CHAT <joueur> <message>` |
| ❌ | **Room** | `CHAT ROOM <message>` | `EVT ROOM CHAT <joueur> <message>` |
| ❌ | **Group** | `CHAT GROUP <message>` | `EVT GROUP CHAT <joueur> <message>` |

- ❌ Zone de saisie + sélection de la portée (onglet actif ou menu déroulant).
- ❌ Historique par portée, avec le nom de l'émetteur.
- ❌ Le message envoyé doit apparaître dans le fil correspondant.

#### 2.3.5 Vue de log

Zone distincte du chat, qui reçoit :

- ❌ Les **réponses** du serveur (`OK ...`) à nos propres commandes.
- ❌ Les **erreurs** (`ERR <code> <message>`).
- ❌ Les **événements d'ambiance** : entrées/sorties de joueurs, résultats de combat,
  progression de quête, invitations de groupe.
- ❌ Horodatage, pour rester cohérent avec le logging serveur.

#### 2.3.6 Interaction NPC — dialogue et quêtes

- ❌ Chaque NPC affiché dans la pièce est cliquable.
- ❌ `TALK <npc>` → affichage du dialogue renvoyé par le serveur.
- ❌ `QUEST <npc>` → affichage de la quête proposée (id, description, récompense, statut).
- ❌ Gestion des erreurs associées : `404 NPC_NOT_FOUND`, `406 NO_QUEST_AVAILABLE`.
- ❌ Le bouton ATTACK n'est pertinent que sur un NPC hostile ; `405 NPC_NOT_HOSTILE` doit être
  affiché proprement plutôt que de casser l'interface.

#### 2.3.7 Panneau de combat

- ❌ Barre de vie du joueur (`hp` / `max_hp`), alimentée par `STATUS`.
- ❌ Déclenchement via `ATTACK <npc>`.
- ❌ Exploitation de la réponse d'attaque :
  `OK {"attacker_hp":…, "target_hp":…, "damage":…, "status":"combat"}` → mise à jour des deux
  barres de vie et écriture du résultat dans le log.
- ❌ La RFC laisse le système de combat ouvert (tour par tour, DEFEND, FLEE, formules de dégâts…).
  Le GUI doit être conçu pour accueillir ces commandes supplémentaires **sans refonte** :
  prévoir une zone d'actions de combat extensible.

#### 2.3.8 Journal de quêtes

- ❌ `QUESTS` → liste JSON des quêtes **actives** et **complétées**, avec progression
  (`"progress": "1/3"`).
- ❌ Affichage du statut par quête (`available`, `active`, `completed`).
- ❌ La RFC ne définit aucun événement de progression de quête : le journal devra se rafraîchir
  sur action du joueur (ou par polling discret).

#### 2.3.9 Panneau de groupe

- ❌ `GROUP CREATE` → création, retourne `OK group=<id>`.
- ❌ `GROUP JOIN <id>` → rejoindre, retourne `OK group=<id>`.
- ❌ `GROUP LEAVE` → quitter.
- ❌ `GROUP INVITE <joueur>` → inviter.
- ❌ `EVT GROUP INVITE <...>` → notification d'invitation reçue, avec acceptation/refus.
- ❌ `EVT GROUP JOIN <joueur>` / `EVT GROUP LEAVE <joueur>` → mise à jour de la liste des membres.
- ❌ Affichage des erreurs `401 NOT_IN_GROUP` et `402 ALREADY_IN_GROUP`.

#### 2.3.10 Barre d'actions

Le sujet exige des boutons pour **toutes** les actions suivantes :

| État | Action |
| --- | --- |
| ❌ | `LOOK` |
| ❌ | `MOVE` |
| ❌ | `TAKE` |
| ❌ | `DROP` |
| ❌ | `TALK` |
| ❌ | `ATTACK` |
| ❌ | `STATUS` |
| ❌ | `QUEST` |
| ❌ | `QUESTS` |
| ❌ | `WHO` |
| ❌ | `GROUP` |
| ❌ | `QUIT` |

Les actions contextuelles (TAKE, DROP, TALK, ATTACK, QUEST) ont vocation à être portées par
les éléments concernés (item, NPC) plutôt que par un bouton générique. Le bouton générique
reste utile quand aucune cible n'est sélectionnable.

- ❌ `QUIT` doit être relié à la fermeture de la fenêtre : envoyer `QUIT`, attendre `OK bye`,
  puis fermer le socket proprement.

---

## 3. Couverture du protocole

### 3.1 Commandes

| État | Commande | Usage GUI |
| --- | --- | --- |
| ❌ | `CONNECT <username>` | Fenêtre de connexion |
| ❌ | `LOOK` | Rafraîchissement de la vue pièce |
| ❌ | `MOVE <direction>` | Boutons de sortie |
| ❌ | `CHAT <scope> <message>` | Vue de chat (3 portées) |
| ❌ | `WHO` | Compteur serveur |
| ❌ | `GROUP CREATE` | Panneau groupe |
| ❌ | `GROUP INVITE <joueur>` | Panneau groupe |
| ❌ | `GROUP JOIN <id>` | Panneau groupe |
| ❌ | `GROUP LEAVE` | Panneau groupe |
| ❌ | `TAKE <item>` | Vue pièce / inventaire |
| ❌ | `DROP <item>` | Vue pièce / inventaire |
| ❌ | `INVENTORY` | Panneau inventaire |
| ❌ | `TALK <npc>` | Dialogue NPC |
| ❌ | `ATTACK <npc>` | Panneau combat |
| ❌ | `STATUS` | Barre de vie |
| ❌ | `QUEST <npc>` | Fenêtre de quête NPC |
| ❌ | `QUESTS` | Journal de quêtes |
| ❌ | `QUIT` | Fermeture de fenêtre |

### 3.2 Événements à traiter

| État | Événement | Impact GUI |
| --- | --- | --- |
| ❌ | `EVT ROOM PRESENCE ENTER <joueur>` | +1 joueur dans la pièce, ligne de log |
| ❌ | `EVT ROOM PRESENCE LEAVE <joueur>` | −1 joueur dans la pièce, ligne de log |
| ❌ | `EVT ROOM CHAT <joueur> <msg>` | Onglet **Room** |
| ❌ | `EVT GLOBAL CHAT <joueur> <msg>` | Onglet **Global** |
| ❌ | `EVT GROUP INVITE <...>` | Notification d'invitation |
| ❌ | `EVT GROUP JOIN <joueur>` | Liste des membres + log |
| ❌ | `EVT GROUP LEAVE <joueur>` | Liste des membres + log |
| ❌ | `EVT GROUP CHAT <joueur> <msg>` | Onglet **Group** |
| ❌ | `EVT STATS players=<n>` | Compteur joueurs serveur |

### 3.3 Codes d'erreur à gérer

| État | Code | Nom | Comportement attendu |
| --- | --- | --- | --- |
| ❌ | 201 | `NAME_IN_USE` | Fenêtre de connexion : message, on ne rentre pas en jeu |
| ❌ | 301 | `NO_EXIT` | Log ; sortie non disponible |
| ❌ | 401 | `NOT_IN_GROUP` | Log + panneau groupe |
| ❌ | 402 | `ALREADY_IN_GROUP` | Log + panneau groupe |
| ❌ | 404 | `ITEM_NOT_FOUND` | Log ; rafraîchir la pièce (l'item a bougé) |
| ❌ | 404 | `ITEM_NOT_IN_INVENTORY` | Log ; rafraîchir l'inventaire |
| ❌ | 404 | `NPC_NOT_FOUND` | Log ; rafraîchir la pièce |
| ❌ | 405 | `NPC_NOT_HOSTILE` | Log ; ne pas ouvrir le panneau de combat |
| ❌ | 406 | `NO_QUEST_AVAILABLE` | Log ; dialogue NPC |
| ❌ | 900 | `CONNECTION_FAILED` | Erreur bloquante → retour connexion |
| ❌ | 901 | `SEND_FAILED` | Erreur bloquante → retour connexion |

---

## 4. Points de vigilance

1. ❌ **`WHO` — divergence sujet / RFC.** La RFC § 5.2.2 spécifie `OK players=<n>`, alors que
   l'exemple du sujet § V.5 montre `OK { "room": ["alice", "bob"], "server": 5 }`. Il faut
   trancher avec l'équipe serveur et gérer les deux formes en attendant.
2. ❌ **`TALK` — même divergence.** RFC § 5.4.4 : `OK <texte du dialogue>` (texte brut). Sujet
   § V.5 : `OK { "npc": "guard", "dialogue": "..." }` (JSON). À arbitrer.
3. ❌ **Aucun événement pour les PV.** Ni les dégâts subis hors de notre propre `ATTACK`, ni les
   contre-attaques ne sont poussés au client. Seul `STATUS` donne l'état courant → prévoir un
   rafraîchissement après action de combat.
4. ❌ **Liste des joueurs de la pièce.** Aucun événement ne fournit la liste complète : elle se
   reconstruit à partir du tableau `players` de `LOOK` puis s'entretient avec les
   `PRESENCE ENTER/LEAVE`. Un joueur arrivé pendant qu'on était hors ligne est invisible
   jusqu'au prochain `LOOK` → prévoir un `LOOK` de resynchronisation.
5. ❌ **Séparation chat / log** : exigence explicite du sujet, facile à rater en mettant tout
   dans une seule zone de texte.

---

## 5. Avancement actuel

### 5.1 Ce qui existe

| État | Composant | Fichier | Remarque |
| --- | --- | --- | --- |
| ✅ | Fenêtre de base | `src/windows/base_window/` | `QWidget` + feuille de style sombre |
| ❌ | — destructeur virtuel | `src/windows/base_window/` | Classe destinée à l'héritage, sans `virtual ~` |
| ❌ | — config commune | `src/windows/base_window/` | Titre et taille non mutualisés, répétés dans chaque fenêtre |
| ✅ | Fenêtre de connexion — UI | `src/windows/connection_window/` | 3 champs + bouton + accesseurs |
| 🟠 | Fenêtre de connexion — réseau | `src/windows/connection_window/` | `connectToServer()` branché sur le bouton, mais ne fait qu'un `std::cout` |
| ✅ | Fenêtre d'erreur — UI | `src/windows/error_window/` | 540×360, un label + setter/getter |
| ❌ | Fenêtre d'erreur — branchement | `src/windows/error_window/` | Instanciée nulle part |
| ✅ | Fenêtre de jeu — squelette | `src/windows/game_window/` | Empile 2 widgets, 720×1080 |
| ❌ | Fenêtre de jeu — fonctionnalités | `src/windows/game_window/` | Voir § 2.3 |
| ✅ | Barre de compteurs — UI | `.../widgets/tap_player_count_bar/` | 2 labels + setters |
| ❌ | Barre de compteurs — données | `.../widgets/tap_player_count_bar/` | Valeurs codées en dur dans `main.cpp` (42 / 21) |
| ✅ | Vue de la pièce — nom + description | `.../widgets/tap_room_view/` | 2 labels + setters |
| ❌ | Vue de la pièce — items/NPCs/sorties/joueurs | `.../widgets/tap_room_view/` | Absents |
| ✅ | Éléments UI custom | `src/custom_ui_elements/` | `TAPLabel`, `TAPLineEdit`, `TAPGameSectionWidget`, `TAPRoomNameLabel` — styles uniquement |
| ✅ | Styles et constantes | `globals.hpp` | Couleurs, dimensions, textes par défaut |
| ✅ | Build | `CMakeLists.txt` + `Makefile` | `all`, `run`, `valgrind`, `clean`, `fclean`, `re`, `help` |

**Détail des fenêtres existantes :**

- **`TAPConnectionWindow`** — champs username (pré-rempli via `whoami`), adresse (`127.0.0.1`)
  et port (`4224`) pré-remplis ; bouton *Connect*. `connectToServer()` se contente d'un
  `std::cout`. Accesseurs `getUsername()` / `getRemoteAddress()` / `getRemotePort()` présents.
- **`TAPErrorWindow`** — 540×360, titre `42 TAP Error`, un label, `setErrorMessage()` /
  `getErrorMessage()`. Instanciée nulle part.
- **`TAPGameWindow`** — 720×1080, empile la barre de compteurs et la vue de pièce, puis un
  stretch. Expose `update_players_in_room_label()`, `update_total_players_label()`,
  `update_room_name()`, `update_room_description()`.
- **`main.cpp`** — `QApplication`, icône, gestion de SIGINT (`quit` via la queue d'événements
  Qt). Les fenêtres connexion et erreur sont **commentées** ; seule la fenêtre de jeu est
  affichée, avec des compteurs codés en dur (42 / 21).

### 5.2 Ce qui manque

- 🟠 **Couche réseau du GUI** — amorcée : `connectToServer()` est branché sur le bouton, mais
  aucun socket n'est ouvert.
- ❌ **`clients/client.cpp` n'est pas listé dans `CMakeLists.txt`** (la classe `Client` existe
  côté CLI mais n'est pas utilisée par le GUI).
- ❌ Aucun envoi de commande.
- ❌ Aucun parsing de réponse.
- ❌ Aucun parsing JSON.
- ❌ Aucun traitement d'événement asynchrone.
- ❌ Vue de chat absente.
- ❌ Vue de log absente.
- ❌ Inventaire absent.
- ❌ Barre d'actions absente.
- ❌ Panneau de combat absent.
- ❌ Journal de quêtes absent.
- ❌ Panneau de groupe absent.
- ❌ Barre de vie absente.
- ❌ Aucune navigation entre fenêtres (connexion → jeu, erreur → connexion).
- ❌ `ressources.qrc` est commenté dans le `CMakeLists.txt`.

### 5.3 Chemin critique

Dans l'ordre d'implémentation :

1. ❌ **Connexion réseau** — brancher `TAPConnectionWindow` sur le socket, gérer
   `greeting → CONNECT → OK connected`, puis la transition vers la fenêtre de jeu.
2. ❌ **Boucle de réception** — lecture non bloquante + découpage par `\n` + dispatch
   `OK` / `ERR` / `EVT`, compatible avec la boucle d'événements Qt.
3. ❌ **Parsing `LOOK`** — alimenter la vue de pièce et les compteurs.
4. ❌ **Chat + log** — les deux vues séparées, avec les trois portées.
5. ❌ **Inventaire et items** — TAKE / DROP / INVENTORY et rafraîchissement automatique.
6. ❌ **NPC** — TALK et QUEST.
7. ❌ **Combat** — STATUS, ATTACK, barres de vie.
8. ❌ **Quêtes et groupe** — QUESTS, QUEST, GROUP *.

Les blocs 1 à 4 constituent le socle : la barre d'actions ne peut être branchée qu'une fois le
dispatch en place, et aucune action contextuelle n'est possible avant.
