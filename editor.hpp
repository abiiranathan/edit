#ifndef CC32F664_04E2_4834_BEE3_A6150554AA1D
#define CC32F664_04E2_4834_BEE3_A6150554AA1D

#include <QAbstractItemView>
#include <QCompleter>
#include <QStringListModel>
#include <QTextEdit>

#include "autoindenttextedit.moc"
#include "highlight.hpp"

class AutoIndentTextEdit : public QTextEdit {
  Q_OBJECT

public:
  explicit AutoIndentTextEdit(QWidget *parent = nullptr);
  void setHighlighter(DraculaCppSyntaxHighlighter *highlighter);
  void setCompleter(QCompleter *completer);
  [[nodiscard]] QCompleter *getCompleter() const;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private slots:
  void highlightCurrentLine();

private slots:
  // Insert the selected completion into the text editor
  void insertCompletion(const QString &completion);

private:
  QCompleter *completer                    = nullptr;
  QStringListModel *completerModel         = nullptr;
  QStringList wordList                     = QStringList{};
  DraculaCppSyntaxHighlighter *highlighter = nullptr;
  void rehighlightCurrentLine();

  // completer
  void completerSetup();
  [[nodiscard]] QString wordUnderCursor() const;
};

#endif /* CC32F664_04E2_4834_BEE3_A6150554AA1D */
