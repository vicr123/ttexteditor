#include "textcaret.h"

#include "texteditor.h"
#include "texteditor_p.h"
#include "texteditorcolorscheme.h"

#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <tvariantanimation.h>

struct TextCaretPrivate {
        TextEditor* editor;
        int line;
        int pos;
        int anchor;

        bool isPrimary = false;

        tVariantAnimation* anim;

        static tVariantAnimation* caretBlinkTimer;
        static QRegularExpression newLineSplit;
};

tVariantAnimation* TextCaretPrivate::caretBlinkTimer = nullptr;
QRegularExpression TextCaretPrivate::newLineSplit = QRegularExpression("\\r\\n|\\r|\\n");

TextCaret::TextCaret(int line, int pos, TextEditor* parent) :
    QObject{parent} {
    d = new TextCaretPrivate();
    d->editor = parent;
    d->anchor = 0;

    if (d->caretBlinkTimer == nullptr) {
        d->caretBlinkTimer = new tVariantAnimation();
        d->caretBlinkTimer->setStartValue(1.0);
        d->caretBlinkTimer->setKeyValueAt(0.5, 0.0);
        d->caretBlinkTimer->setEndValue(1.0);
        d->caretBlinkTimer->setDuration(1000);
        d->caretBlinkTimer->setForceAnimation(true);
        d->caretBlinkTimer->setLoopCount(-1);
        d->caretBlinkTimer->start();
    }

    connect(d->caretBlinkTimer, &tVariantAnimation::valueChanged, this, [=] {
        d->editor->update(d->anim->currentValue().toRect());
    });

    d->anim = new tVariantAnimation();
    d->anim->setDuration(50);
    d->anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->anim, &tVariantAnimation::valueChanged, parent, QOverload<>::of(&TextEditor::update));

    moveCaret(line, pos);
    d->anim->stop();
    d->anim->setStartValue(d->anim->endValue());

    //    parent->installEventFilter(this);
}

TextCaret::~TextCaret() {
    delete d;
}

TextCaret* TextCaret::fromSavedCaret(SavedCaret caret) {
    TextCaret* c = new TextCaret(caret.line, caret.pos, caret.parent);
    c->loadCaret(caret);
    return c;
}

TextCaret::SavedCaret TextCaret::saveCaret() {
    SavedCaret saved;
    saved.parent = d->editor;
    saved.line = d->line;
    saved.pos = d->pos;
    saved.anchor = d->anchor;
    return saved;
}

void TextCaret::loadCaret(SavedCaret caret) {
    // Not a caret for this editor
    if (caret.parent != d->editor) return;
    d->line = caret.line;
    d->pos = caret.pos;
    d->anchor = caret.anchor;
}

void TextCaret::moveCaret(int line, int pos) {
    d->line = line;
    d->pos = pos;
    d->anchor = 0;

    QString lineContents = d->editor->d->lines.at(line)->contents;

    QRect caretRect;
    caretRect.moveTop(d->editor->lineTop(line));
    caretRect.moveLeft(d->editor->leftMarginWidth() + d->editor->fontMetrics().horizontalAdvance(lineContents.left(pos)));
    caretRect.setHeight(d->editor->lineHeight(line));
    caretRect.setWidth(SC_DPI_W(1, d->editor));
    moveCaret(caretRect);
}

void TextCaret::moveCaret(QPoint linePos) {
    moveCaret(linePos.y(), linePos.x());
}

void TextCaret::moveCaretRelative(int lines, int cols) {
    int newLine = d->line + lines;
    //    int newCol = d->pos + cols;

    if (newLine < 0) newLine = 0;
    if (newLine >= d->editor->d->lines.length()) newLine = d->editor->d->lines.length() - 1;

    moveCaret(d->editor->charToLinePos(d->editor->linePosToChar(QPoint(d->pos, newLine)) + cols));
}

void TextCaret::moveCaretToStartOfLine() {
    moveCaret(d->line, 0);
}

void TextCaret::moveCaretToEndOfLine() {
    moveCaret(d->line, d->editor->d->lines.at(d->line)->contents.length());
}

void TextCaret::setAnchor(int line, int pos) {
    int c = d->editor->linePosToChar(QPoint(d->pos, d->line));
    d->anchor = d->editor->linePosToChar(QPoint(pos, line)) - c;
}

void TextCaret::setAnchor(QPoint linePos) {
    setAnchor(linePos.y(), linePos.x());
}

void TextCaret::moveAnchorRelative(int lines, int cols) {
    int c = d->editor->linePosToChar(QPoint(d->pos, d->line));
    QPoint anchorPos = d->editor->charToLinePos(c + d->anchor);
    int newAnchorC = d->editor->linePosToChar(anchorPos + QPoint(cols, lines));
    d->anchor = newAnchorC - c;
}

QPoint TextCaret::firstAnchor() {
    if (d->anchor < 0) {
        return d->editor->charToLinePos(d->editor->linePosToChar(QPoint(d->pos, d->line)) + d->anchor);
    } else {
        return QPoint(d->pos, d->line);
    }
}

QPoint TextCaret::lastAnchor() {
    if (d->anchor > 0) {
        return d->editor->charToLinePos(d->editor->linePosToChar(QPoint(d->pos, d->line)) + d->anchor);
    } else {
        return QPoint(d->pos, d->line);
    }
}

void TextCaret::drawCaret(QPainter* painter) {
    painter->save();
    QRect rect = d->anim->currentValue().toRect();
    rect.moveTopLeft(rect.topLeft() - QPoint(d->editor->d->hScrollBar->value(), d->editor->d->vScrollBar->value()));

    painter->setOpacity(d->caretBlinkTimer->currentValue().toReal());
    painter->fillRect(rect, d->editor->colorScheme()->item(TextEditorColorScheme::NormalText));
    painter->restore();

    if (d->anchor != 0) {
        painter->save();
        // Draw the anchor
        QPolygon anchorPoly;
        int firstAnchor = d->editor->linePosToChar(this->firstAnchor());
        int lastAnchor = d->editor->linePosToChar(this->lastAnchor());
        for (int i = firstAnchor; i < lastAnchor; i++) {
            anchorPoly = anchorPoly.united(d->editor->characterRect(d->editor->charToLinePos(i)));
        }
        painter->setBrush(d->editor->colorScheme()->item(TextEditorColorScheme::HighlightedText));
        painter->setPen(d->editor->colorScheme()->item(TextEditorColorScheme::HighlightedTextBorder).color());
        painter->drawPolygon(anchorPoly);
        painter->restore();
    }
}

void TextCaret::setIsPrimary(bool primary) {
    d->isPrimary = primary;
}

bool TextCaret::isPrimary() {
    return d->isPrimary;
}

void TextCaret::insertText(QString text) {
    // Mutate the text editor's text and update the carets
    TextEditorPrivate::Line* line = d->editor->d->lines.at(d->line);

    QStringList splitLines = text.split(d->newLineSplit);
    QString firstLineText = splitLines.takeFirst();

    line->contents = line->contents.insert(d->pos, firstLineText);

    // Move each affected caret
    for (TextCaret* caret : d->editor->d->carets) {
        if (caret->d->line == d->line && caret->d->pos >= d->pos) {
            caret->moveCaret(caret->d->line, caret->d->pos + firstLineText.length());
        }
    }

    // Remove the rest of the line while handing new lines
    QString restOfLine = line->contents.mid(d->pos);
    line->contents.truncate(d->pos);

    // Handle new lines
    while (!splitLines.isEmpty()) {
        TextEditorPrivate::Line* newLine = new TextEditorPrivate::Line();
        newLine->contents = splitLines.takeFirst();
        d->editor->d->lines.insert(d->line + 1, newLine);

        for (TextCaret* caret : d->editor->d->carets) {
            if (caret->d->line > d->line) {
                caret->moveCaret(caret->d->line + 1, caret->d->pos);
            }
        }
        this->moveCaret(d->line + 1, newLine->contents.length());
    }

    d->editor->d->lines.at(d->line)->contents.append(restOfLine);
}

void TextCaret::backspace() {
    if (d->pos == 0) {
        if (d->line == 0) return; // Can't backspace behind first line

        TextEditorPrivate::Line* lineToRemove = d->editor->d->lines.at(d->line);
        TextEditorPrivate::Line* lineToCombineTo = d->editor->d->lines.at(d->line - 1);
        int newCaretPos = lineToCombineTo->contents.length();
        lineToCombineTo->contents += lineToRemove->contents;

        int currentLine = d->line;
        for (TextCaret* caret : d->editor->d->carets) {
            if (caret->d->line > currentLine) {
                caret->moveCaret(caret->d->line - 1, caret->d->pos);
            } else if (caret->d->line == currentLine) {
                caret->moveCaret(caret->d->line - 1, newCaretPos + caret->d->pos);
            }
        }

        d->editor->d->lines.removeAll(lineToRemove);
    } else {
        TextEditorPrivate::Line* line = d->editor->d->lines.at(d->line);
        line->contents.remove(d->pos - 1, 1);

        for (TextCaret* caret : d->editor->d->carets) {
            if (caret->d->line == d->line && caret->d->pos >= d->pos) {
                caret->moveCaret(caret->d->line, caret->d->pos - 1);
            }
        }
    }
}

QPoint TextCaret::linePos() {
    return QPoint(d->pos, d->line);
}

void TextCaret::discontinueAndDelete() {
    emit discontinued();
    this->deleteLater();
}

void TextCaret::moveCaret(QRect newPos) {
    d->anim->stop();
    d->anim->setStartValue(d->anim->currentValue());
    d->anim->setEndValue(newPos);
    d->anim->start();

    d->caretBlinkTimer->stop();
    d->caretBlinkTimer->setCurrentTime(0);
    d->caretBlinkTimer->start();

    d->editor->simplifyCarets();
}

bool TextCaret::eventFilter(QObject* watched, QEvent* event) {
    if (watched == d->editor) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            Qt::KeyboardModifiers modifiers = keyEvent->modifiers() & ~Qt::KeypadModifier;

            if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
                if (keyEvent->key() == Qt::Key_Backspace) {
                    backspace();
                } else if (!keyEvent->text().isEmpty()) {
                    insertText(keyEvent->text());
                }
            }

            if (modifiers == Qt::NoModifier) {
                if (keyEvent->key() == Qt::Key_Up) {
                    moveCaretRelative(-1, 0);
                } else if (keyEvent->key() == Qt::Key_Down) {
                    moveCaretRelative(1, 0);
                } else if (keyEvent->key() == Qt::Key_Left) {
                    moveCaretRelative(0, -1);
                } else if (keyEvent->key() == Qt::Key_Right) {
                    moveCaretRelative(0, 1);
                }
            }

            d->editor->update();
            return false;
        }
    }
    return QObject::eventFilter(watched, event);
}