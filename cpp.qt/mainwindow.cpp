#include "mainwindow.h"
#include <QKeyEvent>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QFont>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Тренажёр клавиатуры");
    resize(950, 520);

    initData();
    applyTheme();

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *layout = new QVBoxLayout(central);

    setupControlPanel(layout);

    m_layoutLabel = new QLabel(this);
    m_layoutLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_layoutLabel);

    m_bestLabel = new QLabel(this);
    m_bestLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_bestLabel);

    m_textLabel = new QLabel(this);
    m_textLabel->setWordWrap(true);
    m_textLabel->setFont(QFont("Consolas", 14));
    m_textLabel->setMinimumHeight(80);
    m_textLabel->setTextFormat(Qt::RichText);
    layout->addWidget(m_textLabel);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statsLabel);

    buildKeyboard();
    auto *grid = new QGridLayout();
    layout->addLayout(grid);

    for (int row = 0; row < m_rows.size(); ++row) {
        int col = 0;
        for (auto *btn : m_rows[row])
            grid->addWidget(btn, row, col++);
    }

    m_spaceBtn = new QPushButton("Space", this);
    m_spaceBtn->setMinimumHeight(46);
    m_spaceBtn->setFocusPolicy(Qt::NoFocus);
    m_keyButtons.insert(Qt::Key_Space, m_spaceBtn);
    grid->addWidget(m_spaceBtn, m_rows.size(), 2, 1, 9);

    applyTheme();

    m_isRussian = detectRussianLayout();
    m_isShift = detectShiftPressed();
    updateKeyboardLabels();

    connect(&m_layoutTimer, &QTimer::timeout, this, [this]() {
        bool newRu = detectRussianLayout();
        bool newShift = detectShiftPressed();
        if (newRu != m_isRussian || newShift != m_isShift) {
            m_isRussian = newRu;
            m_isShift = newShift;
            updateKeyboardLabels();
        }
    });
    m_layoutTimer.start(100);

    connect(&m_countdownTimer, &QTimer::timeout, this, &MainWindow::onTimedTick);

    loadResults();
    updateBestStats();
    startNewText();
}

// ---------------------------------------------------------------------------
// Данные: уроки и категории текстов
// ---------------------------------------------------------------------------

void MainWindow::initData()
{
    m_lessons = {
                  {"Домашний ряд (RU)", "фыва олдж фыва олдж фыва олдж фыва олдж"},
                  {"Домашний ряд (EN)", "asdf jkl; asdf jkl; asdf jkl; asdf jkl;"},
                  {"Верхний ряд (RU)", "цукенгшщзхъ цукенгшщзхъ цукенгшщзхъ"},
                  {"Верхний ряд (EN)", "qwertyuiop qwertyuiop qwertyuiop"},
                  {"Нижний ряд (RU)", "ячсмитьбю ячсмитьбю ячсмитьбю"},
                  {"Нижний ряд (EN)", "zxcvbnm zxcvbnm zxcvbnm"},
                  {"Алфавит (RU)", "абвгдежзийклмнопрстуфхцчшщъыьэюя"},
                  {"Алфавит (EN)", "abcdefghijklmnopqrstuvwxyz"},
                  {"Цифры", "1234567890 1234567890 1234567890"},
                  };

    m_categories = {
        {"Короткие фразы", {
                               "привет мир как дела сегодня",
                               "кошка сидит на тёплом подоконнике",
                               "ребёнок учится печатать быстро",
                               "тренажёр клавиатуры помогает тренироваться",
                               "яблоко груша банан апельсин лимон",
                               "солнце светит ярко и тепло",
                               "мама папа брат сестра семья",
                               "учитель говорит повторите урок",
                               "hello world how are you today",
                               "the quick brown fox jumps over the lazy dog",
                               "practice makes perfect every day",
                               "typing is a useful skill to learn",
                           }},
        {"Длинные тексты", {
                               "в ясный зимний день когда солнце светило ярко мальчик вышел на улицу чтобы покататься на коньках на замёрзшем пруду",
                               "старый дом стоял на краю леса его окна были тёмными а дверь скрипела при каждом порыве холодного осеннего ветра",
                               "однажды в студёную зимнюю пору я из лесу вышел был сильный мороз смотрю поднимается медленно в гору лошадка везущая хворосту воз",
                               "it was the best of times it was the worst of times it was the age of wisdom it was the age of foolishness",
                               "the old lighthouse stood on the cliff its beam swept across the dark water searching for ships lost in the storm",
                           }},
        {"Код", {
                  "int main() { return 0; }",
                  "for (int i = 0; i < 10; i++) { }",
                  "std::cout << hello << std::endl;",
                  "#include <iostream>",
                  "vector<int> v = {1, 2, 3};",
                  "if (x > 0 && y < 10) { return true; }",
                  "while (!queue.empty()) { auto x = queue.front(); }",
                  "template<typename T> void func(T& t) { }",
                  }},
        {"Цифры", {
                      "123 456 7890 321 654 987",
                      "3.14159 2.71828 1.41421 1.73205",
                      "0 1 1 2 3 5 8 13 21 34 55 89 144",
                      "100 200 300 400 500 600 700 800 900",
                      "42 7 3.14 2.71 0 99 1000 256",
                  }},
    };
}

// ---------------------------------------------------------------------------
// Панель управления
// ---------------------------------------------------------------------------

void MainWindow::setupControlPanel(QVBoxLayout *layout)
{
    auto *panel = new QHBoxLayout();

    m_modeCombo = new QComboBox(this);
    m_modeCombo->addItem("Свободный режим", static_cast<int>(Mode::FreeText));
    m_modeCombo->addItem("Уроки", static_cast<int>(Mode::Lessons));
    m_modeCombo->addItem("На время", static_cast<int>(Mode::Timed));
    m_modeCombo->setFocusPolicy(Qt::NoFocus);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onModeChanged);
    panel->addWidget(new QLabel("Режим:", this));
    panel->addWidget(m_modeCombo);

    m_lessonCombo = new QComboBox(this);
    for (const auto &l : m_lessons)
        m_lessonCombo->addItem(l.name);
    m_lessonCombo->setVisible(false);
    m_lessonCombo->setFocusPolicy(Qt::NoFocus);
    connect(m_lessonCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLessonChanged);
    panel->addWidget(new QLabel("Урок:", this));
    panel->addWidget(m_lessonCombo);

    m_timedCombo = new QComboBox(this);
    m_timedCombo->addItem("30 сек", 30);
    m_timedCombo->addItem("1 мин", 60);
    m_timedCombo->addItem("2 мин", 120);
    m_timedCombo->setVisible(false);
    m_timedCombo->setFocusPolicy(Qt::NoFocus);
    connect(m_timedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTimedChanged);
    panel->addWidget(new QLabel("Время:", this));
    panel->addWidget(m_timedCombo);

    m_categoryCombo = new QComboBox(this);
    for (const auto &c : m_categories)
        m_categoryCombo->addItem(c.name);
    m_categoryCombo->setFocusPolicy(Qt::NoFocus);
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onCategoryChanged);
    panel->addWidget(new QLabel("Текст:", this));
    panel->addWidget(m_categoryCombo);

    m_themeBtn = new QPushButton("🌙", this);
    m_themeBtn->setFixedSize(40, 28);
    m_themeBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_themeBtn, &QPushButton::clicked, this, &MainWindow::onThemeToggled);
    panel->addWidget(m_themeBtn);

    panel->addStretch();
    layout->addLayout(panel);
}

// ---------------------------------------------------------------------------
// Тема оформления
// ---------------------------------------------------------------------------

void MainWindow::applyTheme()
{
    if (m_darkTheme) {
        m_defaultStyle = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                         "background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #555; border-radius: 6px; }";
        m_pressedStyle = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                         "background-color: #4CAF50; color: white; border: 2px solid #2E7D32; border-radius: 6px; }";
        m_wrongStyle   = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                       "background-color: #f44336; color: white; border: 2px solid #c62828; border-radius: 6px; }";
        m_hintStyle    = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                      "background-color: #1565C0; color: white; border: 2px solid #42A5F5; border-radius: 6px; }";
        m_spaceStyle = "QPushButton { font-size: 14px; "
                       "background-color: #2d2d2d; color: #e0e0e0; border: 1px solid #555; border-radius: 6px; }";
        m_spacePressedStyle = "QPushButton { font-size: 14px; "
                              "background-color: #4CAF50; color: white; border: 2px solid #2E7D32; border-radius: 6px; }";
        m_spaceWrongStyle   = "QPushButton { font-size: 14px; "
                            "background-color: #f44336; color: white; border: 2px solid #c62828; border-radius: 6px; }";
        m_spaceHintStyle    = "QPushButton { font-size: 14px; "
                           "background-color: #1565C0; color: white; border: 2px solid #42A5F5; border-radius: 6px; }";
    } else {
        m_defaultStyle = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                         "background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 6px; }";
        m_pressedStyle = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                         "background-color: #4CAF50; color: white; border: 2px solid #2E7D32; border-radius: 6px; }";
        m_wrongStyle   = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                       "background-color: #f44336; color: white; border: 2px solid #c62828; border-radius: 6px; }";
        m_hintStyle    = "QPushButton { font-size: 13px; min-width: 46px; min-height: 46px; "
                      "background-color: #E3F2FD; border: 2px solid #2196F3; border-radius: 6px; }";
        m_spaceStyle = "QPushButton { font-size: 14px; "
                       "background-color: #f0f0f0; border: 1px solid #ccc; border-radius: 6px; }";
        m_spacePressedStyle = "QPushButton { font-size: 14px; "
                              "background-color: #4CAF50; color: white; border: 2px solid #2E7D32; border-radius: 6px; }";
        m_spaceWrongStyle   = "QPushButton { font-size: 14px; "
                            "background-color: #f44336; color: white; border: 2px solid #c62828; border-radius: 6px; }";
        m_spaceHintStyle    = "QPushButton { font-size: 14px; "
                           "background-color: #E3F2FD; border: 2px solid #2196F3; border-radius: 6px; }";
    }

    if (m_textLabel)
        m_textLabel->setStyleSheet(m_darkTheme
                                       ? "padding: 10px; background-color: #1e1e1e; color: #e0e0e0; border: 1px solid #555; border-radius: 6px;"
                                       : "padding: 10px; background-color: #fafafa; border: 1px solid #ddd; border-radius: 6px;");
    if (m_statsLabel)
        m_statsLabel->setStyleSheet(m_darkTheme
                                        ? "font-size: 14px; padding: 6px; color: #bbb;"
                                        : "font-size: 14px; padding: 6px; color: #555;");
    if (m_layoutLabel)
        m_layoutLabel->setStyleSheet(m_darkTheme
                                         ? "font-size: 14px; font-weight: bold; padding: 4px; color: #e0e0e0;"
                                         : "font-size: 14px; font-weight: bold; padding: 4px; color: #333;");
    if (m_bestLabel)
        m_bestLabel->setStyleSheet(m_darkTheme
                                       ? "font-size: 13px; padding: 4px; color: #888;"
                                       : "font-size: 13px; padding: 4px; color: #999;");

    QString comboStyle = m_darkTheme
                             ? "QComboBox { background-color: #2d2d2d; color: #e0e0e0; padding: 4px; }"
                             : "QComboBox { padding: 4px; }";
    QString btnStyle = m_darkTheme
                           ? "QPushButton { background-color: #2d2d2d; color: #e0e0e0; }"
                           : "";
    for (auto *w : {m_modeCombo, m_lessonCombo, m_timedCombo, m_categoryCombo})
        if (w) w->setStyleSheet(comboStyle);

    resetAllKeyStyles();
    updateNextKeyHint();
}

void MainWindow::onThemeToggled()
{
    m_darkTheme = !m_darkTheme;
    m_themeBtn->setText(m_darkTheme ? "☀️" : "🌙");
    applyTheme();
}

// ---------------------------------------------------------------------------
// Клавиатура
// ---------------------------------------------------------------------------

void MainWindow::buildKeyboard()
{
    QVector<QVector<KeyInfo>> keyRows = {
        {
            {Qt::Key_QuoteLeft, "`", "~", "ё", "Ё"},
            {Qt::Key_1, "1", "!", "1", "!"},
            {Qt::Key_2, "2", "@", "2", "\""},
            {Qt::Key_3, "3", "#", "3", "№"},
            {Qt::Key_4, "4", "$", "4", ";"},
            {Qt::Key_5, "5", "%", "5", "%"},
            {Qt::Key_6, "6", "^", "6", ":"},
            {Qt::Key_7, "7", "&", "7", "?"},
            {Qt::Key_8, "8", "*", "8", "*"},
            {Qt::Key_9, "9", "(", "9", "("},
            {Qt::Key_0, "0", ")", "0", ")"},
            {Qt::Key_Minus, "-", "_", "-", "_"},
            {Qt::Key_Equal, "=", "+", "=", "+"}
        },
        {
            {Qt::Key_Q, "q", "Q", "й", "Й"},
            {Qt::Key_W, "w", "W", "ц", "Ц"},
            {Qt::Key_E, "e", "E", "у", "У"},
            {Qt::Key_R, "r", "R", "к", "К"},
            {Qt::Key_T, "t", "T", "е", "Е"},
            {Qt::Key_Y, "y", "Y", "н", "Н"},
            {Qt::Key_U, "u", "U", "г", "Г"},
            {Qt::Key_I, "i", "I", "ш", "Ш"},
            {Qt::Key_O, "o", "O", "щ", "Щ"},
            {Qt::Key_P, "p", "P", "з", "З"},
            {Qt::Key_BracketLeft, "[", "{", "х", "Х"},
            {Qt::Key_BracketRight, "]", "}", "ъ", "Ъ"}
        },
        {
            {Qt::Key_A, "a", "A", "ф", "Ф"},
            {Qt::Key_S, "s", "S", "ы", "Ы"},
            {Qt::Key_D, "d", "D", "в", "В"},
            {Qt::Key_F, "f", "F", "а", "А"},
            {Qt::Key_G, "g", "G", "п", "П"},
            {Qt::Key_H, "h", "H", "р", "Р"},
            {Qt::Key_J, "j", "J", "о", "О"},
            {Qt::Key_K, "k", "K", "л", "Л"},
            {Qt::Key_L, "l", "L", "д", "Д"},
            {Qt::Key_Semicolon, ";", ":", "ж", "Ж"},
            {Qt::Key_Apostrophe, "'", "\"", "э", "Э"}
        },
        {
            {Qt::Key_Z, "z", "Z", "я", "Я"},
            {Qt::Key_X, "x", "X", "ч", "Ч"},
            {Qt::Key_C, "c", "C", "с", "С"},
            {Qt::Key_V, "v", "V", "м", "М"},
            {Qt::Key_B, "b", "B", "и", "И"},
            {Qt::Key_N, "n", "N", "т", "Т"},
            {Qt::Key_M, "m", "M", "ь", "Ь"},
            {Qt::Key_Comma, ",", "<", "б", "Б"},
            {Qt::Key_Period, ".", ">", "ю", "Ю"},
            {Qt::Key_Slash, "/", "?", ".", ","}
        }
    };

    for (const auto &row : keyRows) {
        QVector<QPushButton*> btnRow;
        for (const auto &key : row) {
            auto *btn = new QPushButton(this);
            btn->setStyleSheet(m_defaultStyle);
            btn->setFocusPolicy(Qt::NoFocus);
            m_keyButtons.insert(key.qtKey, btn);
            m_keyInfos.insert(key.qtKey, key);

            m_charToKey.insert(key.enLower.toLower(), key.qtKey);
            m_charToKey.insert(key.enUpper.toLower(), key.qtKey);
            m_charToKey.insert(key.ruLower.toLower(), key.qtKey);
            m_charToKey.insert(key.ruUpper.toLower(), key.qtKey);
            btnRow.append(btn);
        }
        m_rows.append(btnRow);
    }
    m_charToKey.insert(" ", Qt::Key_Space);
}

bool MainWindow::detectRussianLayout()
{
    HKL hkl = GetKeyboardLayout(0);
    return LOWORD(hkl) == 0x0419;
}

bool MainWindow::detectShiftPressed()
{
    return QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier);
}

void MainWindow::updateKeyboardLabels()
{
    for (auto it = m_keyInfos.begin(); it != m_keyInfos.end(); ++it) {
        QPushButton *btn = m_keyButtons.value(it.key());
        if (!btn) continue;
        const KeyInfo &info = it.value();
        QString lower, upper;
        if (m_isRussian) { lower = info.ruLower; upper = info.ruUpper; }
        else             { lower = info.enLower; upper = info.enUpper; }
        if (upper == lower.toUpper() && lower.toUpper() == upper)
            btn->setText(m_isShift ? upper : lower);
        else
            btn->setText(upper + "\n" + lower);
    }
    m_layoutLabel->setText(m_isRussian ? "🇷🇺 Русская раскладка" : "🇬🇧 English layout");
}

int MainWindow::findKeyByEvent(QKeyEvent *event)
{
    int key = event->key();
    if (m_keyButtons.contains(key)) return key;
    if (!event->text().isEmpty()) {
        QString text = event->text().toLower();
        if (m_charToKey.contains(text)) return m_charToKey.value(text);
    }
    return -1;
}

void MainWindow::resetAllKeyStyles()
{
    for (auto it = m_keyButtons.begin(); it != m_keyButtons.end(); ++it) {
        if (it.key() == Qt::Key_Space)
            it.value()->setStyleSheet(m_spaceStyle);
        else
            it.value()->setStyleSheet(m_defaultStyle);
    }
    m_hintKey = -1;
}

// ---------------------------------------------------------------------------
// Подсказка следующей клавиши
// ---------------------------------------------------------------------------

void MainWindow::clearNextKeyHint()
{
    if (m_hintKey >= 0 && m_keyButtons.contains(m_hintKey)) {
        if (m_hintKey == Qt::Key_Space)
            m_keyButtons[m_hintKey]->setStyleSheet(m_spaceStyle);
        else
            m_keyButtons[m_hintKey]->setStyleSheet(m_defaultStyle);
    }
    m_hintKey = -1;
}

void MainWindow::updateNextKeyHint()
{
    clearNextKeyHint();
    if (m_currentPos >= m_targetText.length()) return;
    if (m_timedFinished) return;

    QChar nextChar = m_targetText[m_currentPos].toLower();
    QString charStr = QString(nextChar);
    if (m_charToKey.contains(charStr)) {
        int key = m_charToKey.value(charStr);
        if (m_keyButtons.contains(key)) {
            if (key == Qt::Key_Space)
                m_keyButtons[key]->setStyleSheet(m_spaceHintStyle);
            else
                m_keyButtons[key]->setStyleSheet(m_hintStyle);
            m_hintKey = key;
        }
    }
}

// ---------------------------------------------------------------------------
// Текст и режимы
// ---------------------------------------------------------------------------

void MainWindow::startNewText()
{
    m_currentPos = 0;
    m_errors = 0;
    m_totalTyped = 0;
    m_started = false;
    m_timedFinished = false;

    m_countdownTimer.stop();

    if (m_mode == Mode::Timed) {
        m_timedCorrectChars = 0;
        m_timedErrors = 0;
        m_timedTotalTyped = 0;
        m_countdownSeconds = m_selectedTime;
    }

    if (m_mode == Mode::Lessons) {
        int idx = m_lessonCombo->currentIndex();
        m_targetText = (idx >= 0 && idx < m_lessons.size()) ? m_lessons[idx].text : m_lessons[0].text;
    } else {
        int catIdx = m_categoryCombo->currentIndex();
        if (catIdx < 0) catIdx = 0;
        if (catIdx < m_categories.size()) {
            const auto &texts = m_categories[catIdx].texts;
            m_targetText = texts[QRandomGenerator::global()->bounded(texts.size())];
        } else {
            m_targetText = "привет мир";
        }
    }

    resetAllKeyStyles();
    updateTextDisplay();
    updateNextKeyHint();

    if (m_mode == Mode::Timed)
        m_statsLabel->setText(QString("Осталось: %1 сек — начинай печатать!").arg(m_countdownSeconds));
    else
        m_statsLabel->setText("Начинай печатать — счётчик запустится автоматически");
}

void MainWindow::loadNextTimedText()
{
    m_timedCorrectChars += m_currentPos;
    m_timedErrors += m_errors;
    m_timedTotalTyped += m_totalTyped;

    m_currentPos = 0;
    m_errors = 0;
    m_totalTyped = 0;

    int catIdx = m_categoryCombo->currentIndex();
    if (catIdx < 0) catIdx = 0;
    if (catIdx < m_categories.size()) {
        const auto &texts = m_categories[catIdx].texts;
        m_targetText = texts[QRandomGenerator::global()->bounded(texts.size())];
    } else {
        m_targetText = "привет мир";
    }

    resetAllKeyStyles();
    updateTextDisplay();
    updateNextKeyHint();
}

void MainWindow::updateTextDisplay()
{
    QString html;
    for (int i = 0; i < m_targetText.length(); ++i) {
        QChar ch = m_targetText[i];
        if (i < m_currentPos) {
            html += QString("<span style='color: %1;'>%2</span>")
            .arg(m_darkTheme ? "#66BB6A" : "#2E7D32")
                .arg(ch == ' ' ? "&nbsp;" : QString(ch));
        } else if (i == m_currentPos) {
            html += QString("<span style='background-color: #FFD54F; border-bottom: 2px solid #333;'>%1</span>")
            .arg(ch == ' ' ? "&nbsp;" : QString(ch));
        } else {
            html += QString("<span style='color: %1;'>%2</span>")
            .arg(m_darkTheme ? "#666" : "#999")
                .arg(ch == ' ' ? "&nbsp;" : QString(ch));
        }
    }
    m_textLabel->setText(html);
}

// ---------------------------------------------------------------------------
// Обработчики панели управления
// ---------------------------------------------------------------------------

void MainWindow::onModeChanged()
{
    m_countdownTimer.stop();
    m_timedFinished = false;
    m_mode = static_cast<Mode>(m_modeCombo->currentData().toInt());

    m_lessonCombo->setVisible(m_mode == Mode::Lessons);
    m_timedCombo->setVisible(m_mode == Mode::Timed);
    m_categoryCombo->setVisible(m_mode != Mode::Lessons);

    startNewText();
}

void MainWindow::onLessonChanged() { startNewText(); }
void MainWindow::onCategoryChanged() { startNewText(); }

void MainWindow::onTimedChanged()
{
    m_selectedTime = m_timedCombo->currentData().toInt();
    m_countdownSeconds = m_selectedTime;
    startNewText();
}

void MainWindow::onTimedTick()
{
    m_countdownSeconds--;
    if (m_countdownSeconds <= 0) {
        m_countdownTimer.stop();
        m_timedFinished = true;
        m_timedCorrectChars += m_currentPos;
        m_timedErrors += m_errors;
        m_timedTotalTyped += m_totalTyped;

        double minutes = m_selectedTime / 60.0;
        double wpm = m_timedCorrectChars / 5.0 / minutes;
        double accuracy = (m_timedTotalTyped > 0)
                              ? 100.0 * (m_timedTotalTyped - m_timedErrors) / m_timedTotalTyped : 0;

        clearNextKeyHint();

        m_statsLabel->setText(QString("⏰ Время вышло! WPM: %1 | Точность: %2% | Ошибки: %3 | Символы: %4\n"
                                      "Нажми Escape для нового раунда")
                                  .arg(wpm, 0, 'f', 1)
                                  .arg(accuracy, 0, 'f', 1)
                                  .arg(m_timedErrors)
                                  .arg(m_timedCorrectChars));

        saveResult(wpm, accuracy);
        updateBestStats();
        return;
    }

    qint64 elapsedMs = m_timer.elapsed();
    double minutes = elapsedMs / 60000.0;
    int chars = m_timedCorrectChars + m_currentPos;
    double wpm = (minutes > 0) ? (chars / 5.0 / minutes) : 0;
    int totalTyped = m_timedTotalTyped + m_totalTyped;
    int totalErrors = m_timedErrors + m_errors;
    double accuracy = (totalTyped > 0) ? 100.0 * (totalTyped - totalErrors) / totalTyped : 0;

    m_statsLabel->setText(QString("⏱ Осталось: %1 сек | WPM: %2 | Точность: %3% | Ошибки: %4")
                              .arg(m_countdownSeconds)
                              .arg(wpm, 0, 'f', 1)
                              .arg(accuracy, 0, 'f', 1)
                              .arg(totalErrors));
}

// ---------------------------------------------------------------------------
// Сохранение и загрузка результатов
// ---------------------------------------------------------------------------

void MainWindow::loadResults()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
    + "/keyboard_trainer_stats.json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_savedResults = doc.object();
        file.close();
    }
}

void MainWindow::saveResult(double wpm, double accuracy)
{
    double bestWpm = m_savedResults.value("best_wpm").toDouble();
    double bestAcc = m_savedResults.value("best_accuracy").toDouble();
    if (wpm > bestWpm) m_savedResults["best_wpm"] = wpm;
    if (accuracy > bestAcc) m_savedResults["best_accuracy"] = accuracy;

    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                   + "/keyboard_trainer_stats.json";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_savedResults).toJson());
        file.close();
    }
}

void MainWindow::updateBestStats()
{
    double bestWpm = m_savedResults.value("best_wpm").toDouble();
    double bestAcc = m_savedResults.value("best_accuracy").toDouble();
    if (bestWpm > 0 || bestAcc > 0)
        m_bestLabel->setText(QString("🏆 Рекорд: %1 WPM | Точность: %2%")
                                 .arg(bestWpm, 0, 'f', 1)
                                 .arg(bestAcc, 0, 'f', 1));
    else
        m_bestLabel->setText("");
}

// ---------------------------------------------------------------------------
// Обработка клавиш
// ---------------------------------------------------------------------------

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int key = event->key();

    if (key == Qt::Key_Escape) {
        m_countdownTimer.stop();
        m_timedFinished = false;
        startNewText();
        return;
    }

    if (m_timedFinished) return;

    if (key == Qt::Key_Backspace) {
        if (m_currentPos > 0) {
            m_currentPos--;
            updateTextDisplay();
            updateNextKeyHint();
        }
        return;
    }

    if (event->text().isEmpty() && key != Qt::Key_Space) return;

    int foundKey = findKeyByEvent(event);

    if (!m_started) {
        m_timer.start();
        m_started = true;
        if (m_mode == Mode::Timed) {
            m_countdownTimer.start(1000);
        }
    }

    if (m_currentPos >= m_targetText.length()) return;

    QChar typedChar = event->text().isEmpty() ? QChar(' ') : event->text().at(0).toLower();
    QChar targetChar = m_targetText[m_currentPos].toLower();

    m_totalTyped++;

    // Сброс подсказки перед подсветкой нажатой клавиши
    clearNextKeyHint();

    bool isCorrect = (typedChar == targetChar);

    if (foundKey >= 0 && m_keyButtons.contains(foundKey)) {
        QPushButton *btn = m_keyButtons[foundKey];
        if (isCorrect)
            btn->setStyleSheet(foundKey == Qt::Key_Space ? m_spacePressedStyle : m_pressedStyle);
        else
            btn->setStyleSheet(foundKey == Qt::Key_Space ? m_spaceWrongStyle : m_wrongStyle);
    }

    if (isCorrect) m_currentPos++;
    else            m_errors++;

    updateTextDisplay();

    if (m_currentPos >= m_targetText.length()) {
        if (m_mode == Mode::Timed) {
            loadNextTimedText();
        } else {
            qint64 elapsedMs = m_timer.elapsed();
            double minutes = elapsedMs / 60000.0;
            int words = m_targetText.split(' ', Qt::SkipEmptyParts).size();
            double wpm = (minutes > 0) ? (words / minutes) : 0;
            double accuracy = (m_totalTyped > 0)
                                  ? 100.0 * (m_totalTyped - m_errors) / m_totalTyped : 0;

            clearNextKeyHint();

            m_statsLabel->setText(QString("Готово! 🎉 WPM: %1 | Точность: %2% | Ошибки: %3 | Время: %4 сек\n"
                                          "Нажми Escape для нового текста")
                                      .arg(wpm, 0, 'f', 1)
                                      .arg(accuracy, 0, 'f', 1)
                                      .arg(m_errors)
                                      .arg(elapsedMs / 1000.0, 0, 'f', 1));

            saveResult(wpm, accuracy);
            updateBestStats();
        }
    } else {
        // Показать подсказку для следующей клавиши
        updateNextKeyHint();

        if (m_mode == Mode::Timed) {
            qint64 elapsedMs = m_timer.elapsed();
            double minutes = elapsedMs / 60000.0;
            int chars = m_timedCorrectChars + m_currentPos;
            double wpm = (minutes > 0) ? (chars / 5.0 / minutes) : 0;
            int totalTyped = m_timedTotalTyped + m_totalTyped;
            int totalErrors = m_timedErrors + m_errors;
            double accuracy = (totalTyped > 0) ? 100.0 * (totalTyped - totalErrors) / totalTyped : 0;

            m_statsLabel->setText(QString("⏱ Осталось: %1 сек | WPM: %2 | Точность: %3% | Ошибки: %4")
                                      .arg(m_countdownSeconds)
                                      .arg(wpm, 0, 'f', 1)
                                      .arg(accuracy, 0, 'f', 1)
                                      .arg(totalErrors));
        } else {
            qint64 elapsedMs = m_timer.elapsed();
            double minutes = elapsedMs / 60000.0;
            int words = m_targetText.left(m_currentPos).split(' ', Qt::SkipEmptyParts).size();
            double wpm = (minutes > 0) ? (words / minutes) : 0;
            double accuracy = (m_totalTyped > 0)
                                  ? 100.0 * (m_totalTyped - m_errors) / m_totalTyped : 0;

            m_statsLabel->setText(QString("WPM: %1 | Точность: %2% | Ошибки: %3 | Позиция: %4/%5")
                                      .arg(wpm, 0, 'f', 1)
                                      .arg(accuracy, 0, 'f', 1)
                                      .arg(m_errors)
                                      .arg(m_currentPos)
                                      .arg(m_targetText.length()));
        }
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    int foundKey = findKeyByEvent(event);
    if (foundKey < 0 || !m_keyButtons.contains(foundKey)) return;

    // Сбросить кнопку к обычному стилю
    if (foundKey == Qt::Key_Space)
        m_keyButtons[foundKey]->setStyleSheet(m_spaceStyle);
    else
        m_keyButtons[foundKey]->setStyleSheet(m_defaultStyle);

    // Если отпустили клавишу-подсказку — восстановить подсказку
    if (foundKey == m_hintKey)
        updateNextKeyHint();
}
