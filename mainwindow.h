#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QUrlQuery>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>
#include <OleanderStemmingLibrary/russian_stem.h>
#include "QStandardItemModel"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *manager;
    QNetworkRequest request;
    QUrlQuery params;
    QStringList downloadedThematicDictionaries;
    QStandardItemModel *model = new QStandardItemModel;

    stemming::russian_stem<> StemRussian;

    void writeToFile(QString filename, QStringList words);
    void deleteThematicDictionary(QString name);
    QStringList readFromFile(QString filename);
    QStringList getDownloadedRubrics();
    void analyzeText(QString text);
    void updateDictionaries();
private slots:
    void on_analyzeButton_clicked();
    void replyFinished(QNetworkReply *);
};
#endif // MAINWINDOW_H
