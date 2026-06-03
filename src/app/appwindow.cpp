#include "appwindow.h"
#include "gamemanager.h"
#include "gui/pages/battlepage.h"
#include "gui/pages/eventpage.h"
#include "gui/pages/mappage.h"
#include "gui/pages/startpage.h"
#include <QMessageBox>
#include <QStackedWidget>

AppWindow::AppWindow(QWidget* parent)
    : QMainWindow(parent)
    , manager(new GameManager(this))
    , stack(new QStackedWidget(this))
    , startPage(new StartPage(this))
    , mapPage(new MapPage(this))
    , eventPage(new EventPage(this))
    , battlePage(new BattlePage(this)) {
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

    connect(eventPage, &EventPage::eventFinished,
            manager, &GameManager::finishEvent);
    connect(eventPage, &EventPage::saveRequested,
            manager, &GameManager::saveToSlot);

    connect(battlePage, &BattlePage::battleFinished,
            manager, &GameManager::finishBattle);
    connect(battlePage, &BattlePage::saveRequested,
            manager, &GameManager::saveToSlot);

    connect(manager, &GameManager::stateChanged,
            mapPage, &MapPage::setState);
    connect(manager, &GameManager::stateChanged,
            eventPage, &EventPage::setState);
    connect(manager, &GameManager::stateChanged,
            battlePage, &BattlePage::setState);

    connect(manager, &GameManager::showMapRequested, this, [this]() {
        stack->setCurrentWidget(mapPage);
    });
    connect(manager, &GameManager::showEventRequested, this, [this](int nodeId) {
        eventPage->showEvent(nodeId);
        stack->setCurrentWidget(eventPage);
    });
    connect(manager, &GameManager::showBattleRequested, this, [this](int nodeId) {
        battlePage->startNodeBattle(nodeId);
        stack->setCurrentWidget(battlePage);
    });
    connect(manager, &GameManager::gameCompletedRequested, this, [this]() {
        QMessageBox::information(this, "Complete", "All layers complete. Victory placeholder.");
        stack->setCurrentWidget(startPage);
    });
    connect(manager, &GameManager::messageRequested, this, [this](const QString& title, const QString& message) {
        QMessageBox::information(this, title, message);
    });
}
