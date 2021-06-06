#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::Settings *ui;
};

#endif // SETTINGS_H
