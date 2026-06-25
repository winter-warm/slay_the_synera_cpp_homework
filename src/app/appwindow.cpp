#include "appwindow.h"
#include "gamemanager.h"
#include "gui/pages/battlepage.h"
#include "gui/pages/eventpage.h"
#include "gui/pages/mappage.h"
#include "gui/pages/startpage.h"
#include "gui/widgets/cardpanel.h"
#include "gui/widgets/equipmentpanel.h"
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTimer>

AppWindow::AppWindow(QWidget* parent)
    : QMainWindow(parent)
    , manager(new GameManager(this))
    , stack(new QStackedWidget(this))
    , startPage(new StartPage(this))
    , mapPage(new MapPage(this))
    , eventPage(new EventPage(this))
    , battlePage(nullptr)
    , cardPanel(new CardPanel(this))
    , equipmentPanel(new EquipmentPanel(this))
    , centerToastLabel(new QLabel(this)) {
    setWindowTitle("Synera Starter");
    resize(1280, 720);
    setCentralWidget(stack);
    centerToastLabel->setAlignment(Qt::AlignCenter);
    centerToastLabel->setFixedSize(620, 86);
    centerToastLabel->setStyleSheet(R"(
        color: #eeeeee;
        background-color: rgba(70, 70, 70, 220);
        border: 2px solid rgba(210, 210, 210, 170);
        border-radius: 12px;
        font-size: 32px;
        font-weight: 900;
    )");
    centerToastLabel->hide();

    stack->addWidget(startPage);
    stack->addWidget(mapPage);
    stack->addWidget(eventPage);
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
    connect(mapPage, &MapPage::equipmentRequested, this, [this]() {
        equipmentPanel->togglePanel();
    });
    connect(mapPage, &MapPage::returnToStartRequested, this, [this]() {
        manager->pauseElapsedTimer();
        cardPanel->closePanel();
        equipmentPanel->closePanel();
        stack->setCurrentWidget(startPage);
    });

    connect(eventPage, &EventPage::eventFinished,
            manager, &GameManager::finishEvent);
    connect(eventPage, &EventPage::eventOptionSelected,
            manager, &GameManager::chooseEventOption);
    connect(eventPage, &EventPage::hexTechCardSelected,
            manager, &GameManager::chooseHexTechCard);
    connect(eventPage, &EventPage::restOptionSelected,
            manager, &GameManager::chooseRestOption);
    connect(eventPage, &EventPage::restTrainingCardSelected,
            manager, &GameManager::chooseRestTrainingCard);
    connect(eventPage, &EventPage::eventOwnedCardSelected,
            manager, &GameManager::chooseEventOwnedCard);
    connect(eventPage, &EventPage::trainingPanelRequested, this, [this]() {
        cardPanel->showTraining();
        cardPanel->raise();
    });
    connect(eventPage, &EventPage::saveRequested,
            manager, &GameManager::saveToSlot);
    connect(eventPage, &EventPage::bagRequested, this, [this]() {
        cardPanel->toggleBag();
    });
    connect(eventPage, &EventPage::shopRequested, this, [this]() {
        cardPanel->toggleShop();
    });
    connect(eventPage, &EventPage::equipmentRequested, this, [this]() {
        equipmentPanel->togglePanel();
    });
    connect(eventPage, &EventPage::returnToStartRequested, this, [this]() {
        manager->pauseElapsedTimer();
        cardPanel->closePanel();
        equipmentPanel->closePanel();
        stack->setCurrentWidget(startPage);
    });

    connect(cardPanel, &CardPanel::buyRequested,
            manager, &GameManager::buyShopOffer);
    connect(cardPanel, &CardPanel::mergeRequested,
            manager, &GameManager::mergeOwnedCharacterCards);
    connect(cardPanel, &CardPanel::trainRequested, this, [this](int ownedCardIndex) {
        cardPanel->closePanel();
        manager->chooseRestTrainingCard(ownedCardIndex);
    });
    connect(equipmentPanel, &EquipmentPanel::unequipAllRequested,
            manager, &GameManager::unequipAllEquipment);
    connect(equipmentPanel, &EquipmentPanel::composeRequested,
            manager, &GameManager::composeEquipment);

    connect(manager, &GameManager::stateChanged,
            mapPage, &MapPage::setState);
    connect(manager, &GameManager::stateChanged,
            eventPage, &EventPage::setState);
    connect(manager, &GameManager::stateChanged,
            cardPanel, &CardPanel::setGameState);
    connect(manager, &GameManager::stateChanged,
            equipmentPanel, &EquipmentPanel::setGameState);
    connect(manager, &GameManager::elapsedSecondsChanged,
            mapPage, &MapPage::setElapsedSeconds);
    connect(manager, &GameManager::elapsedSecondsChanged,
            eventPage, &EventPage::setElapsedSeconds);

    connect(manager, &GameManager::showMapRequested, this, [this]() {
        stack->setCurrentWidget(mapPage);
        cardPanel->raise();
        equipmentPanel->setBattlePreparationMode(false);
        equipmentPanel->raise();
    });
    connect(manager, &GameManager::showEventRequested, this, [this](int nodeId) {
        stack->setCurrentWidget(eventPage);
        eventPage->showEvent(nodeId);
        if (manager->state().currentEvent.restTrainingSelection) {
            cardPanel->showTraining();
        } else if (cardPanel->mode() == CardPanel::Mode::Training) {
            cardPanel->closePanel();
        }
        cardPanel->raise();
        equipmentPanel->setBattlePreparationMode(false);
        equipmentPanel->raise();
    });
    connect(manager, &GameManager::showBattleRequested, this, [this](int nodeId) {
        BattlePage* page = ensureBattlePage();
        page->startNodeBattle(nodeId);
        stack->setCurrentWidget(page);
        cardPanel->raise();
        equipmentPanel->raise();
    });
    connect(manager, &GameManager::gameCompletedRequested, this, [this]() {
        cardPanel->closePanel();
        QMessageBox::information(this, QString::fromUtf8("通关"), QString::fromUtf8("恭喜通关，战绩已记录。"));
        stack->setCurrentWidget(startPage);
    });
    connect(manager, &GameManager::gameOverRequested, this, [this]() {
        cardPanel->closePanel();
        QMessageBox::information(this, QString::fromUtf8("Game Over"), QString::fromUtf8("生命归零，战绩已记录。"));
        stack->setCurrentWidget(startPage);
    });
    connect(manager, &GameManager::messageRequested, this, [this](const QString& title, const QString& message) {
        QMessageBox::information(this, title, message);
    });
    connect(manager, &GameManager::centerToastRequested,
            this, &AppWindow::showCenterToast);
}

BattlePage* AppWindow::ensureBattlePage() {
    if (battlePage) {
        return battlePage;
    }

    battlePage = new BattlePage(this);
    stack->addWidget(battlePage);

    connect(battlePage, &BattlePage::battleFinished,
            manager, &GameManager::finishBattle);
    connect(battlePage, &BattlePage::battleChestOpened,
            manager, &GameManager::grantBattleChestGold);
    connect(battlePage, &BattlePage::battleChestEquipmentClaimed,
            manager, &GameManager::grantBattleChestEquipment);
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
    connect(battlePage, &BattlePage::equipmentRequested, this, [this]() {
        equipmentPanel->togglePanel();
    });
    connect(battlePage, &BattlePage::equipEquipmentRequested,
            manager, &GameManager::equipEquipmentToCard);
    connect(battlePage, &BattlePage::battlePreparationModeChanged,
            equipmentPanel, &EquipmentPanel::setBattlePreparationMode);
    connect(battlePage, &BattlePage::panelsShouldClose, cardPanel, &CardPanel::closePanel);
    connect(battlePage, &BattlePage::panelsShouldClose, equipmentPanel, &EquipmentPanel::closePanel);
    connect(battlePage, &BattlePage::returnToStartRequested, this, [this]() {
        manager->pauseElapsedTimer();
        cardPanel->closePanel();
        equipmentPanel->closePanel();
        stack->setCurrentWidget(startPage);
    });
    battlePage->setBattleSystem(&manager->currentBattleSystem());
    connect(battlePage, &BattlePage::startBattleRequested, this, [this]() {
        manager->startPreparedBattle(battlePage->preparedPlacements());
    });
    connect(manager, &GameManager::stateChanged,
            battlePage, &BattlePage::setState);
    connect(manager, &GameManager::elapsedSecondsChanged,
            battlePage, &BattlePage::setElapsedSeconds);

    battlePage->setState(manager->state());
    battlePage->setElapsedSeconds(manager->state().elapsedSeconds);
    return battlePage;
}

void AppWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    positionCenterToast();
    if (cardPanel && cardPanel->isVisible()) {
        cardPanel->relayoutAnimated(false);
        cardPanel->raise();
    }
    if (equipmentPanel && equipmentPanel->isVisible()) {
        equipmentPanel->relayoutAnimated(false);
        equipmentPanel->raise();
    }
}

void AppWindow::showCenterToast(const QString& text) {
    if (!centerToastLabel) {
        return;
    }
    centerToastLabel->setText(text);
    positionCenterToast();
    auto* effect = new QGraphicsOpacityEffect(centerToastLabel);
    effect->setOpacity(1.0);
    centerToastLabel->setGraphicsEffect(effect);
    centerToastLabel->show();
    centerToastLabel->raise();
    QTimer::singleShot(3000, this, [this, effect]() {
        if (!centerToastLabel || centerToastLabel->graphicsEffect() != effect) {
            return;
        }
        auto* fade = new QPropertyAnimation(effect, "opacity", centerToastLabel);
        fade->setDuration(520);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        connect(fade, &QPropertyAnimation::finished, centerToastLabel, &QLabel::hide);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void AppWindow::positionCenterToast() {
    if (!centerToastLabel) {
        return;
    }
    centerToastLabel->move((width() - centerToastLabel->width()) / 2,
                           (height() - centerToastLabel->height()) / 2);
}
