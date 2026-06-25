#include "equipmentpanel.h"
#include <QCoreApplication>
#include <QDir>
#include <QDrag>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

static const char* equipmentMimeType = "application/x-synera-equipment-instance";

static QColor borderColorFor(EquipmentGroup group) {
    if (group == EquipmentGroup::Advanced) {
        return QColor(232, 184, 52);
    }
    if (group == EquipmentGroup::Element) {
        return QColor(86, 184, 94);
    }
    return QColor(73, 145, 224);
}

static int groupRank(EquipmentGroup group) {
    if (group == EquipmentGroup::Advanced) {
        return 3;
    }
    if (group == EquipmentGroup::Mold) {
        return 2;
    }
    return 1;
}

class EquipmentItemWidget : public QWidget {
public:
    EquipmentItemWidget(EquipmentPanel* panel, int instanceId, QWidget* parent = nullptr)
        : QWidget(parent)
        , panel(panel)
        , instanceId(instanceId) {
        setFixedSize(78, 78);
        setMouseTracking(true);
        setAcceptDrops(true);
    }

    int id() const { return instanceId; }

protected:
    void paintEvent(QPaintEvent*) override {
        const OwnedEquipment* owned = panel->ownedByInstanceId(instanceId);
        if (!owned) {
            return;
        }
        const auto equipment = panel->resolveEquipment(*owned);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer = rect().adjusted(4, 4, -4, -4);
        QColor bg(248, 238, 204);
        if (highlighted) {
            bg = QColor(255, 244, 162);
        }
        painter.setBrush(bg);
        painter.setPen(QPen(borderColorFor(owned->group), highlighted ? 5 : 3));
        painter.drawRoundedRect(outer, 8, 8);

        QPixmap icon;
        if (equipment) {
            const QString path = QDir(QCoreApplication::applicationDirPath())
                                     .filePath(QString::fromStdString(equipment->iconPath()));
            icon.load(path);
            if (icon.isNull()) {
                icon.load(QString::fromStdString(equipment->iconPath()));
            }
        }
        const QRect iconRect = rect().adjusted(14, 12, -14, -14);
        if (!icon.isNull()) {
            painter.drawPixmap(iconRect, icon.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 221, 119));
            painter.drawEllipse(iconRect.adjusted(5, 5, -5, -5));
            painter.setPen(QPen(QColor(83, 55, 25), 2));
            painter.setFont(QFont("Arial", 16, QFont::Black));
            painter.drawText(iconRect, Qt::AlignCenter, equipment ? QString::fromStdString(equipment->title()).left(1) : "?");
        }

        if (owned->equippedCardUid >= 0) {
            const QRectF lockRect(width() - 27, height() - 27, 19, 19);
            painter.setBrush(QColor(39, 34, 27, 235));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(lockRect);
            painter.setPen(QPen(QColor(255, 230, 120), 2));
            painter.drawRoundedRect(lockRect.adjusted(5, 8, -5, -4), 2, 2);
            painter.drawArc(lockRect.adjusted(5, 3, -5, -7), 0, 180 * 16);
        }
    }

    void enterEvent(QEnterEvent*) override {
        panel->showDetailFor(this);
    }

    void leaveEvent(QEvent*) override {
        panel->hideDetail();
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (!(event->buttons() & Qt::LeftButton)) {
            return;
        }
        const OwnedEquipment* owned = panel->ownedByInstanceId(instanceId);
        if (!owned || !panel->canDragEquipment(*owned)) {
            return;
        }
        panel->hideDetail();
        auto* drag = new QDrag(this);
        auto* mime = new QMimeData();
        mime->setData(equipmentMimeType, QByteArray::number(instanceId));
        drag->setMimeData(mime);
        QPixmap pixmap(size());
        pixmap.fill(Qt::transparent);
        render(&pixmap);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(width() / 2, height() / 2));
        drag->exec(Qt::MoveAction);
    }

    void dragEnterEvent(QDragEnterEvent* event) override {
        if (!event->mimeData()->hasFormat(equipmentMimeType)) {
            return;
        }
        const int sourceId = event->mimeData()->data(equipmentMimeType).toInt();
        highlighted = panel->canComposeEquipment(sourceId, instanceId);
        update();
        if (highlighted) {
            event->acceptProposedAction();
        }
    }

    void dragLeaveEvent(QDragLeaveEvent*) override {
        highlighted = false;
        update();
    }

    void dropEvent(QDropEvent* event) override {
        highlighted = false;
        update();
        if (!event->mimeData()->hasFormat(equipmentMimeType)) {
            return;
        }
        const int sourceId = event->mimeData()->data(equipmentMimeType).toInt();
        if (!panel->canComposeEquipment(sourceId, instanceId)) {
            return;
        }
        panel->requestCompose(sourceId, instanceId);
        event->acceptProposedAction();
    }

private:
    EquipmentPanel* panel;
    int instanceId = 0;
    bool highlighted = false;
};

class EquipmentSlotPlaceholder : public QWidget {
public:
    explicit EquipmentSlotPlaceholder(QWidget* parent = nullptr)
        : QWidget(parent) {
        setFixedSize(78, 78);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF outer = rect().adjusted(4, 4, -4, -4);
        painter.setBrush(QColor(236, 220, 178, 150));
        painter.setPen(QPen(QColor(255, 218, 116, 220), 2));
        painter.drawRoundedRect(outer, 8, 8);
        painter.setPen(QPen(QColor(96, 66, 28, 150), 2, Qt::DashLine));
        painter.drawRoundedRect(outer.adjusted(8, 8, -8, -8), 6, 6);
    }
};

EquipmentPanel::EquipmentPanel(QWidget* parent)
    : QFrame(parent)
    , unequipButton(new QPushButton(this))
    , scrollArea(new QScrollArea(this))
    , gridHost(new QWidget(this))
    , gridLayout(new QGridLayout(gridHost))
    , detailPopup(new QFrame(this)) {
    setObjectName("EquipmentPanel");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(R"(
        #EquipmentPanel {
            background: rgba(31, 26, 22, 238);
            border-right: 2px solid rgba(255, 221, 138, 180);
        }
        QPushButton {
            min-height: 38px;
            font-size: 15px;
            font-weight: 900;
            border-radius: 8px;
            border: 1px solid rgba(88, 61, 22, 190);
        }
        QPushButton:enabled {
            background: #f2c84d;
            color: #291907;
        }
        QPushButton:disabled {
            background: #77716a;
            color: #2b2824;
        }
        QScrollArea {
            border: 0;
            background: transparent;
        }
    )");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(10);
    layout->addWidget(unequipButton);
    layout->addWidget(scrollArea, 1);

    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(8);
    gridLayout->setVerticalSpacing(8);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(gridHost);

    detailPopup->hide();
    detailPopup->setObjectName("EquipmentDetailPopup");
    detailPopup->setStyleSheet(R"(
        #EquipmentDetailPopup {
            background: rgba(255, 247, 218, 250);
            border: 2px solid rgba(81, 51, 20, 210);
            border-radius: 8px;
        }
        QLabel {
            background: transparent;
            color: #2b1908;
            font-size: 13px;
        }
    )");

    connect(unequipButton, &QPushButton::clicked, this, &EquipmentPanel::unequipAllRequested);
    hide();
}

void EquipmentPanel::setGameState(const GameState& state) {
    currentState = state;
    rebuild();
}

void EquipmentPanel::setBattlePreparationMode(bool enabled) {
    battlePreparationMode = enabled;
    rebuild();
}

void EquipmentPanel::togglePanel() {
    if (isVisible()) {
        closePanel();
        return;
    }
    relayoutAnimated(false);
    show();
    raise();
    rebuild();
}

void EquipmentPanel::closePanel() {
    hideDetail();
    hide();
}

void EquipmentPanel::relayoutAnimated(bool) {
    if (!parentWidget()) {
        return;
    }
    const int top = 72;
    setGeometry(0, top, std::max(260, parentWidget()->width() / 5), std::max(1, parentWidget()->height() - top));
}

std::optional<Equipment> EquipmentPanel::resolveEquipment(const OwnedEquipment& owned) const {
    if (owned.group == EquipmentGroup::Advanced) {
        return repository.findAdvanced(owned.equipmentId);
    }
    return repository.findBasic(owned.group, owned.equipmentId);
}

bool EquipmentPanel::canDragEquipment(const OwnedEquipment& owned) const {
    if (owned.equippedCardUid >= 0) {
        return false;
    }
    if (battlePreparationMode) {
        return true;
    }
    return isRestActive() && owned.group != EquipmentGroup::Advanced;
}

bool EquipmentPanel::canComposeEquipment(int sourceInstanceId, int targetInstanceId) const {
    if (!isRestActive() || sourceInstanceId == targetInstanceId) {
        return false;
    }
    const OwnedEquipment* source = ownedByInstanceId(sourceInstanceId);
    const OwnedEquipment* target = ownedByInstanceId(targetInstanceId);
    if (!source || !target || source->equippedCardUid >= 0 || target->equippedCardUid >= 0) {
        return false;
    }
    const auto left = resolveEquipment(*source);
    const auto right = resolveEquipment(*target);
    return left && right && repository.canCompose(*left, *right);
}

bool EquipmentPanel::isRestActive() const {
    return currentState.currentEvent.active &&
           currentState.currentEvent.eventId == MapEventId::Rest;
}

void EquipmentPanel::showDetailFor(EquipmentItemWidget* item) {
    if (!item) {
        return;
    }
    const OwnedEquipment* owned = ownedByInstanceId(item->id());
    if (!owned) {
        return;
    }
    const auto equipment = resolveEquipment(*owned);
    if (!equipment) {
        return;
    }

    if (QLayout* oldLayout = detailPopup->layout()) {
        while (QLayoutItem* oldItem = oldLayout->takeAt(0)) {
            if (QWidget* widget = oldItem->widget()) {
                delete widget;
            }
            delete oldItem;
        }
        delete oldLayout;
    }
    auto* layout = new QVBoxLayout(detailPopup);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(5);

    auto addLabel = [layout](const QString& text, bool title = false) {
        auto* label = new QLabel(text);
        label->setWordWrap(true);
        label->setTextFormat(Qt::RichText);
        if (title) {
            label->setStyleSheet("font-size: 15px; font-weight: 900; color: #251302; background: transparent;");
        }
        layout->addWidget(label);
    };

    addLabel(QString::fromStdString(equipment->title()).toHtmlEscaped(), true);
    addLabel(richNumberText(QString::fromStdString(equipment->effectText())));
    if (equipment->isBasic()) {
        addLabel(recipeTextFor(*equipment));
    } else if (!equipment->skillText().empty()) {
        addLabel(richNumberText(QString::fromUtf8("技能：") + QString::fromStdString(equipment->skillText())));
    }

    detailPopup->setFixedWidth(230);
    detailPopup->adjustSize();

    QWidget* popupParent = parentWidget() ? parentWidget() : this;
    if (detailPopup->parentWidget() != popupParent) {
        detailPopup->setParent(popupParent);
    }

    const QPoint itemTopLeft = item->mapTo(popupParent, QPoint(0, 0));
    const QPoint itemInGrid = item->mapTo(gridHost, QPoint(item->width() / 2, 0));
    const int gridWidth = std::max(1, gridHost->width());
    const int column = std::min(2, std::max(0, itemInGrid.x() * 3 / gridWidth));

    QPoint pos;
    if (column == 2) {
        pos = itemTopLeft - QPoint(detailPopup->width() + 8, 0);
    } else {
        pos = item->mapTo(popupParent, QPoint(item->width() + 8, 0));
    }
    pos.setX(std::min(std::max(8, pos.x()), std::max(8, popupParent->width() - detailPopup->width() - 8)));
    pos.setY(std::min(std::max(8, pos.y()), std::max(8, popupParent->height() - detailPopup->height() - 8)));
    detailPopup->move(pos);
    detailPopup->show();
    detailPopup->raise();
}

void EquipmentPanel::hideDetail() {
    detailPopup->hide();
    if (QLayout* oldLayout = detailPopup->layout()) {
        while (QLayoutItem* oldItem = oldLayout->takeAt(0)) {
            if (QWidget* widget = oldItem->widget()) {
                delete widget;
            }
            delete oldItem;
        }
        delete oldLayout;
    }
}

void EquipmentPanel::requestCompose(int firstInstanceId, int secondInstanceId) {
    playComposeBurst();
    emit composeRequested(firstInstanceId, secondInstanceId);
}

void EquipmentPanel::resizeEvent(QResizeEvent* event) {
    QFrame::resizeEvent(event);
    hideDetail();
}

void EquipmentPanel::rebuild() {
    unequipButton->setText(isRestActive() ? QString::fromUtf8("脱下所有装备")
                                          : QString::fromUtf8("可以在火堆处脱下"));
    unequipButton->setEnabled(isRestActive());

    while (QLayoutItem* item = gridLayout->takeAt(0)) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    std::vector<OwnedEquipment> sorted = currentState.ownedEquipment;
    std::sort(sorted.begin(), sorted.end(), [this](const OwnedEquipment& left, const OwnedEquipment& right) {
        const bool leftEquipped = left.equippedCardUid >= 0;
        const bool rightEquipped = right.equippedCardUid >= 0;
        if (leftEquipped != rightEquipped) {
            return leftEquipped > rightEquipped;
        }
        const int leftRank = groupRank(left.group);
        const int rightRank = groupRank(right.group);
        if (leftRank != rightRank) {
            return leftRank > rightRank;
        }
        const auto leftEquipment = resolveEquipment(left);
        const auto rightEquipment = resolveEquipment(right);
        const std::string leftName = leftEquipment ? leftEquipment->title() : "";
        const std::string rightName = rightEquipment ? rightEquipment->title() : "";
        if (leftName != rightName) {
            return leftName < rightName;
        }
        return left.instanceId < right.instanceId;
    });

    int index = 0;
    for (const OwnedEquipment& owned : sorted) {
        auto* item = new EquipmentItemWidget(this, owned.instanceId, gridHost);
        gridLayout->addWidget(item, index / 3, index % 3);
        ++index;
    }
    const int minimumSlots = 21;
    while (index < minimumSlots) {
        auto* slot = new EquipmentSlotPlaceholder(gridHost);
        gridLayout->addWidget(slot, index / 3, index % 3);
        ++index;
    }
    gridLayout->setRowStretch((index + 2) / 3, 1);
}

QString EquipmentPanel::richNumberText(const QString& text) const {
    QString escaped = text.toHtmlEscaped();
    static const QRegularExpression numberPattern("([+-]?\\d+%?)");
    return escaped.replace(numberPattern, "<span style=\"color:#d22222;font-weight:900;\">\\1</span>");
}

QString EquipmentPanel::recipeTextFor(const Equipment& equipment) const {
    QStringList lines;
    for (const Equipment& advanced : repository.allAdvanced()) {
        const auto recipe = advanced.recipe();
        if (!recipe) {
            continue;
        }
        if (equipment.group() == EquipmentGroup::Mold && recipe->moldId == equipment.id()) {
            const auto element = repository.findBasic(EquipmentGroup::Element, recipe->elementId);
            lines << QString::fromUtf8("合成：%1 + %2")
                         .arg(QString::fromStdString(element ? element->title() : std::to_string(recipe->elementId)))
                         .arg(QString::fromStdString(advanced.title()));
        } else if (equipment.group() == EquipmentGroup::Element && recipe->elementId == equipment.id()) {
            const auto mold = repository.findBasic(EquipmentGroup::Mold, recipe->moldId);
            lines << QString::fromUtf8("合成：%1 + %2")
                         .arg(QString::fromStdString(mold ? mold->title() : std::to_string(recipe->moldId)))
                         .arg(QString::fromStdString(advanced.title()));
        }
    }
    return lines.isEmpty() ? QString::fromUtf8("暂无可合成清单") : lines.join("<br>");
}

QString EquipmentPanel::groupName(EquipmentGroup group) const {
    if (group == EquipmentGroup::Advanced) {
        return QString::fromUtf8("高级");
    }
    if (group == EquipmentGroup::Element) {
        return QString::fromUtf8("元素");
    }
    return QString::fromUtf8("胚子");
}

OwnedEquipment* EquipmentPanel::ownedByInstanceId(int instanceId) {
    for (OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.instanceId == instanceId) {
            return &equipment;
        }
    }
    return nullptr;
}

const OwnedEquipment* EquipmentPanel::ownedByInstanceId(int instanceId) const {
    for (const OwnedEquipment& equipment : currentState.ownedEquipment) {
        if (equipment.instanceId == instanceId) {
            return &equipment;
        }
    }
    return nullptr;
}

void EquipmentPanel::playComposeBurst() {
    for (int i = 0; i < 18; ++i) {
        auto* dot = new QLabel(this);
        dot->setFixedSize(7, 7);
        dot->setStyleSheet("background:#ffd33f;border-radius:3px;");
        const QPoint start(width() / 2, height() / 2);
        dot->move(start);
        dot->show();
        dot->raise();
        const int dx = QRandomGenerator::global()->bounded(-90, 91);
        const int dy = QRandomGenerator::global()->bounded(-90, 31);
        auto* animation = new QPropertyAnimation(dot, "pos", dot);
        animation->setDuration(620);
        animation->setStartValue(start);
        animation->setEndValue(start + QPoint(dx, dy));
        connect(animation, &QPropertyAnimation::finished, dot, &QLabel::deleteLater);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
