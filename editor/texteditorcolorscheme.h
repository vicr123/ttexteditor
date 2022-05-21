#ifndef TEXTEDITORCOLORSCHEME_H
#define TEXTEDITORCOLORSCHEME_H

#include <QObject>

class TextEditorColorScheme : public QObject {
        Q_OBJECT
    public:
        explicit TextEditorColorScheme(QObject* parent = nullptr);

        enum ColorSchemeItem {
            Background = 0,
            Margin,
            NormalText,
            MarginText,
            Breakpoint,
            ActiveLine,
            ActiveLineMarginText,
            HighlightedText,
            HighlightedTextBorder,
            LineWithCompilationError,
            LineWithCompilationWarning,
        };
        QBrush item(ColorSchemeItem item);

    signals:
};

#endif // TEXTEDITORCOLORSCHEME_H
