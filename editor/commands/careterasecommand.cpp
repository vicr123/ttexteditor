#include "careterasecommand.h"

#include "../textcaret.h"
#include "../texteditor.h"
#include "../texteditor_p.h"

CaretEraseCommand::CaretEraseCommand(TextEditor* editor, bool backwardDelete) :
    TextEditorCommand(editor) {
    // TODO: Check if there are anchors set and delete anchored text only only if so
    for (int i = 0; i < editor->d->carets.length(); i++) {
        TextCaret* caret = editor->d->carets.at(i);

        if (caret->firstAnchor() != caret->lastAnchor()) {
            // Delete the text in the anchor
            this->pushCaretSelectionEraseCommand(i);
        } else {
            QPoint caretPos = caret->linePos();
            if (caretPos.x() == 0 && caretPos.y() == 0) {
                // Caret is blocked from moving backwards at beginning of document
            } else if (caretPos.x() == 0 && backwardDelete) {
                // Erase the line
                this->pushEditorCommand({i, "\n", false, !backwardDelete});
            } else if (caretPos.x() == editor->d->lines.at(caretPos.y())->contents.length() && !backwardDelete) {
                // Erase the next line
                this->pushEditorCommand({i, "\n", false, !backwardDelete});
            } else {
                this->pushEditorCommand({i, editor->d->lines.at(caretPos.y())->contents.at(caretPos.x() + (backwardDelete ? -1 : 0)), false, !backwardDelete});
            }
        }
    }
}

CaretEraseCommand::~CaretEraseCommand() {
}

int CaretEraseCommand::id() const {
    return 2;
}
