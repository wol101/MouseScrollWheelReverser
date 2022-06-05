#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Windows.h>

#include <string>
#include <map>

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
    void about();

private:
    Ui::MainWindow *ui;

    void getListOfKeys();
    void setAllKeys(DWORD value);
    void setCheckedKeys(DWORD value);
    void getConnectedUSB();
    void getConnectedUSB(GUID *ClassGuid);
    int getConnectedPNP();
    void parseVIDandPID(const QString &string, int *vid, int *pid);
    void parseUSBIDs();
    void displayConnectedUSB();
    void displayConnectedPNP(const QString &filter);
    void displayListOfKeys();

    QStringList m_registryKeys;
    QList<DWORD> m_registryKeyValues;

    QStringList m_connectedUSB;
    std::map<int, std::string> m_vidMap;
    std::map<int, std::map<int, std::string>> m_pidMapMap;

    QStringList m_connectedPNPClass;
    QStringList m_connectedPNPFriendlyName;
    QStringList m_connectedPNPInstanceId;
    QStringList m_connectedPNPManufacturer;
};

#endif // MAINWINDOW_H
