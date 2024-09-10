#include "highlight.hpp"

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

DraculaCppSyntaxHighlighter::DraculaCppSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
  // Define C++ keywords
  QStringList keywordPatterns = {
      "\\bclass\\b",
      "\\bconst\\b",
      "\\bdouble\\b",
      "\\benum\\b",
      "\\bexplicit\\b",
      "\\bfriend\\b",
      "\\binline\\b",
      "\\bnamespace\\b",
      "\\boperator\\b",
      "\\bprivate\\b",
      "\\bprotected\\b",
      "\\bpublic\\b",
      "\\bshort\\b",
      "\\bsignals\\b",
      "\\bsigned\\b",
      "\\bslots\\b",
      "\\bstatic\\b",
      "\\bstruct\\b",
      "\\btemplate\\b",
      "\\btypedef\\b",
      "\\btypename\\b",
      "\\bunion\\b",
      "\\bunsigned\\b",
      "\\bvirtual\\b",
      "\\bvoid\\b",
      "\\bvolatile\\b",
      "\\bbool\\b",
      "\\btrue\\b",
      "\\bfalse\\b",
      "\\bfor\\b",
      "\\bif\\b",
      "\\belse\\b",
      "\\bwhile\\b",
      "\\breturn\\b",
      "\\bswitch\\b",
      "\\bcase\\b",
      "\\bdefault\\b",
      "\\bdo\\b",
      "\\bbreak\\b",
      "\\bcontinue\\b",
      "\\bgoto\\b",
      "\\btry\\b",
      "\\bcatch\\b",
      "\\bthrow\\b",
      "\\bconst_cast\\b",
      "\\bdynamic_cast\\b",
      "\\breinterpret_cast\\b",
      "\\bstatic_cast\\b",
      "\\binclude\\b",
      "\\bdefine\\b",
      "\\bifdef\\b",
      "\\bifndef\\b",
      "\\bendif\\b",
      "\\bpragma\\b",
      "\\berror\\b",
      "\\bwarning\\b",
      "\\bundef\\b",

      // Int types
      "\\bchar\\b",
      "\\bint\\b",
      "\\buint8_t\\b",
      "\\buint16_t\\b",
      "\\buint32_t\\b",
      "\\buint64_t\\b",
      "\\bint8_t\\b",
      "\\bint16_t\\b",
      "\\bint32_t\\b",
      "\\bint64_t\\b",
      "\\buint_fast8_t\\b",
      "\\buint_fast16_t\\b",
      "\\buint_fast32_t\\b",
      "\\buint_fast64_t\\b",
      "\\bint_fast8_t\\b",
      "\\bint_fast16_t\\b",
      "\\bint_fast32_t\\b",
      "\\bint_fast64_t\\b",
      "\\buint_least8_t\\b",
      "\\buint_least16_t\\b",
      "\\buint_least32_t\\b",
      "\\buint_least64_t\\b",
      "\\bint_least8_t\\b",
      "\\bint_least16_t\\b",
      "\\bint_least32_t\\b",
      "\\bint_least64_t\\b",
      "\\buintmax_t\\b",
      "\\bintmax_t\\b",
      "\\bsize_t\\b",
      "\\bptrdiff_t\\b",

      // Float types
      "\\bfloat\\b",
      "\\bdouble\\b",
      "\\blong double\\b",

      // stddef.h types
      "\\bptrdiff_t\\b",
      "\\bsize_t\\b",
      "\\bmax_align_t\\b",
      "\\bnullptr_t\\b",

  };

  // Add patterns for preprocessor directives
  QStringList preprocessorPatterns = {"#\\s*include\\b", "#\\s*define\\b", "#\\s*ifdef\\b",
                                      "#\\s*ifndef\\b",  "#\\s*else\\b",   "#\\s*elif\\b",
                                      "#\\s*endif\\b",   "#\\s*pragma\\b", "#\\s*error\\b",
                                      "#\\s*warning\\b", "#\\s*undef\\b"};

  // Formatting for keywords
  QTextCharFormat keywordFormat;
  keywordFormat.setForeground(QColor("#FF79C6")); // Pink
  keywordFormat.setFontWeight(QFont::Bold);

  // Preprocessor directives are also keywords
  for (const QString &pattern : preprocessorPatterns) {
    HighlightingRule rule;
    rule.pattern = QRegularExpression(pattern);
    rule.format  = keywordFormat;
    highlightingRules.append(rule);
  }

  // Add keyword rules
  for (const QString &pattern : keywordPatterns) {
    HighlightingRule rule;
    rule.pattern = QRegularExpression(pattern);
    rule.format  = keywordFormat;
    highlightingRules.append(rule);
  }

  // Formatting for operators
  QTextCharFormat operatorFormat;
  operatorFormat.setForeground(QColor("#FF79C6")); // Pink (for operators, different from comments)
  HighlightingRule operatorRule;
  operatorRule.pattern = QRegularExpression(
      R"(\+|\-|\*|\/|\%|\=|\==|\!=|\>|\<|\>=|\<=|\&|\||\^|\~|\!|\?|\:|\,|\;|\[|\]|\(|\)|\{|\})");
  operatorRule.format = operatorFormat;
  highlightingRules.append(operatorRule);

  // Formatting for strings
  QTextCharFormat stringFormat;
  stringFormat.setForeground(QColor("#F1FA8C")); // Yellow
  HighlightingRule stringRule;
  stringRule.pattern = QRegularExpression("\".*\"|<.*>");
  stringRule.format  = stringFormat;
  highlightingRules.append(stringRule);

  // Formatting for numbers
  QTextCharFormat numberFormat;
  numberFormat.setForeground(QColor("#BD93F9")); // Purple
  HighlightingRule numberRule;
  numberRule.pattern = QRegularExpression("\\b[0-9]+\\b");
  numberRule.format  = numberFormat;
  highlightingRules.append(numberRule);

  // Formatting for function calls
  QTextCharFormat functionFormat;
  functionFormat.setForeground(QColor("#50FA7B")); // Green
  HighlightingRule functionRule;
  functionRule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
  functionRule.format  = functionFormat;
  highlightingRules.append(functionRule);

  // Formatting for include directives
  QTextCharFormat includeFormat;
  includeFormat.setForeground(QColor("#F1FA8C")); // Yellow (same as strings)
  HighlightingRule includeRule;
  includeRule.pattern = QRegularExpression(R"(<[^>]+>|"[^"]+")");
  includeRule.format  = includeFormat;
  highlightingRules.append(includeRule);

  // Formatting for function definitions
  QTextCharFormat functionDefinitionFormat;
  functionDefinitionFormat.setForeground(QColor("#50FA7B")); // Green (same as function calls)
  HighlightingRule functionDefinitionRule;
  functionDefinitionRule.pattern = QRegularExpression(R"(\b[A-Za-z0-9_]+\s*(?=\())");
  functionDefinitionRule.format  = functionDefinitionFormat;
  highlightingRules.append(functionDefinitionRule);

  // Formatting for class definitions
  QTextCharFormat classFormat;
  classFormat.setForeground(QColor("#FF79C6")); // Pink (same as keywords)
  HighlightingRule classRule;
  classRule.pattern = QRegularExpression(R"(\bclass\s+[A-Za-z0-9_]+)");
  classRule.format  = classFormat;
  highlightingRules.append(classRule);

  // Formatting for function arguments
  QTextCharFormat argumentFormat;
  argumentFormat.setForeground(QColor("#FFB86C")); // Orange
  HighlightingRule argumentRule;
  argumentRule.pattern =
      QRegularExpression(R"(\b[A-Za-z_][A-Za-z0-9_]*\s+(?=[A-Za-z_][A-Za-z0-9_]*\s*\())");
  argumentRule.format = argumentFormat;
  highlightingRules.append(argumentRule);

  // Formatting for user-defined types

  QTextCharFormat typeFormat;
  //   Use dracula sky blue for user-defined types
  typeFormat.setForeground(QColor("#8BE9FD")); // Sky Blue
  HighlightingRule typeRule;

  // Regular expression to match user-defined types within a scope
  typeRule.pattern =
      QRegularExpression(R"(\b[A-Za-z_][A-Za-z0-9_]*(?=\s+[A-Za-z_][A-Za-z0-9_]*\s*(?:=|;|\(|,)))");
  typeRule.format = typeFormat;
  highlightingRules.append(typeRule);

  // Formatting for namespaces
  QTextCharFormat namespaceFormat;
  namespaceFormat.setForeground(QColor("#BD93F9")); // Purple
  HighlightingRule namespaceRule;
  namespaceRule.pattern = QRegularExpression("\\bnamespace\\s+[A-Za-z_][A-Za-z0-9_]*");
  namespaceRule.format  = namespaceFormat;
  highlightingRules.append(namespaceRule);

  // Formatting for template parameters
  QTextCharFormat templateParameterFormat;
  templateParameterFormat.setForeground(QColor("#FFB86C")); // Orange
  HighlightingRule templateParameterRule;
  templateParameterRule.pattern = QRegularExpression(R"(\btemplate\s*<[^>]+>)");
  templateParameterRule.format  = templateParameterFormat;
  highlightingRules.append(templateParameterRule);

  // Formatting for single-line comments
  QTextCharFormat singleLineCommentFormat;
  singleLineCommentFormat.setForeground(QColor("#6272A4")); // Comment Blue
  HighlightingRule singleLineCommentRule;
  singleLineCommentRule.pattern = QRegularExpression("//[^\n]*");
  singleLineCommentRule.format  = singleLineCommentFormat;
  highlightingRules.append(singleLineCommentRule);

  // Formatting for multi-line comments as gray text
  QTextCharFormat multiLineCommentFormat;
  multiLineCommentFormat.setForeground(Qt::gray); // Comment Blue
  commentStartExpression = QRegularExpression("/\\*");
  commentEndExpression   = QRegularExpression("\\*/");
}

void DraculaCppSyntaxHighlighter::highlightBlock(const QString &text) {
  for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }

  // Handle multi-line comments
  setCurrentBlockState(0);

  int startIndex = 0;

  // Check if the previous block ended with a multi-line comment
  if (previousBlockState() != 1) {
    startIndex = text.indexOf(commentStartExpression);
  }

  // Iterate through the entire block of text to find multi-line comments
  while (startIndex >= 0) {
    // Find the end of the multi-line comment
    QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
    int endIndex                  = match.capturedStart();
    int commentLength             = 0;

    if (endIndex == -1) {
      // No end expression found, comment continues to the next block
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex; // Extend to the end of the block
    } else {
      // End expression found
      commentLength = endIndex - startIndex + match.capturedLength();
    }

    setFormat(startIndex, commentLength, multiLineCommentFormat);
    // Find the next comment start expression in the text after the current
    // comment
    startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
  }
}
