/**
 * @file console_panel.h
 * @brief Console panel for firmware log output
 */

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QMenu>
#include <QTimer>
#include <deque>
#include <string>

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace esp32sim {

class SimulationEngine;

/**
 * @class ConsolePanel
 * @brief Console widget displaying firmware ESP_LOG output
 *
 * Features:
 * - Real-time log display with color coding
 * - Filter by log level
 * - Search functionality
 * - Timestamp display
 * - Copy/paste support
 * - Save to file
 * - Auto-scroll
 */
class ConsolePanel : public QWidget {
    Q_OBJECT

public:
    explicit ConsolePanel(SimulationEngine* engine, QWidget* parent = nullptr);
    ~ConsolePanel() override;

    /**
     * @brief Append a log message
     */
    void appendLog(const std::string& tag, int level, const std::string& message);

    /**
     * @brief Clear all logs
     */
    void clear();

    /**
     * @brief Get all log text
     */
    std::string allText() const;

    /**
     * @brief Set maximum log lines (0 = unlimited)
     */
    void setMaxLines(uint32_t max_lines) { max_lines_ = max_lines; }

    /**
     * @brief Save logs to file
     */
    bool saveToFile(const std::string& filename) const;

public slots:
    /**
     * @brief Handle new log from engine
     */
    void onLogMessage(const QString& tag, int level, const QString& message);

    /**
     * @brief Clear console
     */
    void clearConsole();

    /**
     * @brief Toggle auto-scroll
     */
    void setAutoScroll(bool enabled) { auto_scroll_ = enabled; }

    /**
     * @brief Set log level filter
     */
    void setLogLevelFilter(int level);

private slots:
    /**
     * @brief Search text changed
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Filter changed
     */
    void onFilterChanged(int index);

    /**
     * @brief Copy selected
     */
    void copySelected();

    /**
     * @brief Select all
     */
    void selectAll();

    /**
     * @brief Save to file
     */
    void saveToFile();

private:
    SimulationEngine* engine_ = nullptr;

    // Main text display
    QTextEdit* text_edit_ = nullptr;

    // Controls
    QPushButton* clear_button_ = nullptr;
    QPushButton* save_button_ = nullptr;
    QCheckBox* auto_scroll_check_ = nullptr;
    QComboBox* level_filter_combo_ = nullptr;
    QLineEdit* search_edit_ = nullptr;
    QCheckBox* show_timestamps_check_ = nullptr;

    // Log storage
    struct LogEntry {
        QString tag;
        int level;  // ESP_LOG_* level
        QString message;
        QString timestamp;
    };
    std::deque<LogEntry> log_buffer_;
    uint32_t max_lines_ = 10000;

    // Filtering
    int min_level_ = 0;  // 0=VERBOSE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR
    QString search_filter_;

    // Color scheme
    struct LogColors {
        QColor error;
        QColor warn;
        QColor info;
        QColor debug;
        QColor verbose;
        QColor timestamp;
        QColor tag;
    } colors_;

    // Menu
    QMenu* context_menu_ = nullptr;

    // Internal methods
    void setupColors();
    QString formatLogEntry(const LogEntry& entry) const;
    bool matchesFilter(const LogEntry& entry) const;
    void trimBuffer();
    void appendEntry(const LogEntry& entry);
    void contextMenuEvent(QContextMenuEvent* event);
    void createContextMenu();
    void updateDisplay();

    /**
     * @brief ESP log level constants
     */
    static constexpr int ESP_LOG_VERBOSE = 0;
    static constexpr int ESP_LOG_DEBUG = 1;
    static constexpr int ESP_LOG_INFO = 2;
    static constexpr int ESP_LOG_WARN = 3;
    static constexpr int ESP_LOG_ERROR = 4;
    static constexpr int ESP_LOG_NONE = 5;
};

/**
 * @brief Console text highlighter with regex-based coloring
 */
class ConsoleHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit ConsoleHighlighter(QTextDocument* parent = nullptr);

protected:
    void highlightBlock(const QString& text) override;

private:
    QTextCharFormat error_format_;
    QTextCharFormat warn_format_;
    QTextCharFormat info_format_;
    QTextCharFormat debug_format_;
    QTextCharFormat timestamp_format_;
    QTextCharFormat tag_format_;
};

} // namespace esp32sim
