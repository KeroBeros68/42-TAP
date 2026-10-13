#ifndef TAP_EXIT_BUTTONS_HPP
# define TAP_EXIT_BUTTONS_HPP

# include "../../../../../custom_ui_elements/custom_ui_elements.hpp"
# include <iostream>
# include <map>
# include <string>
# include <QWidget>
# include <QVBoxLayout>

class TAPExitButtons : public QWidget
{
    private:
        QVBoxLayout _layout;
        TAPButton _buttons[4];
        std::map<std::string, std::string> _exit_names;

    public:
        TAPExitButtons();

        void update_available_exits(std::map<std::string, std::string> data);
};

#endif // !TAP_EXIT_BUTTONS_HPP