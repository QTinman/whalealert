#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "./settings.h"
#include "CurlEasy.h"
#include <QtCore>
#include <QtGui>
#include <QFile>

double usd_to, usd_from, crypt_to, crypt_from, flow_between_exc, unknown2unknown, flow_out_daily=0, flow_in_daily=0;
QDateTime dtc;
qint64 timestamp_latest=0;
int alert1=0,alert2=0,alert3=0,alert4=0;
int timer_minutes;
bool timer_enable;
QTimer *timer;
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
    ui->alert5->clear();
    ui->alert6->clear();
    QDate cd = QDate::currentDate();
    dtc = QDateTime(cd, QTime(23, 59, 59));

    timer = new QTimer(this);
    setGeometry(loadsettings("position").toRect());
    if (loadsettings("compactmode").toBool()) {
        ui->label_5->setText("C>Ex");
        ui->label_2->setText("$>Ex");
        ui->label_3->setText("Ex>C");
        ui->label->setText("Ex>$");
        ui->label_4->setText("Ex>Ex");
        ui->label_6->setText("Un>Un");
        ui->label_7->setText("Df>Ex");
        ui->label_8->setText("Ex>Df");
    }
    transfer = new CurlEasy(this); // Parent it so it will be destroyed automatically

    connect(transfer, &CurlEasy::done, this, &MainWindow::onTransferDone);
    connect(transfer, &CurlEasy::aborted, this, &MainWindow::onTransferAborted);
    connect(transfer, &CurlEasy::progress, this, &MainWindow::onTransferProgress);
    timer_minutes = loadsettings("timer_minutes").toInt();
    timer_enable = loadsettings("timer_enable").toBool();
    if (timer_enable) {
        //QTimer *timer = new QTimer(this);
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
    savesettings("position",this->geometry());
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
    if (api_key.isEmpty()) ui->transferLog->appendPlainText("API key is missing, please update in settings!");
    else {
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
}


void MainWindow::process_json()
{
    crypt_to=0;usd_to=0;crypt_from=0;usd_from=0,flow_between_exc=0,unknown2unknown=0;
    QJsonArray jsonArray = ReadJson("whale_alerts.json");
    qint64 timestamp_temp=0;
    foreach (const QJsonValue & value, jsonArray) {
        QJsonObject transactions = value.toObject();
        QJsonObject from = transactions["from"].toObject();
        QJsonObject to = transactions["to"].toObject();
        QString owner_type_from = from["owner_type"].toString();
        QString owner_type_to = to["owner_type"].toString();
        double usd = transactions["amount_usd"].toDouble();
        QString symbol = transactions["symbol"].toString();
        QString transaction_type = transactions["transaction_type"].toString();
        qint64 timestamp = transactions["timestamp"].toInt();
        QDateTime dt;
        dt.setSecsSinceEpoch(timestamp);
        //qDebug() << dt << " " << timestamp;
        if (timestamp > timestamp_temp) timestamp_temp = timestamp;
        if (timestamp > timestamp_latest) {
            if (owner_type_from == "unknown" && owner_type_to == "exchange" && !symbol.contains("usd") && transaction_type == "transfer") crypt_to+=usd;
            if (owner_type_from == "unknown" && owner_type_to == "exchange" && symbol.contains("usd") && transaction_type == "transfer") usd_to+=usd;
            if (owner_type_from == "exchange" && owner_type_to == "unknown" && !symbol.contains("usd") && transaction_type == "transfer") crypt_from+=usd;
            if (owner_type_from == "exchange" && owner_type_to == "unknown" && symbol.contains("usd") && transaction_type == "transfer") usd_from+=usd;
            if (owner_type_from == "exchange" && owner_type_to == "exchange" && transaction_type == "transfer") flow_between_exc+=usd;
            if (owner_type_from == "unknown" && owner_type_to == "unknown" && transaction_type == "transfer") unknown2unknown+=usd;
        }
    }
    timestamp_latest = timestamp_temp;
    flow_in_daily += crypt_to+usd_to;
    flow_out_daily += crypt_from+usd_from;
    Calc_json();
}

void MainWindow::Calc_json()
{
    qlonglong inflow_crypt=0, inflow_usdt=0, outflow_crypt=0, outflow_usdt=0;
    QFile csv_file;
    QDate cd = QDate::currentDate();
    QTime ct = QTime::currentTime();
    QDateTime cdt = QDateTime::currentDateTime();
    if (cdt > dtc) {
        flow_in_daily = 0;
        flow_out_daily = 0;
    }
    QString csv_string;
    QString q_usd_from = QLocale(QLocale::English).toString(usd_from,'F',0), q_usd_to = QLocale(QLocale::English).toString(usd_to,'F',0),
            q_crypt_from = QLocale(QLocale::English).toString(crypt_from,'F',0), q_crypt_to = QLocale(QLocale::English).toString(crypt_to,'F',0),
            flow_between = QLocale(QLocale::English).toString(flow_between_exc,'F',0), unkn2unkn = QLocale(QLocale::English).toString(unknown2unknown,'F',0),
            q_flow_in_daily = QLocale(QLocale::English).toString(flow_in_daily,'F',0), q_flow_out_daily = QLocale(QLocale::English).toString(flow_out_daily,'F',0);
    ui->total_crypt_to->setText(q_crypt_to);
    ui->tota_usdt_to->setText(q_usd_to);
    ui->total_crypt_from->setText(q_crypt_from);
    ui->total_usdt_from->setText(q_usd_from);
    ui->flow_between->setText(flow_between);
    ui->unknow2unknow->setText(unkn2unkn);
    ui->flow_in_daily->setText(q_flow_in_daily);
    ui->flow_out_daily->setText(q_flow_out_daily);
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
        alert2++;
        ui->alert2->setText("Alerts:"+QString::number(alert2));
    }
    if (outflow_crypt < crypt_from) {
        alert3++;
        ui->alert3->setText("Alerts:"+QString::number(alert3));
    }
    if (outflow_usdt < usd_from) {
        alert4++;
        ui->alert4->setText("Alerts:"+QString::number(alert4));
    }
    if (loadsettings("report").toBool()) {
        QString csv_filename="report_"+cd.toString()+".csv";
        bool header=false;
        csv_file.setFileName(csv_filename);
        if (!csv_file.exists()) {
            header=true;
            csv_file.open(QIODevice::WriteOnly | QIODevice::Text);
            QTextStream outStream(&csv_file);
            csv_string="Crypt to Ex,USDT to Ex,Crypt from Ex,USDT from Ex,Ex to Ex,Un to Un,Flow to Ex,Flow from Ex,Time";
            outStream << csv_string+"\n";
            csv_file.close();
        }
        csv_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        QTextStream outStream(&csv_file);
        csv_string=QString::number(crypt_to,'F',0)+"\t"+QString::number(usd_to,'F',0)+"\t"+QString::number(crypt_from,'F',0)+"\t"+QString::number(usd_from,'F',0)+
                "\t"+QString::number(flow_between_exc,'F',0)+"\t"+QString::number(unknown2unknown,'F',0)+"\t"+QString::number(flow_in_daily,'F',0)+"\t"+QString::number(flow_out_daily,'F',0)+"\t"+ct.toString();
        outStream << csv_string+"\n";
    }
    csv_file.close();
}

void MainWindow::on_settings_clicked()
{
    int threshold1=loadsettings("inflow_crypt").toInt(),threshold2=loadsettings("inflow_usdt").toInt(),threshold3=loadsettings("outflow_crypt").toInt(),threshold4=loadsettings("outflow_usdt").toInt();
    Settings settingsdialog;
    settingsdialog.setModal(true);
    settingsdialog.exec();
    timer_minutes = loadsettings("timer_minutes").toInt();
    timer_enable = loadsettings("timer_enable").toBool();
    if (timer_enable) {
        //QTimer *timer = new QTimer(this);
        if (!timer->isActive()) {
            connect(timer, SIGNAL(timeout()), this, SLOT(on_pushButton_clicked()));
            timer->start(timer_minutes*60000);
        }
    }
    if (threshold1 != loadsettings("inflow_crypt").toInt()){
        crypt_to=0;
        ui->alert1->clear();
    }
    if (threshold2 != loadsettings("inflow_crypt").toInt()){
        usd_to=0;
        ui->alert2->clear();
    }
    if (threshold3 != loadsettings("inflow_crypt").toInt()){
        crypt_from=0;
        ui->alert3->clear();
    }
    if (threshold4 != loadsettings("inflow_crypt").toInt()){
        usd_from=0;
        ui->alert4->clear();
    }
    if (loadsettings("compactmode").toBool()) {
        ui->label_5->setText("C>Ex");
        ui->label_2->setText("$>Ex");
        ui->label_3->setText("Ex>C");
        ui->label->setText("Ex>$");
        ui->label_4->setText("Ex>Ex");
        ui->label_6->setText("Un>Un");
        ui->label_7->setText("Df>Ex");
        ui->label_8->setText("Ex>Df");
    } else {
        ui->label_5->setText("Total Crypto to Exchange:");
        ui->label_2->setText("Total USDT to Exchange:");
        ui->label_3->setText("Total Crypto from Exchange:");
        ui->label->setText("Total USDT from Exchange:");
        ui->label_4->setText("Flow between Exchanges:");
        ui->label_6->setText("Unknow to Unknow");
        ui->label_7->setText("Daily flow in:");
        ui->label_8->setText("Daily flow out:");
    }
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
