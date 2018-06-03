#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Windows.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void normalWheel();
    void reverseWheel();

private:
    Ui::MainWindow *ui;

    void getListOfKeys();
    void setKeys(DWORD value);

    QStringList m_registryKeys;
    QList<DWORD> m_registryKeyValues;
};

#endif // MAINWINDOW_H
