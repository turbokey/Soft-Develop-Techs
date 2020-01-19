#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QSslConfiguration>
#include <QDir>
#include "QStandardItem"

const QString THEMATIC_DICTIONARIES_FOLDER = "Тематические словари";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager(this);

    QUrl url("https://sociation.org/ajax/word_associations/");
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QSslConfiguration config = request.sslConfiguration();
                config.setPeerVerifyMode(QSslSocket::VerifyNone);
                config.setProtocol(QSsl::TlsV1_0);
                request.setSslConfiguration(config);

    connect(manager, SIGNAL(finished(QNetworkReply *)), this, SLOT(replyFinished(QNetworkReply *)));
    connect(ui->pushButton_2, SIGNAL(clicked()),this,SLOT(on_analyzeButton_clicked()));
    connect(ui->refreshButton, SIGNAL(clicked()),this,SLOT(on_refreshButton_clicked()));

    downloadedThematicDictionaries = getDownloadedRubrics();
    for (int i=0;i<downloadedThematicDictionaries.size();i++)
    {
        QString s = downloadedThematicDictionaries.at(i);
        ui->plainTextEdit->insertPlainText(s.replace(".txt",""));
        if (i != downloadedThematicDictionaries.size()-1)
          ui->plainTextEdit->insertPlainText(",");
    }

}

void MainWindow::on_analyzeButton_clicked()
{
    analyzeText(ui->plainTextEdit_2->toPlainText());
}

void MainWindow::on_refreshButton_clicked()
{
    updateDictionaries();
}
void MainWindow::analyzeText(QString text)
{
    model->clear();

    QString str = text.toLower();
    QStringList textWords = str.remove(QRegExp(QString::fromUtf8("[-`~!@#$%^&*()_”+=|:;<>,.?/{}\'\"\\\[\\\]\\\\]"))).split(" ");

    QMultiMap<int,QString> number_of_matches;

    for(int i=0;i<downloadedThematicDictionaries.size();i++)
    {
        QString rubric = downloadedThematicDictionaries.at(i);
        QStringList curDictionary = readFromFile(downloadedThematicDictionaries.at(i));
        int counter = 0;
        for (int j=0;j<textWords.size();j++)
        {
            QString wordFromText = textWords.at(j);
            std::wstring wordFromText_stemmed(wordFromText.toStdWString().c_str());
            StemRussian(wordFromText_stemmed);

            for (int k=0;k<curDictionary.size();k++)
            {
                QString wordFromDictionary = curDictionary.at(k);
                std::wstring wordFromDictionary_stemmed(wordFromDictionary.toStdWString().c_str());
                StemRussian(wordFromDictionary_stemmed);

                if (wordFromText_stemmed == wordFromDictionary_stemmed)
                   counter++;
            }
        }
        number_of_matches.insert(counter,rubric.replace(".txt",""));
    }

    QMapIterator<int, QString> iter(number_of_matches);
    iter.toBack();

    int top_size=downloadedThematicDictionaries.size();

    QStandardItem *item;

    QStringList horizontalHeader;
    horizontalHeader.append("Тематика");
    horizontalHeader.append("Кол-во совпадений");

    QStringList verticalHeader;
    for (int i=0; i<top_size; i++)
    {
       verticalHeader.append(QString::number(i+1));

       auto e = iter.previous();

       item = new QStandardItem(e.value());
       model->setItem(i, 0, item);
       item = new QStandardItem(QString::number(e.key()));
       model->setItem(i, 1, item);
    }

       model->setHorizontalHeaderLabels(horizontalHeader);
       model->setVerticalHeaderLabels(verticalHeader);

       ui->tableView->setModel(model);
}

void MainWindow::updateDictionaries()
{
    QString themes = ui->plainTextEdit->toPlainText();
    QRegExp rx("\\,");
    QStringList query = themes.split(rx);

    for(int i=0;i<downloadedThematicDictionaries.size();i++)
    {
        QString s = downloadedThematicDictionaries.at(i);
        if (!query.contains(s.replace(".txt","")))
        {
            deleteThematicDictionary(downloadedThematicDictionaries.at(i));
            downloadedThematicDictionaries.removeAt(i);
        }
    }

    for (int i = 0; i < query.size(); i++)
    {
        if (!downloadedThematicDictionaries.contains(query.at(i)+".txt"))
        {
            params.clear();
            params.addQueryItem("max_count", "0");
            params.addQueryItem("back", "false");
            params.addQueryItem("word", query.at(i));
            manager->post(request, params.query().toUtf8());
        }
    }
}
void MainWindow::replyFinished(QNetworkReply *reply)
{
    QString response;

    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            const int available = reply->bytesAvailable();
            if (available > 0) {
                QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
                QJsonObject rootObj = document.object();
                QJsonArray associations = rootObj["associations"].toArray();

                QString rubric_name = rootObj["word"].toString();
                QStringList rubric_associations;

                foreach (const QJsonValue & value,associations) {
                    QJsonObject word = value.toObject();
                    rubric_associations.append(word.value("name").toString());
                    /*
                     Популярность слова:
                     word.value("popularity");
                    */

                }
                writeToFile(rubric_name,rubric_associations);
                downloadedThematicDictionaries = getDownloadedRubrics();
            }
        } else {
            response = tr("Error: %1 status: %2").arg(reply->errorString(), reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString());
            qDebug() << response;
        }

        reply->deleteLater();
    }

    if (response.trimmed().isEmpty()) {
        response = tr("Unable to retrieve post response");
    }
}

void MainWindow::writeToFile(QString filename, QStringList words)
{
    if (!QDir(THEMATIC_DICTIONARIES_FOLDER).exists())
        QDir().mkdir(THEMATIC_DICTIONARIES_FOLDER);

    QFile fOut(THEMATIC_DICTIONARIES_FOLDER+"/"+filename+".txt");
    if (fOut.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream s(&fOut);
        for (int i = 0; i < words.size(); ++i)
            s << words.at(i) << '\n';
      } else {
        qDebug() << "error opening output file\n";
        return;
      }
      fOut.close();
}

QStringList MainWindow::readFromFile(QString filename)
{
    if (!QDir(THEMATIC_DICTIONARIES_FOLDER).exists())
        QDir().mkdir(THEMATIC_DICTIONARIES_FOLDER);

    QStringList l;
    QFile fIn(THEMATIC_DICTIONARIES_FOLDER+"/"+filename);
    if (fIn.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream sIn(&fIn);
        while (!sIn.atEnd())
            l += sIn.readLine();
      } else {
        qDebug() << "error opening input file\n";
      }
    return l;
}

void MainWindow::deleteThematicDictionary(QString name)
{
    QFile file(THEMATIC_DICTIONARIES_FOLDER+"/"+name);
    file.remove();
}

QStringList MainWindow::getDownloadedRubrics()
{
    if (!QDir(THEMATIC_DICTIONARIES_FOLDER).exists())
        QDir().mkdir(THEMATIC_DICTIONARIES_FOLDER);

    QDir directory(THEMATIC_DICTIONARIES_FOLDER);
    QStringList dics = directory.entryList(QStringList() << "*.txt" ,QDir::Files);

    return dics;
}

MainWindow::~MainWindow()
{
    delete ui;
    delete manager;
}

