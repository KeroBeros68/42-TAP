#ifndef TAP_ERROR_WINDOW_HPP
# define TAP_ERROR_WINDOW_HPP

# include "../base_window/tap_base_window.hpp"
# include "../../custom_ui_elements/custom_ui_elements.hpp"

class TAPErrorWindow : public TAPBaseWindow
{
    private:
        TAPLabel _error_text_label;
        QVBoxLayout _layout;

    public:
        // Constructor
        TAPErrorWindow();

        // Error message related function
        void        setErrorMessage(std::string message);
        std::string getErrorMessage();

};

#endif // !TAP_ERROR_WINDOW_HPP
