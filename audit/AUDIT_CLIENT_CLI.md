# Audit — Partie Client CLI

> Audit de conformité du client CLI réalisé à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4, §V.3) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-17 — régénéré après les commits `118b360` (signaux) et `d70cf44` / `a002515` (refactor des constantes et des doublons). Les points corrigés sont retirés ; voir « Corrigés depuis l'audit initial » en fin de document.
> **Périmètre** : `clients/cli/` (`main.cpp`, `Makefile`) + `clients/client.cpp`, `clients/client.hpp`, `clients/defines.hpp` + les en-têtes `shared/` que le client utilise (`defines.hpp`, `line.hpp`, `signalFd.*`). Le client GUI (`clients/gui/`) est hors périmètre, mais réutilise la classe `Client` commune.
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++20` sans warning ; exécution réelle contre le serveur actuel compilé avec ASAN/UBSAN sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine) : session interactive, `QUIT`, `SIGINT`, fin de `stdin`. Les points non testés en réel sont marqués *(lecture de code)*.

---

## ⚠️ État général : un client réseau fiable (temps réel, framing, arrêt propre sur Ctrl+C), mais sans interface, sans paramètres et dont le choix n'est toujours pas documenté

Le §V.3 demande trois choses : afficher les messages en temps réel, rester réactif pendant la saisie, et **documenter** le choix d'interface (RFC brute ou commandes traduites). Mesuré :

- ✅ **Temps réel + saisie simultanée** : `poll()` sur la socket, `stdin` et le `signalfd` dans la même boucle — aucun thread. Vérifié : `CONNECT`, `LOOK`, `MOVE`, `QUIT` avec stdin ouvert, greeting et réponses affichés sans perte (JSON de ~700 octets du `LOOK` compris).
- ✅ **Nouveau — Ctrl+C dit proprement au revoir** : `SIGINT`/`SIGTERM`/`SIGQUIT` sont lus via `signalfd` ; le client envoie `QUIT`, lit la réponse, puis sort. Vérifié : `SIGINT` pendant une session → `[OK] bye`, et le log du serveur montre `command received QUIT` puis `client disconnected` (et non une coupure brutale), code retour `0`.
- ✅ **Approche (1) du sujet** (commandes RFC envoyées telles quelles) : c'est ce que fait le client, sans traduction.
- ❌ **Choix toujours non documenté** — le README du projet fait maintenant 299 lignes, mais sa section « Client interface choice » est encore un `TODO` et « Instructions » un titre vide : il ne tranche pas entre RFC brute et commandes traduites, ce que le §V.3 exige. Il documente en revanche honnêtement que l'adresse est codée en dur.
- ❌ **Serveur et port codés en dur** (`127.0.0.1:8080`) : « To use another address, change these values and rebuild » (README). Impossible de se connecter au serveur d'un autre groupe sans recompiler — or le §II exige l'interchangeabilité, ce que l'évaluateur testera.
- ❌ **Fin de `stdin` toujours destructrice** (voir `client.cpp` ❌ 1) : le client ne peut pas être piloté par un script.
- ⚠️ Le serveur répond à 9 commandes sur 15 et n'émet aucun `EVT` : le client n'a pas pu être éprouvé sur les événements asynchrones (le critère central du §V.3 « keeps receiving events while waiting for user input »).

---

## `clients/cli/main.cpp`

### ✅ Bon

1. **Retry de connexion** (3 tentatives, 5 s d'écart) — vérifié avec serveur absent : « Attempting to connect… / Error: Failed to connect to server / Retrying in 5 seconds… », abandon avec code `1` à la 3ᵉ tentative.
2. **Code de sortie `1` + message sur `stderr`** en cas d'échec — exploitable par un script ou `make run`.
3. **Handlers par type (`OK`, `ERR`, `EVT`)** via `defineAction` — les trois familles de la RFC §4.1 ; les erreurs vont sur `stderr` (redirigeables séparément).
4. **`enableSignals()` avec repli explicite** — si le `signalfd` ne s'ouvre pas, le client le dit (« Ctrl+C will not say QUIT to the server ») au lieu de continuer silencieusement.
5. **Boucle principale minimale** — `while (client.update()) {}` ; toute la logique d'E/S est dans `Client`, donc réutilisable par le GUI.

### ❌ Mauvais

1. **Aucune interface utilisateur au-delà de l'écho brut** — pas de prompt, pas d'aide, pas de `CONNECT` automatique. L'utilisateur doit connaître la RFC et taper `CONNECT <nom>` ; les événements s'afficheront au milieu de la ligne en cours de saisie (aucune gestion de la ligne d'entrée).
2. **Sortie du serveur non assainie** *(lecture de code)* — dialogues de PNJ, noms et le futur `EVT … CHAT` sont écrits tels quels sur le terminal. Le serveur valide les noms (UTF-8 sans contrôle), ce qui ferme le vecteur « nom », mais la RFC §9.2 (« Control Characters ») vise aussi le chat : un joueur pourra envoyer des séquences ANSI (effacement d'écran, retitrage).
3. **Aucun paramètre** — `main()` ne lit pas `argv` ; ni `./clientCLI host port`, ni variable d'environnement.
4. **« Disconnected from server. » et code `0` dans tous les cas** — sortie voulue (`QUIT`, Ctrl+C, Ctrl+D) comme perte de connexion (vérifié le 2026-10-05 : serveur tué pendant que le client tourne → même message, code `0`). Un script ne peut pas distinguer une coupure d'une sortie normale.
5. **`sleep(5)` bloquant pendant le retry**, et « Disconnecting from server… » affiché aussi après une connexion *échouée* (vu en réel) — trompeur. Les signaux ne sont armés qu'après la connexion : un Ctrl+C pendant le retry tue le process par défaut (acceptable, mais asymétrique).

---

## `clients/client.cpp` (+ `client.hpp`)

### ✅ Bon

1. **Framing partagé avec le serveur** — `extractLine` / `splitCommand` (`shared/line.hpp`) remplacent les copies locales : une seule implémentation du découpage sur `\n` et du retrait du `\r` (RFC §9.2). Plusieurs lignes par `recv` et lignes coupées restent gérées.
2. **Écriture complète garantie** — `send()` boucle, relance sur `EINTR`, `MSG_NOSIGNAL`.
3. **Un seul `poll()` sur socket + `stdin` + `signalfd`** (indices calculés dynamiquement) — le client reste réactif pendant la saisie (§IV, §V.3).
4. **`quitOnSignal()`** : `QUIT` puis lecture des dernières réponses (borne par `POLL_TIMEOUT_MS` si le serveur se tait) avant de quitter — le serveur retire le joueur normalement au lieu de détecter une coupure. Vérifié.
5. **API réutilisable par les deux clients** (`defineAction`, `send`, `update`, `fd`, `enableKeyboard`/`enableSignals` marqués « CLI only ») ; RAII (`~Client` appelle `disconnect`).

### ❌ Mauvais

1. **🔴 Fin de `stdin` = déconnexion immédiate, avant d'avoir lu la réponse du serveur** — re-vérifié le 2026-10-17 : `printf 'CONNECT yan\nLOOK\n' | ./cli` envoie les commandes puis quitte sans afficher `[OK] hello proto=1`, `[OK] connected` ni le JSON du `LOOK`, alors que le serveur les a traitées. `readKeyboard` renvoie `false` sur EOF et `update()` le propage. Le chemin signal a, lui, le bon comportement (`QUIT` + lecture) : le réutiliser pour l'EOF suffirait. Le client est **impossible à piloter par un script/pipe** (cas typique d'un test d'évaluation).
2. **`inet_addr()` uniquement** — pas de résolution de nom (`getaddrinfo`), pas d'IPv6, `INADDR_NONE` (= `255.255.255.255`) silencieux pour un host invalide ; `connect()` bloquant sans délai.
3. **`_is_authenticated` / `isAuthenticated()` : état mort** — jamais modifié ; le client ne réagit ni à `OK connected` ni à `ERR 201 NAME_IN_USE`. `quitOnSignal()` envoie `QUIT` même avant `CONNECT` (le serveur répond `ERR 400` ; le client attend alors jusqu'à 1 s) *(lecture de code)*.
4. **Signaux bloqués pour tout le process** (`sigprocmask` dans `SignalFd::open`) — Ctrl+C ne tue plus le client : si la boucle reste bloquée ailleurs (un futur appel bloquant), il ne répond plus. Documenté dans le header, mais piège si le GUI, qui réutilise `Client`, appelle `enableSignals` par erreur.
5. **Tampons et sorties parasites** — `_in` et `_kbd` non bornés *(lecture de code)* ; une ligne saisie > 1024 octets part telle quelle et le serveur coupe (`ERR 400` + fermeture) sans que le client prévienne ; `disconnect()` écrit « Disconnecting from server… » et `handleLine` écrit « No action defined… » depuis du code partagé avec le GUI.

> Point mineur : un `recv` par tour de `poll` ; `client.hpp` inclut ~13 en-têtes système.

---

## `clients/defines.hpp` · `shared/defines.hpp` · `shared/line.hpp` · `shared/signalFd.*`

### ✅ Bon

1. **Constantes réseau désormais toutes dans `shared/`** (`POLL_TIMEOUT_MS`, `RECV_BUFFER_SIZE`, `LINE_END`, `MAX_LINE_LENGTH`, `SIGNAL_NONE`, `PROTOCOL_VERSION`, `RESPONSE_OK/ERR`, `CMD_*`, `FIELD_*`) — `clients/defines.hpp` ne garde que `SERVER_IP`.
2. **`SignalFd` partagé serveur/client** (déplacé dans `shared/`) : même code des deux côtés.
3. **`shared/line.hpp`** : deux fonctions `inline` simples, utilisées par le client et le serveur.
4. **Vocabulaire du protocole centralisé** (`STATUS_*`, `FIELD_*`) : le client pourra valider ou mettre en forme les réponses sans chaînes magiques.
5. **`shared/player_name.hpp`** : `isValidPlayerName` utilisable côté client pour refuser un nom avant envoi.

### ❌ Mauvais

1. **🔴 `SERVER_IP "127.0.0.1"` et `SERVER_PORT 8080` en `#define`** — recompilation obligatoire pour changer de serveur.
2. **Garde `DEFINES_HPP` identique dans `clients/defines.hpp` et `server/defines.hpp`** — collision silencieuse si une unité inclut les deux.
3. **Aucune de ces constantes ne sert encore côté client** hors framing/signaux : `PROTOCOL_VERSION`, `FIELD_*`, `STATUS_*` sont déclarées « the clients read them » mais le client CLI n'analyse aucune réponse.
4. **Macros plutôt que `constexpr`** (aucun typage) pour des dizaines de constantes — contraire à l'esprit « type annotations where supported » du §IV.
5. **`shared/defines.hpp` mélange** protocole, valeurs système (`SYSCALL_ERROR`), taille de tampon et règles de jeu (`MAX_NAME_LENGTH`, `INSTANCE_SEPARATOR`) — un client n'a aucune raison de dépendre de ces dernières.

---

## `clients/cli/Makefile`

### ✅ Bon

1. **Flags stricts** `-Wall -Wextra -Werror` et `-std=c++20` (aligné sur le serveur) ; build propre.
2. **Dépendances `.hpp` déclarées**, y compris les nouvelles (`line.hpp`, `signalFd.hpp`) ; `vpath %.cpp ..:../../shared` pour `signalFd.cpp`.
3. **`clean` / `fclean` / `re`** + `run` qui avertit si le binaire n'existe pas.
4. **Binaire ignoré par git** (`/clients/cli/clientCLI`).
5. **Appelé depuis la racine** via `make client` et `make run`.

### ❌ Mauvais

1. **Pas de cible `lint`** (exigée §VII.1) ; le README le note en `TODO`.
2. **Cible `run` sans passage d'arguments** — empêchera `make run HOST=… PORT=…` quand les paramètres existeront.
3. **Pas de variante ASAN/debug.**
4. **`-pthread` inutile** — client mono-thread.
5. **`INCLUDES` énumère les `.hpp` à la main** (le serveur utilise `wildcard`) — un nouvel en-tête partagé oublié ne déclenche pas de recompilation.

---

## Vu depuis le client : ce que change l'évolution du serveur

- **Ids d'instance `#N`** (`taken=item.herbs#3`, `"npcs":["npc.guard#10"]`) : affichés tels quels ; l'utilisateur doit deviner qu'un nom (`TAKE herbs`) suffit. Le README documente maintenant l'écart côté protocole (§ Deviations #3), pas côté client.
- **Commandes insensibles à la casse** (corrigé côté serveur, vérifié : `connect Zoe`, `look`, `Talk guard`) : le client n'a rien à faire ; le README (§ Deviations #4) est désormais périmé sur ce point.
- **Logs serveur** : chaque commande est journalisée avec le joueur et ses arguments ; un utilisateur qui oublie `CONNECT` génère un `WARN "command before CONNECT"` par commande.
- **Respawn de PNJ et d'items** : un `LOOK` répété voit apparaître de nouveaux ids (`npc.wolf#23`) sans qu'aucun événement ne le signale au client.

## Corrigés depuis l'audit initial (2026-10-02)

- **Ctrl+C coupait le client brutalement** (aucun `QUIT`, le serveur voyait une coupure) → `signalfd` + `QUIT` + lecture de la réponse. Vérifié en réel.
- **Framing dupliqué** client/serveur → `shared/line.hpp`.
- **`POLL_TIMEOUT_MS` défini deux fois, `BUFFER_SIZE` en doublon** → `shared/defines.hpp`.
- **Pas de test de bout en bout possible** (serveur muet) → vérifié : `CONNECT`/`LOOK`/`MOVE`/`TALK`/`TAKE`/`QUIT`.
- **Binaire non ignoré par git** → `.gitignore`.
- **`c++23`** → `c++20` (aligné sur le serveur).

## Priorités (ordre suggéré)

1. **Documenter dans le README le choix d'interface** (RFC brute aujourd'hui) — exigé §V.3 et §VI.
2. **Paramètres `host`/`port`** (et `getaddrinfo`) pour passer l'interchangeabilité §II.
3. **EOF sur `stdin`** : réutiliser `quitOnSignal()` (envoi de `QUIT` + lecture des réponses) avant de quitter ; différencier sortie voulue / coupure par le code retour.
4. Assainir les caractères de contrôle affichés (RFC §9.2) avant que `CHAT` n'arrive.
5. Interface plus agréable (approche (2) du sujet : `go north` → `MOVE north`, `say …` → `CHAT ROOM …`, `help`, mise en forme du JSON, prompt qui survit à un événement).
6. Déplacer les `std::cout` de la bibliothèque `Client` vers des callbacks (`onDisconnect`, `onUnknown`).
7. Supprimer ou relier `_is_authenticated` à `OK connected` / `ERR 201`.
