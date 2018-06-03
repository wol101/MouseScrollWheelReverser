#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Settings.h"

#include <iostream>

#include <Windows.h>

// this provides a GUI for the powershell line
//
// Get-ItemProperty HKLM:\SYSTEM\CurrentControlSet\Enum\HID\*\*\Device` Parameters FlipFlopWheel -EA 0 | ForEach-Object { Set-ItemProperty $_.PSPath FlipFlopWheel 1 }
//

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/images/app_icon_32x32.png"));

    connect(ui->actionNormal, SIGNAL(triggered()), this, SLOT(normalWheel()));
    connect(ui->actionReverse, SIGNAL(triggered()), this, SLOT(reverseWheel()));

    getListOfKeys();
    ui->plainTextEdit->clear();
    for (int i = 0; i < m_registryKeys.size(); i++)
        ui->plainTextEdit->appendPlainText(QString("%1 FlipFlopWheel = %2").arg(m_registryKeys[i]).arg(m_registryKeyValues[i]));

    restoreGeometry(Settings::value("_MainWindowGeometry", QByteArray()).toByteArray());
    restoreState(Settings::value("_MainWindowState", QByteArray()).toByteArray());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Settings::setValue("_MainWindowGeometry", saveGeometry());
    Settings::setValue("_MainWindowState", saveState());
    event->accept();
}

void MainWindow::getListOfKeys()
{
    m_registryKeys.clear();
    m_registryKeyValues.clear();
    // open the base key
    std::wstring baseKey(L"SYSTEM\\CurrentControlSet\\Enum\\HID\\");
    HKEY hRegHID;
    LONG res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, baseKey.c_str(), 0, KEY_READ, &hRegHID);
    if (res != ERROR_SUCCESS) return;

    // enumerate the sub keys
    for (DWORD index1=0; ; index1++)
    {
        WCHAR subKeyName[255];
        DWORD cName = 255;
        res = RegEnumKeyExW(hRegHID, index1, subKeyName, &cName, NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS) break;
        // now enumerate these sub sub keys
        std::wstring newBaseKey = baseKey + std::wstring(subKeyName) + std::wstring(L"\\");
        HKEY hRegHIDSubKey;
        res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, newBaseKey.c_str(), 0, KEY_READ, &hRegHIDSubKey);
        if (res != ERROR_SUCCESS) break;
        for (DWORD index2=0; ; index2++)
        {
            WCHAR subSubKeyName[255];
            DWORD cName = 255;
            res = RegEnumKeyExW(hRegHIDSubKey, index2, subSubKeyName, &cName, NULL, NULL, NULL, NULL);
            if (res != ERROR_SUCCESS) break;
            DWORD value;
            DWORD lenValue = sizeof(value);
            std::wstring key = newBaseKey + std::wstring(subSubKeyName) + std::wstring(L"\\Device Parameters");
            qDebug("key = %s", qUtf8Printable(QString::fromStdWString(key)));
            res = RegGetValueW(HKEY_LOCAL_MACHINE, key.c_str(), L"FlipFlopWheel", RRF_RT_REG_DWORD, NULL, &value, &lenValue);
            if (res != ERROR_SUCCESS) break;
            qDebug("value = %04X", value);
            m_registryKeys.append(QString::fromStdWString(key));
            m_registryKeyValues.append(value);
        }
    }
}

void MainWindow::setKeys(DWORD value)
{
    for (int i = 0; i < m_registryKeys.size(); i++)
    {
        std::wstring key = m_registryKeys[i].toStdWString();
        DWORD lenValue = sizeof(value);
        LONG res = RegSetKeyValueW(HKEY_LOCAL_MACHINE, key.c_str(), L"FlipFlopWheel", REG_DWORD, &value, lenValue);
        if (res != ERROR_SUCCESS)
        {
            qDebug("trouble setting key = %s error code = %d", qUtf8Printable(QString::fromStdWString(key)), res);
            ui->statusBar->showMessage(QString("Error setting key = %1 error code = %2").arg(QString::fromStdWString(key)).arg(res), 5000);
        }
        else
        {
            qDebug("key = %s set to %d", qUtf8Printable(QString::fromStdWString(key)), value);
            ui->statusBar->showMessage(QString("Success: key = %1 set to = %2").arg(QString::fromStdWString(key)).arg(value), 5000);
        }
    }
}

void MainWindow::normalWheel()
{
    setKeys(0);
    getListOfKeys();
    ui->plainTextEdit->clear();
    for (int i = 0; i < m_registryKeys.size(); i++)
        ui->plainTextEdit->appendPlainText(QString("%1 FlipFlopWheel = %2").arg(m_registryKeys[i]).arg(m_registryKeyValues[i]));
}

void MainWindow::reverseWheel()
{
    setKeys(1);
    getListOfKeys();
    ui->plainTextEdit->clear();
    for (int i = 0; i < m_registryKeys.size(); i++)
        ui->plainTextEdit->appendPlainText(QString("%1 FlipFlopWheel = %2").arg(m_registryKeys[i]).arg(m_registryKeyValues[i]));
}


