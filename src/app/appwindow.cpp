#include "appwindow.h"
#include "gamemanager.h"
#include "gui/pages/battlepage.h"
#include "gui/pages/eventpage.h"
#include "gui/pages/mappage.h"
#include "gui/pages/startpage.h"
#include "gui/widgets/cardpanel.h"
#include <QMessageBox>
#include <QResizeEvent>
#include <QStackedWidget>

AppWindow::AppWindow(QWidget* parent)
    : QMainWindow(parent)
    , manager(new GameManager(this))
    , stack(new QStackedWidget(this))
    , startPage(new StartPage(this))
    , mapPage(new MapPage(this))
    , eventPage(new EventPage(this))
    , battlePage(new BattlePage(this))
    , cardPanel(new CardPanel(this)) {
    setWindowTitle("Synera Starter");
    resize(1280, 720);
    setCentralWidget(stack);

    stack->addWidget(startPage);
    stack->addWidget(mapPage);
    stack->addWidget(eventPage);
    stack->addWidget(battlePage);
    stack->setCurrentWidget(startPage);

    connect(startPage, &StartPage::newGameRequested,
            manager, &GameManager::newGame);
    connect(startPage, &StartPage::loadGameRequested,
            manager, &GameManager::loadFromSlot);

    connect(mapPage, &MapPage::nodeSelected,
            manager, &GameManager::enterMapNode);
    connect(mapPage, &MapPage::saveRequested,
            manager, &GameManager::saveToSlot);
    connect(mapPage, &MapPage::bagRequested, this, [this]() {
        cardPanel->toggleBag();
    });
    connect(mapPage, &MapPage::shopRequested, this, [this]() {
        cardPanel->toggleShop();
    });

    connect(eventPage, &EventPage::eventFinished,
            manager, &GameManager::finishEvent);
    connect(eventPage, &EventPage::eventOptionSelected,
            manager, &GameManager::chooseEventOption);
    connect(eventPage, &EventPage::hexTechCardSelected,
            manager, &GameManager::chooseHexTechCard);
    connect(eventPage, &EventPage::saveRequested,
            manager, &GameManager::saveToSlot);
    connect(eventPage, &EventPage::bagRequested, this, [this]() {
        cardPanel->toggleBag();
    });
    connect(eventPage, &EventPage::shopRequested, this, [this]() {
        cardPanel->toggleShop();
    });

    connect(battlePage, &BattlePage::battleFinished,
            manager, &GameManager::finishBattle);
    connect(battlePage, &BattlePage::saveRequested,
            manager, &GameManager::saveToSlot);
    connect(battlePage, &BattlePage::bagRequested, this, [this]() {
        cardPanel->toggleBag();
    });
    connect(battlePage, &BattlePage::shopRequested, this, [this]() {
        if (manager->currentBattleSystem().isRunning()) {
            QMessageBox::information(this, QString::fromUtf8("商店"), QString::fromUtf8("商人正在备货"));
            return;
        }
        cardPanel->toggleShop();
    });
    connect(battlePage, &BattlePage::panelsShouldClose, cardPanel, &CardPanel::closePanel);
    battlePage->setBattleSystem(&manager->currentBattleSystem());
    connect(battlePage, &BattlePage::startBattleRequested, this, [this]() {
        manager->startPreparedBattle(battlePage->preparedPlacements());
    });
    connect(cardPanel, &CardPanel::buyRequested,
            manager, &GameManager::buyShopOffer);
    connect(cardPanel, &CardPanel::mergeRequested,
            manager, &GameManager::mergeOwnedCharacterCards);

    connect(manager, &GameManager::stateChanged,
            mapPage, &MapPage::setState);
    connect(manager, &GameManager::stateChanged,
            eventPage, &EventPage::setState);
    connect(manager, &GameManager::stateChanged,
            battlePage, &BattlePage::setState);
    connect(manager, &GameManager::stateChanged,
            cardPanel, &CardPanel::setGameState);

    connect(manager, &GameManager::showMapRequested, this, [this]() {
        stack->setCurrentWidget(mapPage);
        cardPanel->raise();
    });
    connect(manager, &GameManager::showEventRequested, this, [this](int nodeId) {
        eventPage->showEvent(nodeId);
        stack->setCurrentWidget(eventPage);
        cardPanel->raise();
    });
    connect(manager, &GameManager::showBattleRequested, this, [this](int nodeId) {
        battlePage->startNodeBattle(nodeId);
        stack->setCurrentWidget(battlePage);
        cardPanel->raise();
    });
    connect(manager, &GameManager::gameCompletedRequested, this, [this]() {
        QMessageBox::information(this, "Complete", "All layers complete. Victory placeholder.");
        stack->setCurrentWidget(startPage);
    });
    connect(manager, &GameManager::messageRequested, this, [this](const QString& title, const QString& message) {
        QMessageBox::information(this, title, message);
    });
}

void AppWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (cardPanel && cardPanel->isVisible()) {
        cardPanel->relayoutAnimated(false);
        cardPanel->raise();
    }
}
