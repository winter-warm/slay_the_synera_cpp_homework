#include "eventpage.h"
#include "gui/widgets/gamehud.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EventPage::EventPage(QWidget* parent)
    : QWidget(parent)
    , hud(new GameHud(this))
    , titleLabel(new QLabel(this)) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(hud);

    auto* content = new QWidget(this);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(60, 60, 60, 60);
    contentLayout->addStretch();
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700;");
    contentLayout->addWidget(titleLabel);

    auto* text = new QLabel("Temporary event test page", this);
    text->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(text);

    auto* finishButton = new QPushButton("Finish Event", this);
    contentLayout->addWidget(finishButton, 0, Qt::AlignCenter);
    contentLayout->addStretch();
    layout->addWidget(content, 1);

    connect(finishButton, &QPushButton::clicked, this, [this]() {
        emit eventFinished({});
    });
    connect(hud, &GameHud::saveRequested, this, &EventPage::saveRequested);

    setStyleSheet("QWidget { background-color: #25272d; color: #f2f2f2; } QPushButton { padding: 8px 18px; }");
}

void EventPage::setState(const GameState& state) {
    hud->setState(state);
}

void EventPage::showEvent(int nodeId) {
    titleLabel->setText(QString("Event Node %1").arg(nodeId));
}
