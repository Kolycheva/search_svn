#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QProcess>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QProgressBar>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Настройка прогресс-бара
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%v/%m ревизий");
    ui->statusbar->addPermanentWidget(progressBar);

    // Подключение кнопки определения MD5
    connect(ui->detectMd5Button, &QPushButton::clicked,
            this, &MainWindow::on_detectMd5Button_clicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QStringList MainWindow::getFileRevisions(const QString &filePath)
{
    QProcess svn;
    svn.start("svn", {"log", filePath, "--quiet"});

    if (!svn.waitForFinished(30000)) { // 30 секунд таймаут
        throw std::runtime_error(tr("SVN команда выполняется слишком долго").toStdString());
    }

    if (svn.exitCode() != 0) {
        QString error = svn.readAllStandardError();
        if (error.isEmpty()) error = tr("Неизвестная ошибка SVN");
        throw std::runtime_error(error.toStdString());
    }

    const QString output = svn.readAllStandardOutput();
    QStringList revisions;

    for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
        if (line.startsWith("r")) {
            QString rev = line.section(' ', 0, 0).mid(1); // Извлекаем номер ревизии
            if (!rev.isEmpty()) {
                revisions.prepend(rev); // Добавляем в обратном порядке (новые сначала)
            }
        }
    }

    return revisions;
}

QString MainWindow::getFileContentAtRevision(const QString &filePath, const QString &rev)
{
    const QString cacheKey = filePath + "@" + rev;
    if (md5Cache.contains(cacheKey)) {
        return ""; // Контент не кэшируем, только MD5
    }

    QProcess svn;
    svn.start("svn", {"cat", filePath, "-r", rev});

    if (!svn.waitForFinished(30000)) {
        throw std::runtime_error(tr("SVN команда выполняется слишком долго").toStdString());
    }

    if (svn.exitCode() != 0) {
        QString error = svn.readAllStandardError();
        if (error.contains("не найден")) {
            return ""; // Файл не существовал в этой ревизии
        }
        throw std::runtime_error(error.toStdString());
    }

    return svn.readAllStandardOutput();
}

QString MainWindow::calculateFileMd5(const QString &content)
{
    return QCryptographicHash::hash(content.toUtf8(),
                                 QCryptographicHash::Md5).toHex();
}

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Ошибка"), message);
    ui->resultEdit->append("\n[ОШИБКА] " + message);
    progressBar->setVisible(false);
}

void MainWindow::on_searchButton_clicked()
{
    try {
        const QString repoPath = ui->pathEdit->text().trimmed();
        const QString targetMd5 = ui->md5Edit->text().trimmed().toLower();

        if (repoPath.isEmpty()) {
            showError(tr("Укажите путь к файлу в SVN"));
            return;
        }

        ui->resultEdit->clear();
        ui->resultEdit->append(tr("Начинаем поиск...\n"));
        QApplication::processEvents();

        // Получаем список ревизий
        const QStringList revisions = getFileRevisions(repoPath);
        if (revisions.isEmpty()) {
            showError(tr("Не удалось получить историю файла"));
            return;
        }

        // Настраиваем прогресс-бар
        progressBar->setVisible(true);
        progressBar->setRange(0, revisions.size());
        progressBar->setValue(0);

        int foundCount = 0;
        const int totalRevisions = revisions.size();

        for (int i = 0; i < totalRevisions; ++i) {
            const QString &rev = revisions[i];
            const QString cacheKey = repoPath + "@" + rev;

            // Обновляем статус
            progressBar->setValue(i + 1);
            ui->statusbar->showMessage(tr("Проверка ревизии %1...").arg(rev));
            QApplication::processEvents();

            // Пропускаем если кэшировано
            if (md5Cache.contains(cacheKey)) {
                const QString &cachedMd5 = md5Cache[cacheKey];
                if (targetMd5.isEmpty() || cachedMd5 == targetMd5) {
                    ui->resultEdit->append(tr("Ревизия %1 (из кэша)").arg(rev));
                    ui->resultEdit->append(tr("MD5: %1").arg(cachedMd5));
                    ui->resultEdit->append("------");
                    foundCount++;
                }
                continue;
            }

            // Получаем содержимое файла
            QString content;
            try {
                content = getFileContentAtRevision(repoPath, rev);
            } catch (...) {
                continue; // Пропускаем ошибки получения содержимого
            }

            if (content.isEmpty()) continue;

            // Вычисляем MD5
            const QString currentMd5 = calculateFileMd5(content);
            md5Cache.insert(cacheKey, currentMd5);

            // Проверяем совпадение
            if (targetMd5.isEmpty() || currentMd5 == targetMd5) {
                ui->resultEdit->append(tr("Ревизия %1").arg(rev));
                ui->resultEdit->append(tr("MD5: %1").arg(currentMd5));

                if (!targetMd5.isEmpty()) {
                    ui->resultEdit->append(tr("Содержимое:\n%1").arg(content.left(500) +
                                         (content.length() > 500 ? "..." : ""));
                }

                ui->resultEdit->append("------");
                foundCount++;
            }
        }

        // Итоговое сообщение
        ui->resultEdit->append("\n" + tr("Поиск завершен!"));
        ui->resultEdit->append(tr("Проверено ревизий: %1").arg(totalRevisions));
        ui->resultEdit->append(tr("Найдено совпадений: %1").arg(foundCount));

    } catch (const std::exception &e) {
        showError(QString::fromStdString(e.what()));
    } catch (...) {
        showError(tr("Неизвестная ошибка"));
    }

    progressBar->setVisible(false);
    ui->statusbar->clearMessage();
}

void MainWindow::on_detectMd5Button_clicked()
{
    try {
        const QString repoPath = ui->pathEdit->text().trimmed();

        if (repoPath.isEmpty()) {
            showError(tr("Введите путь к файлу в SVN"));
            return;
        }

        progressBar->setVisible(true);
        progressBar->setRange(0, 0); // Неопределенный прогресс
        ui->statusbar->showMessage(tr("Определение MD5 текущей версии..."));

        const QString content = getFileContentAtRevision(repoPath, "HEAD");
        if (content.isEmpty()) {
            showError(tr("Не удалось получить содержимое файла"));
            return;
        }

        const QString md5 = calculateFileMd5(content);
        ui->md5Edit->setText(md5);

        ui->resultEdit->clear();
        ui->resultEdit->append(tr("Текущий MD5 хеш файла:"));
        ui->resultEdit->append(md5);
        ui->resultEdit->append("\n" + tr("Путь: ") + repoPath);

    } catch (const std::exception &e) {
        showError(QString::fromStdString(e.what()));
    } catch (...) {
        showError(tr("Ошибка при вычислении MD5"));
    }

    progressBar->setVisible(false);
    ui->statusbar->clearMessage();
}
