#include "settings.h"
#include "ui_settings.h"
#include "mainwindow.h"
#include <QtCore>

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
    ui->api_key->setText(loadsettings("api_key").toString());
    ui->inflow_crypt->setValue(loadsettings("inflow_crypt").toInt());
    ui->inflow_usdt->setValue(loadsettings("inflow_usdt").toInt());
    ui->outflow_crypt->setValue(loadsettings("outflow_crypt").toInt());
    ui->outflow_usdt->setValue(loadsettings("outflow_usdt").toInt());
    ui->timer_minutes->setValue(loadsettings("timer_minutes").toInt());
    ui->timer_enable->setChecked(loadsettings("timer_enable").toBool());
    ui->compactmode->setChecked(loadsettings("compactmode").toBool());
}

Settings::~Settings()
{
    delete ui;
}


QVariant Settings::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void Settings::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}


void Settings::on_buttonBox_accepted()
{
    savesettings("api_key",ui->api_key->text());
    savesettings("inflow_crypt",ui->inflow_crypt->value());
    savesettings("inflow_usdt",ui->inflow_usdt->value());
    savesettings("outflow_crypt",ui->outflow_crypt->value());
    savesettings("outflow_usdt",ui->outflow_usdt->value());
    savesettings("timer_minutes",ui->timer_minutes->value());
    savesettings("timer_enable",ui->timer_enable->isChecked());
    savesettings("compactmode",ui->compactmode->isChecked());
}
