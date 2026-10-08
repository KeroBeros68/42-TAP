# Architecture serveur / client — 42 TAP

**Public visé :** quelqu'un qui connaît les bases de la programmation mais débute en C++
(classes, pointeurs, sockets et `poll()` ne sont pas supposés acquis).

**Objet :** expliquer comment le serveur et les clients de 42 TAP sont organisés, comment ils
se parlent, et où se trouve chaque morceau de logique dans le code. À la fin de ce document,
tu dois pouvoir lire n'importe quel fichier de `server/`, `clients/` ou `shared/` sans être perdu,
et savoir où ajouter une nouvelle commande.

**Sources :** le code du dépôt, plus `docs/protocol-rfc.html` (le RFC du protocole TAP).

---

## Table des matières

1. [Vue d'ensemble](#1-vue-densemble)
2. [Le socle : TCP et le protocole TAP](#2-le-socle--tcp-et-le-protocole-tap)
3. [Côté serveur](#3-côté-serveur)
4. [Côté client](#4-côté-client)
5. [Le dossier `shared/`](#5-le-dossier-shared)
6. [Trajet complet d'un échange](#6-trajet-complet-dun-échange)
7. [Boîte à outils C++](#7-boîte-à-outils-c)
8. [Compiler et lancer](#8-compiler-et-lancer)
9. [Pièges connus et points de vigilance](#9-pièges-connus-et-points-de-vigilance)

---

## 1. Vue d'ensemble

### 1.1 Ce que fait le programme

42 TAP est un **MUD** (Multi-User Dungeon) : un petit monde textuel partagé. Plusieurs joueurs
s'y connectent en même temps, se déplacent de pièce en pièce, regardent autour d'eux et, à terme,
discutent, ramassent des objets et combattent des monstres.

Deux programmes distincts coopèrent :

| Programme | Rôle | Où |
|---|---|---|
| **Le serveur** | Détient *la vérité* : le monde, les pièces, les objets, les joueurs et leurs positions. Il écoute sur un port TCP et répond aux commandes. | `server/` |
| **Le client** | Envoie des commandes et affiche ce que le serveur répond. Il ne connaît **rien** du monde : il ne sait même pas où mène la sortie « north » avant de demander. | `clients/` |

C'est le principe fondamental du client-serveur : **le client est bête, le serveur est autoritaire.**
Si un client décidait seul qu'il peut traverser un mur, le serveur le lui refuserait quand même
(voir `Game::move` dans `server/game.cpp:79`). Le client ne fait qu'afficher.

> 💡 **Pourquoi cette séparation ?**
> Parce qu'un client peut être modifié par n'importe qui (il tourne sur la machine du joueur).
> Tout ce qui doit être fiable — l'état du monde, la validité d'un déplacement, l'unicité d'un
> pseudo — doit vivre côté serveur. C'est aussi ce qui permet d'avoir *plusieurs* clients
> différents (CLI, GUI, voire le client d'une autre équipe) branchés sur le même serveur.

### 1.2 Qui parle à qui

```
   ┌──────────────────────┐                        ┌──────────────────────┐
   │   client CLI         │                        │   client GUI (Qt)    │
   │   clients/cli/       │                        │   clients/gui/       │
   │                      │                        │   (WIP, pas encore   │
   │   + clients/client   │                        │    branché au réseau)│
   │       .cpp / .hpp    │                        │                      │
   └──────────┬───────────┘                        └──────────────────────┘
              │
              │   TCP, une commande texte par ligne
              │   ex. "LOOK\n"  -->  "OK {...}\n"
              │
   ┌──────────▼───────────────────────────────────────────────────────────┐
   │                          SERVEUR  (server/)                          │
   │                                                                      │
   │   Server ──── sockets, poll(), sessions   (server/server.cpp)        │
   │      │                                                               │
   │      │  appelle les handlers enregistrés                             │
   │      ▼                                                               │
   │   commands.cpp ── CONNECT / LOOK / MOVE / (stubs)                    │
   │      │                                                               │
   │      │  lit et modifie l'état du jeu                                 │
   │      ▼                                                               │
   │   Game ──── joueurs + monde              (server/game.cpp)           │
   │      │                                                               │
   │      ▼                                                               │
   │   World ──── pièces, items, NPC          (game/world.cpp)            │
   │      ▲                                                               │
   │      │  chargés au démarrage depuis le JSON                          │
   │   game/ressources/*.json                                             │
   └──────────────────────────────────────────────────────────────────────┘
```

Le client et le serveur partagent **un seul vocabulaire commun** : les constantes du protocole,
définies dans `shared/defines.hpp` (`CMD_LOOK`, `SERVER_PORT`, `LINE_END`…). C'est ce qui garantit
que les deux côtés écrivent exactement les mêmes chaînes de caractères.

### 1.3 Arborescence

```
42-TAP/
├── server/              # Le serveur (un seul exécutable : "server")
│   ├── main.cpp         #   point d'entrée : crée Server + Game, boucle
│   ├── server.hpp/.cpp  #   cœur réseau : sockets, poll(), sessions
│   ├── connection.hpp/.cpp  # les buffers d'E/S d'UN client
│   ├── user.hpp/.cpp    # l'identité d'UN client (pseudo, authentifié ?)
│   ├── commands.hpp/.cpp#   les handlers du protocole
│   ├── game.hpp/.cpp    #   l'état du jeu (qui est où)
│   ├── signalFd.hpp/.cpp#   arrêt propre sur Ctrl+C
│   └── defines.hpp      #   constantes internes au serveur
│
├── clients/
│   ├── client.hpp/.cpp  # Classe Client : le réseau, partagée CLI + GUI
│   ├── defines.hpp      #   constantes internes aux clients
│   ├── cli/             #   client texte  -> binaire "clientCLI"
│   └── gui/             #   client Qt     -> binaire "42TAP" (WIP)
│
├── game/                # Le monde du jeu — utilisé par le SERVEUR uniquement
│   ├── world.hpp/.cpp   #   le monde entier (toutes les pièces)
│   ├── room.hpp/.cpp    #   une pièce (nom, description, sorties, items)
│   ├── item.hpp/.cpp    #   un objet
│   ├── npc.hpp/.cpp     #   un personnage non-joueur
│   ├── parser.hpp/.cpp  #   lecture du JSON + écriture d'une réponse LOOK
│   ├── stats.hpp        #   points de vie / attaque / défense
│   └── ressources/*.json#   les données du monde (room, npc, items, quests)
│
└── shared/              # Code compilé à la fois par le serveur ET le client
    ├── defines.hpp      #   LE contrat du protocole (commandes, port, séparateurs)
    ├── errors.hpp       #   codes d'erreur et format d'une ligne "ERR ..."
    ├── success.hpp      #   formats de succès et ligne "OK ..."
    ├── response.hpp     #   objet retourné par un handler
    ├── logger.hpp/.cpp  #   logs structurés
    └── random.hpp       #   utilitaires
```

Note une chose importante : `game/` est compilé **dans le serveur** (voir le `vpath` du
`server/Makefile:45`), pas dans les clients. Le client n'a aucune idée de l'existence de `Room`
ou `World` : il ne reçoit que du texte.

---

## 2. Le socle : TCP et le protocole TAP

### 2.1 Une connexion TCP, c'est un tuyau d'octets

Avant de lire `server.cpp`, il faut comprendre une chose que tout le monde rate au début.

Quand un client fait `send("LOOK\n")`, le serveur ne reçoit **pas** forcément `"LOOK\n"`.
TCP est un **flux d'octets**, pas une file de messages. Trois choses peuvent arriver :

```
Ce que le client envoie :        "LOOK\n"

Ce que le serveur peut recevoir, dans le désordre :
  a)  "LOOK\n"            -> tout d'un coup (le cas facile)
  b)  "LO"   puis  "OK\n" -> fragmenté en deux paquets
  c)  "LOOK\nMOVE north\n"-> deux commandes collées dans un même recv()
```

Un serveur correct doit donc **bufferiser** ce qu'il reçoit et découper lui-même aux `\n`.
C'est exactement le rôle des `Connection::_in` / `Connection::_out` et de la boucle
`while ((pos = c.in().find(LINE_END)) != std::string::npos)` dans `server/server.cpp:233`.

Le même raisonnement vaut pour l'écriture : `send()` peut n'écrire **qu'une partie** du message.
D'où la boucle de `Server::flush` qui garde le reste dans un buffer pour le prochain tour
(`server/server.cpp:296`).

> 💡 **À retenir :** on ne raisonne jamais en « messages » reçus, on raisonne en
> « octets reçus → buffer → extraction des lignes complètes ».

### 2.2 Le format des messages

Le protocole TAP est **textuel** et **orienté ligne** : tout message est une ligne d'UTF-8
terminée par un `\n` (0x0A). Trois formes existent.

**Client → Serveur** (une commande, arguments séparés par des espaces) :

```
CONNECT alice
LOOK
MOVE north
```

**Serveur → Client, succès** :

```
OK hello proto=1          <- le message d'accueil, envoyé dès la connexion
OK connected              <- réponse à CONNECT
OK room=loc.square        <- réponse à MOVE
OK {"room":{...}}         <- réponse à LOOK (charge utile JSON)
OK                        <- succès sans données (CHAT, GROUP LEAVE…)
```

**Serveur → Client, erreur** :

```
ERR 400 BAD_REQUEST
ERR 201 NAME_IN_USE
ERR 301 NO_EXIT
```

**Serveur → Client, événement asynchrone** (pas encore implémenté côté serveur) :

```
EVT ROOM CHAT alice: salut !
EVT STATS players=12
```

Un événement, c'est un message que le serveur envoie **de sa propre initiative**, sans avoir
été interrogé — par exemple « un joueur vient d'entrer dans ta pièce ». Le client doit donc
être capable de traiter un message entrant à tout moment, pas seulement en réponse à ce qu'il
a demandé. C'est pour ça que les clients ont une boucle qui écoute en permanence (section 4.3).

Les constantes de tout ça sont dans `shared/defines.hpp` : `LINE_END` (`'\n'`),
`CMD_SEPARATOR` (`' '`), `MAX_LINE_LENGTH` (1024), `SERVER_PORT` (8080).

### 2.3 La machine à états d'une connexion

Une connexion TAP passe par quatre états (RFC § 2.2) :

```
   DISCONNECTED ──TCP connect──► CONNECTED ──CONNECT alice──► AUTHENTICATED
                                     │                              │
                                     │                              │
                                     └────────── QUIT / erreur ─────┴──► TERMINATED
```

Le serveur n'accepte **aucune** commande tant que l'état `AUTHENTICATED` n'est pas atteint,
sauf `CONNECT` bien sûr. C'est ce que vérifie cette ligne de `server.cpp:270` :

```cpp
if (!s.user.isAuthenticated() && cmd != CMD_CONNECT) {
    reply(s.conn, tapErrorLine(TapError::BAD_REQUEST));
    return;
}
```

---

## 3. Côté serveur

### 3.1 Le point d'entrée

`server/main.cpp` fait 25 lignes et contient toute la vie du programme :

```cpp
Server server;                        // 1. l'objet réseau
Game game;                            // 2. l'état du jeu (monde + joueurs)

registerCommands(server, game);       // 3. brancher les commandes sur le jeu

server.setOnDisconnect([&game](User& user) {
    game.removePlayer(user.id());     // 4. nettoyer le jeu quand quelqu'un part
});

server.start(SERVER_PORT);            // 5. ouvrir le port, préparer les sockets

while (server.getStatus()) {          // 6. LA boucle principale
    server.updatePoll();
}
```

> 💡 **C++ : deux fichiers par classe.**
> En C++, une classe s'écrit traditionnellement en deux morceaux : un **header** (`.hpp`) qui
> déclare *ce que la classe a* (ses attributs) et *ce qu'elle sait faire* (ses méthodes) ;
> un **source** (`.cpp`) qui contient le corps des méthodes. Le header est inclus par ceux qui
> utilisent la classe, le `.cpp` est compilé séparément. C'est pour ça que tu vois
> `Server` déclaré dans `server.hpp` et défini dans `server.cpp`.
> Les `#ifndef SERVER_HPP / #define / #endif` autour du header sont un **include guard** :
> ils empêchent le même fichier d'être inclus deux fois dans une même compilation.

> 💡 **C++ : la lambda.**
> `[&game](User& user) { game.removePlayer(user.id()); }` est une **lambda** : une fonction
> écrite à la volée, sans nom. Le `[&game]` est la **capture** : il dit « cette fonction a
> besoin de la variable `game` qui existe autour, et je la prends par référence » (le `&`).
> Le serveur stockera cette fonction et l'appellera plus tard, quand quelqu'un se déconnectera.
> C'est le mécanisme qui permet à la couche réseau de ne rien savoir du jeu : elle se contente
> d'appeler une fonction qu'on lui a donnée.

### 3.2 Ouvrir la porte : `Server::start()`

`Server::start()` (`server/server.cpp:27`) exécute la recette classique de tout serveur TCP.
Chaque étape échoue de manière récupérable, d'où les `throw std::runtime_error` :

```cpp
socket(AF_INET, SOCK_STREAM, 0);   // 1. créer un socket (une "prise" réseau)
setsockopt(..., SO_REUSEADDR, ...);// 2. pouvoir redémarrer sans attendre
bind(..., port);                   // 3. réserver le port 8080
listen(..., SOMAXCONN);            // 4. commencer à accepter des connexions
fcntl(..., O_NONBLOCK);            // 5. ne JAMAIS bloquer sur une lecture
```

L'étape 5 est la plus importante et la moins intuitive.

**Sans `O_NONBLOCK`** : `recv()` sur un socket sans données **endort le programme** jusqu'à
l'arrivée d'un octet. Un serveur qui fait ça ne peut servir qu'un client à la fois — le
deuxième devrait attendre que le premier parle. Inutilisable.

**Avec `O_NONBLOCK`** : `recv()` rend immédiatement la main, en signalant `EAGAIN`/`EWOULDBLOCK`
(« il n'y a rien pour l'instant ») s'il n'y a rien à lire. Le serveur peut alors passer au
client suivant. C'est la brique indispensable du modèle ci-dessous.

Les deux derniers gestes de `start()` installent l'arrêt propre :

```cpp
_sigs.add(SIGINT);       // Ctrl+C
_sigs.add(SIGTERM);      // kill
if (!_sigs.open())       // transformer les signaux en un fd lisible
    throw std::runtime_error("Failed to open signalfd");
```

Voir la section 3.9 pour le pourquoi.

### 3.3 La boucle d'événements : `Server::updatePoll()`

C'est **le** cœur du serveur. Une seule fonction, appelée en boucle, qui gère *tout le monde*
en même temps. La technique s'appelle **multiplexage d'E/S** et repose sur l'appel système
`poll()`.

L'idée : au lieu de demander « est-ce que le client 1 a parlé ? » puis « et le client 2 ? »
(ce qui gaspille du temps), on donne à `poll()` une **liste de descripteurs de fichiers** et on
dit « réveille-moi quand l'un d'eux a quelque chose à me dire ».

```cpp
std::vector<pollfd> fds;               // la liste
fds[POLL_LISTEN_IDX] = _listen_socket; // "quelqu'un veut se connecter ?"
fds[POLL_SIGNAL_IDX] = _sigs.fd();     // "quelqu'un a fait Ctrl+C ?"
pour chaque session :
    fds.push_back( { session.fd, POLLIN | (peut-on écrire ? POLLOUT : 0) } );

poll(fds.data(), fds.size(), POLL_TIMEOUT_MS);  // dort jusqu'à 1 s max
```

Le tableau `fds` est ensuite inspecté : pour chaque entrée, le champ `revents` indique ce qui
s'est réellement passé. Trois cas, dans l'ordre du code :

| Indice | Cas | Traitement |
|---|---|---|
| `POLL_SIGNAL_IDX` | Ctrl+C / SIGTERM | `_running = false` → la boucle de `main` s'arrête |
| `POLL_LISTEN_IDX` | Un nouveau client frappe à la porte | `accept()`, création d'une `Session`, envoi du message d'accueil |
| `POLL_CLIENT_START` et après | Un client a envoyé des données | `readFrom(session)` puis `flush(session)` |

Le `POLL_TIMEOUT_MS` (1000 ms) est le « timeout » : `poll()` se réveille au plus tard après
une seconde même s'il ne s'est rien passé. Ce n'est pas indispensable ici (le serveur ne fait
rien d'autre en attendant), mais c'est une sécurité qui garantit que la boucle ne reste jamais
bloquée indéfiniment.

Deux détails à ne pas rater dans cette fonction :

- **Avant de construire `fds`, on nettoie** (`server.cpp:121-132`) : toute session marquée
  `isClosing()` est fermée, son joueur retiré du jeu (`_onDisconnect`), son id remis dans
  `_available_id` pour être recyclé, et la session effacée de `_sessions`.
  Fermer une connexion est donc **toujours différé** : on ne détruit jamais une session
  au milieu d'une boucle qui la parcourt.
- **`POLLOUT` n'est demandé que si besoin** : `session.second.conn.out().empty() ? 0 : POLLOUT`.
  On ne demande au système de nous dire « tu peux écrire » que s'il reste effectivement quelque
  chose à écrire dans le buffer sortant. Sinon `poll()` se réveillerait en boucle pour rien.

> 💡 **C++ : `std::vector` et `std::unordered_map`.**
> `std::vector<T>` est un tableau dynamique (taille modifiable, accès par indice `fds[i]`).
> `std::unordered_map<K, V>` est un dictionnaire / table de hachage : `map[cle]` retrouve une
> valeur en temps ~constant. Le serveur s'en sert pour retrouver une session par id de client
> (`_sessions`) et une commande par son nom (`_actions`). Les deux vivent dans la bibliothèque
> standard (`<vector>`, `<unordered_map>`) et s'occupent eux-mêmes de leur mémoire : tu n'as
> jamais à appeler `new` ni `delete` avec eux.

### 3.4 Une session = une connexion + un utilisateur

```cpp
struct Session {          // server/server.hpp:29
    Connection conn;      // tout ce qui touche au socket
    User       user;      // tout ce qui touche à l'identité
};
```

Cette séparation est volontaire et très utile à comprendre :

| Classe | Fichier | Ce qu'elle sait | Ce qu'elle ignore |
|---|---|---|---|
| `Connection` | `server/connection.hpp` | le fd du socket, l'IP, les buffers entrant/sortant, le flag `_closing` | qui est le joueur, ce qu'est une pièce |
| `User` | `server/user.hpp` | l'id, le pseudo, `_authenticated` | les sockets, le réseau |

`Connection` est **privée au serveur** (son header le dit explicitement). `User`, lui, est
passé par référence aux handlers de commandes.

Les sessions sont stockées dans `std::unordered_map<long long, Session> _sessions`, indexées
par un **id de client** qui est un simple compteur (`_next_client_id`). Quand un client part,
son id est empilé dans `_available_id` et **réutilisé** pour le prochain arrivant — c'est
pourquoi l'id n'est pas un identifiant éternel, juste un numéro de place.

> 💡 **C++ : `const`, passage par référence.**
> `void reply(Connection& c, const std::string& line)` : le `&` évite de **copier** l'objet
> (copier une structure à chaque appel coûte cher) ; le `const` promet qu'on ne va pas le
> modifier. C'est le passage de paramètre par défaut en C++ moderne : `const T&` pour lire,
> `T&` pour modifier, `T` par valeur seulement pour les petits types (`int`, `bool`, `size_t`).
> Tu verras aussi `const` **après** une méthode (`int fd() const;`) : cela veut dire
> « cette méthode ne modifie pas l'objet », et c'est ce qui autorise à l'appeler sur un objet
> constant.

Le cycle de vie d'une session :

```
accept() ─► Session(fd, ip, id) ─► session vivante ─► setClosing(true) ─► nettoyage au tour suivant
             (envoi "OK hello proto=1")
```

Personne n'appelle jamais `new` ni `delete` : `_sessions.erase(it)` détruit la `Session`, et
avec elle ses `std::string` de buffer, qui libèrent leur mémoire toutes seules. C'est le
principe **RAII** (*Resource Acquisition Is Initialization*) : la durée de vie de la ressource
est liée à celle de l'objet qui la possède.

Le fd, lui, fait exception : `Connection` ne le possède pas au sens RAII, il est fermé
**explicitement** juste avant l'effacement (`close(session.second.conn.fd())`,
`server.cpp:125`). `SignalFd`, à l'inverse, est du RAII pur : son destructeur appelle `close()`
sur son fd (`server/signalFd.cpp:5`).

### 3.5 Lire : `Server::readFrom()`

Appelée quand `poll()` signale qu'un client a des données (`POLLIN`), ou qu'il a raccroché
(`POLLHUP` / `POLLERR`). Trois temps.

**Temps 1 — vider le socket** (`server.cpp:212-231`). Une boucle `while (true)` autour de
`recv()`, car une seule lecture peut ne pas suffire :

```cpp
if (n > 0)                 c.appendIn(buff, n);   // des octets -> dans le buffer
else if (n == 0)           c.setClosing(true);    // 0 = le pair a fermé proprement
else if (EAGAIN/EWOULDBLOCK) break;               // plus rien à lire, on sort
else if (EINTR)            continue;              // interrompu par un signal, on réessaie
else                       c.setClosing(true);    // vraie erreur
```

`n == 0` est le cas à connaître : sur un socket TCP, `recv()` renvoie 0 **uniquement** quand
l'autre côté a fermé la connexion. Ce n'est pas une erreur, c'est une information.

**Temps 2 — découper les lignes** (`server.cpp:233-239`). On cherche chaque `\n` dans le
buffer ; tout ce qui précède est **une** ligne complète, qu'on retire du buffer et qu'on
transmet :

```cpp
while ((pos = c.in().find(LINE_END)) != std::string::npos) {
    std::string line = c.in().substr(0, pos);
    c.eraseIn(pos + 1);
    if (!line.empty() && line.back() == CARRIAGE_RETURN)
        line.pop_back();          // tolère les fins de ligne Windows "\r\n"
    handeLine(s, line);
}
```

Ce qui reste dans le buffer après la boucle, c'est une ligne **incomplète** : on la garde pour
le prochain `recv()`. C'est ici que se règle le problème de fragmentation de la section 2.1.

**Temps 3 — la limite de taille** (`server.cpp:240-244`). Si ce qui reste dépasse
`MAX_LINE_LENGTH` (1024 octets) sans jamais contenir de `\n`, c'est un client cassé ou
malveillant : on répond `ERR 400 BAD_REQUEST` et on ferme. Sans ce garde-fou, un client
pourrait envoyer 10 Go sans retour à la ligne et faire enfler la mémoire du serveur jusqu'à
le tuer.

### 3.6 Interpréter : `Server::handeLine()`

Une ligne = un nom de commande, un espace, des arguments. On coupe au **premier** espace :

```cpp
size_t sp = line.find(CMD_SEPARATOR);
std::string cmd  = line.substr(0, sp);                              // "MOVE"
std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);  // "north"
```

> Le `(sp == std::string::npos)` traite le cas d'une commande sans argument : `find` renvoie
> `npos` quand il n'a rien trouvé, et sans ce test `substr(sp + 1)` partirait en vrille.

Ensuite, dans l'ordre :

1. **Authentification** : pas de commande avant `CONNECT` → `ERR 400`.
2. **Recherche** : `_actions.find(cmd)`.
3. **Trouvée** → on appelle le handler, on envoie sa réponse, et on ferme si elle le demande.
4. **Pas trouvée** → `ERR 400 BAD_REQUEST`.

```cpp
auto it = _actions.find(cmd);
if (it != _actions.end()) {
    Response r = it->second(s.user, args);   // on appelle la fonction enregistrée
    reply(s.conn, r.line());                 // on met la ligne dans le buffer sortant
    if (r.close)
        s.conn.setClosing(true);             // ex. QUIT
} else {
    reply(s.conn, tapErrorLine(TapError::BAD_REQUEST));
}
```

Note que `_actions` est **insensible à rien du tout** : `cmd` est comparé tel quel. Le RFC
dit que les noms de commandes sont insensibles à la casse ; ici `look` en minuscules
retomberait dans la branche « commande inconnue ». C'est un écart au RFC à connaître
(section 9).

### 3.7 Le registre de commandes : `_actions`

C'est le point d'architecture le plus élégant du serveur. Regarde le type :

```cpp
std::unordered_map<std::string, std::function<Response(User&, const std::string&)>> _actions;
```

Traduit en français : « un dictionnaire qui associe un nom de commande à **une fonction
prenant un `User&` et une `std::string`, et renvoyant un `Response`** ».

> 💡 **C++ : `std::function`.**
> `std::function<Signature>` est un type qui peut contenir **n'importe quoi** d'appelable
> ayant cette signature : une fonction libre, une méthode, ou une lambda. C'est ce qui permet
> de ranger des fonctions dans un dictionnaire et de les appeler plus tard sans savoir ce
> qu'elles sont. Le `it->second(s.user, args)` de `handeLine` est littéralement
> « appelle la fonction rangée ici ».

Le résultat : `Server` (le réseau) ne connaît **aucune** commande du jeu. Il sait seulement
« quand une ligne arrive, cherche-la dans le dictionnaire et appelle ». Toute la logique vit
dans `commands.cpp`, branchée au démarrage par `registerCommands` :

```cpp
void registerCommands(Server& server, Game& game) {
    server.defineAction(CMD_CONNECT, [&game](User& user, const std::string& name) {
        return cmdConnect(game, user, name);
    });
    server.defineAction(CMD_LOOK, [&game](User& user, const std::string&) {
        return cmdLook(game, user);
    });
    server.defineAction(CMD_MOVE, [&game](User& user, const std::string& dir) {
        return cmdMove(game, user, dir);
    });
    // ... les commandes non encore implémentées sont branchées sur un stub
}
```

C'est le patron de conception **Command** (ou *dispatch par table*) : ajouter une commande ne
demande aucune modification de `Server`.

> 💡 **C++ : la capture `[&game]` et la durée de vie.**
> Chaque lambda capture `game` **par référence**. Cela fonctionne parce que `game` vit dans
> `main()` aussi longtemps que le serveur. Si `Game` était détruit avant les lambdas, appeler
> un handler deviendrait un comportement indéfini (accès à un objet mort). C'est une
> fragilité à garder en tête — voir section 9.

### 3.8 Les handlers et l'objet `Response`

Chaque handler suit le même contrat. Exemple complet, `cmdConnect` :

```cpp
static Response cmdConnect(Game& game, User& user, const std::string& name) {
    if (!user.setName(name))                 // vide ? trop long ? caractères invalides ?
        return Response::failure(TapError::BAD_REQUEST);
    if (game.isNameTaken(name))              // pseudo déjà utilisé par un autre joueur ?
        return Response::failure(TapError::NAME_IN_USE);
    user.authenticate();                     // CONNECTED -> AUTHENTICATED
    game.addPlayer(user);                    // placer le joueur dans la pièce de départ
    return Response::success(TapOk::CONNECTED);
}
```

`Response` (`shared/response.hpp`) est une petite structure qui représente « ce que je vais
répondre » avant d'en faire une ligne de texte :

```cpp
struct Response {
    bool        isOk;        // succès ou échec
    TapOk       okType;      // quelle variante de "OK" (DATA, ROOM, CONNECTED…)
    TapError    error;       // quel code d'erreur
    std::string payload;     // charge utile éventuelle (le JSON de LOOK)
    bool        close;       // faut-il fermer la connexion après ?

    std::string line() const {
        return isOk ? tapOkLine(okType, payload) : tapErrorLine(error);
    }
};
```

Et les deux constructeurs d'un `Response` sont des méthodes **statiques**, ce qui permet
d'écrire `Response::success(...)` comme une évocation de fonction :

```cpp
Response::success(TapOk::DATA, game.look(user.id()))
// -> okType = DATA, payload = le JSON  -> "OK {…}"
```

> 💡 **C++ : `enum class`.**
> `TapOk` et `TapError` sont des `enum class` (`shared/success.hpp`, `shared/errors.hpp`) :
> une liste fermée de valeurs nommées, typées. `TapOk::DATA` ne peut pas être confondu avec
> `TapError::NO_EXIT` ni avec un `int`, contrairement aux `enum` anciens style. Les tables
> `tapOkPrefix()` / `tapErrorCode()` / `tapErrorName()` sont là pour convertir ces valeurs en
> texte destiné au réseau.

Le handler `cmdLook` mérite un coup d'œil, car il montre le passage du monde vers le JSON :

```cpp
static Response cmdLook(Game& game, User& user) {
    if (!game.hasPlayer(user.id()))
        return Response::failure(TapError::BAD_REQUEST);
    return Response::success(TapOk::DATA, game.look(user.id()));
}
```

`Game::look` (`server/game.cpp:67`) récupère la pièce du joueur, la liste des **noms** des
joueurs présents, puis délègue à `World::look` → `serializeLook` (`game/parser.cpp:118`), qui
construit un objet JSON avec `nlohmann::json` et le sérialise sur **une seule ligne**
(`dump(-1, ...)` : `-1` = pas d'indentation, justement parce qu'un message TAP ne peut pas
contenir de `\n`).

```json
{"room":{"id":"loc.square","name":"Square","description":"…","exits":{"north":"loc.village",…}},"players":["alice"],"items":["item.herbs","item.fountain"],"npcs":[]}
```

### 3.9 S'arrêter proprement : `SignalFd`

Un Ctrl+C envoie `SIGINT` au processus. Par défaut, le comportement est de **tuer le programme
immédiatement**, sans exécuter une seule ligne de nettoyage — pas de fermeture de sockets, pas
de destructeur. Pour un serveur, c'est inacceptable (et pour le GUI Qt, c'est la source des
« fuites » décrites dans le `README.md`).

La solution est le **`signalfd`** (spécifique à Linux), implémenté dans
`server/signalFd.cpp` :

```cpp
void SignalFd::add(int sig)  { sigaddset(&_mask, sig); }        // "je veux ce signal"

bool SignalFd::open() {
    sigprocmask(SIG_BLOCK, &_mask, NULL);                       // 1. bloquer les signaux
    _fd = signalfd(INVALID_FD, &_mask, SFD_NONBLOCK | SFD_CLOEXEC);  // 2. en faire un fd
}
```

Le truc : `SIGINT` est **bloqué** au niveau du processus (il ne tue donc plus rien), et il est
redirigé vers un **descripteur de fichiers** qui devient lisible dès qu'un signal arrive.
Comme ce fd est dans la liste de `poll()`, il s'intègre tout seul à la boucle d'événements :
plus besoin de gestionnaire de signal asynchrone, plus de fonction qui peut être appelée
n'importe où.

```
Ctrl+C ─► SIGINT bloqué ─► signalfd devient lisible ─► poll() se réveille
        ─► updatePoll lit le signal ─► _running = false ─► main sort de la boucle
        ─► ~Server() ferme tous les sockets
```

L'arrêt devient donc **un événement comme un autre**, traité au même endroit que le reste,
dans l'ordre, sans interruption sauvage.

### 3.10 Le monde du jeu : `Game` et `World`

Le réseau mis de côté, voici comment l'état du jeu est rangé. Deux niveaux.

**`Game`** (`server/game.hpp`) — l'état *vivant*, celui qui change à l'exécution :

```cpp
struct Player {
    std::string name;
    std::string current_map;      // l'id de la pièce où se trouve le joueur
    // plus tard : hp, inventaire, groupe, quêtes...
};
std::unordered_map<long long, Player> _players;   // id de client -> joueur
World _world;                                     // la carte (statique, chargée au boot)
```

**`World`** (`game/world.hpp`) — la carte, statique, chargée une fois pour toutes :

```cpp
RoomMap     rooms;            // id de pièce -> Room
std::string start_room;       // où apparaissent les nouveaux joueurs ("loc.square")
std::string respawn_room;
NpcDb       npc_database;
ItemDb      item_database;
```

`World::World()` appelle `loadWorld()`, qui lit `game/ressources/room.json` et `npc.json` via
`parser.cpp` et la bibliothèque `nlohmann/json`, puis **valide** le contenu
(`game/world.cpp:39`) :

- toute sortie doit pointer vers une pièce qui existe ;
- sinon on lève une exception → le serveur refuse de démarrer. Mieux vaut un crash net au
  démarrage qu'un monde silencieusement cassé en production ;
- une sortie sans retour (« one-way exit ») n'est qu'un avertissement (`LOG_WARN`).

Le chemin des fichiers JSON est injecté à la compilation par le Makefile
(`-DPROJECT_ROOT="$(abspath ..)"`, `server/Makefile:108`), ce qui explique la macro
`#ifndef PROJECT_ROOT` en haut de `world.cpp` : le code reste compilable sans elle.

Qui contient quoi, en résumé :

| Question | Réponse | Où |
|---|---|---|
| Quelles pièces existent ? Que contiennent-elles ? | `World` | `game/room.hpp`, `room.json` |
| Quel est le pseudo du client 3 ? | `User` | `server/user.hpp` |
| Dans quelle pièce est le client 3 ? | `Game::_players` | `server/game.hpp` |
| Quels joueurs sont **physiquement** dans la pièce X ? | `Room::_current_player` | `game/room.hpp` |

Cette dernière ligne surprend souvent : il y a **deux** façons de savoir où est un joueur.
`Player::current_map` dit « le joueur 3 est dans `loc.square` » ; et la pièce `loc.square`
maintient de son côté la liste des ids présents. Les deux sont mis à jour ensemble dans
`Game::addPlayer`, `Game::removePlayer` et `Game::move`. La liste dans la pièce sert à
répondre vite à « qui est là ? » sans parcourir tous les joueurs.

`Game::move` est le bon exemple à lire en entier (`server/game.cpp:79`) :

```cpp
auto player = _players.find(player_id);
if (player == _players.end())                    return false;   // joueur inconnu
auto origin = _world.rooms.find(player->second.current_map);
if (origin == _world.rooms.end())                return false;   // pièce inconnue
auto exit = origin->second.exits().find(direction);
if (exit == origin->second.exits().end())        return false;   // pas de sortie ici -> ERR 301
auto destination = _world.rooms.find(exit->second);
if (destination == _world.rooms.end())           return false;   // sortie cassée

origin->second.removePlayer(player_id);          // quitter l'ancienne pièce
destination->second.addPlayer(player_id);        // entrer dans la nouvelle
player->second.current_map = exit->second;       // mettre à jour le joueur
return true;
```

Toutes les vérifications passent **avant** la moindre modification : soit le déplacement
réussit entièrement, soit rien ne bouge. C'est important, car un état à moitié modifié
(le joueur retiré de l'ancienne pièce mais pas ajouté à la nouvelle) serait un bug très
difficile à retrouver.

### 3.11 Ajouter une nouvelle commande — tutoriel

Les commandes `CHAT`, `TAKE`, `DROP`, `INVENTORY`, `TALK`, `ATTACK`, `STATUS`, `QUEST`,
`QUESTS`, `WHO`, `GROUP` et `QUIT` existent dans le protocole mais sont aujourd'hui branchées
sur `cmdStub`, qui répond invariablement `OK Message received` (`server/commands.cpp:33`).
Prenons `WHO` pour voir comment en implémenter une pour de vrai.

**Étape 1 — la logique dans `Game`.** `Game` expose déjà `playerCount()` :

```cpp
size_t Game::playerCount() const { return _players.size(); }
```

**Étape 2 — le handler, dans `commands.cpp` :**

```cpp
static Response cmdWho(Game& game, User&) {
    return Response::success(TapOk::PLAYERS, std::to_string(game.playerCount()));
}
```

`TapOk::PLAYERS` a pour préfixe `"players="` (`shared/success.hpp:28`), donc `line()` produit
`OK players=3` — exactement ce que demande le RFC § 5.2.2.

**Étape 3 — le branchement.** Dans `registerCommands`, retirer `CMD_WHO` du tableau `stubs[]`
et ajouter :

```cpp
server.defineAction(CMD_WHO, [&game](User& user, const std::string&) {
    return cmdWho(game, user);
});
```

C'est tout. `Server` n'a pas été touché, `handeLine` non plus. Le client, lui, recevra
`OK players=3` dans son handler `"OK"` sans rien changer s'il se contente d'afficher la ligne.

**Le chemin à retenir :** `shared/defines.hpp` (le nom de la commande) → `commands.cpp`
(le handler + le branchement) → `game.cpp` (la logique d'état). Cette séparation en trois
couches est ce qui rend le serveur lisible.

---

## 4. Côté client

### 4.1 La classe `Client`

`clients/client.hpp` définit **une seule** classe, volontairement générique, utilisée aussi
bien par le CLI que (demain) par le GUI :

```cpp
class Client {
    int _fd;                    // le socket
    std::string _in;            // buffer des données reçues du serveur
    std::string _kbd;           // buffer du clavier (CLI uniquement)
    bool _keyboard = false;     // lit-on le clavier ?
    std::unordered_map<std::string, std::function<void(const std::string&)>> _actions;
    //                              ^ même patron "dispatch par table" que le serveur,
    //                                mais indexé par "OK" / "ERR" / "EVT"
};
```

Le miroir est frappant, et volontaire : là où le serveur dispatche sur les **commandes**
reçues, le client dispatche sur les **types de réponse** reçues.

| | Serveur | Client |
|---|---|---|
| Dictionnaire | `_actions` : `"LOOK"` → handler | `_actions` : `"OK"` → handler |
| Clé | nom de commande (`CMD_LOOK`) | type de message (`"OK"`, `"ERR"`, `"EVT"`) |
| Appelé par | `Server::handeLine` | `Client::handleLine` |
| Enregistré par | `registerCommands` | le `main` du client |
| Type stocké | `std::function<Response(User&, const std::string&)>` | `std::function<void(const std::string&)>` |

Seule différence : un handler serveur **renvoie** un `Response` (le serveur doit répondre),
un handler client **ne renvoie rien** (il se contente d'afficher ou de mettre à jour l'UI).

### 4.2 Se connecter et envoyer

`Client::connect()` (`clients/client.cpp:9`) est le symétrique exact de `Server::start()`,
en version client : `socket()` puis `connect()` vers `127.0.0.1:8080`.

```cpp
_fd = socket(AF_INET, SOCK_STREAM, 0);
server_addr.sin_port = htons(_port);
server_addr.sin_addr.s_addr = inet_addr(_servername.c_str());
::connect(_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

Remarque le `::` devant `connect` : c'est parce que `Client::connect` est elle-même une
méthode nommée `connect`. Sans le `::`, le compilateur croirait à un appel récursif de la
méthode sur elle-même. Le `::` force « la fonction `connect` globale, celle du système ».

Contrairement au serveur, le socket du client reste **bloquant** : c'est acceptable, car le
client n'a qu'un seul serveur à servir, et `poll()` (section suivante) empêche justement de
s'y endormir quand il n'y a rien à lire.

`Client::send()` (`clients/client.cpp:50`) écrit la ligne complète avec son `\n` :

```cpp
std::string data = message + LINE_END;
size_t sent = 0;
while (sent < data.size()) {
    ssize_t n = ::send(_fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
    if (n > 0)      { sent += n; continue; }     // écriture partielle : on continue
    if (n < 0 && errno == EINTR) continue;       // interrompu : on réessaie
    disconnect(); return false;                  // vraie erreur : on raccroche
}
```

C'est la même boucle que `Server::flush`, en version bloquante : `sent` avance du nombre
d'octets réellement écrits, et on recommence jusqu'au bout.

> 💡 `MSG_NOSIGNAL` évite que l'écriture sur un socket fermé déclenche `SIGPIPE`, un signal
> dont le comportement par défaut est de **tuer le programme**. Sans ce drapeau, un serveur
> qui plante ferait mourir le client brutalement au lieu de lui rendre `false`.

### 4.3 La boucle du client : `update()`

Même idée que `Server::updatePoll()`, mais au lieu de N clients, elle surveille **deux**
sources :

```cpp
pollfd fds[2] = {};
fds[0].fd = _fd;              fds[0].events = POLLIN;   // le serveur
nfds_t count = 1;
if (_keyboard) {
    fds[1].fd = STDIN_FILENO; fds[1].events = POLLIN;   // le clavier
    count = 2;
}
poll(fds, count, timeout_ms);
```

- Si `fds[0]` est lisible → `readServer()`.
- Si `fds[1]` est lisible → `readKeyboard()` (le CLI uniquement).

`readServer()` (ligne 104) fait exactement ce que fait le serveur côté réception : `recv()`,
ajout au buffer, puis découpage sur `\n`, avec le même traitement du `\r` final. La symétrie
est voulue : les deux côtés parlent le même protocole, ils doivent donc le découper pareil.

`readKeyboard()` (ligne 126) lit `stdin`, découpe les lignes tapées, et **envoie chacune telle
quelle au serveur** :

```cpp
if (line.empty()) continue;
if (!send(line)) return false;
```

C'est ce qui rend le client CLI si simple : taper `LOOK` envoie littéralement `LOOK\n`. Le
client n'interprète rien du tout — un vrai client bête, comme prévu en section 1.1.

> 💡 `STDIN_FILENO` vaut 0 : c'est le descripteur de l'entrée standard. Sous Unix, **tout** est
> un descripteur de fichiers — un fichier, un socket, le clavier, un pipe. C'est pour ça que
> `poll()` peut surveiller les trois avec le même mécanisme.

### 4.4 Recevoir : `handleLine()` et les handlers

```cpp
void Client::handleLine(const std::string& line) {
    size_t sp = line.find(CMD_SEPARATOR);
    std::string type = line.substr(0, sp);                    // "OK" / "ERR" / "EVT"
    std::string args = (sp == std::string::npos) ? "" : line.substr(sp + 1);

    auto it = _actions.find(type);
    if (it != _actions.end())
        it->second(args);                                     // appeler le handler
    else
        std::cerr << "No action defined for server message: " << line << std::endl;
}
```

Le client CLI enregistre trois handlers dans son `main` (`clients/cli/main.cpp:25`) :

```cpp
client.defineAction("OK",  [](const std::string& args) { std::cout << "[OK] "  << args << std::endl; });
client.defineAction("ERR", [](const std::string& args) { std::cerr << "[ERR] " << args << std::endl; });
client.defineAction("EVT", [](const std::string& args) { std::cout << "[EVT] " << args << std::endl; });
```

Et la boucle principale tient en une ligne :

```cpp
client.enableKeyboard();
while (client.update()) {}          // tourne jusqu'à déconnexion ou Ctrl+D
```

`update()` renvoie `false` dans deux cas : la connexion est perdue, ou le clavier a renvoyé
`0` — c'est-à-dire un Ctrl+D (`clients/client.cpp:132`).

### 4.5 Le client GUI (Qt) — état actuel

⚠️ **Le GUI n'est pas encore branché sur le réseau.** C'est un chantier en cours, et il faut
le savoir avant de lire `clients/gui/`, sinon on cherche partout une connexion qui n'existe pas.

Ce qui existe : les fenêtres Qt (`TAPConnectionWindow`, `TAPErrorWindow`, `TAPGameWindow`) et
leurs widgets, avec des méthodes de mise à jour propres (`update_room_name`,
`update_room_description`, `update_available_exits`, `update_total_players_label`…).

Ce qui n'existe pas encore :

- `clients/gui/CMakeLists.txt` ne compile **pas** `clients/client.cpp` : le binaire `42TAP`
  ne contient aucune classe `Client`.
- Le `main` du GUI (`clients/gui/main.cpp`) affiche la fenêtre de jeu avec des **données
  codées en dur** (le nom `"42 datacenter"`, une description d'ambiance, quatre sorties) et la
  fenêtre de connexion est en commentaire.
- `app.exec;` en ligne 51 : il manque les parenthèses — c'est `app.exec();` qu'il faut écrire
  pour entrer dans la boucle d'événements Qt. En l'état, l'instruction ne fait rien.
- Le port par défaut du formulaire de connexion est `"4224"` (`tap_connection_window.hpp:13`)
  alors que le serveur écoute sur `8080` (`shared/defines.hpp:6`).

**Le problème d'architecture à résoudre** pour brancher le GUI : Qt a sa propre boucle
d'événements (`app.exec()`), et `Client::update()` a la sienne (`poll`). Les deux ne peuvent
pas bloquer le même thread. Les solutions habituelles :

1. **Un `QSocketNotifier` sur le fd du client** — Qt surveille le socket et appelle un slot
   quand des données arrivent. C'est la solution la plus « Qt », mais elle demande d'exposer
   le `fd` et de découper les lignes côté GUI, ou d'ajouter un mode non bloquant à `Client`.
2. **Un `QTimer`** qui appelle `client.update(0)` toutes les X ms — `update(0)` étant un
   `poll` à timeout nul, il ne bloque jamais. Simple, mais introduit une latence et du
   polling inutile.
3. **Un thread dédié** au réseau, qui communique avec le thread GUI par signaux/slots Qt
   (connexion `Qt::QueuedConnection`). Plus lourd, mais c'est le plus propre si le réseau
   devient complexe.

La contrainte du sujet est claire : le client doit rester réactif **en permanence** pendant la
réception d'événements asynchrones (voir `docs/GUI/cahier-des-charges.md` § 1). Une boucle
bloquante dans le thread graphique est donc interdite — ce qui élimine d'office un
`while (client.update()) {}` dans le `main` du GUI.

---

## 5. Le dossier `shared/`

L'intention de `shared/` est d'être **le contrat commun** : ce que le serveur et le client
doivent impérativement écrire de la même façon. En pratique aujourd'hui, un seul de ces
fichiers est réellement inclus par les deux côtés (`defines.hpp`) ; les autres sont la
description du protocole côté serveur, qu'un client pourra réutiliser quand il voudra
interpréter `"OK"` / `"ERR"` autrement qu'en affichant la ligne brute.

| Fichier | Contenu | Inclus par |
|---|---|---|
| `defines.hpp` | `SERVER_PORT`, `LINE_END`, `CMD_SEPARATOR`, `MAX_LINE_LENGTH`, `CMD_*`, `DIR_*` | **serveur + client** |
| `errors.hpp` | `enum class TapError`, `tapErrorCode()`, `tapErrorName()`, `tapErrorLine()` | serveur (via `response.hpp`) |
| `success.hpp` | `enum class TapOk`, `tapOkPrefix()`, `tapOkLine()` | serveur (via `response.hpp`) |
| `response.hpp` | `struct Response` (+ `success()` / `failure()`) | serveur |
| `logger.hpp/.cpp` | logs structurés en JSON, avec niveaux et couleurs | serveur |
| `random.hpp` | utilitaires aléatoires | serveur (`game/npc.cpp`) |

À noter : `clients/cli/Makefile` ne compile **pas** `shared/logger.cpp` — le client CLI
n'utilise pas le logger du tout et écrit directement sur `std::cout` / `std::cerr`. C'est
cohérent : les logs structurés sont un besoin de serveur (exploiter des traces), pas de client.

`defines.hpp` est le plus important : c'est **le** fichier où l'on regarde quand on se demande
« comment s'écrit déjà la commande pour bouger ? ». Grâce à lui, le serveur et le client ne
peuvent pas diverger sur l'orthographe d'une commande : `CMD_MOVE` vaut `"MOVE"` des deux
côtés, par construction.

Le `logger` mérite un mot, car il est utilisé à peu près partout côté serveur. Il s'emploie via
des macros :

```cpp
LOG_INFO("player moved", {"client", player_id}, {"player", player->second.name},
                        {"from", origin->first}, {"to", exit->second});
```

Chaque paire `{"clé", valeur}` devient un champ du log. Le résultat est du JSON, ce qui le rend
lisible par un humain **et** exploitable par un outil :

```json
{"level":"info","msg":"player moved","client":3,"player":"alice","from":"loc.square","to":"loc.village"}
```

> 💡 **C++ : les macros et `__VA_OPT__`.**
> `LOG_INFO(msg, ...)` est une **macro** : une substitution de texte faite par le
> préprocesseur avant la compilation. Les `...` sont des arguments variables (variadic), et
> `__VA_OPT__` gère le cas où il n'y en a aucun, pour ne pas laisser une virgule orpheline.
> La macro vérifie d'abord `enabled(level)` : si le niveau n'est pas activé, **rien** n'est
> évalué — c'est ce qui rend les `LOG_DEBUG` gratuits en production.
> La classe `Logger` est un **singleton** (`Logger::instance()` renvoie toujours le même
> objet) protégé par un `std::mutex`, car un serveur peut vouloir logger depuis plusieurs
> threads.

---

## 6. Trajet complet d'un échange

Rassemblons tout. Voici ce qui se passe réellement, fonction par fonction.

### 6.1 Le client arrive

```
CLIENT                                    SERVEUR
  │                                          │
  │  socket() + connect()  ────────────────► │  poll() voit POLLIN sur le socket d'écoute
  │                                          │  accept() -> fd du nouveau client
  │                                          │  fcntl(O_NONBLOCK)
  │                                          │  id = _next_client_id++
  │                                          │  _sessions[id] = Session(fd, ip, id)
  │  ◄──────── "OK hello proto=1\n" ─────────│  reply(HELLO) + flush()
  │                                          │
  │  _actions["OK"] affiche "[OK] hello proto=1"
```

Le message d'accueil part **avant** que le client ait dit quoi que ce soit : c'est le serveur
qui parle en premier (RFC § 3.2). Il permet au client de vérifier qu'il parle bien à un
serveur TAP, et quelle version de protocole.

### 6.2 Le client s'authentifie

```
CLIENT                                    SERVEUR
  │  send("CONNECT alice\n")  ────────────► │  Recv: _in = "CONNECT alice\n"
  │                                          │  Extraction de la ligne "CONNECT alice"
  │                                          │  handeLine: cmd="CONNECT", args="alice"
  │                                          │  pas encore authentifié, mais cmd == CONNECT -> OK
  │                                          │  _actions["CONNECT"] -> cmdConnect()
  │                                          │    user.setName("alice")      (validations)
  │                                          │    game.isNameTaken("alice")  (201 si pris)
  │                                          │    user.authenticate()        -> AUTHENTICATED
  │                                          │    game.addPlayer(user)       -> dans loc.square
  │  ◄──────────── "OK connected\n" ────────│  Response::success(TapOk::CONNECTED)
```

Le joueur est maintenant dans `Game::_players` **et** dans la liste `_current_player` de la
pièce `loc.square`. Son id de client est la clé partout.

### 6.3 Le client regarde autour de lui (`LOOK`)

```
CLIENT                                    SERVEUR
  │  send("LOOK\n")  ─────────────────────► │  handeLine -> cmdLook
  │                                          │    game.hasPlayer(id) ? oui
  │                                          │    game.look(id)
  │                                          │      map = "loc.square"
  │                                          │      pour chaque id dans rooms["loc.square"].currentPlayer()
  │                                          │          -> chercher son nom dans _players
  │                                          │      world.look(map, noms)
  │                                          │        serializeLook(room, players, {})
  │                                          │          -> JSON sur UNE ligne
  │  ◄── "OK {"room":{"id":"loc.square",…}}\n"
  │  _actions["OK"] affiche la ligne
```

À noter : `serializeLook` reçoit `{}` comme liste de NPC (`game/world.cpp:49`). La clé `"npcs"`
du JSON est donc **toujours vide** aujourd'hui, alors que `room.json` déclare bien des
`"spawns"`. C'est une partie du travail non encore faite, pas un bug.

Chaque `LOOK` recalcule tout depuis zéro et renvoie l'état complet. Le client ne stocke rien
d'autoritaire : il redemande.

### 6.4 Le client se déplace (`MOVE`)

```
CLIENT                                    SERVEUR
  │  send("MOVE north\n")  ───────────────► │  cmdMove(game, user, "north")
  │                                          │    game.move(id, "north")
  │                                          │      origin = rooms["loc.square"]
  │                                          │      exit = origin.exits()["north"] -> "loc.village"
  │                                          │      destination = rooms["loc.village"]
  │                                          │      origin.removePlayer(id)
  │                                          │      destination.addPlayer(id)
  │                                          │      player.current_map = "loc.village"
  │  ◄────────── "OK room=loc.village\n" ────│
  │                                          │
  │  (le client redemande souvent LOOK ensuite pour afficher la nouvelle pièce)
```

Si la direction n'existe pas, `game.move` renvoie `false` dès la première vérification et le
handler répond `ERR 301 NO_EXIT`. L'état du monde n'a pas été touché.

### 6.5 Le client s'en va

Deux scénarios, qui convergent vers le même nettoyage.

**Fermeture brutale** (le client tue son processus, coupe le réseau) : `recv()` renvoie `0`,
`setClosing(true)`. Au tour de boucle suivant, `updatePoll` ferme la session et appelle
`_onDisconnect`, donc `game.removePlayer(id)`.

**Fermeture propre** (Ctrl+D sur le CLI) : `readKeyboard` renvoie `false`, `update()` renvoie
`false`, la boucle du `main` s'arrête, `Client::~Client` appelle `disconnect()` qui fait
`close(_fd)`. Le serveur voit alors le même `recv() == 0`.

**Arrêt du serveur** (Ctrl+C) : le `signalfd` se réveille, `_running = false`, la boucle de
`main` s'arrête, `~Server()` ferme le socket d'écoute **et** tous les sockets clients
(`server.cpp:18-24`) — les clients voient leur connexion se fermer.

---

## 7. Boîte à outils C++

Les notions croisées dans ce document, dans l'ordre où elles apparaissent.

| Notion | À quoi ça sert | Où le voir |
|---|---|---|
| **Header / source** (`.hpp` / `.cpp`) | Déclarer dans le header, définir dans le source | `server.hpp` vs `server.cpp` |
| **Include guard** (`#ifndef`) | Empêcher une double inclusion du même header | en haut de chaque `.hpp` |
| **Classe / `struct`** | Regrouper des données et les fonctions qui les manipulent | `Connection`, `User`, `Game` |
| **`private` / `public`** | Cacher l'intérieur, n'exposer que l'interface | `Server::_sessions` est privé |
| **`const` + `&`** | Lire sans copier | `const std::string& line` partout |
| **`const` après la méthode** | « ne modifie pas l'objet » | `int fd() const;` |
| **`std::vector<T>`** | Tableau dynamique | `fds`, `_available_id`, `_current_player` |
| **`std::unordered_map<K,V>`** | Dictionnaire | `_sessions`, `_actions`, `_players` |
| **`std::string`** | Chaîne d'octets, avec `find`/`substr`/`erase` | partout |
| **`std::function`** | Contenir n'importe quelle fonction appelable | `_actions` |
| **Lambda** `[capture](args){...}` | Fonction anonyme écrite sur place | `registerCommands`, `setOnDisconnect` |
| **`enum class`** | Liste fermée de valeurs typées | `TapOk`, `TapError` |
| **Méthode statique** | Fonction rangée dans une classe, sans objet | `Response::success()` |
| **RAII** | La ressource meurt avec l'objet qui la possède | `SignalFd`, `std::string`, `std::vector` |
| **Exception** (`try` / `throw` / `catch`) | Interrompre une opération impossible | `Server::start`, `World::loadWorld` |
| **Singleton** | Un seul objet global accessible partout | `Logger::instance()` |
| **Macro préprocesseur** | Substitution de texte avant compilation | `LOG_INFO`, `LINE_END` |
| **Erreur système** : `errno` | Code de la dernière erreur système | `strerror(errno)` dans les logs |

### Les appels système utilisés

Ce sont les fonctions du système d'exploitation (POSIX/Linux), pas du C++. Elles sont
documentées dans les *man pages* (`man 2 socket`, `man 2 poll`, `man 2 recv`…).

| Appel | Rôle |
|---|---|
| `socket()` | Créer un point de communication |
| `setsockopt(SO_REUSEADDR)` | Réutiliser le port tout de suite après un arrêt |
| `bind()` | Réserver une adresse et un port |
| `listen()` | Passer en écoute |
| `accept()` | Accepter une connexion entrante → nouveau fd |
| `connect()` | Se connecter à un serveur (côté client) |
| `fcntl(O_NONBLOCK)` | Rendre un fd non bloquant |
| `recv()` / `send()` | Lire / écrire sur un socket |
| `poll()` | Attendre un événement sur plusieurs fds à la fois |
| `close()` | Libérer un fd |
| `sigprocmask()` / `signalfd()` | Transformer un signal en fd lisible |
| `inet_ntop()` | Convertir une IP binaire en texte (`"127.0.0.1"`) |
| `htons()` | Convertir un port en ordre d'octets réseau (*big endian*) |

### Comment lire une boucle de serveur sans se perdre

Quand tu ouvres `server.cpp`, garde ce squelette en tête :

```
updatePoll()
├── 1. nettoyer les sessions fermées            (lignes 121-132)
├── 2. construire la liste des fds à surveiller (lignes 134-147)
├── 3. poll()                                   (ligne 149)
├── 4. un signal ?        -> _running = false   (lignes 160-166)
├── 5. un nouveau client ? -> accept()          (lignes 168-195)
└── 6. des données clients ? -> readFrom + flush (lignes 197-205)
```

Tout le reste du fichier n'est que l'implémentation de ces six étapes.

---

## 8. Compiler et lancer

Depuis la racine du dépôt :

```bash
make            # installe les dépendances, compile, puis lance serveur + client
make build      # compile seulement (server + client CLI)
make client     # compile et lance le client CLI seul, dans le terminal courant
make gui        # compile et lance le client Qt
make clean      # supprime les .o
make fclean     # supprime aussi les binaires
make re         # fclean + all
```

`make` délègue à trois sous-Makefiles, et c'est **le même code compilé deux fois** :

| Sous-make | Binaire | Compile |
|---|---|---|
| `server/Makefile` | `server/server` | `server/*.cpp` + `game/*.cpp` + `shared/*.cpp` |
| `clients/cli/Makefile` | `clients/cli/clientCLI` | `clients/cli/main.cpp` + `clients/client.cpp` |
| `clients/gui/CMakeLists.txt` | `clients/gui/build/42TAP` | les fichiers Qt + `main.cpp` (**pas** `client.cpp`) |

Le `vpath %.cpp ../game:../shared` du `server/Makefile:45` est ce qui permet de compiler des
fichiers situés hors du dossier `server/` ; comme tous les objets atterrissent à plat dans
`Obj/` via `notdir`, **deux fichiers de même nom dans des dossiers différents
s'écraseraient**. C'est une contrainte à connaître avant d'ajouter un fichier.

La dépendance externe est `nlohmann/json` (un header unique), téléchargé par
`make install-deps` et vérifié par une somme SHA-256 avant d'être utilisé — il n'est pas
versionné dans git.

Compilation en `-Wall -Werror -Wextra -std=c++17` : **aucun avertissement n'est toléré**.

---

## 9. Pièges connus et points de vigilance

Cette section liste ce qui surprend ou ce qui mériterait un passage. Rien n'y est bloquant
pour la compréhension, mais tout est bon à savoir.

### Comportements à ne pas prendre pour des bugs

- **`_actions` est sensible à la casse.** Le RFC § 4.2 annonce des commandes insensibles à la
  casse ; `handeLine` compare la chaîne telle quelle. `look` en minuscules → `ERR 400`.
- **`LOOK` renvoie toujours `"npcs":[]`.** `Game::look` passe une liste vide à
  `serializeLook` (`game/world.cpp:49`), alors que `room.json` déclare des `"spawns"`.
  Les NPC sont chargés dans `World::npc_database` mais jamais peuplés dans les pièces.
- **`item_database` n'est pas chargé.** `World::loadWorld` lit `npc.json` et `room.json` mais
  pas `items.json` : les objets d'une pièce sont de simples identifiants (`"item.herbs"`),
  sans nom ni description.
- **Les commandes non implémentées répondent `OK Message received`.** Un client qui teste
  `TAKE` croira que ça a marché. C'est un stub assumé (`server/commands.cpp:33`).
- **Aucun événement `EVT` n'est jamais envoyé.** Le handler `"EVT"` du client CLI existe mais
  ne se déclenchera pas tant que le serveur n'émet rien. Or les joueurs ne sont pas notifiés
  quand quelqu'un entre ou sort d'une pièce.
- **`MOVE` construit `"room="` à la main** avec `TapOk::DATA` (`server/commands.cpp:27`) alors
  que `TapOk::ROOM` existe et produit exactement le même préfixe. Ça marche, mais la constante
  dédiée serait plus lisible.
- **Le port du GUI (`4224`) ne correspond pas à celui du serveur (`8080`).**
  `tap_connection_window.hpp:13` vs `shared/defines.hpp:6`.
- **`app.exec;` sans parenthèses** dans `clients/gui/main.cpp:51` : l'instruction ne fait rien.

### Fragilités de conception

- **Ordre de destruction dans `main`.** `Server server;` est déclaré **avant** `Game game;`,
  donc `game` est détruit **en premier**. Or les lambdas de `_actions` capturent `game` par
  référence. Aujourd'hui c'est sans conséquence (`~Server` ne fait que fermer des sockets et
  n'appelle aucun handler), mais c'est un piège latent : si un jour `~Server` devait exécuter
  une action, elle toucherait un `Game` déjà détruit. Déclarer `Game game;` **avant**
  `Server server;` supprimerait le problème.
- **`SignalFd::open()` renvoie un `int` converti en `bool`** (`server/signalFd.cpp:16`) :
  `if (_fd != INVALID_FD) return _fd;`. Un descripteur valant `0` serait donc interprété comme
  un échec. Dans la pratique `_fd` ne vaut jamais 0 ici, mais un `return _fd != INVALID_FD;`
  serait correct.
- **`Server::handeLine` est appelée sur une session qui peut avoir été marquée fermée**
  pendant `readFrom`. Une ligne reçue juste avant un `recv() == 0` est tout de même traitée.
  Ce n'est pas faux (les données reçues avant la fermeture sont valides) mais c'est un
  enchaînement à connaître.
- **`Game::playerMap` et `Game::look` utilisent `.at()`**, qui lève `std::out_of_range` si la
  clé n'existe pas. Les handlers vérifient `hasPlayer` avant, donc c'est couvert — sauf que
  `handeLine` n'attrape aucune exception : une exception qui remonterait ferait sortir par le
  `catch` de `main` et tuerait le serveur entier. Un `try/catch` autour de l'appel au handler
  limiterait les dégâts à une connexion.
- **Pas de verrou sur l'état du jeu.** Tout est mono-thread aujourd'hui, ce qui rend le
  problème de concurrence inexistant — mais cela signifie aussi que `poll()` doit rester la
  seule boucle. Introduire un thread (par exemple pour les combats en temps réel) demanderait
  de protéger `_sessions`, `_players` et les `Room`.
- **Rien n'empêche un client d'inonder le serveur de commandes.** Le garde-fou de
  `MAX_LINE_LENGTH` limite la taille d'**une** ligne, pas le débit. Le RFC § 9.2 évoque ces
  questions sans les imposer.

### Côté GUI

- Le `main` du GUI utilise des données de test en dur : rien n'est reçu du serveur.
- La fenêtre de connexion n'est pas reliée à `TAPConnectionWindow::connectToServer`, qui se
  contente d'un `std::cout` (`tap_connection_window.cpp:53`).
- Le `main` ne vérifie pas le retour de `show()` ni ne gère la fermeture de fenêtre.
  Voir `docs/GUI/cahier-des-charges.md` § 4 et § 5 pour l'état détaillé et le chemin critique.
- Rappel du `README.md` : ne **jamais** arrêter le GUI avec Ctrl+C si l'on veut un rapport
  valgrind propre — le signal saute les destructeurs Qt et fait apparaître des « fuites »
  qui n'en sont pas.

---

## 10. En résumé

Si tu ne devais retenir que cinq choses de ce document :

1. **Le serveur détient la vérité, le client ne fait qu'afficher.** Toute validation vit dans
   `server/` et `game/`.
2. **TCP est un flux d'octets, pas une file de messages.** D'où les buffers `_in`/`_out` et le
   découpage sur `\n` des deux côtés.
3. **Une seule boucle `poll()` sert tout le monde**, sans thread : c'est pourquoi les sockets
   sont non bloquants.
4. **Le dispatch par table** (`_actions` + `std::function`) est ce qui découple le réseau de la
   logique : ajouter une commande ne touche jamais `Server`.
5. **`shared/` est le contrat commun.** C'est là qu'on vérifie l'orthographe d'une commande,
   d'un code d'erreur ou d'un port.

### Où lire quoi, dans l'ordre conseillé

| Ordre | Fichier | Pourquoi |
|---|---|---|
| 1 | `shared/defines.hpp` | Le vocabulaire commun, 38 lignes |
| 2 | `server/main.cpp` | La structure générale en 25 lignes |
| 3 | `server/server.cpp` → `updatePoll` + `readFrom` | Le cœur réseau |
| 4 | `server/commands.cpp` | Les handlers, très lisibles |
| 5 | `server/game.cpp` | La logique de jeu, séparée du réseau |
| 6 | `game/world.cpp` + `game/parser.cpp` | Le chargement du monde |
| 7 | `clients/client.cpp` | Le miroir côté client |
| 8 | `clients/cli/main.cpp` | Un client complet en 39 lignes |
