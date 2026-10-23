#include "tap_log_view.hpp"
#include <iostream>

// ============================================================
// TAPLogStreamBuf — l'évier custom branché dans std::cout
// ============================================================

// Constructeur : on garde juste un pointeur vers le widget à alimenter.
// (Le "&" n'est pas nécessaire ici ; "class TAPLogView *" déclare un
//  pointeur vers une classe définie plus bas dans le .hpp.)
TAPLogStreamBuf::TAPLogStreamBuf(class TAPLogView *view)
    : _view(view)
{
};

// Appelé par std::cout pour UN SEUL caractère.
// On accumule dans _pending ; quand le caractère est un '\n',
// la ligne est complète : on l'envoie au widget et on vide le tampon.
// Renvoyer ch = "tout est allé bien" (EOF signifierait une erreur).
int TAPLogStreamBuf::overflow(int ch)
{
    if (ch != EOF)
    {
        if (ch == '\n')
        {
            this->_view->append_log(this->_pending); // Ligne complète -> widget
            this->_pending.clear();                  // On repart à zéro
        }
        else
            this->_pending.push_back(static_cast<char>(ch)); // Ligne pas finie : on empile
    }
    return ch;
};

// Appelé par std::cout pour un LOT de n caractères
// (ex : std::cout << "[WIP] LOOK" envoie tout d'un coup).
// Le plus simple : boucler sur overflow(), la logique d'empilage
// et de découpage par ligne est déjà écrite là-bas.
// Renvoyer n = "les n caractères ont été acceptés".
std::streamsize TAPLogStreamBuf::xsputn(const char *text, std::streamsize n)
{
    for (std::streamsize i = 0; i < n; i++)
        this->overflow(text[i]);
    return n;
};

// Inutile pour le moment, mais sert à clear le buffer en loggant
// ce qui est déjà dedans, malgré le manque de \n
// qui définit normalement la fin d'une log.
void TAPLogStreamBuf::flush_pending(void)
{
    if (!this->_pending.empty())
    {
        this->_view->append_log(this->_pending);
        this->_pending.clear();
    }
};

// ============================================================
// TAPLogView — le widget visible à l'écran
// ============================================================

// Constructeur.
// La liste après ":" est la liste d'initialisation : elle construit
// les membres avec leurs arguments dès le départ (_stream_buf a besoin
// de connaître "this" dès sa création, d'où ce passage).
TAPLogView::TAPLogView()
    : _stream_buf(this),
    _old_cout_buf(nullptr) // nullptr = "aucun évier sauvegardé pour l'instant"
{
    // Titre de la section, en haut et centré
    this->_section_title.setText("LOGS");
    this->_layout.addWidget(&this->_section_title, 0, Qt::AlignCenter);

    // Zone de texte : lecture seule = l'utilisateur ne peut pas taper dedans,
    // mais elle reste défilante et scrolle automatiquement vers le bas
    // quand on y ajoute du texte.
    this->_text_edit.setReadOnly(true);
    this->_text_edit.setStyleSheet(TAP_LABEL_PROPERTIES);
    this->_layout.addWidget(&this->_text_edit);

    // "(no logs yet)" centré, tant qu'aucune ligne n'est arrivée
    this->_no_logs_label.setText("(no logs yet)");
    this->_layout.addWidget(&this->_no_logs_label, 0, Qt::AlignCenter);

    // Installer la disposition (layout) dans le widget
    this->setLayout(&this->_layout);
};

// DESTRUCTEUR : exécuté automatiquement quand le widget est détruit
// (équivalent de __del__ en Python, mais garanti).
// Indispensable : si le widget meurt sans ça, std::cout pointerait vers
// notre évier détruit -> crash au prochain std::cout du programme.
TAPLogView::~TAPLogView()
{
    // On ne remet l'évier d'origine que si on l'a bien sauvegardé
    // En gros, l'objet est détruit, et on remet l'ancien buffer pour ne
    // pas crash
    if (this->_old_cout_buf != nullptr)
        std::cout.rdbuf(this->_old_cout_buf); // rdbuf(ptr) = installer ptr
};

// Afficher une ligne dans la zone de texte.
// fromStdString convertit std::string (C++) -> QString (Qt).
// appendPlainText ajoute la ligne + un '\n' et scrolle en bas tout seul.
void TAPLogView::append_log(const std::string &line)
{
    this->_text_edit.appendPlainText(QString::fromStdString(line));

    // On a maintenant au moins une ligne : cacher le "(no logs yet)"
    this->_no_logs_label.hide();
};

// Remplacer tout le journal par les lignes données.
// clear() puis append_log() ligne à ligne : le widget montre alors
// exactement le journal de l'état, sans garder d'anciennes lignes.
void TAPLogView::update_logs(const std::deque<std::string> &lines)
{
    this->_text_edit.clear();
    if (lines.empty())
    {
        this->_no_logs_label.show();
        return;
    }
    for (const std::string &line : lines)
        this->append_log(line);
}

// L'ÉCHANGE D'ÉVIER (à appeler une seule fois).
//
// rdbuf() a deux comportements :
//   - rdbuf(&buf)  : INSTALLE buf comme évier de std::cout
//                    et RENVOIE l'ancien (le terminal).
//   - rdbuf()      : renvoie simplement l'évier actuel, sans rien changer.
//
// Cette ligne fait donc deux choses d'un coup :
//   1. std::cout écrit désormais dans _stream_buf -> tout atterrit dans le widget
//   2. on récupère l'évier d'origine dans _old_cout_buf, pour le restaurer
//      dans le destructeur.
void TAPLogView::install_as_cout(void)
{
    this->_old_cout_buf = std::cout.rdbuf(&this->_stream_buf);
};
