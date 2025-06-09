#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "trackchangesdialog.h"
#include <QProcess>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QProgressBar>
#include <QApplication>
#include <QGroupBox>
#include <QFormLayout>
#include <QPropertyAnimation>
#include <QFileDialog>
#include <QDebug>
#include <QRegularExpression>
#include <QElapsedTimer>
#include <QCheckBox>
// Конструкор класса (запускается при открытии окна)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. Настройка главного окна
    setWindowTitle("SVN-поисковик");
    // Убрана жёсткая привязка к пути иконки
    setWindowIcon(QIcon(":/icons/icon_app2.png"));
    resize(800, 600);

    // 2. Создание меню
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("Репозиторий");
    fileMenu->addAction("Открыть репозиторий", this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Выберите репозиторий");
        if (!path.isEmpty()) ui->pathEdit->setText(path);
    });
    fileMenu->addSeparator();

    QMenu *helpMenu = menuBar->addMenu("Справка");
    helpMenu->addAction("О программе", this, []() {
        QMessageBox::about(nullptr, "О программе", "Поисковик по репозиторию subversion.");
    });
    setMenuBar(menuBar);

    // 3. Центральный виджет и основной layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 4. Группа параметров поиска
    QGroupBox *searchGroup = new QGroupBox("Параметры поиска");
    QFormLayout *formLayout = new QFormLayout(searchGroup);

    // Добавление чекбокса для чувствительности к регистру
    QCheckBox *caseSensitiveCheckBox = new QCheckBox("Чувствительность к регистру");
    formLayout->addRow(caseSensitiveCheckBox);

    // Настройка полей ввода
    ui->pathEdit->setPlaceholderText("https://svn.example.com/trunk/ или путь к рабочей копии");
    ui->filenameEdit->setPlaceholderText("main.cpp (необязательно)");
    ui->md5Edit->setPlaceholderText("d41d8cd... (необязательно)");

    formLayout->addRow(new QLabel("Путь в SVN:"), ui->pathEdit);
    formLayout->addRow(new QLabel("Имя файла:"), ui->filenameEdit);
    formLayout->addRow(new QLabel("MD5 хэш:"), ui->md5Edit);

// 5. Панель кнопок
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *searchBtn = new QPushButton(QIcon("D:/Users/kolycheva_s/Documents/QtProjects/icons/search.png"), " Поиск");
    QPushButton *md5Btn = new QPushButton(QIcon("D:/Users/kolycheva_s/Documents/QtProjects/icons/icon_md5.png"), " Узнать MD5");
    QPushButton *closeBtn = new QPushButton(QIcon("D:/Users/kolycheva_s/Documents/QtProjects/icons/icon_close.png"), " Закрыть");

    QString btnStyle = R"(
        QPushButton {
            padding: 8px;
            border-radius: 4px;
            min-width: 100px;
            font-weight: bold;
        }
        QPushButton:hover {
            opacity: 0.9;
        }
    )";

    searchBtn->setStyleSheet(btnStyle + "background: #4CAF50; color: black; border: 2px solid #000; font-size: 13px;");
    md5Btn->setStyleSheet(btnStyle + "background: #2196F3; color: black; border: 2px solid #000;font-size: 13px;");
    closeBtn->setStyleSheet(btnStyle + "background: #f44336; color: black; border: 2px solid #000;font-size: 13px;");

    buttonLayout->addWidget(searchBtn);
    buttonLayout->addWidget(md5Btn);
    buttonLayout->addWidget(closeBtn);

    //5.5 Создание кнопки отслеживания изменений
    QPushButton *trackBtn = new QPushButton(QIcon("D:/Users/kolycheva_s/Documents/QtProjects/icons/eye.png"), " Отслеживать изменения");
    trackBtn->setStyleSheet(btnStyle + "background: #2196F3; color: black; border: 2px solid #000;font-size: 13px;");
    buttonLayout->addWidget(trackBtn);
    connect(trackBtn, &QPushButton::clicked, this, &MainWindow::on_trackChanges_clicked);

    // 6. Вкладки результатов
    QTabWidget *resultTabs = new QTabWidget();
    ui->resultEdit->setReadOnly(true);
    resultTabs->addTab(ui->resultEdit, "Результаты поиска");

    // 7. Строка состояния и прогресс-бар
    statusBar()->showMessage("Готово к работе");
    progressBar = new QProgressBar();
    progressBar->setMaximumWidth(200);
    progressBar->setVisible(false);
    statusBar()->addPermanentWidget(progressBar);

    // 8. Компоновка
    mainLayout->addWidget(searchGroup);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(resultTabs);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // 9. Подключение сигналов
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::on_searchButton_clicked);
    connect(md5Btn, &QPushButton::clicked, this, &MainWindow::on_detectMd5Button_clicked);
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::close);


    // 10. Глобальная стилизация
    setStyleSheet(R"(
        QMainWindow {
            background: #f5f5f5;
            font-family: 'Segoe UI';
        }
        QGroupBox {
            border: 1px solid #808080;
            border-radius: 5px;
            margin-top: 10px;
            padding-top: 15px;
            font-weight: bold;
            color: #555;
            font-size: 15px;
        }
        QLineEdit, QTextEdit {
            border: 1px solid #ccc;
            border-radius: 4px;
            padding: 5px;
            font-size: 14px;
        }
        QLabel {
            font-weight: bold;
            font-size: 13px;
        }
        QTabWidget::pane {
            border: 1px solid #ddd;
            border-radius: 4px;
            margin-top: 5px;
        }
        QTabBar::tab {
            padding: 5px 10px;
        }
        QStatusBar {
            color: #008000;
        }
    )");
}
// Деструктор (запускается при закрытии окна)
MainWindow::~MainWindow()
{
    delete ui; // Удаляет интерфейс
}

// Получение списка ревизий файла
/*Задачи функции:
- "Какие версии этого файла существуют?"
- Разобрать ответ: Вытащить номера ревизий из текста.
- Вернуть список: Чтобы другие части программы могли использовать эти номера.*/

QStringList MainWindow::getAllRevisions(const QString &repoPath)
{
    QProcess process;
    process.start("svn", QStringList() << "log" << "-q" << repoPath);
    if (!process.waitForFinished(120000)) { // Увеличим до 2 минут
        throw std::runtime_error(tr("SVN команда выполняется слишком долго: %1").arg(process.errorString()).toStdString());
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        if (error.isEmpty()) error = tr("Неизвестная ошибка SVN");
        throw std::runtime_error(error.toStdString());
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList revisions;
    QRegularExpression revRegex("^r(\\d+) \\|");

    for (const QString &line : output.split('\n', QString::SkipEmptyParts)) {
        QRegularExpressionMatch match = revRegex.match(line);
        if (match.hasMatch()) {
            revisions.prepend(match.captured(1)); // Старые версии сначала
        }
    }

    return revisions;
}

QStringList MainWindow::getFilesInRevision(const QString &repoPath, const QString &revision)
{
    QProcess process;
    QString command = "svn";

#ifdef Q_OS_WIN
    // Используем TortoiseSVN вместо SlikSvn
    QString tortoisePath = "C:\\Program Files\\TortoiseSVN\\bin\\svn.exe";
    if (QFile::exists(tortoisePath)) {
        command = tortoisePath;
    }
#endif

    process.start(command, QStringList() << "list" << "-R" << "-r" << revision << repoPath);

    // Увеличиваем таймаут до 5 минут
    if (!process.waitForFinished(300000)) {
        qDebug() << "SVN list timeout for revision" << revision;
        throw std::runtime_error(tr("SVN command timeout").toStdString());
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        qDebug() << "SVN ls error:" << error;
        throw std::runtime_error(error.toStdString());
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList files;
    QStringList lines = output.split('\n', QString::SkipEmptyParts);

    qDebug() << "Files in revision" << revision << ":" << lines.size();

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        // Удаляем информацию о ревизии и авторах
        if (trimmed.contains(' ')) {
            trimmed = trimmed.section(' ', -1); // Берем только последнюю часть
        }

        // Пропускаем директории
        if (!trimmed.endsWith('/') && !trimmed.isEmpty()) {
            files.append(trimmed);
        }
    }
    return files;
}
QString MainWindow::getFileHash(const QString &repoPath, const QString &filePath, const QString &revision)
{
    QProcess process;
    QString command = "svn";

#ifdef Q_OS_WIN
    QString tortoisePath = "C:\\Program Files\\TortoiseSVN\\bin\\svn.exe";
    if (QFile::exists(tortoisePath)) {
        command = tortoisePath;
    }
#endif

    // Корректное формирование URL
    QString target = repoPath;
    if (!target.endsWith('/') && !filePath.startsWith('/')) {
        target += '/';
    }
    target += filePath;

    // Нормализация URL (убираем двойные слеши, кроме протокола)
    target = QUrl(target).toString(QUrl::NormalizePathSegments);

    process.start(command, QStringList() << "cat" << "-r" << revision << target);

    // Увеличиваем таймаут до 1 минуты
    if (!process.waitForFinished(60000)) {
        throw std::runtime_error(tr("Ошибка получения содержимого файла").toStdString());
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        throw std::runtime_error(error.toStdString());
    }

    const QByteArray data = process.readAllStandardOutput();
    if (data.isEmpty()) {
        return QString();
    }

    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
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
        if (!caseSensitiveCheckBox) {
            showError(tr("Ошибка инициализации чекбокса чувствительности"));
            return;
        }
        // Получаем параметры поиска
        const QString repoPath = ui->pathEdit->text().trimmed();
        QString fileName = ui->filenameEdit->text().trimmed();
        QString md5Hash = ui->md5Edit->text().trimmed().toLower();
        const bool caseSensitive = caseSensitiveCheckBox->isChecked();

        if (repoPath.isEmpty()) {
            showError(tr("Укажите путь к репозиторию SVN"));
            return;
        }
        if (fileName.isEmpty() && md5Hash.isEmpty()) {
            showError(tr("Укажите имя файла или MD5 хэш для поиска"));
            return;
        }

        // Подготовка интерфейса
        ui->resultEdit->clear();
        ui->resultEdit->append(tr("Начинаем поиск в репозитории: %1").arg(repoPath));
        ui->resultEdit->append(tr("Критерии поиска:"));
        if (!fileName.isEmpty()) ui->resultEdit->append(tr(" - Имя файла: %1").arg(fileName));
        if (!md5Hash.isEmpty()) ui->resultEdit->append(tr(" - MD5 хэш: %1").arg(md5Hash));
        ui->resultEdit->append("");

        QApplication::processEvents();

        // Нормализация параметров
        if (!caseSensitive) fileName = fileName.toLower();

        // Получаем все ревизии репозитория
        progressBar->setVisible(true);
        progressBar->setRange(0, 0); // Неопределённый прогресс
        statusBar()->showMessage(tr("Получение списка ревизий..."));
        QApplication::processEvents();

        const QStringList revisions = getAllRevisions(repoPath);
        if (revisions.isEmpty()) {
            showError(tr("Не удалось получить список ревизий или репозиторий пуст"));
            return;
        }


        // Настройка прогресс-бара
        progressBar->setRange(0, revisions.size());
        progressBar->setValue(0);

        int foundCount = 0;
        int totalFilesScanned = 0;
        QElapsedTimer timer;
        timer.start();

        QProcess testProcess;
        testProcess.start("svn", QStringList() << "info" << repoPath);
        if (!testProcess.waitForFinished(10000)) {
            showError(tr("Не удается подключиться к репозиторию: %1").arg(testProcess.errorString()));
            return;
        }
        if (testProcess.exitCode() != 0) {
            QString error = testProcess.readAllStandardError();
            showError(tr("Ошибка доступа к репозиторию: %1").arg(error));
            return;
        }

        // Поиск по каждой ревизии
        for (int i = 0; i < revisions.size(); ++i) {
            const QString rev = revisions[i];
            progressBar->setValue(i + 1);
            statusBar()->showMessage(tr("Обработка ревизии %1 (%2/%3)...")
                                    .arg(rev).arg(i+1).arg(revisions.size()));
            QApplication::processEvents();

            try {

                // Получаем все файлы в ревизии
                const QStringList files = getFilesInRevision(repoPath, rev);
                qDebug() << "Revision" << rev << "files:" << files;

                // Проверяем каждый файл
                for (const QString &file : files) {
                    bool fullPathMatch = !fileName.isEmpty() && file.contains(fileName, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                    totalFilesScanned++;

                    QString baseName = QFileInfo(file).fileName();
                    if (!caseSensitive) baseName = baseName.toLower();

                    bool nameMatch = !fileName.isEmpty() && (baseName == fileName);
                    bool md5Match = false;

                    // Проверка по MD5 (если нужно)
                    if (!md5Hash.isEmpty() && (nameMatch || fileName.isEmpty())) {
                        const QString fileHash = getFileHash(repoPath, file, rev);
                        if (!fileHash.isEmpty() && fileHash == md5Hash) {
                            md5Match = true;
                        }
                    }

                    // Если есть совпадение
                    if ((nameMatch && md5Hash.isEmpty()) ||
                        (md5Match && fileName.isEmpty()) ||
                        (nameMatch && md5Match)) {

                        foundCount++;
                        ui->resultEdit->append(tr("НАЙДЕНО:"));
                        ui->resultEdit->append(tr(" - Путь: %1").arg(file));
                        ui->resultEdit->append(tr(" - Ревизия: %1").arg(rev));
                        ui->resultEdit->append(tr(" - Имя файла: %1").arg(QFileInfo(file).fileName()));

                        if (!md5Hash.isEmpty()) {
                            ui->resultEdit->append(tr(" - MD5: %1").arg(md5Match ? md5Hash : getFileHash(repoPath, file, rev)));
                        }

                        ui->resultEdit->append("----------------------------------");
                    }
                }
            }
            catch (const std::exception &e) {
                QString error = QString::fromStdString(e.what());
                error.replace("SVM", "SVN"); // Исправляем опечатку
                ui->resultEdit->append(tr("[ОШИБКА в ревизии %1]: %2").arg(rev).arg(error));
        }
        }

        // Итоговый отчёт
        const qint64 elapsed = timer.elapsed() / 1000.0;
        ui->resultEdit->append("\n" + tr("===== ПОИСК ЗАВЕРШЕН ====="));
        ui->resultEdit->append(tr("Проверено ревизий: %1").arg(revisions.size()));
        ui->resultEdit->append(tr("Проверено файлов: %1").arg(totalFilesScanned));
        ui->resultEdit->append(tr("Найдено совпадений: %1").arg(foundCount));
        ui->resultEdit->append(tr("Затраченное время: %1 сек").arg(elapsed));

        statusBar()->showMessage(tr("Поиск завершен. Найдено %1 совпадений").arg(foundCount), 5000);

    }
    catch (const std::exception &e) {
        showError(QString::fromStdString(e.what()));
    }
    catch (...) {
        showError(tr("Неизвестная ошибка"));
    }

    progressBar->setVisible(false);
}

void MainWindow::on_detectMd5Button_clicked()
{
    try {
        const QString repoPath = ui->pathEdit->text().trimmed();
        const QString fileName = ui->filenameEdit->text().trimmed();

        if (repoPath.isEmpty()) {
            showError(tr("Введите путь к репозиторию"));
            return;
        }
        if (fileName.isEmpty()) {
            showError(tr("Введите имя файла"));
            return;
        }

        progressBar->setVisible(true);
        progressBar->setRange(0, 0);
        statusBar()->showMessage(tr("Вычисление MD5 для файла..."));

        // Получаем хэш для HEAD-ревизии
        const QString fileHash = getFileHash(repoPath, fileName, "HEAD");
        if (fileHash.isEmpty()) {
            showError(tr("Не удалось получить содержимое файла"));
            return;
        }

        ui->md5Edit->setText(fileHash);
        ui->resultEdit->clear();
        ui->resultEdit->append(tr("Текущий MD5 хэш файла:"));
        ui->resultEdit->append(fileHash);
        ui->resultEdit->append(tr("\nПуть: %1").arg(repoPath));
        ui->resultEdit->append(tr("Имя файла: %1").arg(fileName));

        statusBar()->showMessage(tr("MD5 успешно вычислен"), 3000);
    }
    catch (const std::exception &e) {
        showError(QString::fromStdString(e.what()));
    }
    catch (...) {
        showError(tr("Ошибка при вычислении MD5"));
    }

    progressBar->setVisible(false);
}

void MainWindow::on_trackChanges_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл для отслеживания");
    if (filePath.isEmpty()) return;

    TrackChangesDialog *dialog = new TrackChangesDialog(this);
    dialog->setFile(filePath);
    dialog->show();
}
