#include "editor.hpp"

#include <QFont>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

AutoIndentTextEdit::AutoIndentTextEdit(QWidget *parent) : QTextEdit(parent) {
  setCursorWidth(2);

  // highlight current line when cursor position changes
  connect(this, &AutoIndentTextEdit::cursorPositionChanged, this,
          &AutoIndentTextEdit::highlightCurrentLine);

  QFont font = this->font();
  font.setFixedPitch(true);
  font.setWeight(QFont::Normal);
  this->setFont(font);

  QFontMetrics metrics(font);
  setTabStopDistance(2 * metrics.horizontalAdvance(' '));

  // Apply Dracula theme colors
  QPalette p = palette();

  // Background color (Dracula: #282a36)
  p.setColor(QPalette::Base, QColor("#282a36"));

  // Text color (Dracula: #f8f8f2)
  p.setColor(QPalette::Text, QColor("#f8f8f2"));

  // Cursor color (optional)
  p.setColor(QPalette::Highlight, QColor("#44475a"));
  p.setColor(QPalette::HighlightedText, QColor("#f8f8f2"));

  // Apply the palette to the text editor
  setPalette(p);

  // Set additional styles via stylesheet
  setStyleSheet(
      "QTextEdit {"
      "  background-color: #282a36;"
      "  color: #f8f8f2;"
      "  selection-background-color: #44475a;"
      "  selection-color: #f8f8f2;"
      "}");

  // Optionally, configure scrollbar styles to match the theme
  verticalScrollBar()->setStyleSheet(
      "QScrollBar:vertical {"
      "  background-color: #282a36;"
      "  width: 15px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background-color: #6272a4;"
      "  min-height: 20px;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  background-color: #282a36;"
      "}");

  horizontalScrollBar()->setStyleSheet(
      "QScrollBar:horizontal {"
      "  background-color: #282a36;"
      "  height: 15px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background-color: #6272a4;"
      "  min-width: 20px;"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  background-color: #282a36;"
      "}");

  setPalette(p);

  // enable wheel zoom

  wordList = {"auto",      "break",    "case",    "char",      "class",    "const",    "continue",
              "default",   "delete",   "do",      "double",    "else",     "enum",     "extern",
              "float",     "for",      "goto",    "if",        "inline",   "int",      "long",
              "namespace", "operator", "private", "protected", "public",   "return",   "short",
              "signed",    "sizeof",   "static",  "struct",    "switch",   "template", "this",
              "throw",     "try",      "typedef", "union",     "unsigned", "virtual",  "void",
              "volatile",  "while"};

  completerSetup();
}

void AutoIndentTextEdit::wheelEvent(QWheelEvent *event) {
  if (event->modifiers() == Qt::ControlModifier) {
    // Zoom in or out depending on the direction of the wheel scroll
    if (event->angleDelta().y() > 0) {
      // Zoom in
      QFont font = this->font();
      font.setPointSize(font.pointSize() + 1);
      this->setFont(font);
    } else {
      // Zoom out
      QFont font = this->font();
      if (font.pointSize() > 1) { // Ensure font size doesn't go below 1
        font.setPointSize(font.pointSize() - 1);
      }
      this->setFont(font);
    }
  } else {
    // Default behavior (scrolling)
    QTextEdit::wheelEvent(event);
  }
}

void AutoIndentTextEdit::setCompleter(QCompleter *c) {
  if (completer) QObject::disconnect(completer, nullptr, this, nullptr);

  completer = c;
  if (!completer) return;

  completer->setWidget(this);
  completer->setCompletionMode(QCompleter::PopupCompletion);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  connect(completer, QOverload<const QString &>::of(&QCompleter::activated), this,
          &AutoIndentTextEdit::insertCompletion);
}

QCompleter *AutoIndentTextEdit::getCompleter() const { return completer; }

void AutoIndentTextEdit::insertCompletion(const QString &completion) {
  QTextCursor cursor = textCursor();
  cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                      completer->completionPrefix().length());
  cursor.insertText(completion);
  setTextCursor(cursor);
}

void AutoIndentTextEdit::setHighlighter(DraculaCppSyntaxHighlighter *highlighter) {
  this->highlighter = highlighter;
}

void AutoIndentTextEdit::keyPressEvent(QKeyEvent *event) {
  if (completer && completer->popup()->isVisible()) {
    switch (event->key()) {
      case Qt::Key_Enter:
      case Qt::Key_Return:
      case Qt::Key_Escape:
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
        event->ignore();
        return; // Let completer handle these keys
      default:
        break;
    }
  }

  if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
    QTextCursor cursor = textCursor();

    // Move cursor to the previous line to get its leading whitespace
    cursor.movePosition(QTextCursor::PreviousBlock);
    QString prevLine = cursor.block().text();

    // Find leading whitespace
    int leadingWhitespace = 0;
    while (leadingWhitespace < prevLine.size() && prevLine[leadingWhitespace].isSpace()) {
      ++leadingWhitespace;
    }

    // Insert the new line with the same leading whitespace
    cursor.movePosition(QTextCursor::NextBlock); // Go back to the current line
    QTextEdit::keyPressEvent(event);             // Process the Enter key press to insert a new line

    cursor = textCursor();                               // Update cursor after the new line
    cursor.insertText(prevLine.left(leadingWhitespace)); // Add leading whitespace
    setTextCursor(cursor); // Ensure the cursor is restored to the right position
                           //  Ctrl + / to comment or uncomment the current line
  } else if (event->key() == Qt::Key_Tab) {
    QTextCursor cursor = textCursor();
    cursor.insertText("  "); // Insert two spaces for tab
  } else if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_Slash) {
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QString line = cursor.selectedText();

    if (line.trimmed().startsWith("//")) {
      // Uncomment the line
      int commentPos = line.indexOf("//");
      cursor.setPosition(cursor.block().position() + commentPos);

      // find the first non-space character after the comment
      int nonSpace = commentPos + 2;
      while (nonSpace < line.length() && line[nonSpace].isSpace()) {
        nonSpace++;
      }
      cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                          nonSpace - commentPos);
      cursor.removeSelectedText();
      if (cursor.selectedText() == " ") {
        cursor.deleteChar(); // Remove extra space if present
      }
    } else {
      // Comment the line
      int nonSpace = 0;
      while (nonSpace < line.length() && line[nonSpace].isSpace()) {
        nonSpace++;
      }
      cursor.setPosition(cursor.block().position() + nonSpace);
      cursor.insertText("// ");
    }

    setTextCursor(cursor);
    rehighlightCurrentLine();
  } else {
    QTextEdit::keyPressEvent(event);
  }

  // Trigger completer after relevant keystrokes:
  if (event->text().length() >= 1) {
    QString completionPrefix = wordUnderCursor();
    if (!completionPrefix.isEmpty()) {
      completer->setCompletionPrefix(completionPrefix);

      // Only show the popup if it isn't already visible
      if (!completer->popup()->isVisible()) {
        QRect rect = cursorRect();

        // Adjust the width of the popup to fit the completions
        int popupWidth = completer->popup()->sizeHintForColumn(0) +
                         completer->popup()->verticalScrollBar()->sizeHint().width();

        rect.setWidth(popupWidth);

        // Complete at the cursor position
        completer->complete(rect);
      }

      // If only one completion is available, auto-select it
      if (completer->completionCount() == 1) {
        completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
      }
    }
  } else {
    completer->popup()->hide(); // Hide popup if there's no input
  }

  // if the user has type at least one character, show the completer
  if (event->text().isEmpty()) {
    return;
  }

  QString completionPrefix = wordUnderCursor();
  if (completionPrefix.isEmpty() || event->text().isEmpty()) {
    completer->popup()->hide();
    return;
  }

  if (completionPrefix != completer->completionPrefix()) {
    completer->setCompletionPrefix(completionPrefix);
    completer->popup()->setCurrentIndex(completer->completionModel()->index(0, 0));
  }

  QRect rect = cursorRect();
  rect.setWidth(completer->popup()->sizeHintForColumn(0) +
                completer->popup()->verticalScrollBar()->sizeHint().width());
  completer->complete(rect);
}

// Extract the word under the cursor for completion
QString AutoIndentTextEdit::wordUnderCursor() const {
  QTextCursor cursor = textCursor();
  cursor.select(QTextCursor::WordUnderCursor);
  return cursor.selectedText();
}

void AutoIndentTextEdit::highlightCurrentLine() {
  QList<QTextEdit::ExtraSelection> extraSelections;

  if (!isReadOnly()) {
    QTextEdit::ExtraSelection selection;

    auto lineColor = QColor("#44475a"); // Dracula selection color
    selection.format.setBackground(lineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
  }

  setExtraSelections(extraSelections);
}

void AutoIndentTextEdit::rehighlightCurrentLine() {
  if (highlighter) {
    highlighter->rehighlightBlock(textCursor().block());
  }
  viewport()->update(); // Force a repaint of the viewport
}

void AutoIndentTextEdit::completerSetup() {
  completerModel = new QStringListModel(this);

  completerModel->setStringList(wordList);
  completer = new QCompleter(this);
  completer->setModel(completerModel);
  completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  completer->setWrapAround(false);
  setCompleter(completer);

  // set the same font as the text editor
  QFont font = this->font();
  font.setPointSize(14);
  font.setFamily("Monospace");
  completer->popup()->setFont(font);
  completer->popup()->setStyleSheet(
      "QListView {"
      "  background-color: #282a22;"
      "  color: #f8f8f8;"
      "  selection-background-color: #44475a;"
      "  selection-color: #f8f8f2;"
      "  border: 1px solid #44475a;"
      "}");
  //   set minimum height of the popup
  completer->popup()->setMinimumHeight(50);
}
