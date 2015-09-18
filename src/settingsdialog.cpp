#include "settingsdialog.h"
#include "settings_network.h"
#include "settings_player.h"
#include "settings_plugins.h"
#include "settings_video.h"
#include "settings_danmaku.h"
#include "ui_settingsdialog.h"
#include "accessmanager.h"
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QDir>
#include <QSettings>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include "settings_audio.h"
#include "plugins.h"

QString Settings::aout;
QString Settings::vout;
QString Settings::path;
QString Settings::proxy;
QString Settings::downloadDir;
QString Settings::danmakuFont;
QStringList Settings::skinList;
int Settings::currentSkin;
int Settings::port;
int Settings::cacheSize;
int Settings::cacheMin;
int Settings::maxTasks;
int Settings::volume;
int Settings::danmakuSize;
int Settings::durationScrolling;
int Settings::durationStill;
bool Settings::framedrop;
bool Settings::doubleBuffer;
bool Settings::fixLastFrame;
bool Settings::ffodivxvdpau;
bool Settings::autoResize;
bool Settings::enableScreenshot;
bool Settings::softvol;
bool Settings::rememberUnfinished;
bool Settings::autoCombine;
bool Settings::autoCloseWindow;
double Settings::danmakuAlpha;
enum Settings::Quality Settings::quality;

using namespace Settings;

//Show settings dialog
SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    connect(this, SIGNAL(accepted()), this, SLOT(saveSettings()));
    connect(this, SIGNAL(rejected()), this, SLOT(loadSettings()));
    connect(ui->dirButton, SIGNAL(clicked()), this, SLOT(onDirButton()));
    connect(ui->fontPushButton, &QPushButton::clicked, this, &SettingsDialog::onFontButton);
    connect(ui->viewPluginsButton, SIGNAL(clicked()), this, SLOT(showPluginsMsg()));
    connect(ui->combineCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkFFMPEG(bool)));

    group = new QButtonGroup(this);
    group->addButton(ui->normalRadioButton, 0);
    group->addButton(ui->highRadioButton, 1);
    group->addButton(ui->superRadioButton, 2);
    ui->skinComboBox->addItems(skinList);

#ifdef Q_OS_LINUX
    ui->aoComboBox->addItem("pulse");
    ui->aoComboBox->addItem("alsa");
    ui->aoComboBox->addItem("oss");
#else
	ui->danmakuTab->setEnabled(false);
#endif

    loadSettings();
}

//Load settings
void SettingsDialog::loadSettings()
{
    ui->voComboBox->setCurrentIndex(ui->voComboBox->findText(vout));
    ui->aoComboBox->setCurrentIndex(ui->aoComboBox->findText(aout));
    ui->skinComboBox->setCurrentIndex(currentSkin);
    ui->proxyEdit->setText(proxy);
    ui->portEdit->setText(QString::number(port));
    ui->cacheSpinBox->setValue(cacheSize);
    ui->cacheMinSpinBox->setValue(cacheMin);
    ui->maxTaskSpinBox->setValue(maxTasks);
    ui->dirButton->setText(downloadDir);
    ui->dropCheckBox->setChecked(framedrop);
    ui->doubleCheckBox->setChecked(doubleBuffer);
    ui->ffodivxvdpauCheckBox->setChecked(ffodivxvdpau);
    ui->fixCheckBox->setChecked(fixLastFrame);
    ui->resizeCheckBox->setChecked(autoResize);
    ui->screenshotCheckBox->setChecked(enableScreenshot);
    ui->softvolCheckBox->setChecked(softvol);
    ui->rememberCheckBox->setChecked(rememberUnfinished);
    ui->combineCheckBox->setChecked(autoCombine);
    ui->autoCloseWindowCheckBox->setChecked(autoCloseWindow);

    ui->alphaDoubleSpinBox->setValue(danmakuAlpha);
    ui->fontPushButton->setText(danmakuFont);
    ui->fontSizeSpinBox->setValue(danmakuSize);
    ui->dmSpinBox->setValue(durationScrolling);
    ui->dsSpinBox->setValue(durationStill);

    switch (quality)
    {
    case NORMAL: ui->normalRadioButton->setChecked(true);break;
    case HIGH:   ui->highRadioButton->setChecked(true);  break;
    default:     ui->superRadioButton->setChecked(true); break;
    }
}

void SettingsDialog::onDirButton()
{
    QString dir = QFileDialog::getExistingDirectory(this);
    if (!dir.isEmpty())
        ui->dirButton->setText(dir);
}

void SettingsDialog::onFontButton()
{
    if (ui->fontPushButton->text().isEmpty())
    {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, this);
        if (ok)
            ui->fontPushButton->setText(font.key().section(',', 0, 0));
    }
    else
        ui->fontPushButton->setText("");
}

//Save settings
void SettingsDialog::saveSettings()
{
    vout = ui->voComboBox->currentText();
    proxy = ui->proxyEdit->text().simplified();
    port = ui->portEdit->text().toInt();
    cacheSize = ui->cacheSpinBox->value();
    cacheMin = ui->cacheMinSpinBox->value();
    maxTasks = ui->maxTaskSpinBox->value();
    downloadDir = ui->dirButton->text();
    framedrop = ui->dropCheckBox->isChecked();
    doubleBuffer = ui->doubleCheckBox->isChecked();
    ffodivxvdpau = ui->ffodivxvdpauCheckBox->isChecked();
    fixLastFrame = ui->fixCheckBox->isChecked();
    softvol = ui->softvolCheckBox->isChecked();
    quality = (enum Quality) group->checkedId();
    currentSkin = ui->skinComboBox->currentIndex();
    autoResize = ui->resizeCheckBox->isChecked();
    enableScreenshot = ui->screenshotCheckBox->isChecked();
    rememberUnfinished = ui->rememberCheckBox->isChecked();
    autoCombine = ui->combineCheckBox->isChecked();
    autoCloseWindow = ui->autoCloseWindowCheckBox->isChecked();

    danmakuAlpha = ui->alphaDoubleSpinBox->value();
    danmakuFont = ui->fontPushButton->text();
    danmakuSize = ui->fontSizeSpinBox->value();
    durationScrolling = ui->dmSpinBox->value();
    durationStill = ui->dsSpinBox->value();

    if (proxy.isEmpty())
        access_manager->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    else
        access_manager->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxy, port));
}


SettingsDialog::~SettingsDialog()
{
    //open file
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\moonplayer", QSettings::NativeFormat);
#else
    QSettings settings(QDir::homePath() + "/.config/moonplayer.ini", QSettings::IniFormat);
#endif
    settings.setValue("Player/current_skin", currentSkin);
    settings.setValue("Player/auto_resize", autoResize);
    settings.setValue("Player/screenshot", enableScreenshot);
    settings.setValue("Player/remember_unfinished", rememberUnfinished);
    settings.setValue("Video/out", vout);
    settings.setValue("Video/framedrop", framedrop);
    settings.setValue("Video/double", doubleBuffer);
    settings.setValue("Video/fixlastframe", fixLastFrame);
    settings.setValue("Video/ffodivxvdpau", ffodivxvdpau);
    settings.setValue("Audio/out", aout);
    settings.setValue("Audio/softvol", softvol);
    settings.setValue("Audio/volume", volume);
    settings.setValue("Net/proxy", proxy);
    settings.setValue("Net/port", port);
    settings.setValue("Net/cache_size", cacheSize);
    settings.setValue("Net/cache_min", cacheMin);
    settings.setValue("Net/max_tasks", maxTasks);
    settings.setValue("Net/download_dir", downloadDir);
    settings.setValue("Plugins/quality", (int) quality);
    settings.setValue("Plugins/auto_combine", autoCombine);
    settings.setValue("Plugins/auto_close_window", autoCloseWindow);
    settings.setValue("Danmaku/alpha", danmakuAlpha);
    settings.setValue("Danmaku/font", danmakuFont);
    settings.setValue("Danmaku/size", danmakuSize);
    settings.setValue("Danmaku/dm", durationScrolling);
    settings.setValue("Danmaku/ds", durationStill);
    delete ui;
}

//Init settings
void initSettings()
{
    //open file
#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\moonplayer", QSettings::NativeFormat);
#else
    QDir dir = QDir::home();
    if (!dir.cd(".moonplayer"))
    {
        dir.mkdir(".moonplayer");
        dir.cd(".moonplayer");
    }
    if (!dir.exists("plugins"))
        dir.mkdir("plugins");
    if (!dir.exists("skins"))
        dir.mkdir("skins");

    QSettings settings(QDir::homePath() + "/.config/moonplayer.ini", QSettings::IniFormat);
#endif

    //read settings
#ifdef Q_OS_WIN
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA)
        vout = settings.value("Video/out", "direct3d").toString();
    else
        vout = settings.value("Video/out", "directx").toString();
#else
    vout = settings.value("Video/out", "xv").toString();
    path = "/usr/share/moonplayer";
#endif
    framedrop = settings.value("Video/framedrop", true).toBool();
    doubleBuffer = settings.value("Video/double", true).toBool();
    fixLastFrame = settings.value("Video/fixlastframe", false).toBool();
    ffodivxvdpau = settings.value("Video/ffodivxvdpau", true).toBool();
    aout = settings.value("Audio/out", "auto").toString();
    softvol = settings.value("Audio/softvol", false).toBool();
    volume = settings.value("Audio/volume", 10).toInt();
    currentSkin = settings.value("Player/current_skin", 0).toInt();
    autoResize = settings.value("Player/auto_resize", true).toBool();
    enableScreenshot = settings.value("Player/screenshot", true).toBool();
    rememberUnfinished = settings.value("Player/remember_unfinished", true).toBool();
    proxy = settings.value("Net/proxy").toString();
    port = settings.value("Net/port").toInt();
    cacheSize = settings.value("Net/cache_size", 4096).toInt();
    cacheMin = settings.value("Net/cache_min", 50).toInt();
    maxTasks = settings.value("Net/max_tasks", 3).toInt();
    downloadDir = settings.value("Net/download_dir", QDir::homePath()).toString();
    quality = (Quality) settings.value("Plugins/quality", (int) SUPER).toInt();
    autoCombine = settings.value("Plugins/auto_combine", false).toBool();
    autoCloseWindow = settings.value("Plugins/auto_close_window", true).toBool();
    danmakuAlpha = settings.value("Danmaku/alpha", 0.9).toDouble();
    danmakuFont = settings.value("Danmaku/font", "").toString();
    danmakuSize = settings.value("Danmaku/size", 0).toInt();
    durationScrolling = settings.value("Danmaku/dm", 0).toInt();
    durationStill = settings.value("Danmaku/ds", 6).toInt();

    //init proxy
    if (proxy.isEmpty())
        access_manager->setProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    else
        access_manager->setProxy(QNetworkProxy(QNetworkProxy::HttpProxy, proxy, port));

    //init skins
    QDir skinDir(path);
    skinDir.cd("skins");
    skinList = skinDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
#ifdef Q_OS_LINUX
    dir.cd("skins");
    skinList.append(dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name));
#endif
    if (currentSkin >= skinList.size())
        currentSkin = 0;
}

void SettingsDialog::showPluginsMsg()
{
    QMessageBox::information(this, "Plugins", plugins_msg);
}

void SettingsDialog::checkFFMPEG(bool toggled)
{
    if (toggled)
    {
#ifdef Q_OS_WIN
        QDir dir(Settings::path);
#else
        if (QDir("/usr/share/moonplayer").exists("ffmpeg"))
            return;
        QDir dir = QDir::home();
        dir.cd(".moonplayer");

#endif
        if (!dir.exists("ffmpeg"))
        {
            QMessageBox::warning(this, "Error", tr("FFMPEG is not installed. Please download it from") +
                                 "\n    http://johnvansickle.com/ffmpeg/\n" +
                                tr("and place file \"ffmpeg\" into ~/.moonplayer/ or /usr/share/moonplayer/"));
            ui->combineCheckBox->setChecked(false);
        }
    }
}
