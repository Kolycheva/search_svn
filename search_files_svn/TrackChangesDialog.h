#include <QDialog>
#include <QTableWidget>

class TrackChangesDialog : public QDialog {
    Q_OBJECT
public:
    explicit TrackChangesDialog(QWidget *parent = nullptr);
    void setFile(const QString &filePath);

private:
    void loadHistory();
    void parseSvnLogXml(const QByteArray &xmlData);
    QString getFileHash(const QString &revision);
    QString filePath;
    QTableWidget *table;
};
