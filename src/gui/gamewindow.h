#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>

class Game;
class QGraphicsView;
class QPushButton;
class QVBoxLayout;
class QResizeEvent;

class GameWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

private slots:
    void onResetButtonClicked();

private:
    void setupUI();
    void fitSceneInView();

    void resizeEvent(QResizeEvent* event) override;

    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    QGraphicsView* view;
    QPushButton* resetButton;
    Game* game;
};

#endif // GAMEWINDOW_H
