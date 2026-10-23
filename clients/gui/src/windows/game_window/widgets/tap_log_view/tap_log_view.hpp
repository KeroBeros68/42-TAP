#ifndef TAP_LOG_VIEW_HPP
# define TAP_LOG_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include <QPlainTextEdit>
# include <deque>
# include <string>
# include <streambuf>
# include <iostream>

// L'ÉVIER DU COUT.
//
// std::cout n'écrit jamais rien lui-même : il délègue tout à un objet
// interne appelé "streambuf" (son évier). En héritant de std::streambuf
// et en redéfinissant ses méthodes d'écriture, on fabrique un évier
// personnalisé qui envoie les lignes vers le widget de logs.
//
// Équivalent Python : la classe "Capturer" qu'on met dans sys.stdout.
class TAPLogStreamBuf : public std::streambuf
{
    private:
        class TAPLogView *_view;   // Le widget à qui envoyer les lignes complètes
        std::string _pending;      // Tampon : caractères reçus mais pas encore
                                   // suivis d'un '\n' (la ligne n'est pas finie)

    protected:
        // Appelé par std::cout pour UN SEUL caractère (ex : std::endl
        // arrive comme un '\n' isolé). "override" = on redéfinit la
        // méthode de la classe mère std::streambuf.
        int overflow(int ch) override; // Single char

        // Appelé par std::cout pour un LOT de n caractères
        // (ex : std::cout << "hello" passe par ici).
        std::streamsize xsputn(const char *text, std::streamsize n) override; // String

    public:
        TAPLogStreamBuf(class TAPLogView *view);

        // Afficher de force ce qui reste dans _pending
        // (ligne reçue sans '\n' final).
        void flush_pending(void);
};

// LE WIDGET DE LOGS.
//
// Zone de texte défilante et en lecture seule, qui affiche chaque ligne
// que le TAPLogStreamBuf lui transmet. C'est aussi lui qui fait l'échange
// d'évier de std::cout (install_as_cout) et qui le défait à sa destruction.
class TAPLogView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout     _layout;          // Empilement vertical : titre puis zone de texte
        TAPRoomNameLabel _section_title;  // Le titre "LOGS" en haut de la section
        QPlainTextEdit  _text_edit;       // La zone défilante où les lignes s'affichent
        TAPLabel        _no_logs_label;   // "(no logs yet)" tant qu'aucune ligne n'est arrivée
        TAPLogStreamBuf _stream_buf;      // Notre évier custom, propriété de ce widget
        std::streambuf  *_old_cout_buf;   // L'évier d'origine de std::cout (le terminal),
                                          //  sauvegardé pour pouvoir le remettre à la fin

    public:
        TAPLogView();
        ~TAPLogView(); // Destructeur : remet l'évier d'origine dans std::cout

        // Afficher une ligne complète dans la zone de texte
        // (appelé uniquement par TAPLogStreamBuf)
        void append_log(const std::string &line);

        // Replace the whole journal by the lines given.
        // Used by the GUI when the state's journal changed.
        void update_logs(const std::deque<std::string> &lines);

        // BRANCHEMENT : installer notre évier dans std::cout.
        // Équivalent Python :  sys.stdout = Capturer()
        // À appeler une seule fois, après la construction du widget.
        void install_as_cout(void);
};

#endif // !TAP_LOG_VIEW_HPP
