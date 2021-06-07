#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>

extern QString appgroup;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
class QFile;
class CurlEasy;
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QJsonArray ReadJson(const QString &path);
    void Calc_json();
    void process_json();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    void rapport();

private slots:
    void on_pushButton_clicked();
    void onTransferProgress(qint64 downloadTotal, qint64 downloadNow, qint64 uploadTotal, qint64 uploadNow);
    void onTransferDone();
    void onTransferAborted();
    void on_settings_clicked();

private:
    Ui::MainWindow *ui;
    void log(QString text);
    CurlEasy *transfer = nullptr;
    QFile *downloadFile = nullptr;
};
#endif // MAINWINDOW_H
