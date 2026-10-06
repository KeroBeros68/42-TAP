#ifndef TAP_EXIT_BUTTONS_HPP
# define TAP_EXIT_BUTTONS_HPP

# include <QPushButton>
# include <QWidget>
# include <QHBoxLayout>

class TAPExitButtons : public QWidget
{
    private:
        QHBoxLayout _layout;
        QPushButton _buttons[4];
        std::map<std::string, std::string> _exit_names;

    public:
        TAPExitButtons();

        void update_available_exits(std::map<std::string, std::string> data);
};

#endif // !TAP_EXIT_BUTTONS_HPP