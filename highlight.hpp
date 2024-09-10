#ifndef C7005E20_38B5_4A1A_A4D4_D097DB950A3E
#define C7005E20_38B5_4A1A_A4D4_D097DB950A3E

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include "highlight.moc"

class DraculaCppSyntaxHighlighter : public QSyntaxHighlighter {
  Q_OBJECT
public:
  DraculaCppSyntaxHighlighter(QTextDocument* parent = nullptr);

protected:
  void highlightBlock(const QString& text) override;

private:
  struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };

  QVector<HighlightingRule> highlightingRules;

  QRegularExpression commentStartExpression;
  QRegularExpression commentEndExpression;
  QTextCharFormat multiLineCommentFormat;
};

#endif /* C7005E20_38B5_4A1A_A4D4_D097DB950A3E */
