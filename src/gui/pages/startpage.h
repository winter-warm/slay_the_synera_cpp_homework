#ifndef GUI_PAGES_STARTPAGE_H
#define GUI_PAGES_STARTPAGE_H

#include <QWidget>

class StartPage : public QWidget {
    Q_OBJECT

public:
    explicit StartPage(QWidget* parent = nullptr);

signals:
    void newGameRequested(int seed);
    void loadGameRequested(int slot);
};

#endif // GUI_PAGES_STARTPAGE_H
