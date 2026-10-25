#ifndef TAP_PLAYER_INFO_BAR_HPP
# define TAP_PLAYER_INFO_BAR_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include <QWidget>

/**
 * Our own player: name, hit points and status, on one line.
 *
 * Read-only, like TAPPlayerCountBar: it draws what the Session read from the
 * server, it never sends anything. STATUS is the only source of the hit
 * points; the Session asks for it again after every combat answer.
 */
class TAPPlayerInfoBar : public TAPGameSectionWidget
{
    private:
        QHBoxLayout     _layout;
        TAPLabel        _label_name;    // our name, known once CONNECT is accepted
        TAPLabel        _label_status;  // healthy / wounded / combat / dead
        QProgressBar    _hp_bar;        // hp out of max_hp, empty until STATUS answers

    public:
        TAPPlayerInfoBar();

        // One redraw: name, hit points and status all come from the same
        // STATUS answer, so they are set together.
        void update_player_info(const std::string &name, int hp, int max_hp,
            const std::string &status);
};

#endif // !TAP_PLAYER_INFO_BAR_HPP
