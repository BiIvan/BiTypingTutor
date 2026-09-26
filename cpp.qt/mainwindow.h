#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHash>
#include <QPushButton>
#include <QLabel>
#include <QElapsedTimer>
#include <QTimer>
#include <QComboBox>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QHBoxLayout>

struct KeyInfo {
    int qtKey;
    QString enLower;
    QString enUpper;
    QString ruLower;
    QString ruUpper;
};

struct Lesson {
    QString name;
    QString text;
};

struct TextCategory {
    QString name;
    QStringList texts;
};

enum class Mode { FreeText = 0, Lessons = 1, Timed = 2 };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void initData();
    void setupControlPanel(QVBoxLayout *layout);
    void buildKeyboard();
    void updateTextDisplay();
    void startNewText();
    void loadNextTimedText();
    void updateKeyboardLabels();
    bool detectRussianLayout();
    bool detectShiftPressed();
    int findKeyByEvent(QKeyEvent *event);
    void resetAllKeyStyles();
    void applyTheme();
    void updateNextKeyHint();
    void clearNextKeyHint();
    void loadResults();
    void saveResult(double wpm, double accuracy);
    void updateBestStats();
    void onModeChanged();
    void onLessonChanged();
    void onCategoryChanged();
    void onTimedChanged();
    void onTimedTick();
    void onThemeToggled();

    QHash<int, QPushButton*> m_keyButtons;
    QHash<int, KeyInfo> m_keyInfos;
    QHash<QString, int> m_charToKey;
    QVector<QVector<QPushButton*>> m_rows;
    QPushButton *m_spaceBtn = nullptr;
    int m_hintKey = -1;

    QLabel *m_textLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
    QLabel *m_layoutLabel = nullptr;
    QLabel *m_bestLabel = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QComboBox *m_lessonCombo = nullptr;
    QComboBox *m_timedCombo = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QPushButton *m_themeBtn = nullptr;

    QVector<Lesson> m_lessons;
    QVector<TextCategory> m_categories;

    QString m_targetText;
    int m_currentPos = 0;
    int m_errors = 0;
    int m_totalTyped = 0;

    QElapsedTimer m_timer;
    bool m_started = false;
    QTimer m_countdownTimer;
    int m_selectedTime = 30;
    int m_countdownSeconds = 30;
    bool m_timedFinished = false;
    int m_timedCorrectChars = 0;
    int m_timedErrors = 0;
    int m_timedTotalTyped = 0;

    Mode m_mode = Mode::FreeText;
    bool m_darkTheme = false;

    QString m_defaultStyle;
    QString m_pressedStyle;
    QString m_wrongStyle;
    QString m_hintStyle;
    QString m_spaceStyle;
    QString m_spacePressedStyle;
    QString m_spaceWrongStyle;
    QString m_spaceHintStyle;

    QTimer m_layoutTimer;
    bool m_isRussian = false;
    bool m_isShift = false;

    QJsonObject m_savedResults;
};

#endif // MAINWINDOW_H
