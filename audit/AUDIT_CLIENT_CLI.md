# Audit — Partie Client CLI

> Audit de conformité du client CLI réalisé à ce jour, par rapport au sujet officiel (`docs/TAP-subject-1.4.md`, v1.4, §V.3) et à la RFC 42TAP (`docs/protocol-rfc.html`).
>
> **Date de l'audit** : 2026-10-02
> **Dernière mise à jour** : 2026-10-05 — **le code client n'a pas changé** depuis le 2026-10-02 (aucun commit sur `clients/`) ; en revanche le serveur répond désormais à `CONNECT`/`LOOK`/`MOVE`, ce qui a permis de **vérifier en réel des points restés « lecture de code »** (marqués ci-dessous). Seul `shared/defines.hpp` a évolué (voir « Mouvements côté serveur »).
> **Périmètre** : `clients/cli/` (`main.cpp`, `Makefile`) + `clients/client.cpp`, `clients/client.hpp`, `clients/defines.hpp` + `shared/defines.hpp`. Le client GUI (`clients/gui/`) est hors périmètre, mais réutilise la classe `Client` commune.
> **Méthode** : 5 points positifs max et 5 points négatifs max par fichier, chacun justifié par référence au sujet / à la RFC, ou par le comportement réel du code. Même méthodologie que `AUDIT_SANDBOX.md` (projet agent_smith).
> **Vérification** : build `-Wall -Wextra -std=c++23` propre ; exécution réelle contre le serveur actuel compilé avec ASAN/UBSAN sur le port 18080 (le 8080 est utilisé par un autre `server` sur la machine). Les points non testés en réel sont marqués *(lecture de code)*.

---

## ⚠️ État général : un client « netcat amélioré » correct sur le plan réseau, mais sans identité, sans ergonomie et non testable contre un autre serveur

Le §V.3 demande trois choses : afficher les messages en temps réel, rester réactif pendant la saisie, et **documenter** le choix d'interface (RFC brute ou commandes traduites). Mesuré :

- ✅ **Temps réel + saisie simultanée** : `poll()` sur la socket **et** `stdin` dans la même boucle — aucun thread, aucun blocage sur `std::cin`.
- ✅ **Approche (1) du sujet** (commandes RFC envoyées telles quelles) : c'est de fait ce que fait le client, sans traduction.
- ❌ **Choix non documenté** — le README fait 8 lignes ; il manque la mention du choix d'interface exigée au §V.3, ainsi que toutes les sections du §VI.
- ❌ **Serveur et port codés en dur** (`127.0.0.1:8080`, `clients/defines.hpp`, `shared/defines.hpp`) : impossible de se connecter au serveur d'un autre groupe — or le §II exige que les clients soient **interchangeables entre groupes**, ce qui est précisément ce qu'un évaluateur testera.
- ✅ **Premier test de bout en bout réussi** (nouveau) : stdin ouvert, saisie différée : `CONNECT zed` → `[OK] connected`, `LOOK` → `[OK] {"room":…}`, `MOVE south` → `[OK] room=loc.forest`. Greeting, réponses et JSON longs affichés sans perte.
- ⚠️ **Seules 3 commandes sur 15 sont réelles côté serveur** : le client ne peut pas encore être éprouvé sur les `EVT` (aucun émis) ni sur `QUIT` (le serveur répond `OK Message received` sans fermer).

---

## `clients/cli/main.cpp`

### ✅ Bon

1. **Logique de connexion avec retry** (3 tentatives, 5 s d'écart) — vérifié en réel avec serveur absent : « Attempting to connect… / Error: Failed to connect to server / Retrying in 5 seconds… », puis abandon avec code retour `1` après la 3ᵉ tentative.
2. **Code de sortie `1` explicite en cas d'échec de connexion**, avec message sur `stderr` — exploitable par un script ou un `make run`.
3. **Handlers par type de message (`OK`, `ERR`, `EVT`)** via `defineAction` — les trois familles de la RFC §4.1 sont distinguées ; les erreurs vont sur `stderr`, ce qui permet de les rediriger séparément.
4. **Boucle principale minimale** — `while (client.update()) {}` : toute la logique d'E/S est dans `Client`, `main` ne fait qu'enregistrer les handlers, ce qui rend la réutilisation par le GUI naturelle.
5. **Préfixes `[OK]` / `[ERR]` / `[EVT]` lisibles** — l'utilisateur distingue réponse à sa commande et événement asynchrone (§V.3 « displays incoming messages in real time »).

### ❌ Mauvais

1. **Aucune interface utilisateur au-delà de l'écho brut** — pas de prompt, pas d'aide, pas de liste de commandes, pas d'envoi automatique de `CONNECT`. L'utilisateur doit connaître la RFC et taper `CONNECT <nom>` à la main ; les événements arrivent en plein milieu de la ligne en cours de saisie (aucune gestion de la ligne d'entrée, ni `readline`, ni redessin).
2. **Sortie des événements non assainie** *(lecture de code)* — le contenu de `EVT … CHAT` est écrit tel quel sur le terminal. La RFC §9.2 (« Control Characters ») demande de rejeter ou traiter prudemment les caractères de contrôle : un autre joueur peut envoyer des séquences d'échappement ANSI (effacement d'écran, retitrage du terminal) via le chat.
3. **Serveur, port, nom : aucun paramètre** — `main()` ne lit pas `argv`. Ni `./clientCLI host port`, ni variable d'environnement. Bloque les tests multi-machines et inter-groupes (§II).
4. **« Disconnected from server. » affiché dans tous les cas, `return 0` aussi** — sur Ctrl+D (fin de `stdin`) comme sur perte de connexion — **vérifié en réel le 2026-10-05** : serveur tué par `SIGTERM` pendant que le client tourne → « Disconnecting from server… / Disconnected from server. », code `0`, strictement identique à une sortie volontaire. Impossible de distinguer une sortie voulue d'une coupure ; pas de message de départ distinct après `OK bye`.
5. **`sleep(5)` bloquant** pendant le retry — pas d'interruption propre, aucune possibilité de saisie, et le message « Disconnecting from server… » sort aussi après une connexion *échouée* (vu en réel), ce qui est trompeur.

---

## `clients/client.cpp` (+ `client.hpp`)

### ✅ Bon

1. **Framing TCP correct à la réception** — `readServer` accumule dans `_in`, extrait chaque ligne sur `\n`, retire le `\r` final, et traite plusieurs lignes d'un même `recv` ; une ligne coupée reste en tampon jusqu'au `\n` (RFC §9.2, côté client aussi).
2. **Écriture complète garantie** — `send()` boucle jusqu'à ce que tout soit parti, relance sur `EINTR`, utilise `MSG_NOSIGNAL` (pas de `SIGPIPE` si le serveur a fermé).
3. **Un seul `poll()` sur socket + `stdin`** — répond au critère « keeps receiving events while waiting for user input » (§V.3) et à la règle globale « both clients must remain responsive » (§IV) sans thread ni verrou. Vérifié en réel (2026-10-05) : greeting et réponses à `CONNECT`/`LOOK`/`MOVE` reçus pendant que `stdin` reste ouvert.
4. **API réutilisable par les deux clients** — `defineAction`, `send`, `update(timeout)`, `fd()` : le GUI Qt peut brancher `fd()` sur un `QSocketNotifier` ou appeler `update()` périodiquement, et `enableKeyboard()` est explicitement « CLI only ».
5. **Lignes vides de l'utilisateur ignorées** et **détection de coupure** — `recv == 0` → `disconnect()` + `update()` renvoie `false`, la boucle de `main` sort proprement. RAII : le destructeur appelle `disconnect()`, pas de fuite de fd.

### ❌ Mauvais

1. **🔴 Fin de `stdin` = déconnexion immédiate, avant d'avoir lu la réponse du serveur** — re-vérifié le 2026-10-05 contre le serveur fonctionnel : `printf 'CONNECT yan\nLOOK\n' | ./cli` envoie les commandes puis quitte sans jamais afficher `[OK] hello proto=1`, `[OK] connected` ni le JSON du `LOOK`, alors que le serveur a bien traité les commandes (le joueur apparaît dans son log). `readKeyboard` renvoie `false` sur EOF, et `update()` propage ce `false` à la boucle (`client.cpp:97-100`). Ça rend le client **impossible à piloter par un script/pipe** (cas typique d'un test d'évaluation), et aucun `QUIT` n'est envoyé au serveur avant la coupure.
2. **`inet_addr()` uniquement** — pas de résolution de nom (`getaddrinfo`), pas d'IPv6, et un host invalide donne `INADDR_NONE` (= `255.255.255.255`) sans erreur explicite. `connect()` est bloquant sans délai d'expiration.
3. **`_is_authenticated` / `isAuthenticated()` : état mort** — jamais modifié (`grep` : une seule déclaration, un seul accesseur). Le client ne sait pas s'il est « CONNECTED » ou « AUTHENTICATED » (RFC §2.2) et ne réagit à aucun `OK connected` / `ERR 201 NAME_IN_USE`.
4. **Tampons non bornés** *(lecture de code)* — `_in` (serveur) et `_kbd` (clavier) croissent sans limite si aucun `\n` n'arrive ; une ligne saisie > 1024 octets est envoyée telle quelle (`send(line)`), et le serveur répond `ERR 400` puis **coupe la connexion** (vérifié côté serveur) — le client ne prévient pas l'utilisateur avant.
5. **Sorties parasites dans la bibliothèque** — `disconnect()` écrit « Disconnecting from server… » sur `stdout` depuis du code partagé avec le GUI ; `handleLine` écrit « No action defined for server message » sur `stderr` pour tout préfixe inconnu au lieu de laisser l'appelant décider. Un client GUI hériterait de ce bruit.

> Point mineur : un seul `recv` par tour de `poll` (suffisant puisque `poll` repart immédiatement s'il reste des données, mais coûte un appel système de plus par rafale) ; `#include` en tête du header (`iostream`, `functional`…) alourdit chaque unité de compilation.

---

## `clients/defines.hpp` · `shared/defines.hpp`

### ✅ Bon

1. **Constantes partagées serveur/clients dans `shared/`** (`LINE_END`, `CARRIAGE_RETURN`, `MAX_LINE_LENGTH`, `SERVER_PORT`) — une seule définition du framing pour les deux côtés.
2. **`INVALID_FD` / `SYSCALL_ERROR`** nommés plutôt que `-1` répété — lisible en relecture/soutenance.
3. **`MAX_LINE_LENGTH 1024`** aligné sur la recommandation de la RFC §9.4.
4. **Séparation constantes réseau / constantes propres au client** (`BUFFER_SIZE`, `POLL_TIMEOUT_MS`).
5. **Gardes d'inclusion présentes** (`SHARED_DEFINES_HPP`).

### ❌ Mauvais

1. **🔴 `SERVER_IP "127.0.0.1"` et `SERVER_PORT 8080` en `#define`** — changer de serveur impose une recompilation (voir `main.cpp` ❌ 3).
2. **Garde `DEFINES_HPP` identique dans `clients/defines.hpp` et `server/defines.hpp`** — sans conséquence tant que les deux ne sont jamais inclus dans la même unité, mais une collision silencieuse dès qu'un fichier partagé inclut l'un puis l'autre (le deuxième est ignoré sans erreur).
3. **`POLL_TIMEOUT_MS` défini deux fois** (client et serveur) pour la même notion avec la même valeur — à faire remonter dans `shared/`.
4. **Pas de constante pour le nom de protocole / version** (`proto=1`) — le client n'a aucun moyen de vérifier la version annoncée au greeting.
5. **Macros plutôt que `constexpr`** — pas de typage (`-1` sans type pour un `int`/`ssize_t`), contraire à l'esprit « type annotations where supported » du §IV.

---

## `clients/cli/Makefile`

### ✅ Bon

1. **Flags stricts** `-Wall -Wextra -Werror -std=c++23` ; build propre.
2. **Dépendances `.hpp` déclarées** (`INCLUDES = ../client.hpp ../defines.hpp ../../shared/defines.hpp`) : modifier un en-tête recompile.
3. **Sources hors dossier gérées proprement** (`../client.cpp` compilé dans `Obj/`).
4. **`clean` / `fclean` / `re`** + `run` qui avertit si le binaire n'existe pas.
5. **Appelé depuis la racine** via `make client` (build + exécution dans le terminal courant) et `make run` (deux terminaux).

### ❌ Mauvais

1. **Pas de cible `lint`** (exigée §VII.1) et aucun outil d'analyse (`clang-tidy`, `cppcheck`) installé ici.
2. **Cible `run` sans passage d'arguments** — cohérent avec le client actuel, mais empêche de lancer `make run HOST=… PORT=…` une fois les paramètres ajoutés.
3. **Le binaire `clients/cli/clientCLI` n'est pas ignoré par git** (apparaît comme non suivi après build).
4. **Pas de variante ASAN/debug** (`make debug`) — utile pour la soutenance, où l'évaluateur peut demander une modification à chaud.
5. **`-pthread` inutile** — le client est mono-thread ; flag hérité du serveur.

---

## Mouvements côté serveur qui touchent le client

- `shared/defines.hpp` expose maintenant `CMD_CONNECT … CMD_QUIT`, `DIR_*` et `MAX_NAME_LENGTH` : le client peut les réutiliser (validation locale du nom, table de traduction pour l'approche (2)) au lieu de redéfinir les chaînes.
- Le serveur impose `CONNECT <nom>` sans espace, 1 à 32 octets, et répond `ERR 201` / `ERR 400` : le client n'a toujours aucune logique de nouvelle saisie du nom (`client.cpp` ❌ 3).
- Les réponses `LOOK` font ~700 octets de JSON sur une seule ligne (vérifié) : peu lisibles sans mise en forme côté client, que le client brut n'offre pas.

## Priorités (ordre suggéré)

1. **Documenter dans le README le choix d'interface** (RFC brute ici) — exigé §V.3 et §VI.
2. Paramètres `host`/`port` (et, idéalement, `getaddrinfo`) pour passer l'interchangeabilité §II.
3. EOF sur `stdin` : continuer à lire le serveur jusqu'à `OK bye`/coupure, envoyer `QUIT` avant de partir ; distinguer sortie volontaire / coupure par le code retour et le message.
4. Assainir les caractères de contrôle des événements avant affichage (RFC §9.2).
5. Interface plus agréable (approche (2) du sujet : `go north` → `MOVE north`, `say …` → `CHAT ROOM …`, `help`, prompt qui survit à l'arrivée d'un événement) — optionnel mais c'est ce qui fait la différence côté utilisateur.
6. Déplacer les `std::cout` de la bibliothèque `Client` vers des callbacks (`onDisconnect`, `onUnknown`) pour que le GUI ne les subisse pas.
7. Supprimer l'état mort `_is_authenticated` ou le relier aux réponses `OK connected` / `ERR 201`.
