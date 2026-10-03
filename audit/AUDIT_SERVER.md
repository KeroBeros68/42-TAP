# Audit — Partie Serveur

> Audit de conformité de la partie serveur réalisée à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Périmètre** : `server/` (tous fichiers) + `shared/` (`defines.hpp`, `errors.hpp`, `success.hpp`, `response.hpp`) + `Makefile` racine et `server/Makefile`. `game/ressources/*.json` est cité uniquement pour savoir s'il est consommé.
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++23` propre (le Makefile ajoute `-Werror`) ; tests réseau réels sur une copie compilée avec `-fsanitize=address,undefined` sur le port 18080 (le 8080 était déjà pris par un autre `server` en cours d'exécution). Les points non testés en réel sont marqués *(lecture de code)*.

---

## 🔴 État général : l'infrastructure réseau est solide, mais aucune commande du protocole n'est implémentée — et aucune ne peut l'être en l'état

Le serveur est aujourd'hui un **squelette réseau** (boucle `poll`, framing, dispatcher générique) et non un serveur de jeu. Mesuré :

- **0 commande RFC sur 15 implémentée** (CONNECT, LOOK, MOVE, CHAT, TAKE, DROP, INVENTORY, TALK, ATTACK, STATUS, QUEST, QUESTS, WHO, GROUP, QUIT). Le seul `defineAction` du binaire est le `"TEXT"` de `testMain.cpp`, qui n'existe pas dans la RFC.
- **Blocage structurel** : `handeLine` rejette toute ligne tant que `user.authenticated` est faux (`server.cpp:232`), et **rien dans le code ne passe ce booléen à `true`**. Vérifié en réel : `CONNECT alice` puis `TEXT hi` → `ERR 400 BAD_REQUEST` ×2. Même la commande de test est donc injoignable, et `CONNECT` ne pourra jamais fonctionner tant que la garde n'exempte pas `CONNECT`.
- **Aucun chargement du monde** : `game/ressources/{maps,npc,items,quests}.json` existent (commit `4f5fd9c`) mais aucun code ne les lit — ni parsing, ni validation des sorties (§V.1).
- **Aucun logging conforme** : uniquement des `std::cout` de debug non structurés, sans horodatage, sans IP, sans niveau.
- **README** : 8 lignes (palette de couleurs). Aucune des 9 sections exigées au §VI, ni la première ligne italique obligatoire.

Ce qui est bon est réellement bon (voir ci-dessous) : la couche réseau est la partie la plus délicate d'un serveur MUD et elle est propre. Le travail restant est de la logique de jeu au-dessus d'une base saine.

### Tableau de conformité §V.1 / §IV (serveur)

| Exigence | État |
| --- | --- |
| Commandes RFC (15) | ❌ 0/15 |
| Événements `EVT` (ROOM / GLOBAL / GROUP / STATS) | ❌ aucun émetteur |
| Chargement monde JSON/YAML + validation des références | ❌ |
| Réponse d'erreur conforme sur commande mal formée | ✅ `ERR 400` (code hors RFC, voir `errors.hpp`) |
| Broadcast sans interruption si un client se déconnecte en cours d'envoi | ✅ (voir `server.cpp`) |
| Suppression de l'état joueur **avant** le broadcast de départ | ❌ pas de hook de déconnexion |
| Items uniques / combat / quêtes / monde ≥ 8 salles | ❌ |
| Logging structuré (JSON, niveaux, timestamps, anti-abus) | ❌ |
| Gestion du fragmentation/coalescing TCP (RFC §9.2) | ✅ |
| Limite de longueur de ligne (RFC §9.4, 1024) | ⚠️ partielle |
| Lint / install / run-server / clean (§VII.1) | ⚠️ `lint` et `install` absents |
| README (§VI) | ❌ |

---

## `server/server.cpp` (+ `server.hpp`)

### ✅ Bon

1. **Boucle `poll()` mono-thread avec sockets non bloquantes** — un seul thread, pas de verrous, pas de `std::thread` par client. C'est le bon choix de concurrence pour un monde partagé : l'état du jeu (items uniques, salles, combat) sera modifié séquentiellement, donc les courses sur « qui prend l'objet en premier » sont impossibles par construction. Vérifié : 3 connexions successives + 200 000 lignes envoyées d'un coup, serveur toujours vivant et sans erreur ASAN/UBSAN.
2. **Framing TCP correct dans les deux sens** (RFC §9.2) — `readFrom` accumule dans `c.in`, découpe sur `\n`, gère le `\r` final, et traite plusieurs lignes arrivées dans un seul `recv` (boucle `while ((pos = c.in.find(LINE_END)))`). Un message coupé en deux paquets reste en tampon jusqu'au `\n`. Côté sortie, `reply()` met en file dans `c.out` et `flush()` gère l'écriture partielle avec `EAGAIN` ; `POLLOUT` n'est demandé que si `out` est non vide.
3. **`MSG_NOSIGNAL` sur `send`** + `SIGINT`/`SIGTERM` capturés via `signalfd` plutôt qu'un handler async-signal — pas de `SIGPIPE` fatal quand un client disparaît pendant un broadcast, et l'arrêt propre passe par la boucle principale. Vérifié : `SIGTERM` → fermeture du socket d'écoute puis de chaque socket client, sortie propre.
4. **Dispatcher générique `defineAction(type, std::function<Response(User&, const std::string&)>)`** — le sujet laisse le choix « dispatcher/router ou inline » (§III) ; cette table `unordered_map` permet d'ajouter chaque commande sans toucher à la boucle réseau. Le type de retour `Response` (succès *ou* erreur) force chaque action à produire une ligne protocolaire valide.
5. **Ligne > 1024 octets sans `\n` → `ERR 400` puis fermeture** (`server.cpp:220-223`) — borne la mémoire du tampon d'entrée contre un client qui n'enverrait jamais de `\n` (RFC §9.4 recommande 1024). Vérifié en réel avec 3000 octets sans `\n` : `ERR 400 BAD_REQUEST` reçu avant la coupure.

### ❌ Mauvais

1. **🔴 `authenticated` n'est jamais mis à `true` et `CONNECT` est rejeté par la garde** (`server.cpp:232-235`) — la garde s'applique avant la recherche d'action, donc même une action `CONNECT` définie serait inatteignable. Conséquence vérifiée : toute ligne reçoit `ERR 400`. Correctif minimal : exempter `CONNECT` de la garde et laisser son action poser `user.authenticated = true` + `user.name`, après contrôle d'unicité (`ERR 201 NAME_IN_USE`).
2. **🔴 Aucun hook de déconnexion** — `updatePoll` fait `erase` de la session (`server.cpp:109-118`) sans aucun callback. Le sujet (§IV) exige « remove player state before broadcasting leave events » ; il n'y a aujourd'hui aucun endroit où retirer le joueur de sa salle/groupe, remettre ses objets dans la salle, ni émettre `EVT ROOM PRESENCE LEAVE` / `EVT STATS players=`. À ajouter : un `onDisconnect(User&)` appelé avant l'`erase`.
3. **🔴 Logging absent** (§V.1, 10 exigences) — `std::cout` brut, aucun timestamp, aucun niveau, aucun JSON, `Connection::ip` jamais renseigné (le `client_addr` d'`accept` est lu puis jeté, `server.cpp:153-155`), commandes et réponses non journalisées, aucune détection de flood / connexions rapides. Les messages de debug (« Sending message to client ID… » à chaque `sendTo`) polluent stdout et ralentissent les broadcasts.
4. **Aucune protection contre un client lent / un flood sortant** *(lecture de code)* — `c.out` croît sans limite si un client ne lit pas (`reply` fait `+=`, jamais de seuil). Un seul client malveillant peut faire grossir la mémoire du serveur. Pas non plus de limite de connexions simultanées ni de limite de débit de commandes (RFC §9.4 : « connection count », « chat message frequency »). Si `accept` échoue en boucle (ex. `EMFILE`), `poll` rend aussitôt la main : boucle active avec spam sur `stderr`.
5. **Exception dans une action = crash du serveur** *(lecture de code)* — `it->second(s.user, args)` n'est pas encadré par un `try/catch` (`server.cpp:239`). Une `std::exception` levée par une future action (JSON invalide, `std::stoi`, `at()`) tue tous les joueurs. À encadrer et à convertir en `ERR`.

> Points mineurs : `handeLine` (faute de frappe, `server.cpp:226`) ; la limite de 1024 n'est testée que sur le **reliquat** sans `\n` — une ligne complète de 2000 octets terminée par `\n` passe donc au dispatcher *(lecture de code ; en réel la réponse `ERR 400` observée vient de la garde d'authentification, donc non discriminante)* ; les commandes sont recherchées en sensible à la casse alors que la RFC §4.2 les veut insensibles ; les ids de session sont recyclés (`_available_id`), ce qui peut faire router un événement en attente vers le mauvais joueur si un futur code garde un id au-delà de la vie d'une session ; `sendToAll` envoie aussi aux sessions non authentifiées.

---

## `server/user.hpp` · `server/connection.hpp` (+ `.cpp`)

### ✅ Bon

1. **Séparation `Connection` (réseau) / `User` (jeu)** — `User` ne contient « aucune donnée réseau » (commentaire de `user.hpp`, et c'est vrai). La logique de jeu ne peut donc pas toucher à un `fd`, et les tests de gameplay pourront instancier un `User` sans socket.
2. **`Session` regroupe les deux** et est la seule clé de `_sessions` — une seule table à nettoyer à la déconnexion, pas deux maps à garder cohérentes.
3. **`Connection` porte ses deux tampons et son drapeau `closing`** — le marquage « à fermer » est différé à `updatePoll`, ce qui évite d'invalider un itérateur au milieu d'un broadcast.
4. **`User.id` en `long long`** — pas de débordement réaliste, et un identifiant stable séparé du nom (qui pourra changer de casse / être réutilisé).
5. **Aucune copie lourde** — `Session` est construite sur place via `emplace`, les `Connection&`/`User&` sont passés par référence aux actions.

### ❌ Mauvais

1. **`User` n'a quasiment aucun état de jeu** — pas de `hp`, `room`, `inventory`, `group`, `quests`. Tout le contenu requis par §V.1 (HP 100, respawn, inventaire, quêtes actives/complétées) reste à concevoir ; les `User&` passés aux actions ne leur permettent d'agir sur rien.
2. **`Connection::ip` jamais alimentée** — champ présent mais mort ; c'est la donnée exigée par le logging (« IP addresses »).
3. **`.cpp` de 3 lignes (`connection.cpp`, `user.cpp`)** — sans constructeur utile, ils n'apportent que des objets de plus à compiler. Acceptable tant que ces structs évoluent, sinon à fusionner dans le header.
4. **Pas de `ntohs`/`inet_ntop` ni de port client** conservés — nécessaire dès qu'on veut journaliser « connexions rapides » par adresse.
5. **Pas de notion d'état de protocole** (CONNECTED / AUTHENTICATED / TERMINATED, RFC §2.2) — un seul booléen `authenticated`, pas de moyen de représenter « en cours de fermeture après QUIT » côté `User`.

---

## `server/signalFd.cpp` (+ `.hpp`)

### ✅ Bon

1. **`signalfd` + `sigprocmask(SIG_BLOCK)`** — transforme les signaux en événement `poll`, le seul design correct pour intégrer l'arrêt à une boucle `poll` sans handler asynchrone. Vérifié : `SIGTERM` arrête proprement le serveur.
2. **`SFD_NONBLOCK | SFD_CLOEXEC`** — la lecture ne peut pas bloquer la boucle, et le fd ne fuit pas dans un futur `exec`.
3. **Classe non copiable** (constructeur de copie et `operator=` privés non définis) — empêche un double `close` du même fd.
4. **RAII** — le destructeur ferme le fd.
5. **`read()` retourne `SIGNAL_NONE` si lecture incomplète** — pas de lecture partielle d'une `signalfd_siginfo` interprétée à tort.

### ❌ Mauvais

1. **`open()` renvoie `_fd` (un entier) dans une fonction `bool`** quand déjà ouvert (`signalFd.cpp:16`) — `return _fd;` convertit en `true`, correct par accident (un fd valide est ≥ 0, mais `0` donnerait `false`). À écrire `return true;`.
2. **`SIGPIPE` jamais traité par masque** — le serveur s'appuie uniquement sur `MSG_NOSIGNAL`. OK aujourd'hui, mais tout futur `write()`/`send` sans le flag tuerait le process.
3. **`SIGINT` ignoré si le process est lancé en arrière-plan par un shell non interactif** (observé pendant l'audit : `kill -INT` sans effet, `SIGTERM` OK) — ce n'est pas un bug du code mais à savoir en soutenance : arrêter avec `Ctrl-C` au premier plan, pas `kill -INT` sur un job `&`.
4. **Un seul signal lu par tour de `poll`** (`_sigs.read()` appelé une fois) — sans conséquence ici, mais `SIGHUP`/`SIGUSR1` (rechargement de log, par exemple) ne seront pas distingués sans extension.
5. **Pas de `sigaction` de `SIGPIPE`/`SIG_IGN`** documenté — voir point 2.

---

## `server/testMain.cpp`

### ✅ Bon

1. **Montre l'usage prévu de l'API** — `defineAction` → `start` → `while (getStatus()) updatePoll();` : boucle principale de 4 lignes, lisible.
2. **`try/catch` autour de `start()`** avec code retour 1 — une erreur de `bind` (observée : `Address already in use` quand 8080 est pris) sort proprement.
3. **Pas de `sleep` dans la boucle** — `poll(1000)` fait déjà l'attente, pas d'attente active (le commentaire du fichier le dit à tort comme risque).
4. **Utilise la constante partagée `SERVER_PORT`** plutôt qu'un littéral.
5. **Compile sans warning** avec `-Wall -Wextra` (le Makefile impose `-Werror`).

### ❌ Mauvais

1. **🔴 C'est le `main` du serveur livré** (`server/Makefile`, `SRCS`) alors que le nom et le contenu sont ceux d'un test. Aucun `main.cpp` réel ; aucune action RFC enregistrée.
2. **Action `"TEXT"` hors RFC** — le serveur répond `OK Message received` à une commande qui n'existe pas dans le protocole (et, à cause de la garde d'authentification, elle n'est de toute façon jamais atteinte).
3. **Port non configurable** — pas d'`argv`, ni de variable d'environnement ; `SERVER_PORT` compilé en dur à 8080. Constaté en pratique : impossible de lancer deux instances ou de contourner un port occupé sans recompiler.
4. **Aucun chargement du monde ni initialisation de l'état de jeu** avant `start()`.
5. **Pas de message de log au démarrage/arrêt exploitable** (adresse d'écoute, version du protocole, nombre de salles chargées).

---

## `shared/errors.hpp` · `shared/success.hpp` · `shared/response.hpp`

### ✅ Bon

1. **Les 9 codes d'erreur de la RFC §8.2 sont exacts** (201, 301, 401, 402, 404 ×3, 405, 406, 900, 901) avec les bons noms symboliques ; `tapErrorLine()` produit exactement `ERR <code> <NAME>`. Un `enum class` + `switch` sans `default` fait avertir le compilateur si une valeur est oubliée.
2. **Un seul endroit pour formater les lignes de réponse** — `tapOkLine` / `tapErrorLine` / `Response::line()`. Les formes `OK connected`, `OK bye`, `OK room=`, `OK players=`, `OK group=`, `OK taken=`, `OK dropped=` et `OK hello proto=1` de la RFC sont toutes couvertes par `TapOk`. Empêche les écarts de format entre commandes.
3. **`Response::success/failure` en fabriques statiques** — une action ne peut pas renvoyer un état incohérent (`isOk` + erreur).
4. **Partagé avec les clients** (`shared/`) — les deux côtés parlent le même vocabulaire, bon pour l'interchangeabilité inter-groupes du §II.
5. **`Response.close`** — permet à `QUIT` de demander la fermeture après envoi de `OK bye`, sans que la logique de jeu touche au socket.

### ❌ Mauvais

1. **`BAD_REQUEST` = `ERR 400`, code absent de la RFC** — la RFC ne définit pas de code pour commande inconnue/mal formée, alors que le sujet exige des « protocol-compliant error responses » et un README listant toute déviation. L'écart est commenté dans le code (`errors.hpp:7`) mais **pas documenté dans un README** (§VI « Protocol Implementation »).
2. **Pas de support des événements** — aucun `TapEvt`/`evtLine()` pour `EVT ROOM PRESENCE ENTER|LEAVE`, `EVT ROOM|GLOBAL|GROUP CHAT`, `EVT GROUP INVITE|JOIN|LEAVE`, `EVT STATS players=`. Les formater à la main dans chaque action rouvre le risque d'écart de format que `Response` évite pour les réponses.
3. **Trois enums distincts pour le même code 404** — correct pour la clarté, mais `tapErrorCode` ne permet pas le chemin inverse (code → enum) dont le client aura besoin pour réagir aux erreurs.
4. **Pas de variante `OK` pour les charges JSON structurées** — `TapOk::DATA` prend une `std::string` brute ; aucun échappement/sérialisation JSON partagé n'existe, donc `LOOK`/`ATTACK`/`STATUS`/`QUEST(S)` devront construire du JSON à la main (risque d'injection par nom de joueur ou texte de chat contenant `"` — RFC §9.3).
5. **`defines.hpp` partagé : `MAX_LINE_LENGTH 1024` compte-t-il le `\n` ?** — non précisé ; le serveur teste `c.in.size() > 1024` sur le reliquat, ce qui accepte jusqu'à 1024 octets *sans* `\n`, puis traite sans limite une ligne complète plus longue (vu dans `server.cpp`).

---

## `server/Makefile` · `Makefile` (racine)

### ✅ Bon

1. **Flags stricts** : `-Wall -Wextra -Werror -std=c++23`, build propre.
2. **Dépendances d'en-têtes déclarées côté CLI** (`INCLUDES` liste `client.hpp`, `defines.hpp`…) — modifier un `.hpp` partagé recompile.
3. **Règles de nettoyage cohérentes** (`clean`, `fclean`, `re`) et propagées depuis la racine vers `server`, `clients/cli`, `clients/gui`.
4. **`make run` racine** : build + ouverture du serveur et du client dans deux terminaux détectés automatiquement, avec message d'erreur clair si aucun terminal n'est trouvé.
5. **`Obj/` hors des sources** et binaire cible séparé.

### ❌ Mauvais

1. **Pas de cible `lint` ni `install`** — le §VII.1 exige « install dependencies, run-server, run-client, run-client-gui, lint, clean ». Ici : `run`, `client`, `gui`, `build`, `clean`. Aucun `clang-tidy`/`cppcheck`/`clang-format` n'est installé sur cette machine non plus.
2. **`INCLUDES =` vide dans `server/Makefile`** — modifier `server.hpp`, `connection.hpp`, `../shared/*.hpp` **ne recompile rien** ; risque de binaire incohérent (ODR) après un changement de `User`/`Session`. (Le Makefile CLI le fait correctement.)
3. **`all: $(NAME) run` dans `server/Makefile`** — `make` dans `server/` build **et lance** le serveur en avant-plan ; incompatible avec une CI ou un enchaînement `make && make test`.
4. **Cible `help` vide** (racine et serveur) — affiche seulement « Available commands: » sans rien lister.
5. **`.gitignore` laisse passer les binaires** — `server/server` et `clients/cli/clientCLI` apparaissaient comme non suivis après un build (retirés avec `make fclean` à la fin de l'audit).

---

## Priorités (ordre suggéré)

1. Exempter `CONNECT` de la garde + poser `authenticated`/`name` + unicité du nom (déverrouille tout le reste).
2. Charger `game/ressources/*.json` et valider les sorties (§V.1) ; ajouter un état de jeu à `User`.
3. Hook `onDisconnect` (état retiré **avant** `EVT … LEAVE`).
4. Implémenter les 15 commandes + `EVT` (avec un helper `evtLine` dans `shared/`).
5. Logger JSON (timestamp, niveau, IP, joueur, commande, code de réponse) + détection flood.
6. Plafonner `c.out`, le nombre de connexions et le débit de commandes ; `try/catch` autour des actions.
7. Cibles `lint` / `install`, dépendances `.hpp`, README §VI.
