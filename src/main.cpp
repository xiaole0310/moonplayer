#include <QApplication>
#include <QTranslator>
#include "settingsdialog.h"
#include "playlist.h"
#include "accessmanager.h"
#include "updatechecker.h"
#include <locale.h>
#include <QDir>
#include <QIcon>
#include <QLocale>
#include <QDebug>
#include <QSurfaceFormat>
#include <QTextCodec>
#include <QNetworkAccessManager>
#include <Python.h>
#include "pyapi.h"
#include "platforms.h"
#include "playerview.h"
#include "plugin.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_MAC
#include <QFileOpenEvent>
class MyApplication : public QApplication
{
public:
    MyApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
    }
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            QString file = openEvent->file();
            if (file.isEmpty()) //url
            {
                QUrl url = openEvent->url();
                if (url.scheme() == "moonplayer")
                    url.setScheme("http");
                else if (url.scheme() == "moonplayers")
                    url.setScheme("https");
                playlist->addUrl(url.toString());
            }
            else if (file.endsWith(".m3u") || file.endsWith(".m3u8") || file.endsWith(".xspf"))
                playlist->addList(file);
            else
                playlist->addFileAndPlay(file.section('/', -1), file);
        }
        return QApplication::event(event);
    }
};
#else
#include "localserver.h"
#include "localsocket.h"
#endif


int main(int argc, char *argv[])
{
    QDir currentDir = QDir::current();

    // OpenGL version >= 3.0 is required for hardware decoding
    QSurfaceFormat surface = QSurfaceFormat::defaultFormat();
    surface.setVersion(3, 2);
    surface.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(surface);

#ifdef Q_OS_MAC
    MyApplication a(argc, argv);
#else
    QApplication a(argc, argv);

    //check whether another MoonPlayer instance is running
    LocalSocket socket;
    if (socket.state() == LocalSocket::ConnectedState) //Is already running
    {
        if (argc == 1)
        {
            qDebug("Another instance is running. Quit.\n");
            return EXIT_FAILURE;
        }

        for (int i = 1; i < argc; i++)
        {
            QByteArray f = argv[i];
            //online resource
            if (f.startsWith("http://") || f.startsWith("https://"))
                socket.addUrl(f);

            //playlist
            else if (f.endsWith(".m3u") || f.endsWith("m3u8") || f.endsWith(".xspf"))
                socket.addList(f);

            //local videos
            else
            {
                if (!QDir::isAbsolutePath(f))
                    f = currentDir.filePath(QTextCodec::codecForLocale()->toUnicode(f)).toLocal8Bit();

                if (i == 1)   //first video
                    socket.addFileAndPlay(f);
                else
                    socket.addFile(f);
            }
        }
        return EXIT_SUCCESS;
    }

    // This is the first instance, create server
    LocalServer server;
#endif

#ifdef Q_OS_WIN
    // optimize font
    if (QLocale::system().country() == QLocale::China)
        QApplication::setFont(QFont("Microsoft Yahei", 9));
    // debug mode
    for (int i = 1; i < argc; i++)
    {
        if (QByteArray(argv[i]) == "--win-debug")
        {
            win_debug = AttachConsole(ATTACH_PARENT_PROCESS);
            if (win_debug)
            {
                freopen("CON", "w", stdout);
                freopen("CON", "w", stderr);
                freopen("CON", "r", stdin);
            }
            break;
        }
    }
#endif

    //for mpv
    setlocale(LC_NUMERIC, "C");

    //init
    access_manager = new QNetworkAccessManager(&a);
    printf("Initialize settings...\n");
    initSettings();

    printf("Initialize API for Python...\n");
    initAPI();
    initPlugins();

    //translate moonplayer
    printf("Initialize language support...\n");
    QDir path(getAppPath());
#ifdef Q_OS_MAC
    path.cd("translations");
    //translate application menu
    QTranslator qtTranslator;
    if (qtTranslator.load(path.filePath("qt_" + QLocale::system().name())))
        a.installTranslator(&qtTranslator);
#endif
    QTranslator translator;
    if (translator.load(path.filePath("moonplayer_" + QLocale::system().name())))
        a.installTranslator(&translator);

    PlayerView *player_view = new PlayerView;
    player_view->show();

    for (int i = 1; i < argc; i++)
    {
        QTextCodec* codec = QTextCodec::codecForLocale();
        QString file = codec->toUnicode(argv[i]);

        if (file.startsWith("--"))
            continue;
        if (file.startsWith("http://") || file.startsWith("https://"))
            playlist->addUrl(file);
        else if (file.endsWith(".m3u") || file.endsWith("m3u8") || file.endsWith(".xspf")) //playlist
            playlist->addList(file);
        else
        {
            if (!QDir::isAbsolutePath(file))    //not an absolute path
                file = currentDir.filePath(file);
            if (i == 1) //first video
                playlist->addFileAndPlay(file.section('/', -1), file);
            else
                playlist->addFile(file.section('/', -1), file);
        }
    }

    // Check new version
    UpdateChecker checker;
    checker.check();

    a.exec();
    Py_Finalize();
    delete player_view;
    return 0;
}
