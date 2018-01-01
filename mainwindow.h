#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "filerecordqueue.h"
#include "processlistener.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
private:
    Ui::MainWindow *ui;
    ProcessListener* listener;
};

#endif // MAINWINDOW_H
