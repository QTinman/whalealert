#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

extern QString appgroup;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
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

public slots:
    void replyFinished (QNetworkReply *reply);


private slots:
    void on_pushButton_clicked();
    void on_settings_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *manager;

};
#endif // MAINWINDOW_H
