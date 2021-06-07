#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./settings.h"
#include "CurlEasy.h"
#include <QtCore>
#include <QtGui>
#include <QFile>

double usd_to, usd_from, crypt_to, crypt_from;
int alert1=0,alert2=0,alert3=0,alert4=0;
int timer_minutes;
bool timer_enable;
QString appgroup="whalealert";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->alert1->clear();
    ui->alert2->clear();
    ui->alert3->clear();
    ui->alert4->clear();
    transfer = new CurlEasy(this); // Parent it so it will be destroyed automatically

    connect(transfer, &CurlEasy::done, this, &MainWindow::onTransferDone);
    connect(transfer, &CurlEasy::aborted, this, &MainWindow::onTransferAborted);
    connect(transfer, &CurlEasy::progress, this, &MainWindow::onTransferProgress);
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

    }
    return jsonArray;
}

void MainWindow::on_pushButton_clicked()
{
    ui->transferLog->clear();
    ui->progressBar->setValue(0);
    QString api_key = loadsettings("api_key").toString();
    // Prepare target file
    downloadFile = new QFile("whale_alerts.json");
    if (!downloadFile->open(QIODevice::WriteOnly)) {
        log("Failed to open file for writing.");
        delete downloadFile;
        downloadFile = nullptr;
        return;
    }
    // Set a simple file writing function
    transfer->setWriteFunction([this](char *data, size_t size)->size_t {
        qint64 bytesWritten = downloadFile->write(data, static_cast<qint64>(size));
        return static_cast<size_t>(bytesWritten);
    });
    // Print headers to the transfer log box
    transfer->setHeaderFunction([this](char *data, size_t size)->size_t {
        log(QString::fromUtf8(data, static_cast<int>(size)));
        return size;
    });
    transfer->set(CURLOPT_URL, QUrl("https://api.whale-alert.io/v1/transactions?api_key="+api_key));
    transfer->set(CURLOPT_FOLLOWLOCATION, long(1)); // Follow redirects
    transfer->set(CURLOPT_FAILONERROR, long(1)); // Do not return CURL_OK in case valid server responses reporting errors.


    log("Transfer started.");

    transfer->perform();
}


void MainWindow::process_json()
{
    crypt_to=0;usd_to=0;crypt_from=0;usd_from=0;
    QJsonArray jsonArray = ReadJson("whale_alerts.json");
    foreach (const QJsonValue & value, jsonArray) {
        QJsonObject transactions = value.toObject();
        QJsonObject from = transactions["from"].toObject();
        QJsonObject to = transactions["to"].toObject();
        QString owner_type_from = from["owner_type"].toString();
        QString owner_type_to = to["owner_type"].toString();
        double usd = transactions["amount_usd"].toDouble();
        QString symbol = transactions["symbol"].toString();
        QString transaction_type = transactions["transaction_type"].toString();
        if (owner_type_to == "exchange" && !symbol.contains("usd") && transaction_type == "transfer") crypt_to+=usd;
        if (owner_type_to == "exchange" && symbol.contains("usd") && transaction_type == "transfer") usd_to+=usd;
        if (owner_type_to != "exchange" && !symbol.contains("usd") && transaction_type == "transfer") crypt_from+=usd;
        if (owner_type_to != "exchange" && symbol.contains("usd") && transaction_type == "transfer") usd_from+=usd;
        //if (owner_type_from != "exchange" && owner_type_to != "exchange") qDebug() << owner_type_from << "  " << owner_type_to;
    }
    Calc_json();
}

void MainWindow::Calc_json()
{
    qlonglong inflow_crypt=0, inflow_usdt=0, outflow_crypt=0, outflow_usdt=0;
    QString q_usd_from = QLocale(QLocale::English).toString(usd_from,'F',0), q_usd_to = QLocale(QLocale::English).toString(usd_to,'F',0), q_crypt_from = QLocale(QLocale::English).toString(crypt_from,'F',0), q_crypt_to = QLocale(QLocale::English).toString(crypt_to,'F',0);
    ui->total_usdt_from->setText(q_usd_from);
    ui->tota_usdt_to->setText(q_usd_to);
    ui->total_crypt_to->setText(q_crypt_to);
    ui->total_crypt_from->setText(q_crypt_from);
    inflow_crypt=loadsettings("inflow_crypt").toInt();
    inflow_usdt=loadsettings("inflow_usdt").toInt();
    outflow_crypt=loadsettings("outflow_crypt").toInt();
    outflow_usdt=loadsettings("outflow_usdt").toInt();
    inflow_crypt *= 1000000;
    inflow_usdt *= 1000000;
    outflow_crypt *= 1000000;
    outflow_usdt *= 1000000;
    if (inflow_crypt < crypt_to) {
        //ui->alert->setText("Warning Cryptocurrency deposit into exchanges exeeds "+QLocale(QLocale::English).toString(inflow_crypt));
        alert1++;
        ui->alert1->setText("Alerts:"+QString::number(alert1));
    }
    if (inflow_usdt < usd_to) {
        //ui->alert_2->setText("Warning USDT deposit into exchanges exeeds "+QLocale(QLocale::English).toString(inflow_usdt));
        alert2++;
        ui->alert2->setText("Alerts:"+QString::number(alert2));
    }
    if (outflow_crypt < crypt_from) {
        //ui->alert_3->setText("Warning Cryptocurrency withdraw from exchanges exeeds "+QLocale(QLocale::English).toString(outflow_crypt));
        alert3++;
        ui->alert3->setText("Alerts:"+QString::number(alert3));
    }
    if (outflow_usdt < usd_from) {
        //ui->alert_4->setText("Warning USDT withdraw from exchanges exeeds "+QLocale(QLocale::English).toString(outflow_usdt));
        alert4++;
        ui->alert4->setText("Alerts:"+QString::number(alert4));
    }
}

void MainWindow::on_settings_clicked()
{
    Settings settingsdialog;
    settingsdialog.setModal(true);
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
    ui->alert1->clear();
    ui->alert2->clear();
    ui->alert3->clear();
    ui->alert4->clear();
}

void MainWindow::rapport()
{
    QFile file;
}

void MainWindow::onTransferProgress(qint64 downloadTotal, qint64 downloadNow, qint64 uploadTotal, qint64 uploadNow)
{
    Q_UNUSED(uploadTotal);
    Q_UNUSED(uploadNow);

    if (downloadTotal > 0) {
        if (downloadNow > downloadTotal) downloadNow = downloadTotal;
        qint64 progress = (downloadNow * ui->progressBar->maximum())/downloadTotal;
        ui->progressBar->setValue(static_cast<int>(progress));
    } else {
        ui->progressBar->setValue(0);
    }
}

void MainWindow::onTransferDone()
{
    if (transfer->result() != CURLE_OK) {
        log(QString("Transfer failed with curl error '%1'")
                    .arg(curl_easy_strerror(transfer->result())));
        downloadFile->remove();
    } else {
        log(QString("Transfer complete. %1 bytes downloaded.")
                    .arg(downloadFile->size()));
        ui->progressBar->setValue(ui->progressBar->maximum());
        process_json();
    }

    delete downloadFile;
    downloadFile = nullptr;


}

void MainWindow::onTransferAborted()
{
    log(QString("Transfer aborted. %1 bytes downloaded.")
        .arg(downloadFile->size()));

    downloadFile->remove();
    delete downloadFile;
    downloadFile = nullptr;
}

void MainWindow::log(QString text)
{
    // Remove extra newlines for headers to be printed neatly
    if (text.endsWith("\n")) text.chop(1);
    if (text.endsWith('\r')) text.chop(1);
    ui->transferLog->appendPlainText(text);
}
