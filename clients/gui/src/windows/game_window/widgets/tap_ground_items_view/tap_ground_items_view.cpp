#include "tap_ground_items_view.hpp"
# include "globals.hpp"

TAPGroundItemsView::TAPGroundItemsView()
    : TAPGameSectionWidget(),
      _layout(this),
      _validate_button("TAKE")
{
    this->_validate_button.setStyleSheet(BUTTON_PROPERTIES);
    this->_layout.addWidget(&this->_validate_button);

    QObject::connect(&this->_validate_button, &QPushButton::clicked,
        this, &TAPGroundItemsView::on_validate_clicked);
};

void TAPGroundItemsView::update_ground_items(std::map<std::string, std::string> data)
{
    this->_items = data;
    this->rebuild_buttons();
};

void TAPGroundItemsView::rebuild_buttons(void)
{
    // Destroy every radio button of the previous update
    QLayoutItem *item;
    while ((item = this->_layout.takeAt(0)) != nullptr)
    {
        if (item->widget() && item->widget() != &this->_validate_button)
            delete item->widget();
        delete item;
    }

    // Rebuild one radio button per item on the ground
    for (const auto &[id, name] : this->_items)
    {
        QRadioButton *radio = new QRadioButton();
        radio->setText(name.c_str());
        radio->setStyleSheet(TAP_LABEL_PROPERTIES);
        // Store the canonical id inside the widget itself
        radio->setProperty("item_id", id.c_str());
        this->_layout.addWidget(radio);
    }

    // Keep the validate button last
    this->_layout.addWidget(&this->_validate_button);
};

std::string TAPGroundItemsView::get_selected_item_id(void) const
{
    QList<QRadioButton *> radios = this->findChildren<QRadioButton *>();
    for (QRadioButton *radio : radios)
    {
        if (radio->isChecked())
            return radio->property("item_id").toString().toStdString();
    }
    return "";
};

void TAPGroundItemsView::on_validate_clicked(void)
{
    std::string id = this->get_selected_item_id();
    if (id.empty())
        return; // nothing selected
    qDebug() << "Took item :" << id.c_str();
    // TODO : envoyer la demande de ramassage au serveur
};
