// в этом файле, содержаться все подробности работы функций
#include "mainwindow.h" // подключаем заголовочный файл, нужен чтобы функции работали
#include "ui_mainwindow.h" // автоматически созданный Qt файл для формы. Нужен, чтобы код мог обращаться к элементам интерфейса.
#include <QProcess> // Библиотека нужна для запуска других программ, в нашем случае SVN.
#include <QCryptographicHash> // Библиотека нужна для вычисления и сравнения md5
#include <QMessageBox> // всплывающее окно для общения с пользователем в данном коде отвечает за вывод ошибок
#include <QProgressBar> // полоса загрузки, которая визуально показывает прогресс выполнения
#include <QApplication> // делает интрефейс отзывчивым во время долгих опреаций, правильно запускает приложение, обрабатывает действия пользователя даже когда программа занята
#include <QGroupBox>
#include <QFormLayout>
#include <QPropertyAnimation>
// Конструкор класса (запускается при открытии окна)

MainWindow::MainWindow(QWidget *parent) // объявление конструктора
    : QMainWindow(parent) // Инициализация родительского класса, создание родительского окна
    , ui(new Ui::MainWindow) // Создание объекта интерфейса

{
    setStyleSheet(R"(
    QMainWindow {
        background-color: #f5f5f5;
        font-family: 'Segoe UI', Arial;
    }
    QLineEdit, QTextEdit {
        border: 1px solid #ccc;
        border-radius: 4px;
        padding: 5px;
        background: white;
    }
    QPushButton {
        background-color: #4CAF50;
        color: white;
        border: none;
        padding: 8px 16px;
        border-radius: 4px;
        min-width: 80px;
    }
    QPushButton:hover {
        background-color: #45a049;
    }
    QPushButton#closeButton {
        background-color: #f44336;
    }
    QPushButton#closeButton:hover {
        background-color: #d32f2f;
    }
    QLabel {
        font-weight: bold;
        color: #333;
    }
)");

    // В конструкторе:
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Группа для полей ввода
    QGroupBox *inputGroup = new QGroupBox("Параметры поиска");
    QFormLayout *formLayout = new QFormLayout(inputGroup);
    formLayout->addRow("Путь в SVN:", ui->pathEdit);
    formLayout->addRow("Имя файла:", ui->filenameEdit);
    formLayout->addRow("MD5 хэш:", ui->md5Edit);

    // Группа для кнопок
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(ui->pushButton);
    buttonLayout->addWidget(ui->detectMd5Button);
    buttonLayout->addWidget(ui->button_close);

    mainLayout->addWidget(inputGroup);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(ui->resultEdit);
    setCentralWidget(central);

    ui->setupUi(this); // Создает все элементы из ui. файла

    // Настройка прогресс-бара
    progressBar = new QProgressBar(this); // Выделяем память для полоски прогресса
    progressBar->setVisible(false); // Скрытие прогресс-бара, будет показываться только при поиске
    progressBar->setTextVisible(true); // Включаем отображение
    progressBar->setFormat("%v/%m ревизий"); // Задаём формат текста
    ui->statusbar->addPermanentWidget(progressBar);

    // Подключение кнопки определения MD5
    connect(ui->detectMd5Button, &QPushButton::clicked,
            this, &MainWindow::on_detectMd5Button_clicked);

    QObject::connect(ui->button_close, &QPushButton::clicked, [&]() {
        // создаем диалогое окно
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Подтверждение", "Вы действительно хотите выйти?", QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            qApp->quit();
        }
    });

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

QStringList MainWindow::getFileRevisions(const QString &filePath) // Объявление функции и передача ей пути к файлу SVN
{
    QProcess svn; // Создание процесса для svn
    svn.start("svn", {"log", filePath, "--quiet"}); // Запуск команды svn

    // Ожидание завершения программы
    if (!svn.waitForFinished(30000)) { // 30 секунд таймаут
        throw std::runtime_error(tr("SVN команда выполняется слишком долго").toStdString());
    }

    // Проверка на ошибки
    if (svn.exitCode() != 0) {
        QString error = svn.readAllStandardError();
        if (error.isEmpty()) error = tr("Неизвестная ошибка SVN");
        throw std::runtime_error(error.toStdString());
    }

    const QString output = svn.readAllStandardOutput(); // Читаем вывод команды svn log
    QStringList revisions; // Создание пустого списка для номеров ревизий

    for (const QString &line : output.split('\n', QString::SkipEmptyParts)) { // Цикл по каждой строке
        if (line.startsWith("r")) {
            QString rev = line.section(' ', 0, 0).mid(1); // Извлекаем номер ревизии
            if (!rev.isEmpty()) {
                revisions.prepend(rev); // Добавляем в обратном порядке (новые сначала)
            }
        }
    }

    return revisions; // Возвращаем список
}

// Получение содержимого файла

QString MainWindow::getFileContentAtRevision(const QString &filePath, const QString &rev) // Объявление функции, переча ей пути к файлу SVN и номера ревизии
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

    return svn.readAllStandardOutput(); // Возврат содержимого файла
}

// Вычисление MD5

QString MainWindow::calculateFileMd5(const QString &content)
{
    // Преобразуем текст в байты и вычисляем хеш
    return QCryptographicHash::hash(content.toUtf8(),
                                 QCryptographicHash::Md5).toHex();
}

// Если перед фунцкией стоит void, то функция не возвращает результат (просто выполняет действия)

// Показ ошибок

void MainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Ошибка"), message); // Красное окно с ошибкой
    ui->resultEdit->append("\n[ОШИБКА] " + message); // Пишем в лог
    progressBar->setVisible(false); // Закрываем прогресс-бар
}

// Основная функция поиска

void MainWindow::on_searchButton_clicked() // Срабатывает при клике на кнопку "Поиск"
{
    try {

        const QString repoPath = ui->pathEdit->text().trimmed(); // Берем текст из поля ввода пути и чистим текст от пробелов по краям
        const QString targetMd5 = ui->md5Edit->text().trimmed().toLower(); // Берем текст из поля ввода md5 хэш, переводим в нижний регистр и чистим текст от пробелов по краям

        if (repoPath.isEmpty()) {
            showError(tr("Укажите путь к файлу в SVN"));
            return;
        }

        ui->resultEdit->clear(); // Чистим поле вывода перед, следующим поиском
        ui->resultEdit->append(tr("Начинаем поиск...\n"));
        QApplication::processEvents();

        // Получаем список ревизий
        const QStringList revisions = getFileRevisions(repoPath);
        if (revisions.isEmpty()) {
            showError(tr("Не удалось получить историю файла"));
            return;

        QPropertyAnimation *anim = new QPropertyAnimation(ui->pushButton, "geometry");
        anim->setDuration(200);
        anim->setKeyValueAt(0, ui->pushButton->geometry());
        anim->setKeyValueAt(0.5, ui->pushButton->geometry().adjusted(-5, -5, 5, 5));
        anim->setKeyValueAt(1, ui->pushButton->geometry());
        anim->start();
        }

        // Настраиваем прогресс-бар
        progressBar->setVisible(true);
        progressBar->setRange(0, revisions.size());
        progressBar->setValue(0);

        int foundCount = 0;
        const int totalRevisions = revisions.size();

        for (int i = 0; i < totalRevisions; ++i) { // Перебираем все ревизии по очереди
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
                                                                     (content.length() > 500 ? "..." : "")));
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

// Определение MD5

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
