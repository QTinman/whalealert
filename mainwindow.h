#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QtSql/QSqlTableModel>
#include <QtSql/QSql>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDatabase>

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
    void GetJson();
    void process_json();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);

private slots:
    void on_pushButton_clicked();

    void on_settings_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
