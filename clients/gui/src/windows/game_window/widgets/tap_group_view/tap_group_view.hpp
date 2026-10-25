#ifndef TAP_GROUP_VIEW_HPP
# define TAP_GROUP_VIEW_HPP

# include "globals.hpp"
# include "src/custom_ui_elements/custom_ui_elements.hpp"
# include "../../../../../../state/session.hpp"

class TAPGroupView : public TAPGameSectionWidget
{
    private:
        QVBoxLayout     _layout;
        TAPRoomNameLabel _section_title;
        TAPLabel        _status_label;      // the id of the group, or "(no group)"
        TAPLabel        _invite_label;      // "<leader> invites you", empty when nobody invited us

        // GROUP INVITE <player>: the name is typed, nobody else knows it
        QHBoxLayout     _invite_row;
        TAPLineEdit     _invite_field;
        TAPButton       _invite_button;

        // GROUP JOIN <leader>: prefilled with the leader of the invitation
        QHBoxLayout     _join_row;
        TAPLineEdit     _join_field;
        TAPButton       _join_button;

        // CREATE and LEAVE side by side, neither takes an argument
        QHBoxLayout     _actions_row;
        TAPButton       _create_button;
        TAPButton       _leave_button;

        Session     &_session;

    public:
        explicit TAPGroupView(Session &session);
        void update_group(bool in_group, const std::string &group_id,
            const std::string &pending_invite);
};

#endif // !TAP_GROUP_VIEW_HPP
