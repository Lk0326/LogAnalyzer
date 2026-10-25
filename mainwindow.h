
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QFutureWatcher>

class QProgressBar;
class QTextEdit;
class QLabel;
class QLineEdit;
class QComboBox;
class QPushButton;
class QDateTimeEdit;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupUi();
    void loadDemoLogs();
    void openLogFile();
    void appendLogLine(const QString &line);
    void filterlogs();
    void searchLogs();
    void updateLogStats();
    void onLogLoadingFinished();
    void loadLogFileAsync(const QString &filePath);

    QTextEdit *logView = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *statsLabel = nullptr;
    QPushButton *openButton = nullptr;
    QFutureWatcher<QStringList> *logWatcher = nullptr;
    QLineEdit *searchEdit = nullptr;
    QComboBox *levelCombo = nullptr;
    QDateTimeEdit *startTimeEdit = nullptr;
    QDateTimeEdit *endTimeEdit = nullptr;
    QStringList allLogs;
    QProgressBar *infoBar = nullptr;
    QProgressBar *warnBar = nullptr;
    QProgressBar *errorBar = nullptr;
    QLabel *fileNameLabel = nullptr;
};

#endif // MAINWINDOW_H
