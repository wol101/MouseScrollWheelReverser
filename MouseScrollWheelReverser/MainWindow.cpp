#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "Settings.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QCloseEvent>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <iostream>

#include <Windows.h>
#include <setupapi.h>
#include <initguid.h>

// this provides a GUI for the powershell line (but with option to select which devices and a bit of feedback)
//
// Get-ItemProperty HKLM:\SYSTEM\CurrentControlSet\Enum\HID\*\*\Device` Parameters FlipFlopWheel -EA 0 | ForEach-Object { Set-ItemProperty $_.PSPath FlipFlopWheel 1 }
//

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/images/app_icon.svg"));
    ui->mainToolBar->setIconSize(QSize(48, 48));

    connect(ui->actionNormal, SIGNAL(triggered()), this, SLOT(normalWheel()));
    connect(ui->actionReverse, SIGNAL(triggered()), this, SLOT(reverseWheel()));

    QAction *exitAct = new QAction(QIcon(":/images/exit.svg"), tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &MainWindow::close);
    QMenu *fileMenu = ui->menuBar->addMenu(tr("&File"));
    fileMenu->addAction(exitAct);

    QAction *aboutAct = new QAction(QIcon(":/images/app_icon.svg"), tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);
    QMenu *helpMenu = ui->menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);

    parseUSBIDs();
    if (getConnectedPNP() == 0)
    {
        displayConnectedPNP("USB");
    }
    else // fall back to the C version (which misses things)
    {
        getConnectedUSB();
        displayConnectedUSB();
    }

    getListOfKeys();
    displayListOfKeys();

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
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, baseKey.c_str(), 0, KEY_READ, &hRegHID);
    if (res != ERROR_SUCCESS) return;

    // enumerate the sub keys
    for (DWORD index1=0; ; index1++)
    {
        WCHAR subKeyName[255];
        DWORD cName = 255;
        res = RegEnumKeyEx(hRegHID, index1, subKeyName, &cName, NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS) break;
        // now enumerate these sub sub keys
        std::wstring newBaseKey = baseKey + std::wstring(subKeyName) + std::wstring(L"\\");
        HKEY hRegHIDSubKey;
        res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, newBaseKey.c_str(), 0, KEY_READ, &hRegHIDSubKey);
        if (res != ERROR_SUCCESS) break;
        for (DWORD index2=0; ; index2++)
        {
            WCHAR subSubKeyName[255];
            DWORD cName = 255;
            res = RegEnumKeyEx(hRegHIDSubKey, index2, subSubKeyName, &cName, NULL, NULL, NULL, NULL);
            if (res != ERROR_SUCCESS) break;
            DWORD value;
            DWORD lenValue = sizeof(value);
            std::wstring key = newBaseKey + std::wstring(subSubKeyName) + std::wstring(L"\\Device Parameters");
            // key will look something like
            // "SYSTEM\CurrentControlSet\Enum\HID\VID_045E&PID_07B2&MI_00\8&34bbdf7c&0&0000\Device Parameters"
            qDebug("key = %s", qUtf8Printable(QString::fromStdWString(key)));
            res = RegGetValue(HKEY_LOCAL_MACHINE, key.c_str(), L"FlipFlopWheel", RRF_RT_REG_DWORD, NULL, &value, &lenValue);
            if (res != ERROR_SUCCESS) break;
            qDebug("value = %04X", value);
            m_registryKeys.append(QString::fromStdWString(key));
            m_registryKeyValues.append(value);
        }
    }
}

void MainWindow::setAllKeys(DWORD value)
{
    for (int i = 0; i < m_registryKeys.size(); i++)
    {
        std::wstring key = m_registryKeys[i].toStdWString();
        DWORD lenValue = sizeof(value);
        LONG res = RegSetKeyValue(HKEY_LOCAL_MACHINE, key.c_str(), L"FlipFlopWheel", REG_DWORD, &value, lenValue);
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

void MainWindow::setCheckedKeys(DWORD value)
{
    for (int i = 0; i < m_registryKeys.size(); i++)
    {
        if (ui->tableWidgetScrollEntries->item(i, 0)->checkState() == Qt::Checked)
        {
            std::wstring key = m_registryKeys[i].toStdWString();
            DWORD lenValue = sizeof(value);
            LONG res = RegSetKeyValue(HKEY_LOCAL_MACHINE, key.c_str(), L"FlipFlopWheel", REG_DWORD, &value, lenValue);
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
}

void MainWindow::normalWheel()
{
    setCheckedKeys(0);
    getListOfKeys();
    displayListOfKeys();
}

void MainWindow::reverseWheel()
{
    setCheckedKeys(1);
    getListOfKeys();
    displayListOfKeys();
}

void MainWindow::getConnectedUSB()
{
    GUID GUID_DEVINTERFACE_USB_DEVICE = { 0xA5DCBF10L, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED} };
    GUID GUID_DEVINTERFACE_USB_HUB = { 0xf18a0e88, 0xc30c, 0x11d0, {0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8} };
    GUID GUID_DEVINTERFACE_USB_HOST_CONTROLLER = { 0x3abf6f2d, 0x71c4, 0x462a, {0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27} };
    getConnectedUSB(&GUID_DEVINTERFACE_USB_DEVICE);
    getConnectedUSB(&GUID_DEVINTERFACE_USB_HUB);
    getConnectedUSB(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER);
}

void MainWindow::getConnectedUSB(GUID *ClassGuid)
{
    HDEVINFO                         hDevInfo;
    SP_DEVICE_INTERFACE_DATA         DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA                  DevData;

    DWORD dwMemberIdx = 0;
    DWORD dwSize;

    // We will try to get device information set for all USB devices that have a
    // device interface and are currently present on the system (plugged in).
    hDevInfo = SetupDiGetClassDevs(ClassGuid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        // Prepare to enumerate all device interfaces for the device information
        // set that we retrieved with SetupDiGetClassDevs(..)
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        // Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
        // function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
        // call the dwMemberIdx value needs to be incremented to retrieve the next
        // device interface information.

        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, ClassGuid, dwMemberIdx, &DevIntfData);

        while(GetLastError() != ERROR_NO_MORE_ITEMS)
        {
            // As a last step we will need to get some more details for each
            // of device interface information we are able to retrieve. This
            // device interface detail gives us the information we need to identify
            // the device (VID/PID), and decide if it's useful to us. It will also
            // provide a DEVINFO_DATA structure which we can use to know the serial
            // port name for a virtual com port.
            DevData.cbSize = sizeof(DevData);

            // Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
            // a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
            // of zero, and a valid RequiredSize variable. In response to such a call,
            // this function returns the required buffer size at dwSize.
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

            // Allocate memory for the DeviceInterfaceDetail struct.
            DevIntfDetailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize));
            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData))
            {
                // Finally we can start checking if we've found a useable device,
                // by inspecting the DevIntfDetailData->DevicePath variable.
                // The DevicePath looks something like this:
                // "\\\\?\\usb#vid_0b05&pid_1939#9876543210#{a5dcbf10-6530-11d2-901f-00c04fb951ed}"
                QString devicePath = QString::fromWCharArray(DevIntfDetailData->DevicePath);
                qDebug() << devicePath;
                if (devicePath.startsWith("\\\\?\\usb#vid"))
                        m_connectedUSB.push_back(devicePath);
            }

            HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

            // Continue looping
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, ClassGuid, ++dwMemberIdx, &DevIntfData);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
}

void MainWindow::parseVIDandPID(const QString &string, int *vid, int *pid)
{
    bool ok;
    int index = string.indexOf("VID_", 0, Qt::CaseInsensitive);
    if (index == -1) { *vid = -1; }
    else
    {
        *vid = string.mid(index + 4, 4).toInt(&ok, 16);
        if (!ok) *vid = -1;
    }
    index = string.indexOf("PID_", 0, Qt::CaseInsensitive);
    if (index == -1) { *pid = -1; }
    else
    {
        *pid = string.mid(index + 4, 4).toInt(&ok, 16);
        if (!ok) *pid = -1;
    }
}

void MainWindow::parseUSBIDs()
{
    bool ok;
    static QRegularExpression hexMatcher("^[0-9A-F]+$", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match;
    QFile usbIDsFile(":/usb.ids");
    usbIDsFile.open(QIODevice::ReadOnly);
    QStringList lines = QString::fromUtf8(usbIDsFile.readAll()).split('\n');
    usbIDsFile.close();
    for (int i = 0; i < lines.size(); i++)
    {
        QString &line = lines[i];
        if (line.startsWith('#') || line.size() < 4) continue;
        QString left4 = line.left(4);
//        if (left4 == "046c")
//        {
//            qDebug() << left4;
//        }
        match = hexMatcher.match(left4);
        if (!match.hasMatch()) continue;
        int vid = left4.toInt(&ok, 16);
        m_vidMap[vid] = line.mid(5).trimmed().toStdString();
        if (++i > lines.size()) break;
        std::map<int, std::string> vidMap;
        for (; i < lines.size(); i++)
        {
            line = lines[i];
            if (line.startsWith('#') || line.size() < 5) continue;
            if (!line.startsWith('\t')) { i--; break; }
            QString afterTab = line.mid(1, 4);
            match = hexMatcher.match(afterTab);
            if (!match.hasMatch()) break;
            vidMap[afterTab.toInt(&ok, 16)] = line.mid(5).trimmed().toStdString();
        }
        m_pidMapMap[vid] = std::move(vidMap);
    }
}

void MainWindow::displayConnectedUSB()
{
    ui->tableWidgetConnectedDeviced->setColumnCount(4);
    QStringList labels;
    labels << "VID" << "Vendor" << "PID" << "Part";
    ui->tableWidgetConnectedDeviced->setHorizontalHeaderLabels(labels);
    ui->tableWidgetConnectedDeviced->setRowCount(m_connectedUSB.size());
    int vid, pid;
    QTableWidgetItem *cell;
    for (int i = 0; i < m_connectedUSB.size(); i++)
    {
        qDebug() << m_connectedUSB[i];
        parseVIDandPID(m_connectedUSB[i], &vid, &pid);
        auto vidIt = m_vidMap.find(vid);
        auto pidMapIt = m_pidMapMap.find(vid);
        if (vidIt != m_vidMap.end())
        {
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(vid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 0, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString::fromStdString(vidIt->second));
            ui->tableWidgetConnectedDeviced->setItem(i, 1, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(pid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 2, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            auto pidIt = pidMapIt->second.find(pid);
            if (pidIt != pidMapIt->second.end()) { cell->setText(QString::fromStdString(pidIt->second)); }
            else { cell->setText(QString("Unknown")); }
            ui->tableWidgetConnectedDeviced->setItem(i, 3, cell);
        }
    }
    ui->tableWidgetConnectedDeviced->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetConnectedDeviced->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::displayConnectedPNP(const QString &filter)
{
    // start but doing the filter
    int vid, pid;
    QVector<int> indexList;
    for (int i = 0; i < m_connectedPNPInstanceId.size(); i++)
    {
        if (filter.size() == 0 || m_connectedPNPInstanceId[i].startsWith(filter, Qt::CaseInsensitive))
        {
            parseVIDandPID(m_connectedPNPInstanceId[i], &vid, &pid);
            if (vid != -1) indexList.push_back(i);
        }
    }
    ui->tableWidgetConnectedDeviced->setColumnCount(5);
    QStringList labels;
    labels << "VID" << "Vendor" << "PID" << "Part" << "FriendlyName";
    ui->tableWidgetConnectedDeviced->setHorizontalHeaderLabels(labels);
    ui->tableWidgetConnectedDeviced->setRowCount(indexList.size());
    QTableWidgetItem *cell;
    for (int i = 0; i < indexList.size(); i++)
    {
        qDebug() << m_connectedPNPInstanceId[indexList[i]];
        parseVIDandPID(m_connectedPNPInstanceId[indexList[i]], &vid, &pid);
        auto vidIt = m_vidMap.find(vid);
        auto pidMapIt = m_pidMapMap.find(vid);
        if (vidIt != m_vidMap.end())
        {
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(vid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 0, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString::fromStdString(vidIt->second));
            ui->tableWidgetConnectedDeviced->setItem(i, 1, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(pid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 2, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            auto pidIt = pidMapIt->second.find(pid);
            if (pidIt != pidMapIt->second.end()) { cell->setText(QString::fromStdString(pidIt->second)); }
            else { cell->setText(""); }
            ui->tableWidgetConnectedDeviced->setItem(i, 3, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(m_connectedPNPFriendlyName[indexList[i]]);
            ui->tableWidgetConnectedDeviced->setItem(i, 4, cell);
        }
        else
        {
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(vid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 0, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(m_connectedPNPManufacturer[indexList[i]]);
            ui->tableWidgetConnectedDeviced->setItem(i, 1, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(pid, 4, 16, QChar('0')));
            ui->tableWidgetConnectedDeviced->setItem(i, 2, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText("");
            ui->tableWidgetConnectedDeviced->setItem(i, 3, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(m_connectedPNPFriendlyName[indexList[i]]);
            ui->tableWidgetConnectedDeviced->setItem(i, 4, cell);
        }
    }
    ui->tableWidgetConnectedDeviced->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetConnectedDeviced->horizontalHeader()->setStretchLastSection(true);
}


void MainWindow::displayListOfKeys()
{
    QIcon normal(":/images/norm_icon.svg");
    QIcon reverse(":/images/rev_icon.svg");
    ui->tableWidgetScrollEntries->setColumnCount(6);
    QStringList labels;
    labels << "" << "" << "VID" << "Vendor" << "PID" << "Part";
    ui->tableWidgetScrollEntries->setHorizontalHeaderLabels(labels);
    ui->tableWidgetScrollEntries->setRowCount(m_registryKeys.size());
    int vid, pid;
    QTableWidgetItem *cell;
    for (int i = 0; i < m_registryKeys.size(); i++)
    {
        parseVIDandPID(m_registryKeys[i], &vid, &pid);
        auto vidIt = m_vidMap.find(vid);
        auto pidMapIt = m_pidMapMap.find(vid);
        if (vidIt != m_vidMap.end())
        {
            cell = ui->tableWidgetScrollEntries->item(i, 0); // necessary because otherwise we loose the selection
            if (!cell)
            {
                cell = new QTableWidgetItem();
                cell->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                cell->setCheckState(Qt::Unchecked);
                ui->tableWidgetScrollEntries->setItem(i, 0, cell);
            }
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsEnabled);
            if (m_registryKeyValues[i]) cell->setIcon(reverse);
            else cell->setIcon(normal);
            ui->tableWidgetScrollEntries->setItem(i, 1, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(vid, 4, 16, QChar('0')));
            ui->tableWidgetScrollEntries->setItem(i, 2, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString::fromStdString(vidIt->second));
            ui->tableWidgetScrollEntries->setItem(i, 3, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            cell->setText(QString("%1").arg(pid, 4, 16, QChar('0')));
            ui->tableWidgetScrollEntries->setItem(i, 4, cell);
            cell = new QTableWidgetItem();
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            auto pidIt = pidMapIt->second.find(pid);
            if (pidIt != pidMapIt->second.end()) { cell->setText(QString::fromStdString(pidIt->second)); }
            else { cell->setText(QString("Unknown")); }
            ui->tableWidgetScrollEntries->setItem(i, 5, cell);
        }
    }
    ui->tableWidgetScrollEntries->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableWidgetScrollEntries->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::about()
{
    QMessageBox msgBox;
    msgBox.setText(tr("<b>MouseScrollWheelReverser</b>"));
    msgBox.setInformativeText(tr("This application edits the registry to allow the direction of the scroll wheel to be reversed.\nA restart is required for it to take effect\nCopyright William Sellers 2022.\nReleased under GPL v3."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.setIconPixmap(QIcon(":/images/app_icon.svg").pixmap(QSize(256, 256))); // going via a QIcon means the scaling happens before the conversion to a pixmap
    int ret = msgBox.exec();
    switch (ret)
    {
    case QMessageBox::Ok:
        // Ok was clicked
        break;
    default:
        // should never be reached
        break;
    }
}

int MainWindow::getConnectedPNP()
{
    QProcess process;
    QString program = "PowerShell";
    QStringList arguments;
    // PowerShell -Command "& {Get-PnpDevice | Select-Object Status,Class,FriendlyName,InstanceId,Manufacturer,Present | ConvertTo-Json}"
    arguments << "-Command" << "& {Get-PnpDevice | Select-Object Status,Class,FriendlyName,InstanceId,Manufacturer,Present | ConvertTo-Json}";
    process.start(program, arguments);
    if (!process.waitForStarted()) return __LINE__;
    if (!process.waitForFinished()) return __LINE__;
    QByteArray result = process.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(result);
    if (jsonDoc.isNull()) return __LINE__;
    if (!jsonDoc.isArray()) return __LINE__;
    QJsonArray jsonArray = jsonDoc.array();
    for (auto &&it : jsonArray)
    {
        if (!it.isObject()) continue;
        QJsonObject obj = it.toObject();
        if (obj["Status"].toString() != "OK") continue;
        if (obj["Present"].toBool() != true) continue;
        m_connectedPNPClass.push_back(obj["Class"].toString());
        m_connectedPNPFriendlyName.push_back(obj["FriendlyName"].toString());
        m_connectedPNPInstanceId.push_back(obj["InstanceId"].toString());
        m_connectedPNPManufacturer.push_back(obj["Manufacturer"].toString());
    }
    if (m_connectedPNPClass.size() == 0) return __LINE__;
    return 0;
}



