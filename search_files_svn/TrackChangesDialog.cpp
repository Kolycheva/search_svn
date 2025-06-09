#include "TrackChangesDialog.h"
#include <QHeaderView>
#include <QProcess>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QXmlStreamReader>
#include <QFileInfo>
#include <QCryptographicHash>

TrackChangesDialog::TrackChangesDialog(QWidget *parent) : QDialog(parent) {
    // ... [существующий код инициализации] ...
}

void TrackChangesDialog::setFile(const QString &path) {
    filePath = path;
    setWindowTitle(tr("История изменений: %1").arg(QFileInfo(path).fileName()));
    loadHistory();
}

void TrackChangesDialog::loadHistory() {
    // Очищаем таблицу перед загрузкой новых данных
    table->setRowCount(0);

    QProcess process;
    process.start("svn", QStringList() << "log" << "--xml" << "-v" << filePath);

    if (!process.waitForFinished(30000)) {
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Превышено время ожидания выполнения команды SVN"));
        return;
    }

    if (process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        QMessageBox::critical(this, tr("Ошибка SVN"),
            tr("Не удалось получить историю файла:\n%1").arg(error));
        return;
    }

    QByteArray output = process.readAllStandardOutput();
    parseSvnLogXml(output);
}

void TrackChangesDialog::parseSvnLogXml(const QByteArray &xmlData) {
    QXmlStreamReader xml(xmlData);
    int row = 0;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement() && xml.name() == "logentry") {
            QString revision = xml.attributes().value("revision").toString();
            QString author, date, message;
            QStringList changedPaths;

            while (!(xml.isEndElement() && xml.name() == "logentry")) {
                xml.readNext();

                if (xml.isStartElement()) {
                    if (xml.name() == "author") {
                        author = xml.readElementText();
                    }
                    else if (xml.name() == "date") {
                        date = xml.readElementText();
                        // Форматируем дату: "2023-10-05T12:34:56.789Z" -> "2023-10-05 12:34"
                        if (date.length() > 16) {
                            date = date.left(10) + " " + date.mid(11, 5);
                        }
                    }
                    else if (xml.name() == "msg") {
                        message = xml.readElementText();
                    }
                    else if (xml.name() == "path") {
                        QString action = xml.attributes().value("action").toString();
                        QString path = xml.readElementText();
                        changedPaths.append(action + " " + path);
                    }
                }
            }

            // Добавляем запись в таблицу
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(revision));
            table->setItem(row, 1, new QTableWidgetItem(date));
            table->setItem(row, 2, new QTableWidgetItem(author));

            // Преобразуем действие: A->Добавлен, M->Изменен, D->Удален
            QString actionSymbol = changedPaths.contains(filePath) ?
                changedPaths.filter(filePath).first().left(1) : "?";

            QString actionText;
            if (actionSymbol == "A") actionText = tr("Добавлен");
            else if (actionSymbol == "M") actionText = tr("Изменен");
            else if (actionSymbol == "D") actionText = tr("Удален");
            else actionText = tr("Неизвестно");

            table->setItem(row, 3, new QTableWidgetItem(actionText));

            // Получаем хэш файла (только для существующих версий)
            if (actionSymbol != "D") {
                QString hash = getFileHash(revision);
                table->setItem(row, 4, new QTableWidgetItem(hash));
            } else {
                table->setItem(row, 4, new QTableWidgetItem("N/A"));
            }

            row++;
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML parse error:" << xml.errorString();
        QMessageBox::warning(this, tr("Ошибка"),
            tr("Ошибка разбора XML: %1").arg(xml.errorString()));
    }
}

QString TrackChangesDialog::getFileHash(const QString &revision) {
    QProcess process;
    process.start("svn", QStringList() << "cat" << "-r" << revision << filePath);

    if (!process.waitForFinished(10000)) {
        return tr("Таймаут");
    }

    if (process.exitCode() != 0) {
        return tr("Ошибка");
    }

    QByteArray data = process.readAllStandardOutput();
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}
