#include "savemanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

std::string SaveManager::savePath(int slot) const {
    const QString dirPath = QCoreApplication::applicationDirPath() + "/saves";
    return QDir(dirPath).filePath(QString("save%1.json").arg(slot)).toStdString();
}

bool SaveManager::saveGame(const GameState& state, int slot, std::string* error) const {
    if (slot < 1 || slot > 3) {
        if (error) {
            *error = "Save slot must be 1, 2, or 3.";
        }
        return false;
    }

    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.exists("saves") && !dir.mkdir("saves")) {
        if (error) {
            *error = "Failed to create saves directory.";
        }
        return false;
    }

    QFile file(QString::fromStdString(savePath(slot)));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString().toStdString();
        }
        return false;
    }

    file.write(QJsonDocument(state.toJson()).toJson(QJsonDocument::Indented));
    return true;
}

bool SaveManager::loadGame(GameState* state, int slot, std::string* error) const {
    if (!state || slot < 1 || slot > 3) {
        if (error) {
            *error = "Invalid save slot.";
        }
        return false;
    }

    QFile file(QString::fromStdString(savePath(slot)));
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString().toStdString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = parseError.errorString().toStdString();
        }
        return false;
    }

    *state = GameState::fromJson(doc.object());
    return true;
}

std::vector<int> SaveManager::existingSlots() const {
    std::vector<int> saveSlots;
    for (int slot = 1; slot <= 3; ++slot) {
        if (QFile::exists(QString::fromStdString(savePath(slot)))) {
            saveSlots.push_back(slot);
        }
    }
    return saveSlots;
}
