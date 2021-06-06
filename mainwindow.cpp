#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./settings.h"
#include <QtCore>
#include <QtGui>
#include <QtSql>

double usd_to, usd_from, crypt_to, crypt_from;
int timer_minutes;
bool timer_enable;
QString appgroup="whalealert";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    timer_minutes = loadsettings("timer_minutes").toInt();
    timer_enable = loadsettings("timer_enable").toBool();
    if (timer_enable) {
        QTimer *timer = new QTimer(this);
        if (!timer->isActive()) {
            connect(timer, SIGNAL(timeout()), this, SLOT(on_pushButton_clicked()));
            timer->start(timer_minutes*60000);
        }
    }
    this->setWindowTitle("Whale Alert!!");
    on_pushButton_clicked();

}

MainWindow::~MainWindow()
{
    delete ui;
}

QVariant MainWindow::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void MainWindow::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

QJsonArray MainWindow::ReadJson(const QString &path)
{
    QFile file( path );
    QJsonArray jsonArray;
    if( file.open( QIODevice::ReadOnly ) )
    {
        QByteArray bytes = file.readAll();
        file.close();

        QJsonParseError parserError;
        QJsonDocument cryptolist = QJsonDocument::fromJson(bytes, &parserError);
        //qDebug() << parserError.errorString();

        QJsonObject jsonObject = cryptolist.object();
        jsonArray = jsonObject["transactions"].toArray();
        //if ((ui->maxcoins->text() == "" || maxcoins == 0) || maxcoins > jsonArray.count()) ui->maxcoins->setText(QString::number(jsonArray.count()));
        //qDebug() << "Number of cryptocurrencies " << jsonArray.count();

    }
    return jsonArray;
}

void MainWindow::GetJson()
{
    QStringList commandlist;
    QString api_key = loadsettings("api_key").toString();
    if (!api_key.isEmpty()) {
        commandlist.append("https://api.whale-alert.io/v1/transactions?api_key="+api_key);
        QProcess *myProcess = new QProcess(this);
        myProcess->setStandardOutputFile("whale_alerts.json");
        myProcess->start("curl",commandlist);
        //if (myProcess->exitCode() > 0) ui->messages->setText("Error executing cURL : " + myProcess->errorString() + " Exitcode: " + QString::number(myProcess->exitCode()));
        myProcess->waitForFinished(-1);
    } else ui->alert->setText("API key missing, insert API key in Settings.");
}


void MainWindow::process_json()
{
    QJsonArray jsonArray = ReadJson("whale_alerts.json");
    foreach (const QJsonValue & value, jsonArray) {

            QJsonObject transactions = value.toObject();
            int id = transactions["id"].toInt();

            QJsonObject from = transactions["from"].toObject();
            QJsonObject to = transactions["to"].toObject();
            QString owner_type_from = from["owner_type"].toString();
            QString owner_type_to = to["owner_type"].toString();
            double usd = transactions["amount_usd"].toDouble();
            QString symbol = transactions["symbol"].toString();
            if (owner_type_to == "exchange" && symbol == "usdt") usd_to+=usd;
            if (owner_type_to != "exchange" && symbol == "usdt") usd_from+=usd;
            if (owner_type_to == "exchange" && symbol != "usdt") crypt_to+=usd;
            if (owner_type_to != "exchange" && symbol != "usdt") crypt_from+=usd;
            //if (owner_type_from != "exchange" && owner_type_to != "exchange") qDebug() << owner_type_from << "  " << owner_type_to;
    }
}

void MainWindow::on_pushButton_clicked()
{
    int inflow_crypt, inflow_usdt, outflow_crypt, outflow_usdt;
    GetJson();
    process_json();
    QString q_usd_from = QLocale(QLocale::English).toString(usd_from,'F',0), q_usd_to = QLocale(QLocale::English).toString(usd_to,'F',0), q_crypt_from = QLocale(QLocale::English).toString(crypt_from,'F',0), q_crypt_to = QLocale(QLocale::English).toString(crypt_to,'F',0);
    ui->total_usdt_from->setText(q_usd_from);
    ui->tota_usdt_to->setText(q_usd_to);
    ui->total_crypt_to->setText(q_crypt_to);
    ui->total_crypt_from->setText(q_crypt_from);
    inflow_crypt=loadsettings("inflow_crypt").toInt();
    inflow_usdt=loadsettings("inflow_usdt").toInt();
    outflow_crypt=loadsettings("outflow_crypt").toInt();
    outflow_usdt=loadsettings("outflow_usdt").toInt();
    if (inflow_crypt < crypt_to) ui->alert->setText("Warning Cryptocurrency deposit into exchanges exeeds "+QLocale(QLocale::English).toString(inflow_crypt));
    if (inflow_usdt < usd_to) ui->alert_2->setText("Warning USDT deposit into exchanges exeeds "+QLocale(QLocale::English).toString(inflow_usdt));
    if (outflow_crypt < crypt_from) ui->alert_3->setText("Warning Cryptocurrency withdraw from exchanges exeeds "+QLocale(QLocale::English).toString(outflow_crypt));
    if (outflow_usdt < usd_from) ui->alert_4->setText("Warning USDT withdraw from exchanges exeeds "+QLocale(QLocale::English).toString(outflow_usdt));
}

void MainWindow::on_settings_clicked()
{
    Settings settingsdialog;
    //QObject::connect(&settingsdialog, SIGNAL(destroyed()), this, SLOT(reload_model()));
    settingsdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    settingsdialog.exec();
    timer_minutes = loadsettings("timer_minutes").toInt();
    timer_enable = loadsettings("timer_enable").toBool();
    if (timer_enable) {
        QTimer *timer = new QTimer(this);
        if (!timer->isActive()) {
            connect(timer, SIGNAL(timeout()), this, SLOT(on_pushButton_clicked()));
            timer->start(timer_minutes*60000);
        }
    }

}
