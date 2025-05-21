// в этом файле перчисленны функции и элементы, которые есть в классе
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QProgressBar>

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

    /*Вы рисуете форму в Qt Designer → сохраняете как mainwindow.ui
    Qt автоматически генерирует ui_mainwindow.h при сборке*/

    Ui::MainWindow *ui;  // Указатель на интерфейс

    void on_searchButton_clicked();
    void on_detectMd5Button_clicked();

    // для поиска по истории файла
    QStringList getFileRevisions(const QString &filePath);
    QString getFileContentAtRevision(const QString &filePath, const QString &rev);

    // Общая функция для MD5
    QString getFileMd5(const QString &content);

    // Для рекурсивного поиска по репозиторию
    QStringList getSvnFileList(const QString &repoUrl);

    // Чтение содержимого файла из SVN
    QString getFileContent(const QString &fileUrl);

    QProgressBar *progressBar;
    QHash<QString, QString> md5Cache; // Кэш MD5 (ключ: "путь@ревизия")

    void showError(const QString &message);
    QString calculateFileMd5(const QString &content);

};
#endif // MAINWINDOW_H
