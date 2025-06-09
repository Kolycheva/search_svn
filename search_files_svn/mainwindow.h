#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProgressBar>
#include <QCheckBox>
#include <QTextEdit>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:

    Ui::MainWindow *ui;
    QProgressBar *progressBar;
    QCheckBox *caseSensitiveCheckBox; // Добавляем чекбокс как член класса

    void on_searchButton_clicked();
    void on_detectMd5Button_clicked();
    void on_trackChanges_clicked();

    // Вспомогательные методы
    QStringList getAllRevisions(const QString &repoPath);
    QStringList getFilesInRevision(const QString &repoPath, const QString &revision);
    QString getFileHash(const QString &repoPath, const QString &filePath, const QString &revision);
    void showError(const QString &message);
    QString getFileHistory(const QString &repoPath, const QString &filePath);
};

#endif // MAINWINDOW_H
