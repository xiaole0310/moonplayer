#ifndef DANMAKUDELAYGETTER_H
#define DANMAKUDELAYGETTER_H

#include <QObject>
#include <QStringList>
class QProcess;

// Get the danmaku delay for videos which is cut into clips so that danmaku can patch all clips
// After finished, the getter object will delete itself

class DanmakuDelayGetter : public QObject
{
    Q_OBJECT
public:
    DanmakuDelayGetter(QStringList &names, QStringList &urls, const QString &danmakuUrl, QObject *parent = 0);
signals:
    void newPlay(const QString &name, const QString &url, const QString &danmakuUrl);
    void newFile(const QString &name, const QString &url, const QString &danmakuUrl);
private:
    void start(void);
    QProcess *process;
    QStringList names;
    QStringList urls;
    QString danmakuUrl;
    double delay;
private slots:
    void onFinished(void);
};

#endif // DANMAKUDELAYGETTER_H
