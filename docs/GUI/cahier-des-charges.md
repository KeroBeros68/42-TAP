# Cahier des charges — Client graphique (GUI) de 42TAP

| Champ | Valeur |
| --- | --- |
| Projet | 42TAP — *The Answer Protocol*, MUD textuel multi-joueurs |
| Objet du document | Client graphique (GUI) |
| Version | 1.0 |
| Date | 2026-09-26 |
| Statut | À valider par le groupe |
| Références | `docs/TAP-subject-1.4.md` (§V.4), `docs/protocol-rfc.html` (RFC 42TAP) |
| Rédigé par | Équipe 42TAP |

---

## 1. Contexte

### 1.1 Le projet

42TAP est un jeu d'aventure textuel multi-joueurs (« shared-world retro text adventure ») composé de trois programmes et d'un jeu de données :

- un **serveur** qui implémente l'intégralité du protocole et héberge le monde ;
- un **client CLI** (interface texte) ;
- un **client GUI** (interface graphique) — objet de ce document ;
- des **données de monde** statiques (YAML ou JSON).

Le serveur et le client communiquent en TCP, en UTF-8, un message par ligne terminée par `\n`, selon le protocole décrit dans le RFC 42TAP.

### 1.2 Le client GUI dans le projet

Le sujet (§V.4) impose un client graphique qui « donne vie » au monde : affichage temps réel de la pièce, gestion des objets par boutons, chat multi-canaux, combat, quêtes. C'est le composant qui rend le jeu jouable sans mémoriser la syntaxe du protocole.

Deux propriétés le distinguent du client CLI :

1. **Il est évalué sur son ergonomie**, pas seulement sur sa conformité protocolaire.
2. **Il doit être interchangeable** : le client GUI d'un groupe doit pouvoir se connecter au serveur d'un autre groupe, et inversement. Toute facilité d'affichage (traduire une commande conviviale en paquet RFC) est donc interne au client et ne doit jamais modifier le protocole.

### 1.3 Objet du présent document

Définir de manière vérifiable ce que le client GUI **doit** faire, ce qu'il **ne doit pas** faire, et comment sa conformité sera constatée (critères d'acceptation, plan de recette). Il sert de contrat interne au groupe et de base de vérification lors de la soutenance par les pairs.

### 1.4 Conventions

Le vocabulaire de contrainte suit la RFC 2119, traduit ainsi :

| Terme | Sens |
| --- | --- |
| **doit** | Obligatoire. Le non-respect invalide le client. |
| **devrait** | Recommandé. Un écart doit être justifié dans le README. |
| **peut** | Optionnel. |
| **ne doit pas** | Interdit. |

Chaque exigence porte un identifiant (`EF-xx`, `ENT-xx`, `ET-xx`, `ENF-xx`) et une priorité :

| Priorité | Signification |
| --- | --- |
| **M** | *Must* — exigé explicitement par le sujet ou le RFC. |
| **S** | *Should* — nécessaire à une bonne évaluation ergonomique. |
| **C** | *Could* — bonus, à ne traiter qu'après le reste. |

Notations protocole utilisées dans ce document :

| Notation | Sens |
| --- | --- |
| `C → S: …` | message envoyé par le client au serveur |
| `S → C: OK …` | réponse du serveur suite à une commande |
| `S → C: ERR <code> <MESSAGE>` | erreur protocolaire |
| `S → C: EVT <catégorie> <type> <données>` | événement asynchrone |

---

## 2. Périmètre

### 2.1 Dans le périmètre

- La fenêtre (ou les fenêtres) de l'application graphique, leur disposition et leur ergonomie.
- Le **module réseau du client** : connexion TCP, envoi des commandes, réception et analyse des réponses `OK` / `ERR` et des événements `EVT`.
- La traduction entre l'état interne (messages du serveur) et l'état affiché (widgets).
- La consommation de **toutes** les commandes et de **tous** les événements définis par le RFC 42TAP.
- La gestion des cas d'erreur protocolaires et réseau côté client.
- La cible de compilation et l'intégration à l'outil de build du projet.

### 2.2 Hors périmètre

- Le serveur, sa logique de jeu, son système de combat et de quêtes (décisions documentées dans le README du serveur).
- Le format et le contenu des données de monde.
- Le client CLI.
- La persistance (aucune exigence : l'état peut être perdu au redémarrage du serveur).
- Toute modification ou extension du protocole : le client **ne doit pas** inventer de commande ni d'événement.

---

## 3. Contraintes imposées

Ces contraintes viennent du sujet et du RFC ; elles ne sont pas négociables au niveau du groupe.

| ID | Contrainte | Source |
| --- | --- | --- |
| CT-01 | Langage : **C ou C++** (Rust, Go, Zig également autorisés). **Python est strictement interdit** — son usage entraîne l'échec automatique de la peer-review. | §IV |
| CT-02 | Le client doit être une **interface graphique réelle**. `curses` est explicitement considéré comme du texte et non comme un GUI. | §V.4 |
| CT-03 | Transport **TCP**, encodage **UTF-8**, **un message par ligne** terminé par `\n` (`\r\n` accepté en tolérance). | §IV, RFC §2.1 |
| CT-04 | **Conformité stricte au RFC 42TAP** : commandes, arguments, formats de réponse, codes d'erreur, événements. Le client doit supporter **chaque** commande et **chaque** événement du RFC. | §III, §IV |
| CT-05 | Le client doit **rester réactif** pendant la réception d'événements asynchrones : l'interface ne doit jamais se figer. | §IV |
| CT-06 | Aucune persistance n'est requise. | §IV |
| CT-07 | Le code suit les pratiques de lint du langage et inclut les annotations de type là où le langage le permet. | §IV |
| CT-08 | Le projet doit être compilable et exécutable via l'outil de build du groupe (Makefile / CMake), avec au minimum les cibles `install`, `run-client-gui`, `lint`, `clean`. | §VII.1 |

**Décision technique retenue :** C++17 + Qt 5 Widgets, construits via CMake piloté par le `Makefile` existant. Le choix du toolkit est libre d'après le sujet ; il est fixé ici pour le groupe et devra être justifié dans le README.

---

## 4. Exigences fonctionnelles

### 4.1 Connexion et cycle de vie de la session

#### EF-01 — Écran de connexion — **M**
- **Description** : au lancement, l'application affiche un écran de connexion comportant au minimum : un champ **adresse du serveur** (défaut `127.0.0.1`), un champ **port** (le sujet ne fixe pas de défaut ; le groupe doit convenir d'un port et le documenter), un champ **nom de joueur**, et un bouton **Se connecter**.
- **Protocole** : ouverture TCP, puis réception de `S → C: OK hello proto=1`, puis envoi de `C → S: CONNECT <username>`.
- **Critères d'acceptation**
  - Le bouton déclenche la connexion TCP ; l'interface reste utilisable pendant la tentative.
  - L'écran affiche l'issue : succès (`OK connected`) ou erreur (voir EF-02, EF-03).
  - Le champ adresse refusé/non numérique côté port est signalé à l'utilisateur avant toute tentative réseau.
  - L'écran ne passe à la fenêtre de jeu qu'après réception effective de `OK connected`.

#### EF-02 — Gestion de l'échec d'identification — **M**
- **Description** : si le serveur refuse le nom (`S → C: ERR 201 NAME_IN_USE`), l'écran de connexion doit rester affiché, présenter le message d'erreur de façon lisible et permettre de saisir un autre nom sans relancer l'application.
- **Critères d'acceptation**
  - `ERR 201 NAME_IN_USE` affiche un message explicite en français ou en anglais (ex. « Ce nom est déjà utilisé »).
  - Le champ nom reprend le focus ; une nouvelle tentative est possible immédiatement.
  - L'erreur n'est pas noyée dans le journal technique (voir EF-15).

#### EF-03 — Erreur de connexion réseau — **M**
- **Description** : serveur injoignable, port fermé, connexion coupée, ou `ERR 900 CONNECTION_FAILED`.
- **Critères d'acceptation**
  - Un message clair est affiché (adresse + port concernés), sans crash ni blocage de l'application.
  - L'utilisateur peut relancer une tentative.
  - Une déconnexion **inattendue** en cours de partie (serveur qui disparaît) ferme proprement la session : la fenêtre de jeu signale la perte de connexion et propose de revenir à l'écran de connexion. Aucune commande n'est envoyée après la coupure.

#### EF-04 — Déconnexion volontaire — **M**
- **Description** : un bouton **QUITTER** envoie `C → S: QUIT`.
- **Critères d'acceptation**
  - `OK bye` reçu : la socket est fermée et l'application revient à l'écran de connexion (ou se ferme, selon décision à documenter).
  - Une confirmation est demandée avant l'envoi, afin d'éviter une fermeture accidentelle.

#### EF-05 — Machine à états du client — **M**
- **Description** : le client suit les états du RFC (§2.2) : `DISCONNECTED` → `CONNECTED` → `AUTHENTICATED` → `TERMINATED`.
- **Critères d'acceptation**
  - Aucune commande autre que `CONNECT` n'est émise avant l'état `AUTHENTICATED`.
  - L'interface reflète l'état courant (indicateur de connexion visible en permanence).

### 4.2 Exploration du monde

#### EF-06 — Affichage de la pièce courante — **M**
- **Description** : la fenêtre de jeu affiche en temps réel les données de la pièce : identifiant, **nom**, **description**, **sorties**, **objets présents**, **PNJ présents**, **joueurs présents**.
- **Protocole** : `C → S: LOOK` → `S → C: OK <json>` avec la structure
  `{ "room": { "id", "name", "description", "exits": { direction: room-id } }, "players": [...], "items": [...], "npcs": [...] }`.
- **Critères d'acceptation**
  - `LOOK` est émis automatiquement après la connexion et après chaque changement de pièce.
  - Le JSON est analysé avec un parseur réel (pas de découpage par sous-chaînes).
  - Une réponse `OK` non-JSON, vide ou malformée n'entraîne ni crash ni gel : elle est journalisée comme anomalie (ENF-02).
  - Les quatre listes sont affichées, y compris vides (afficher « aucun » plutôt qu'une zone vide ambiguë).

#### EF-07 — Déplacement par les sorties — **M**
- **Description** : les sorties de la pièce sont présentées comme des éléments cliquables (boutons ou liste). Un clic envoie la direction correspondante.
- **Protocole** : `C → S: MOVE <direction>` → `OK room=<room-id>` ou `ERR 301 NO_EXIT`.
- **Critères d'acceptation**
  - Un clic sur une sortie émet exactement la direction fournie par le serveur (le client ne doit pas supposer `north`/`south` : la direction affichée est celle reçue dans `exits`).
  - Après `OK room=…`, un `LOOK` est émis pour rafraîchir la vue.
  - `ERR 301 NO_EXIT` produit un message d'erreur non bloquant et la vue reste cohérente (pas de déplacement fantôme).

#### EF-08 — Présence des joueurs dans la pièce — **M**
- **Description** : la liste des joueurs présents dans la pièce se met à jour **sans action de l'utilisateur**.
- **Protocole** : `S → C: EVT ROOM PRESENCE ENTER <username>`, `S → C: EVT ROOM PRESENCE LEAVE <username>`, plus la liste `players` du `LOOK`.
- **Critères d'acceptation**
  - L'arrivée d'un autre joueur est visible dans la seconde suivant l'événement, sans clic.
  - Le départ retire le joueur de la liste.
  - Un même joueur n'apparaît pas en double (l'événement et le `LOOK` peuvent se recouvrir).

#### EF-09 — Compteurs de joueurs — **M**
- **Description** : le sujet exige des compteurs « joueurs dans la pièce » et « joueurs sur le serveur ». Ils doivent être visibles en permanence.
- **Protocole** : `C → S: WHO`, `S → C: EVT STATS players=<count>`.
- **Critères d'acceptation**
  - Le compteur « pièce » reflète la taille de la liste `players` de la pièce courante.
  - Le compteur « serveur » est initialisé par un `WHO` à la connexion puis maintenu à jour par les événements `EVT STATS players=…` ; le client devrait rafraîchir périodiquement par `WHO` en secours.
  - Voir la **réserve bloquante** §9.1 : le format de réponse à `WHO` diffère entre le RFC et le sujet. Le client doit accepter les deux formes, ou le groupe documente la forme retenue côté serveur.

### 4.3 Objets et inventaire

#### EF-10 — Affichage des objets de la pièce — **M**
- **Description** : les objets présents dans la pièce (liste `items` du `LOOK`) sont affichés avec, pour chacun, une action **PRENDRE**.
- **Critères d'acceptation**
  - La liste reflète l'état du dernier `LOOK`.
  - Le nom affiché est lisible par un humain (voir EF-12 pour ID/nom).

#### EF-11 — Inventaire — **M**
- **Description** : un panneau dédié affiche le contenu de l'inventaire, rafraîchi après chaque opération sur les objets.
- **Protocole** : `C → S: INVENTORY` → `S → C: OK ["item.herbs", "item.loaf_bread"]`.
- **Critères d'acceptation**
  - `INVENTORY` est émis après la connexion, après chaque `TAKE` réussi et après chaque `DROP` réussi.
  - L'inventaire vide est affiché comme tel.
  - Chaque ligne de l'inventaire propose une action **DÉPOSER**.

#### EF-12 — Prise et dépôt d'objet — **M**
- **Description** : boutons **PRENDRE** (parmi les objets de la pièce) et **DÉPOSER** (parmi ceux de l'inventaire).
- **Protocole** : `C → S: TAKE <item-identifier>` → `OK taken=<item-id>` ou `ERR 404 ITEM_NOT_FOUND` ; `C → S: DROP <item-identifier>` → `OK dropped=<item-id>` ou `ERR 404 ITEM_NOT_IN_INVENTORY`.
- **Critères d'acceptation**
  - Le client **doit** accepter aussi bien un **identifiant canonique** (`item.herbs`) qu'un **nom d'affichage** (`Herbs`, `Frothy Ale`) comme `item-identifier` (RFC §8.3). En pratique : le client envoie l'identifiant canonique issu du `LOOK`/`INVENTORY` quand il le connaît, et doit savoir transmettre un nom si l'utilisateur en saisit un manuellement (EF-19).
  - Les **noms multi-mots** sont supportés sans troncature (RFC §8.4) : `TAKE Frothy Ale` est envoyé comme une seule ligne complète, sans découpage sur l'espace au-delà de l'identifiant.
  - Un `ERR 404` n'enlève pas l'objet de la liste (pas de mise à jour optimiste durable) et affiche un message explicite.

#### EF-13 — Mise à jour automatique après TAKE / DROP — **M**
- **Description** : exigence explicite du sujet — après une opération réussie, la vue de la pièce reflète immédiatement la disponibilité des objets **sans action de l'utilisateur**.
- **Critères d'acceptation**
  - Après `OK taken=…` : `LOOK` puis `INVENTORY` sont émis automatiquement ; l'objet disparaît de la liste de la pièce et apparaît dans l'inventaire.
  - Après `OK dropped=…` : `LOOK` puis `INVENTORY` sont émis automatiquement ; l'objet suit le trajet inverse.
  - Un autre joueur prenant/déposant un objet dans la même pièce ne doit pas produire d'état incohérent à l'écran (le `LOOK` reste la source de vérité ; le client devrait rafraîchir périodiquement ou après toute action locale).

### 4.4 Communication

#### EF-14 — Chat à trois canaux séparé du journal — **M**
- **Description** : le sujet exige une vue de chat **distincte** de la vue de log, avec les trois portées **Global**, **Room** (pièce) et **Group** (groupe).
- **Protocole** : `C → S: CHAT <scope> <message>` avec `scope ∈ {GLOBAL, ROOM, GROUP}` ; réception `EVT GLOBAL CHAT <user> <msg>`, `EVT ROOM CHAT <user> <msg>`, `EVT GROUP CHAT <user> <msg>`.
- **Critères d'acceptation**
  - Trois zones ou onglets distincts (Global / Pièce / Groupe), chacun ne recevant que les messages de sa portée.
  - Le message envoyé par l'utilisateur apparaît dans le canal concerné.
  - Un message contenant des espaces est transmis intégralement après le scope, sans re-découpage.
  - Un message `EVT` de chat ne fait **pas** apparaître de ligne dans le journal technique (les deux vues ne doivent pas être confondues).
  - Sélection du canal par l'onglet actif ou par un sélecteur explicite ; le canal d'envoi est visible sans ambiguïté.

#### EF-15 — Journal (log) — **S**
- **Description** : un volet distinct affiche l'historique technique : réponses `OK`, erreurs `ERR <code> <MESSAGE>`, événements non liés au chat.
- **Critères d'acceptation**
  - Chaque ligne est horodatée et distingue visuellement succès / erreur / événement.
  - Le journal est défilable, conserve un historique borné (par ex. les 500 dernières lignes) pour ne pas croître sans limite.
  - Les codes d'erreur sont affichés avec leur libellé (`ERR 404 ITEM_NOT_FOUND`) et, si possible, une traduction lisible.
  - Le journal est le moyen de diagnostic en cas de comportement inattendu (utile à la peer-review).

### 4.5 PNJ, combat et état du joueur

#### EF-16 — Interaction avec les PNJ — **M**
- **Description** : les PNJ de la pièce sont listés avec une action **PARLER** ; le dialogue reçu est affiché de façon lisible (zone dédiée ou bulle dans le journal selon décision à documenter).
- **Protocole** : `C → S: TALK <npc-name>` → `OK <dialogue>` ou `ERR 404 NPC_NOT_FOUND`.
- **Critères d'acceptation**
  - Le clic sur un PNJ émet `TALK` avec l'identifiant du PNJ tel que fourni par le `LOOK`.
  - Le dialogue est affiché intégralement ; voir §9.2 pour les deux formes de réponse possibles (`OK <texte>` vs `OK { "npc": …, "dialogue": … }`) — le client doit gérer les deux ou la forme retenue est documentée.
  - `ERR 404 NPC_NOT_FOUND` affiche une erreur non bloquante.

#### EF-17 — Combat — **M**
- **Description** : les PNJ hostiles proposent une action **ATTAQUER** ; le résultat du combat est affiché (points de vie de l'attaquant et de la cible, dégâts, statut).
- **Protocole** : `C → S: ATTACK <npc-name>` → `OK {"attacker_hp": …, "target_hp": …, "damage": …, "status": …}` ; erreurs `ERR 404 NPC_NOT_FOUND`, `ERR 405 NPC_NOT_HOSTILE`.
- **Critères d'acceptation**
  - Le bouton **ATTAQUER** n'est proposé que pour les cibles attaquables si le serveur fournit l'information ; sinon la commande est envoyée et `ERR 405` est affichée proprement (aucune exception, aucun crash).
  - Les dégâts et les PV sont affichés en clair à chaque échange.
  - Les commandes de combat supplémentaires définies par le groupe (DEFEND, FLEE…) doivent être exposées dans l'interface **si** le serveur les implémente ; leur libellé et leur sémantique sont documentés dans le README.

#### EF-18 — État du joueur (PV et statut) — **M**
- **Description** : une zone permanente (barre d'état ou panneau) affiche les points de vie courants / maximum et l'état de combat.
- **Protocole** : `C → S: STATUS` → `OK {"hp": …, "max_hp": …, "status": "…"}`.
- **Critères d'acceptation**
  - `STATUS` est émis après la connexion, après chaque `ATTACK` et après chaque réception d'un résultat de combat.
  - Une jauge (ou un texte `80 / 100`) reflète la valeur reçue ; la valeur n'est jamais devinée localement.
  - Un état de mort (`hp = 0`) et le respawn sont reflétés dans l'interface (message explicite, mise à jour des PV après respawn).

### 4.6 Quêtes

#### EF-19 — Consultation d'une quête — **S**
- **Description** : depuis un PNJ donneur de quête, une action **QUÊTE** affiche les informations de la quête (identifiant, description, récompense, statut).
- **Protocole** : `C → S: QUEST <npc-name>` → `OK {"quest_id": …, "description": …, "reward": …, "status": …}`, ou `ERR 404 NPC_NOT_FOUND`, `ERR 406 NO_QUEST_AVAILABLE`.
- **Critères d'acceptation**
  - Le JSON de quête est affiché sous forme lisible (pas de dump brut illisible comme seul rendu).
  - `ERR 406 NO_QUEST_AVAILABLE` est présenté comme une information, non comme une panne.

#### EF-20 — Liste des quêtes — **S**
- **Description** : un bouton **QUÊTES** affiche la liste des quêtes actives et terminées avec leur progression.
- **Protocole** : `C → S: QUESTS` → `OK [{"quest_id": …, "status": "active", "progress": "1/3"}, …]`.
- **Critères d'acceptation**
  - Chaque quête est affichée avec son statut et sa progression quand elle est fournie.
  - La liste se met à jour après chaque événement de progression ou action susceptible de la modifier.

### 4.7 Groupes

#### EF-21 — Gestion des groupes — **S**
- **Description** : actions **CRÉER UN GROUPE**, **INVITER**, **REJOINDRE**, **QUITTER LE GROUPE**.
- **Protocole** : `GROUP CREATE` → `OK group=<group-id>` ; `GROUP INVITE <username>` → `OK` ; `GROUP JOIN <leader-name>` → `OK group=<group-id>` ; `GROUP LEAVE` → `OK`. Erreurs `ERR 401 NOT_IN_GROUP`, `ERR 402 ALREADY_IN_GROUP`.
- **Événements attendus** : `EVT GROUP INVITE <leader>`, `EVT GROUP JOIN <user>`, `EVT GROUP LEAVE <user>`.
- **Critères d'acceptation**
  - Une invitation reçue (`EVT GROUP INVITE`) est signalée de manière visible et non intrusive, avec un moyen d'y répondre (`GROUP JOIN <leader>`).
  - L'appartenance au groupe est visible en permanence (nom du groupe ou du chef).
  - Les erreurs 401 / 402 sont affichées avec un message compréhensible.
  - En l'absence de membres de groupe connus, la saisie du nom à inviter/rejoindre est possible manuellement.

### 4.8 Barre d'actions et saisie libre

#### EF-22 — Boutons d'action — **M**
- **Description** : le sujet impose des boutons pour **LOOK, MOVE, TAKE, DROP, TALK, ATTACK, STATUS, QUEST, QUESTS, WHO, GROUP, QUIT**. Ces douze actions doivent être accessibles sans connaître la syntaxe du protocole.
- **Critères d'acceptation**
  - Les douze actions sont présentes et fonctionnelles.
  - Chaque action qui requiert un argument (MOVE, TAKE, DROP, TALK, ATTACK, QUEST, GROUP) guide l'utilisateur vers le choix de la cible (liste déroulante, sélection dans le panneau, boîte de saisie) — le client ne doit pas envoyer de commande à argument vide.
  - Chaque action déclenche l'envoi de **la commande RFC exacte**, vérifiable dans le journal (EF-15).

#### EF-23 — Saisie libre de commande — **C**
- **Description** : un champ de saisie permettant d'envoyer une ligne de commande brute au serveur.
- **Critères d'acceptation**
  - La ligne est envoyée telle quelle, terminée par `\n`, encodée en UTF-8.
  - Une ligne vide ou composée d'espaces n'est pas envoyée.
  - Cette fonction est un outil de diagnostic ; elle ne remplace aucune des exigences précédentes.

#### EF-24 — Affichage des erreurs protocolaires — **M**
- **Description** : toutes les erreurs du RFC (§8.2) sont présentées à l'utilisateur sans jamais interrompre la session : 201, 301, 401, 402, 404 (`ITEM_NOT_FOUND`, `ITEM_NOT_IN_INVENTORY`, `NPC_NOT_FOUND`), 405, 406.
- **Critères d'acceptation**
  - Le code **et** le libellé sont affichés (le code 404 est réutilisé pour trois significations distinctes : seul le libellé lève l'ambiguïté).
  - Une erreur non reconnue est affichée brute, sans être masquée.
  - Une erreur ne doit jamais bloquer l'interface ni laisser la vue dans un état incohérent.

#### EF-25 — Tolérance aux événements inconnus — **S**
- **Description** : le client doit accepter sans erreur un message `EVT` ou `ERR` qu'il ne sait pas interpréter (serveur d'un autre groupe, extension locale).
- **Critères d'acceptation**
  - Un `EVT` inconnu est journalisé et ignoré ; il ne provoque ni exception ni gel.
  - Un message vide, trop long, ou non terminé par `\n` n'interrompt pas la lecture du flux.

---

## 5. Exigences d'interface et d'ergonomie

#### ENT-01 — Écran de connexion — **M**
Reprend EF-01 : titre, champs adresse / port / nom, bouton **Se connecter**, zone d'état (connexion en cours, erreurs). Mise en page centrée, cohérente avec la charte (§ENT-04).

#### ENT-02 — Fenêtre de jeu : zones fonctionnelles — **M**
La fenêtre principale doit comporter au minimum six zones distinctes et identifiable visuellement :

| Zone | Contenu |
| --- | --- |
| **Pièce** | nom, identifiant, description, sorties cliquables |
| **Entités présentes** | objets (action PRENDRE), PNJ (actions PARLER / ATTAQUER / QUÊTE), joueurs présents |
| **Inventaire** | objets portés, action DÉPOSER |
| **Chat** | trois canaux séparés : Global / Pièce / Groupe |
| **Journal** | historique technique horodaté (OK / ERR / EVT) |
| **Barre d'actions et d'état** | les douze boutons d'action, compteurs de joueurs, PV / statut |

#### ENT-03 — Maquette de référence — **S**
Disposition cible (à adapter, mais les regroupements sont contractuels) :

```
┌────────────────────────────────────────────────────────────────────────────┐
│ 42TAP — Village Square                              [connecté : alice]     │
├───────────────────────────────────────┬────────────────────────────────────┤
│ PIÈCE                                 │ CHAT                               │
│ Village Square (loc.square)           │ [ GLOBAL ][ PIÈCE ][ GROUPE ]      │
│ Une place pavée animée, des étals...  │ alice : salut                      │
│                                       │ bob   : on bouge ?                 │
│ Sorties :  ( nord )  ( est )          │                                    │
│                                       │ [ message…                    ][↵] │
├───────────────────────────────────────┼────────────────────────────────────┤
│ OBJETS        [PRENDRE]               │ JOURNAL                            │
│  - Frothy Ale                         │ 12:03:11 OK connected              │
│  - Clé rouillée                       │ 12:03:12 EVT ROOM PRESENCE ENTER   │
│ PNJ           [PARLER][ATTAQUER][QUÊTE]│ 12:03:20 ERR 404 ITEM_NOT_FOUND   │
│  - Village Guard                      │                                    │
│ JOUEURS : alice, bob                  │                                    │
├───────────────────────────────────────┴────────────────────────────────────┤
│ INVENTAIRE :  - item.herbs  [DÉPOSER]                                      │
│               - item.bread  [DÉPOSER]                                      │
├────────────────────────────────────────────────────────────────────────────┤
│ [LOOK][MOVE][TAKE][DROP][TALK][ATTACK][STATUS][QUEST][QUESTS][WHO][GROUP]  │
│ [QUIT]                     Joueurs — pièce : 2 | serveur : 5   PV : 80/100 │
└────────────────────────────────────────────────────────────────────────────┘
```

#### ENT-04 — Charte graphique — **S**
La palette est déjà fixée dans le README du projet ; elle doit être appliquée de façon cohérente (fonds, boutons primaires/secondaires, texte) :

| Rôle | Code |
| --- | --- |
| Fond principal | `#fefae0` |
| Accent / actions principales | `#606c38` |
| Texte foncé / en-têtes | `#283618` |
| Accent secondaire | `#dda15e` |
| Alerte / combat | `#bc6c25` |

- Couleurs sémantiques distinctes pour succès / erreur / information.
- Contraste texte/fond suffisant (lisibilité, y compris pour l'évaluateur).
- Fenêtre par défaut **1080 × 720**, redimensionnable, taille minimale garantissant la lisibilité de toutes les zones.

#### ENT-05 — Retour d'action et états — **S**
- Toute action de l'utilisateur produit un retour visible (envoi, succès, échec).
- Les états notables sont distincts à l'écran : hors ligne, connecté, en combat, mort / respawn, dans un groupe, invite en attente.
- Les zones qui ne sont pas pertinentes à un instant donné sont désactivées plutôt que masquées brutalement (évite les sauts de mise en page).

#### ENT-06 — Respect des limites du sujet — **M**
`curses` et toute interface purement textuelle sont exclus (CT-02) : la fenêtre doit être une vraie fenêtre graphique.

---

## 6. Exigences techniques

#### ET-01 — Séparation réseau / interface — **M**
Le code réseau (socket, encodage, découpage en lignes, analyse des messages) doit être isolé de la couche d'affichage. L'interface **ne doit jamais** contenir d'appel réseau bloquant.

#### ET-02 — Lecture en flux, fragmentation et coalescence — **M**
Le flux TCP n'a aucune notion de message : une ligne peut arriver en plusieurs paquets, et plusieurs lignes peuvent arriver dans un seul paquet (RFC §9.2). Le client doit donc **tamponner** les données reçues et n'émettre un message qu'à la réception du `\n`.

- **Critères d'acceptation** : un message délibérément fragmenté est reconstitué ; deux messages collés dans un même paquet sont traités séparément ; un `\r` final éventuel est retiré ; le décodage est explicite en UTF-8.

#### ET-03 — Modèle d'exécution Qt — **M**
La réception réseau ne doit pas bloquer la boucle d'événements Qt (CT-05). Deux approches acceptables :

- **recommandée** : un objet réseau vivant dans un `QThread` dédié, communiquant avec les widgets par **signaux/slots** (connexions *queued*) ;
- alternative : `QTcpSocket` dans le thread principal (son API est déjà asynchrone et non bloquante).

Dans les deux cas, aucun `waitForReadyRead()` ni boucle d'attente ne doit figer la fenêtre.

#### ET-04 — Analyse des messages — **M**
Un analyseur unique décompose chaque ligne reçue :

| Préfixe | Interprétation | Suite |
| --- | --- | --- |
| `OK` | succès d'une commande | données éventuelles (texte, `clé=valeur`, ou JSON) |
| `ERR <3 chiffres> <MESSAGE>` | échec | affichage en erreur + journal |
| `EVT <catégorie> <type> <données>` | événement asynchrone | routage vers la vue concernée |

- **Critères d'acceptation** : chaque ligne est routée par un seul point d'entrée ; aucune comparaison de chaîne sur le message complet ; les commandes sont reconnues indépendamment de la casse (RFC §4.2) ; les événements sont routés par couple (catégorie, type).

#### ET-05 — Analyse JSON — **M**
Les réponses structurées (`LOOK`, `INVENTORY`, `ATTACK`, `STATUS`, `QUEST`, `QUESTS`) doivent être analysées par un vrai parseur JSON (`QJsonDocument` en Qt), avec gestion explicite des cas d'échec.

- **Critères d'acceptation** : un JSON invalide n'entraîne ni exception ni crash ; l'échec est journalisé ; les champs manquants ont une valeur par défaut sûre.

#### ET-06 — Encodage — **M**
Tout message sortant est encodé en UTF-8 et terminé par `\n`, une ligne par commande. Les accents et caractères non ASCII dans les noms et messages sont transmis sans altération.

#### ET-07 — Tolérance aux ambiguïtés du protocole — **M**
Voir §9 : le client doit gérer les divergences entre le RFC et le sujet, ou la forme retenue doit être documentée côté serveur. Aucune des deux interprétations ne doit provoquer un crash.

#### ET-08 — Limites et robustesse réseau — **S**
- Taille de ligne plafonnée à 1024 octets (RFC §9.4) : une ligne plus longue doit être tronquée ou rejetée proprement, sans saturer la mémoire.
- Aucune écriture bloquante : si l'envoi échoue (`ERR 901 SEND_FAILED` côté serveur, ou erreur socket locale), l'utilisateur est prévenu et la session est arrêtée proprement.
- La fermeture de la socket est idempotente (appelée une seule fois, quel que soit le chemin : QUIT, erreur, fermeture de fenêtre).

#### ET-09 — Structure du code et build — **S**
- Le code du client GUI vit dans `clients/gui/` (l'emplacement de travail actuel `test/` est provisoire et doit être migré).
- Découpage attendu : fenêtre de connexion, fenêtre de jeu, couche réseau, modèles d'état (pièce, inventaire, quête), widgets réutilisables.
- Cibles de build : `install`, `run-client-gui`, `lint`, `clean` (§VII.1). Le client ne doit pas dépendre de `test/` pour être construit.
- Pas de fuite mémoire : la parenté Qt (`QObject` parent/enfant) doit être utilisée pour l'appropriation des widgets, plutôt que des `new` non libérés.

---

## 7. Exigences non fonctionnelles

| ID | Exigence | Critère de vérification |
| --- | --- | --- |
| ENF-01 | **Réactivité** — l'interface ne doit jamais se figer, même en réception continue d'événements. | Test manuel : rafale de messages de chat et de présence ; la fenêtre reste redimensionnable et cliquable pendant l'arrivée des messages. |
| ENF-02 | **Robustesse** — aucun crash sur : serveur absent, coupure brutale, ligne malformée, JSON invalide, événement inconnu, `EVT` sans donnée. | Chaque cas est provoqué et documenté dans le plan de recette (§10). |
| ENF-03 | **Maintenabilité** — code lisible, nommage cohérent, en-têtes séparés pour les déclarations, lint sans erreur. | `make lint` (ou équivalent) passe. |
| ENF-04 | **Performance** — comportement correct avec plusieurs joueurs connectés et un flux de chat soutenu. | Test à 3+ clients, envoi rapide de messages. |
| ENF-05 | **Sécurité côté client** — aucun contenu reçu n'est interprété comme du code ; les caractères de contrôle reçus ne doivent pas perturber l'affichage. | Injection d'une chaîne contenant des caractères de contrôle : affichage neutralisé, aucun effet sur les widgets. |
| ENF-06 | **Documentation** — le README décrit compilation, exécution, architecture, choix de toolkit, et les éventuelles déviations protocolaires. | Relecture du README par un pair non impliqué. |

---

## 8. Matrice de traçabilité (sujet → exigences)

| Exigence du sujet / RFC | Exigences couvrantes |
| --- | --- |
| §V.4 Toolkit graphique réel, `curses` exclu | ENT-06, CT-02 |
| §V.4 Affichage pièce, objets, PNJ, sorties, temps réel | EF-06, EF-07, EF-08, EF-10, ENT-02 |
| §V.4 Inventaire + boutons TAKE / DROP | EF-11, EF-12, EF-22 |
| §V.4 Prise en charge des IDs **et** des noms d'affichage | EF-12, ET-07 |
| §V.4 Mise à jour de la vue après TAKE / DROP | EF-13 |
| §V.4 Chat (Global / Room / Group) séparé du log | EF-14, EF-15, ENT-02 |
| §V.4 Boutons LOOK…QUIT (12 actions) | EF-22, ENT-02 |
| §V.4 Compteurs joueurs pièce / serveur | EF-09, ENT-03 |
| §V.4 Interaction PNJ via TALK + dialogue | EF-16 |
| §IV Client réactif sous événements asynchrones | ET-02, ET-03, ENF-01 |
| §IV TCP / UTF-8 / une ligne par message | CT-03, ET-06 |
| §IV Conformité stricte RFC 42TAP | CT-04, EF-24, EF-25 |
| §IV Pas de persistance | §2.2 |
| §IV Lint et annotations de type | ENF-03, CT-07 |
| §VII.1 Cibles de build `run-client-gui`, `lint`, `clean` | ET-09, CT-08 |
| RFC §3 Greeting + CONNECT + ERR 201 | EF-01, EF-02, EF-05 |
| RFC §5 LOOK / MOVE / ERR 301 | EF-06, EF-07 |
| RFC §5 CHAT / WHO / EVT STATS | EF-09, EF-14 |
| RFC §5 GROUP * / ERR 401, 402 | EF-21 |
| RFC §5 TAKE / DROP / INVENTORY / ERR 404 | EF-10, EF-11, EF-12, EF-13 |
| RFC §5 TALK / ATTACK / STATUS / ERR 405 | EF-16, EF-17, EF-18 |
| RFC §5 QUEST / QUESTS / ERR 406 | EF-19, EF-20 |
| RFC §6 Événements ROOM / GLOBAL / GROUP / STATS | EF-08, EF-09, EF-14, EF-21 |
| RFC §8 Gestion dynamique des ressources (unicité, cycle de vie) | EF-12, EF-13 |
| RFC §8.4 Noms multi-mots | EF-12 |
| RFC §9.2 Fragmentation / coalescence / UTF-8 | ET-02, ET-06 |
| RFC §9.4 Limites (1024 octets) | ET-08 |
| §V.5 Exemples d'interactions | §9 (divergences), plan de recette §10 |

---

## 9. Points ouverts et divergences à traiter

Ces points doivent être tranchés par le groupe **et** documentés dans le README (section « Protocol Implementation »). Le client ne doit dans tous les cas jamais planter sur l'une ou l'autre forme.

### 9.1 Réponse à `WHO` — **divergence RFC / sujet**
- RFC §5.2.2 : `S → C: OK players=<count>`
- Sujet §V.5 : `S → C: OK { "room": ["alice", "bob"], "server": 5 }`

**Décision à prendre** : quelle forme le serveur du groupe émet-il ? Si la forme JSON est retenue, le compteur « pièce » vient de `room` et le compteur « serveur » de `server`. Le client **devrait** accepter les deux formes pour rester compatible avec les serveurs des autres groupes.

### 9.2 Réponse à `TALK` — **divergence RFC / sujet**
- RFC §5.4.4 : `S → C: OK Welcome to my bakery!` (texte libre)
- Sujet §V.5 : `S → C: OK { "npc": "guard", "dialogue": "Stay safe, traveler." }` (JSON)

**Décision à prendre** : le client doit gérer les deux formes ou la forme retenue est documentée. À noter : `OK <json>` et `OK <texte>` sont indiscernables sans tester le préfixe `{`.

### 9.3 Casse de l'événement de statistiques
La table du RFC écrit `EVT STATS players=<count>` (type en minuscules) alors que les autres événements sont en majuscules. Le client **devrait** comparer sans tenir compte de la casse (ET-04).

### 9.4 Absence d'identifiant de corrélation
Le protocole n'associe pas explicitement une réponse `OK` à la commande qui l'a provoquée. Plusieurs commandes peuvent être en vol simultanément (ex. `LOOK` puis `INVENTORY`). **Décision à prendre** : soit le client sérialise ses requêtes (une commande à la fois, réponse attendue avant la suivante), soit il interprète les réponses par leur contenu. La première option est recommandée pour la simplicité et la robustesse.

### 9.5 Codes d'erreur manquants
Le RFC ne définit pas d'erreur pour : commande avant `CONNECT`, commande inconnue, argument manquant, message trop long. Le client **devrait** afficher ces erreurs brutes sans supposer un format (EF-25).

### 9.6 Décisions propres au groupe, à refléter dans l'interface
- Commandes de combat supplémentaires (DEFEND, FLEE, USE_ITEM…) : à exposer si et seulement si le serveur les implémente.
- Commandes de quête supplémentaires (COMPLETE_QUEST, ABANDON_QUEST…) : idem.
- Comportement de `QUIT` côté client : retour à l'écran de connexion ou fermeture de l'application.
- Emplacement et forme d'affichage des dialogues de PNJ et du JSON de quête.

---

## 10. Plan de recette

Chaque scénario est exécuté avant la soutenance. La colonne « Résultat attendu » sert de critère de réussite.

| # | Scénario | Résultat attendu |
| --- | --- | --- |
| T-01 | Connexion nominale : serveur démarré, nom libre | Écran de connexion → fenêtre de jeu ; journal : `OK hello proto=1`, `OK connected`, `OK` de `LOOK` |
| T-02 | Nom déjà utilisé par un autre client | `ERR 201 NAME_IN_USE` affiché sur l'écran de connexion ; nouvel essai avec un autre nom possible sans redémarrer |
| T-03 | Serveur non démarré / mauvais port | Message d'erreur clair, aucune fenêtre de jeu, application toujours réactive |
| T-04 | Couper le serveur en pleine partie | Message de perte de connexion, retour à l'écran de connexion, aucun crash |
| T-05 | Deux clients dans la même pièce | `EVT ROOM PRESENCE ENTER` visible chez l'autre joueur sans action ; compteur « pièce » = 2 |
| T-06 | Chat sur les trois canaux | Messages rangés dans le bon onglet ; aucun message de chat dans le journal ; multi-mots préservés |
| T-07 | `TAKE` puis `DROP` | `OK taken=…` → objet retiré de la pièce, ajouté à l'inventaire **sans clic supplémentaire** ; `DROP` inverse le trajet |
| T-08 | `TAKE` d'un objet absent | `ERR 404 ITEM_NOT_FOUND` affiché ; la liste de la pièce reste inchangée |
| T-09 | `TAKE` avec un nom d'affichage multi-mots (`Herbs`, `Frothy Ale`) | L'identifiant canonique est renvoyé par le serveur ; l'objet se déplace correctement |
| T-10 | `MOVE` vers une sortie valide puis invalide | Sortie valide : nouvelle pièce affichée ; sortie invalide : `ERR 301 NO_EXIT`, vue inchangée |
| T-11 | `TALK` sur un PNJ présent, puis sur un PNJ absent | Dialogue affiché dans les deux formes (§9.2) ; `ERR 404 NPC_NOT_FOUND` propre |
| T-12 | `ATTACK` sur un PNJ hostile, puis sur un PNJ pacifique | Résultat de combat affiché (dégâts, PV) ; `ERR 405 NPC_NOT_HOSTILE` affiché sans crash |
| T-13 | `STATUS` après combat ; joueur à 0 PV | Jauge de PV cohérente ; mort et respawn reflétés à l'écran |
| T-14 | `QUEST` puis `QUESTS` | Informations de quête lisibles ; `ERR 406` présenté comme information |
| T-15 | `GROUP CREATE` / `INVITE` / `JOIN` / `LEAVE` sur deux clients | Invitation signalée au destinataire ; appartenance visible ; `ERR 401` / `402` propres |
| T-16 | `WHO` en cours de partie | Compteur serveur à jour, quel que soit le format de réponse (§9.1) |
| T-17 | Envoi de lignes brutes via la saisie libre (EF-23) | Commandes correctes, visibles dans le journal |
| T-18 | Rafale de messages (flood de chat) | Interface fluide, pas de gel, mémoire stable |
| T-19 | Message entrant contenant des caractères de contrôle / accentues / très long | Affichage neutralisé ou tronqué, aucun effet parasite (ENF-05, ET-08) |
| T-20 | **Interchangeabilité** : client GUI connecté au serveur d'un autre groupe | Connexion et commandes de base fonctionnelles, ou écarts identifiés et expliqués |

---

## 11. Livrables

| Livrable | Emplacement attendu |
| --- | --- |
| Sources du client GUI | `clients/gui/` |
| Intégration au build (`run-client-gui`, `lint`, `clean`) | `Makefile` / `CMakeLists.txt` |
| Sections README : « Building and Running », « Architecture », « Protocol Implementation », « Group Contributions » | `README.md` |
| Le présent cahier des charges, mis à jour si des décisions du §9 sont tranchées | `docs/GUI/cahier-des-charges.md` |

---

## 12. État actuel du code et écarts à combler

Constat au 2026-09-26, à titre indicatif pour la planification (à mettre à jour au fil du projet) :

| Élément | État | Écart avec ce cahier des charges |
| --- | --- | --- |
| `test/gui/connection_window/` | Fenêtre de connexion existante : titre, dimension, couleur de fond, champ adresse, bouton | Il manque le champ **port**, le champ **nom de joueur**, la connexion TCP réelle, l'affichage des erreurs (EF-01, EF-02, EF-03) |
| `test/gui/game_window/` | Fichiers **vides** | Toute la fenêtre de jeu reste à écrire (EF-06 à EF-25, ENT-02 à ENT-05) |
| Couche réseau | **Inexistante** | ET-01 à ET-08 : socket, tampon de lignes, analyseur `OK`/`ERR`/`EVT`, parseur JSON |
| `CMakeLists.txt` | Ne compile que `test/main.cpp` et la fenêtre de connexion | Ajouter la fenêtre de jeu et la couche réseau ; cible Qt5 Widgets déjà en place |
| `Makefile` | Cibles `install`, `clean`, `fclean`, `run`, `re` | Ajouter `run-client-gui`, `run-server`, `run-client`, `lint` (§VII.1) |
| `clients/gui/` | **Vide** | Destination finale du code (ET-09) |
| Palette graphique | Définie dans le README | À appliquer de façon cohérente (ENT-04) |
| Fenêtre par défaut | 1080 × 720, couleur de fond `#fefae0` | Conforme à ENT-04 ; ajouter taille minimale et redimensionnement des zones |
