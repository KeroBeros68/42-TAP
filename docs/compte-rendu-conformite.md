# Compte-rendu de conformité — 42-TAP

| Champ | Valeur |
| --- | --- |
| Projet | TAP — *The Answer Protocol* (MUD multi-joueurs) |
| Référentiel | `docs/TAP-subject-1.4.md` (sujet v1.4) + RFC 42TAP (`docs/protocol-rfc.html`) |
| Branche évaluée | `feat/gui` @ `ef1bfc9` |
| Date | 2026-10-28 |
| Équipe | `kebertra` (Kevin Bertrand), `gtourdiat` (Gaspard TOURDIAT) |

> **Mise à jour du 28 octobre 2026 (même jour).** Ce compte-rendu a été rédigé sur l'état du
> dépôt, puis les corrections de documentation ont été appliquées. Sont désormais **traités** :
> B1 (Instructions), B2 (Group Contributions), B3 (Resources), B4 (Testing), B5 (choix
> d'interface CLI — approche 1 documentée), I1 à I6 (toutes les inexactitudes du README), ainsi
> que F4 dans la documentation (`make lint` : l'écart vient de clang 12, `g++` passe les 28
> fichiers). Restent ouverts, et sont les seuls points bloquants : **B6** (boutons `STATUS` et
> `QUESTS`), **F1** (GUI figé après `QUIT` ou perte de connexion), **F2** (dialogue `TALK`
> seulement dans le journal), **F3** (pas de champ libre par nom), **F5** (pas de tests
> automatisés) et l'hygiène git du §5.4 (le PDF supprimé et les trois identités de Gaspard).
> Le verdict ci-dessous est conservé tel qu'il a été rendu ; il reste valable pour ces points.

---

## 1. Verdict

> **Le code peut être rendu. La documentation de rendu l'est désormais aussi ; le client GUI a encore trois finitions.**

Le cœur du projet — serveur + client CLI + données du monde — est **conforme et fonctionnel** : les 15 commandes de la RFC sont implémentées, les événements sont poussés, le monde dépasse largement les minimums, le combat et les quêtes sont jouables, le logging est structuré et l'anti-abus opérationnel. Sur ces points, le projet est prêt pour la soutenance.

La documentation de rendu, qui bloquait ce verdict, est **réparée** : les quatre sections exigées par le chapitre VI sont remplies, le choix d'interface CLI du §V.3 est tranché et justifié, et les six affirmations fausses ont été corrigées. Il reste au GUI trois finitions visibles (§5.3) : les boutons `STATUS` et `QUESTS` du §V.4, l'absence de retour à l'écran de connexion après `QUIT` ou une coupure, et un dialogue `TALK` qui n'apparaît que dans le journal.

**Estimation** : toutes les exigences « serveur » sont couvertes ; il manque environ une demi-journée de rédaction pour que le rendu soit complet, et deux à trois heures de finitions GUI.

---

## 2. Méthode de vérification

Tout n'a pas été lu : les affirmations ci-dessous viennent soit de **tests réels** sur le binaire serveur, soit de la **lecture du code**, et la distinction est indiquée.

- Build complet à froid (`make fclean && make build`, `make build-gui`) : **OK**, aucun warning sur les flags de build (`-Wall -Wextra -Werror`).
- Sessions réseau réelles avec `nc` sur des ports dédiés : connexion, exploration, objets, combat à deux joueurs, mort/respawn, groupes, quêtes, événements, cas d'erreur, flood.
- Inspection des données : `game/ressources/{room,npc,items,quests}.json` (comptages et graphe d'exitus recalculés par script).
- Validation du monde : exit cassé injecté volontairement → le serveur **refuse de démarrer** avec un message explicite (puis données restaurées).
- **Non testé** : l'interface Qt en conditions graphiques réelles (pas de serveur X disponible ici ; seul un lancement `offscreen` a été fait). Les points GUI ci-dessous sont donc issus de la lecture du code.

---

## 3. Conformité par exigence du sujet

### 3.1 Chapitre IV — Règles globales

| Exigence | État | Preuve |
| --- | --- | --- |
| Langage : C, C++, Rust, Go ou Zig (Python interdit) | ✅ | C++20 uniquement, aucun fichier Python dans le dépôt |
| TCP, UTF-8, un message par ligne terminé par `\n` | ✅ | `shared/protocol/line.hpp`, `shared/text/utf8.*` |
| Conformité stricte RFC (codes, formats) | ✅ | Réponses `OK`/`ERR <code> <NOM>` vérifiées ; 12 écarts documentés et justifiés |
| Lint + annotations de types | ⚠️ | `make lint` existe et passe **avec g++**, mais **échoue avec le `c++` par défaut de la machine** (clang 12) — voir §5.4 |
| Déconnexion propre, état retiré avant broadcast du départ | ✅ | `onDisconnect` appelé avant `erase` (`server/network/server.cpp:329`), vérifié à l'exécution |
| Clients réactifs pendant les événements asynchrones | ✅ | CLI : `poll` sur socket + clavier + `signalfd` ; GUI : `QTimer` 50 ms sur une socket non bloquante |
| Pas de persistance requise | ✅ | Aucune écriture d'état, cohérent |

### 3.2 Chapitre V.1 — Serveur

| Exigence | État | Preuve |
| --- | --- | --- |
| Toutes les commandes et événements du protocole | ✅ | `registerCommands` en enregistre 15 + `DEFEND`, `FLEE`, `USE` (`server/protocol/commands.cpp:378-432`) |
| Monde statique YAML/JSON | ✅ | 4 fichiers JSON dans `game/ressources/` |
| Validation des exits et des références | ✅ | Test réel : exit `north → loc.nowhere` ⇒ démarrage refusé, code retour 1 |
| Commandes malformées → erreur conforme | ✅ | Vérifié : `LOOK` avant `CONNECT` → `ERR 400`, `MOVE up` → `ERR 301 NO_EXIT`, ligne de 2000 octets → `ERR 400` + déconnexion, noms en double → `ERR 201` |
| Broadcast sans interruption si un client se coupe | ✅ | Sockets non bloquantes + `MSG_NOSIGNAL` ; `Broken pipe` journalisé en `WARN`, serveur intact (`server/network/server.cpp:264`) |
| **Objets : instances uniques** | ✅ | Ids d'instance internes (`item.herbs#3`), ids de définition sur le fil, comme la RFC |
| **Prise retire de la salle / dépôt libère** | ✅ | Vérifié en bi-joueur : `TAKE` puis `DROP` d'Alice ⇒ Bob reçoit `EVT ROOM ITEM DROPPED` et voit l'objet au `LOOK` |
| **Référencement par id ou nom, noms multi-mots** | ✅ | `TAKE herbs`, `TAKE healing herbs`, `TALK guard hector` tous acceptés |
| Monde : ≥ 8 salles, boucle(s), branche optionnelle | ✅ (large) | **15 salles**, **2 boucles** indépendantes, 3 impasses + 2 branches |
| Monde : ≥ 3 rôles de PNJ | ✅ | 16 définitions : 3 `dialogue_npc`, 3 `quest_giver`, 10 `enemy` |
| Monde : ≥ 4 objets dont ≥ 2 récupérables | ✅ | 8 objets, 7 récupérables (`item.fountain` volontairement non prenable) |
| Quêtes : ≥ 2, progression/validation/récompense conçues | ✅ | 3 quêtes, 3 types (`deliver`, `fetch`, `defeat`) ; boucle complète vérifiée (herbes → prêtresse → charme + `EVT INVENTORY CHANGED quest`) |
| Exploration : circuit complet possible | ✅ | Vérifié sur le graphe : `square → village → plain → plain2 → square` |
| Combat : 100 PV de départ | ✅ | `STATUS` → `{"hp":100,"max_hp":100,...}` |
| Combat : PV ennemis variables | ✅ | 5 PV (louveteau) à 200 PV (Dark Lord, défini non placé) |
| Combat : `ATTACK` blesse et riposte possible | ✅ | Vérifié : riposte du loup dans la même réponse (`turns[]`) |
| Combat : `STATUS` affiche PV et état | ✅ | Vérifié, y compris champs de combat additionnels |
| Combat : 0 PV ⇒ respawn avec PV réduits | ✅ | Testé jusqu'à la mort contre un orc : `hp:0, status:"dead"` puis `EVT ROOM DEATH loc.chapel`, réapparition à 50 PV |
| Combat : résultats journalisés et diffusés | ✅ | `EVT ROOM COMBAT *` observés par le second joueur de la salle ; `combat started/turn/player died` dans les logs |
| Combat : tour par tour, initiative, `DEFEND`/`FLEE` documentés | ✅ | README §Combat System complet ; règles pures dans `game/combat/combat.cpp` |
| **Logging : connexions/déconnexions avec horodatage + IP** | ✅ | `client connected` / `client disconnected` avec `ts` et `ip` |
| **Logging : chaque commande avec joueur et paramètres** | ✅ | `command received` avec `player` et `args` |
| **Logging : chaque réponse et code d'erreur** | ✅ | `response sent` avec `ok` et la ligne |
| **Logging : changements du monde (objets, PNJ, combat)** | ✅ | `player took/dropped an item`, `npc died`, `combat turn`, `… respawned` |
| **Logging : progression et fin de quête** | ✅ | `quest started`, `quest progress`, `quest completed` |
| **Logging : format structuré (JSON)** | ✅ | Une ligne JSON par événement, vérifié sur un log réel |
| **Logging : niveaux INFO/WARN/ERROR** | ✅ | + `DEBUG`, filtrable par `LOG_LEVEL` |
| **Logging : détection d'abus** | ✅ | Vérifié : 120 `WHO` en rafale ⇒ 19 `OK` + **101 `ERR 429 RATE_LIMITED`** et `WARN command flooding` ; plafonds clients/IP/connexions documentés |
| **Logging : horodatage précis, flux appropriés** | ✅ | millisecondes + fuseau ; INFO→stdout, WARN/ERROR→stderr, `LOG_FILE` optionnel |
| **Logging : pas d'impact sur les performances** | ✅ | Niveau testé **avant** construction de la ligne ; `info` recommandé en production |

### 3.3 Chapitre V.3 — Client CLI

| Exigence | État | Preuve |
| --- | --- | --- |
| Connexion + affichage temps réel | ✅ | Vérifié : réponses et `EVT` s'affichent pendant l'attente de saisie |
| Choix d'interface (RFC brute **ou** commandes traduites) | ✅ *(était un `TODO`)* | Approche 1 assumée et justifiée dans le README (`### 💬 Client interface choice`) |
| Reçoit les événements pendant la saisie | ✅ | Boucle `poll` unique, vérifié avec un second joueur qui entre/sort |

### 3.4 Chapitre V.4 — Client GUI

| Exigence | État | Commentaire |
| --- | --- | --- |
| Toolkit graphique (pas curses) | ✅ | Qt 5 Widgets, build CMake OK |
| Salles, objets, PNJ, sorties, mises à jour temps réel | ✅ | Widgets dédiés + `LOOK` de resynchronisation |
| Inventaire + boutons `TAKE` / `DROP` | ✅ | `tap_ground_items_view`, `tap_inventory_view` (+ `USE`) |
| Gère ids **et** noms d'affichage | ⚠️ partiel | Affiche les noms, renvoie les ids ; pas de champ libre acceptant un nom |
| Rafraîchissement auto après `TAKE`/`DROP` | ✅ | `applyTake`/`applyDrop` envoient `LOOK` + `INVENTORY` (`clients/state/session.cpp:499-530, 690-715`) |
| Chat (Global/Room/Group) séparé du journal | ✅ | Onglets vs `TAPLogView` |
| Boutons pour **toutes** les actions du §V.4 | ⚠️ | **`STATUS` et `QUESTS` n'ont pas de bouton** (`tap_action_buttons.cpp:8-10` : `LOOK`, `WHO`, `QUIT` seulement). Les données sont rafraîchies automatiquement, mais l'exigence porte sur les boutons |
| Compteurs joueurs salle + serveur | ✅ | `TAPPlayerCountBar` alimentée par `LOOK` et `EVT STATS` |
| Interaction PNJ via `TALK` **avec affichage du dialogue** | ⚠️ | La réplique part **uniquement dans le journal technique** (`Session::applyTalk` → `log()`), il n'y a pas d'affichage dédié |

**Défaut fonctionnel GUI à corriger** : le bouton `QUIT` envoie bien la commande, mais `Session` n'a **aucun gestionnaire pour `QUIT`** ni pour l'état `TERMINATED` ; côté GUI, rien ne réagit à `Phase::TERMINATED`. Après un `QUIT` (ou une coupure serveur), la fenêtre de jeu **reste ouverte avec des données périmées**, sans retour à l'écran de connexion ni message. C'est visible en démonstration.

### 3.5 Chapitre VI — Exigences du README

| Exigence | État |
| --- | --- |
| Première ligne en italique avec les logins | ✅ `*This project has been created as part of the 42 curriculum by kebertra and gtourdiat.*` |
| Section **Description** | ✅ |
| Section **Instructions** | ✅ *(était vide)* — prérequis et démarrage rapide, renvoi vers *Building and Running* |
| Section **Resources** (références + usage de l'IA) | ✅ *(était vide)* — RFC, man pages, Qt, Valgrind, palette ; usage de l'IA détaillé |
| Section **Architecture** | ✅ |
| Section **Protocol Implementation** (écarts justifiés) | ✅ 12 écarts documentés |
| Section **Combat System** | ✅ complet (formules, initiative, commandes) |
| Section **Quest System** | ✅ complet |
| Section **World Design** | ✅ (voir inexactitudes §5.2) |
| Section **Server Logging** | ✅ complet |
| Section **Group Contributions** | ✅ *(était vide)* — tableau par zone de code + détail par membre, avec les commits cités |
| Section **Building and Running** | ✅ détaillée |
| Section **Testing** | ✅ *(était vide)* — multi-joueurs, combat, quêtes et un tableau de cas limites, tous tirés de sessions réelles |
| README rédigé en anglais | ✅ |

> ⚠️ La correction porte aussi sur les sections « Instructions » et « Client interface choice », **explicitement nommées** dans le sujet.

### 3.6 Chapitre VII — Rendu et évaluation par les pairs

| Exigence | État |
| --- | --- |
| Sources serveur, CLI, GUI | ✅ |
| Fichiers de données du monde | ✅ |
| Outil de build avec `install deps`, `run-server`, `run-client`, `run-client-gui`, `lint`, `clean` | ✅ toutes présentes dans le `Makefile` racine (+ `fclean`, `re`, `valgrind`) |
| README (exécution, architecture, contributions) | ⚠️ contributions absentes, Instructions vides |
| Équipe de 2-3 | ✅ 2 |
| Contributions de chaque membre dans git | ✅ 151 commits / 95+46 commits selon l'identité utilisée* |
| Messages de commit propres, gestion de branches | ✅ convention `feat(scope)/fix(scope)` respectée |

\* Gaspard apparaît sous trois identités git (`Gaspard TOURDIAT`, `sousampere`, `gtourdia`) : c'est cosmétique, mais si un correcteur filtre par auteur il verra une contribution sous-estimée. Fusionner ces identités avant le rendu est un plus.

---

## 4. Ce qui est solide (à ne pas retoucher)

1. **Le serveur est de qualité professionnelle.** Séparation nette réseau / jeu, dispatcher par table de handlers, boucle `poll` mono-thread assumée et justifiée (l'état du monde n'est jamais accédé concurremment — la course sur un objet identique se résout gratuitement). Codes d'erreur centralisés dans `shared/`, aucun texte libre dans les lignes `ERR`.
2. **La validation du monde au démarrage** est réelle et utile : exit cassé, référence inconnue, statistiques invalides ⇒ refus de démarrer avec un message nommant fichier et salle.
3. **Le protocole est respecté au point où l'interopérabilité inter-groupes fonctionne** : ids de définition sur le fil, JSON tolérant les champs supplémentaires, `WHO players=<n>` conforme à la RFC (le sujet page 12 le contredit — le choix est argumenté, c'est la bonne décision).
4. **Les événements sont cohérents et pensés** : `COMBAT RESOLVED` et `ROOM DEATH` sont envoyés à toute la salle alors que les autres événements excluent l'acteur, pour une raison documentée (sinon le joueur ne sait jamais qu'il est mort ou que le butin est tombé).
5. **L'anti-abus est réellement actif**, pas décoratif : mesure à l'appui.
6. **Les quêtes sont complètes** : les trois types, validation manuelle, refus si la récompense ne rentre pas dans l'inventaire sans rien voler au joueur, événement de changement d'inventaire pour les clients.
7. **La couche `clients/state/`** (Session + GameState) est une bonne architecture : le GUI ne parse pas le protocole, il lit un état ; la file `_pending` recolle chaque réponse à sa commande grâce à l'ordre garanti du serveur.

---

## 5. Points à corriger avant le rendu

### 5.1 Bloquants (exigences explicites du sujet)

| # | Point | Où |
| --- | --- | --- |
| ~~B1~~ | ~~Remplir **Instructions**~~ | **fait** |
| ~~B2~~ | ~~Remplir **Group Contributions**~~ | **fait** |
| ~~B3~~ | ~~Remplir **Resources**~~ | **fait** |
| ~~B4~~ | ~~Remplir **Testing**~~ | **fait** |
| ~~B5~~ | ~~Trancher le **choix d'interface CLI**~~ | **fait** |
| B6 | Ajouter des **boutons `STATUS` et `QUESTS`** au GUI (§V.4) | `clients/gui/.../tap_action_buttons.cpp:8-10` |

### 5.2 Inexactitudes factuelles du README (faux ⇒ pénalisant en soutenance)

| # | Affirmation | Réalité |
| --- | --- | --- |
| I1 | « 9 of the 15 RFC commands are implemented » (`:46`) | **15/15** + 3 hors RFC — *corrigé* |
| I2 | « The GUI client is not connected to the server yet » | Connecté, fonctionnel — *corrigé* |
| I3 | « 15 NPC definitions » | **16** (3 dialogue, 3 quêtes, 10 ennemis) — *corrigé* |
| I4 | « no item of the current world uses it yet » (respawn) | `item.herbs` et `item.silver_bell` ont `respawn_seconds: 60` — *corrigé* |
| I5 | `tools/ mapviz` | Le dossier `tools/` n'existe pas — ligne *supprimée* |
| I6 | « This section will be updated as the missing parts are completed » | Statut faux — section *réécrite* |

### 5.3 Défauts fonctionnels à traiter

| # | Défaut | Sévérité |
| --- | --- | --- |
| F1 | GUI : après `QUIT` ou coupure serveur, la fenêtre de jeu reste figée, sans message ni retour à la connexion (aucun traitement de `Phase::TERMINATED`, aucun handler `QUIT` dans `Session`) | **Élevée** (visible en démo) |
| F2 | GUI : le dialogue `TALK` n'apparaît que dans le journal, pas dans une zone dédiée (§V.4 « with dialogue display ») | Moyenne |
| F3 | GUI : pas de champ libre permettant de désigner un objet/PNJ par nom (seulement par sélection d'id) | Faible |
| ~~F4~~ | ~~`make lint` échoue avec le `c++` par défaut~~ — **documenté** : l'échec vient de clang 12 (`-Wgnu-zero-variadic-macro-arguments` sur les `LOG_*` sans argument variable, `-Wsign-conversion` sur `recv`) ; avec g++ et les mêmes flags, les 28 fichiers passent. Le README et `docs/architecture-serveur-client.md` le disent | **fait** (documentation) |
| F5 | Aucun test automatisé dans le dépôt | Faible (le sujet demande d'expliquer comment tester, pas d'automatiser) |

### 5.4 Hygiène du dépôt

- `docs/TAP-subject-1.4.pdf` est **supprimé du working tree sans commit** (`git status: D`). Le restaurer (`git checkout -- docs/TAP-subject-1.4.pdf`) ou committer la suppression, mais ne pas rendre avec un arbre sale.
- Le README cite le PDF ; le supprimer créerait un lien mort.
- Trois identités git pour le même membre (voir §3.6).

---

## 6. Conclusion

**Le projet peut être rendu**, et il peut passer avec une bonne note : l'essentiel du travail — un serveur conforme, robuste, journalisé, un monde plus grand que demandé, un combat et des quêtes réellement jouables — est fait et vérifié en conditions réelles.

La documentation n'est plus le problème : elle est complète et conforme au barème du chapitre VI. Ce qui reste tient en trois finitions GUI et une demi-heure d'hygiène git.

**Ordre de priorité** :

1. **B6** — ajouter les boutons `STATUS` et `QUESTS` à `tap_action_buttons.cpp` (deux lignes) : c'est la dernière exigence écrite du §V.4 non satisfaite.
2. **F1** — réagir à `Phase::TERMINATED` : fermer la fenêtre de jeu, rouvrir l'écran de connexion, afficher un message. C'est le seul défaut visible en démonstration.
3. **F2** — afficher le dialogue `TALK` dans un panneau dédié ; puis F3 et F5 si le temps le permet.
4. **Hygiène git** — restaurer ou committer la suppression du PDF, et fusionner les trois identités de Gaspard avant le rendu.

Une fois les points 1 et 2 traités, le projet est prêt à être rendu sans réserve.
