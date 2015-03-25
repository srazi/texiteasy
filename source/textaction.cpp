#include "textaction.h"
#include "widgetfile.h"
#include "widgettextedit.h"
#include "completionengine.h"
#include "blockdata.h"
#include "syntaxhighlighter.h"
#include "filemanager.h"
#include "mainwindow.h"

#include <QDebug>
#include <QTextCursor>


QVector<AbstractTextAction*> TextActions::_textActions = QVector<AbstractTextAction*>()
        << new CustomCommandTextAction()
        << new RefLinkTextAction()
        << new CiteLinkTextAction();


QString escapeStringForRegex(const QString &in)
{
    QString out = in;
    return out.replace(QRegExp("([\\-\\[\\]\\/\\{\\}\\(\\)\\*\\+\\?\\.\\\\\^\\$\\|])"), "\\\\1");
}


QTextCursor TextActions::match(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    Q_ASSERT_X(TextActions::_textActions.size(), "TextActions::match", "the size of TextActions::_textActions.size() must not be null");
    foreach(AbstractTextAction * action, TextActions::_textActions)
    {
        QTextCursor cursor = action->match(clickCursor, widgetFile);
        if(!cursor.isNull())
        {
            return cursor;
        }
    }
    return QTextCursor();
}

bool TextActions::execute(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    Q_ASSERT_X(TextActions::_textActions.size(), "TextActions::execute", "the size of TextActions::_textActions.size() must not be null");

    foreach(AbstractTextAction * action, TextActions::_textActions)
    {
        QTextCursor cursor = action->match(clickCursor, widgetFile);
        if(!cursor.isNull())
        {
            return action->execute(clickCursor, widgetFile);
        }
    }
    return false;
}


CustomCommandTextAction::CustomCommandTextAction()
{
}


bool CustomCommandTextAction::execute(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    QTextCursor commandCursor = this->match(clickCursor, widgetFile);
    if(commandCursor.isNull())
    {
        return false;
    }
    QString command = commandCursor.selectedText();
    foreach(const QString &word, widgetFile->widgetTextEdit()->completionEngine()->customWords())
    {
        if(word == command)
        {
            QString newCommand = "\\\\newcommand\\{\\"+command+"\\}";
            QTextCursor findResult = widgetFile->widgetTextEdit2()->document()->find(QRegExp(newCommand));
            if(!findResult.isNull())
            {
                widgetFile->widgetTextEdit2()->setTextCursor(findResult);
                widgetFile->splitEditor(true);
                widgetFile->widgetTextEdit2()->ensureCursorVisible();
                return true;
            }
        }
    }
    return false;
}

QTextCursor CustomCommandTextAction::match(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    QTextBlock block = clickCursor.block();
    BlockData *data = static_cast<BlockData *>( block.userData() );
    if(!data && data->characterData.size() <= clickCursor.positionInBlock())
    {
        return QTextCursor();
    }
    CharacterData charData = data->characterData.at(clickCursor.positionInBlock());
    if(charData.state != SyntaxHighlighter::Command)
    {
        return QTextCursor();
    }

    int left, right;
    left = right = clickCursor.position();
    while(widgetFile->widgetTextEdit()->nextChar(clickCursor) != '\\')
    {
        --left;
        clickCursor.setPosition(left, QTextCursor::MoveAnchor);
    }
    clickCursor.setPosition(right, QTextCursor::MoveAnchor);
    while(QString(widgetFile->widgetTextEdit()->nextChar(clickCursor)).contains(QRegExp("[a-zA-Z0-9*]")))
    {
        ++right;
        clickCursor.setPosition(right, QTextCursor::MoveAnchor);
    }
    clickCursor.setPosition(left, QTextCursor::KeepAnchor);
    QString command = clickCursor.selectedText();
    widgetFile->widgetTextEdit()->updateCompletionCustomWords();
    foreach(const QString &word, widgetFile->widgetTextEdit()->completionEngine()->customWords())
    {
        if(word == command)
        {
            return clickCursor;
        }
    }
    return QTextCursor();
}


/* RefLink Text Action */


RefLinkTextAction::RefLinkTextAction()
{
}


bool RefLinkTextAction::execute(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    QTextCursor commandCursor = this->match(clickCursor, widgetFile);
    if(commandCursor.isNull())
    {
        return false;
    }
    QString label = commandCursor.selectedText();
    QString labelCommand = "\\\\label[ ]*\\{"+escapeStringForRegex(label)+"\\}";
    QTextCursor findResult = widgetFile->widgetTextEdit2()->document()->find(QRegExp(labelCommand));
    if(!findResult.isNull())
    {
        widgetFile->widgetTextEdit2()->setTextCursor(findResult);
        widgetFile->splitEditor(true);
        widgetFile->widgetTextEdit2()->ensureCursorVisible();
        return true;
    }
    return false;
}

QTextCursor RefLinkTextAction::match(QTextCursor clickCursor, WidgetFile *widgetFile)
{
     QTextBlock block = clickCursor.block();
     BlockData *data = static_cast<BlockData *>( block.userData() );
     if(!data && data->characterData.size() <= clickCursor.positionInBlock())
     {
         return QTextCursor();
     }

     CharacterData charData = data->characterData.at(clickCursor.positionInBlock());
     if(charData.state != SyntaxHighlighter::Other && charData.state != SyntaxHighlighter::Command)
     {
         return QTextCursor();
     }
    int left, right;
    left = clickCursor.positionInBlock();

    while(left < data->characterData.size() && data->characterData.at(left).state == SyntaxHighlighter::Command)
    {
        ++left;
    }
    clickCursor.setPosition(left+block.position());
    while(left < data->characterData.size() && (widgetFile->widgetTextEdit()->nextChar(clickCursor) == ' ' || widgetFile->widgetTextEdit()->nextChar(clickCursor) == '\t'))
    {
        ++left;
        clickCursor.setPosition(left+block.position());
    }
    right = left;

    if(data->characterData.at(left).state != SyntaxHighlighter::Other)
    {
        return QTextCursor();
    }


    while(left >= 0 && data->characterData.at(left).state == SyntaxHighlighter::Other)
    {
        --left;
    }
    if(data->characterData.at(left).state != SyntaxHighlighter::Other)
    {
        ++left;
    }
    while(right < data->characterData.size() && data->characterData.at(right).state == SyntaxHighlighter::Other)
    {
        ++right;
    }
    if(data->characterData.at(right).state != SyntaxHighlighter::Other)
    {
        --right;
    }


    int commandLeft = left - 1;
    clickCursor.setPosition(commandLeft+block.position());
    while(commandLeft >= 0 && (widgetFile->widgetTextEdit()->nextChar(clickCursor) == ' ' || widgetFile->widgetTextEdit()->nextChar(clickCursor) == '\t'))
    {
        --commandLeft;
          clickCursor.setPosition(commandLeft+block.position());
    }
    while(commandLeft >= 0 && data->characterData.at(commandLeft).state == SyntaxHighlighter::Command)
    {
        --commandLeft;
    }
    if(data->characterData.at(left).state != SyntaxHighlighter::Command)
    {
        ++commandLeft;
    }
    clickCursor.setPosition(commandLeft+block.position(), QTextCursor::MoveAnchor);
    clickCursor.setPosition(left+block.position(), QTextCursor::KeepAnchor);

    QString command = clickCursor.selectedText();
    if(command != "\\ref")
    {
        return QTextCursor();
    }

    clickCursor.setPosition(left + 1 + block.position(), QTextCursor::MoveAnchor);
    clickCursor.setPosition(right + 1 + block.position(), QTextCursor::KeepAnchor);

    return clickCursor;
}

/* Cite Link Text Action */


CiteLinkTextAction::CiteLinkTextAction()
{
}


bool CiteLinkTextAction::execute(QTextCursor clickCursor, WidgetFile *widgetFile)
{
    QTextCursor commandCursor = this->match(clickCursor, widgetFile);
    if(commandCursor.isNull())
    {
        return false;
    }
    QString key = commandCursor.selectedText();
    QStringList bibtexFiles = widgetFile->file()->bibtexFiles();
    foreach(const QString filename, bibtexFiles)
    {
        QFile f(filename);
        if(!f.open(QFile::ReadOnly | QFile::Text))
        {
            continue;
        }
        QString text = f.readAll();
        if(text.contains(QRegExp("\\{[ \\t]*"+escapeStringForRegex(key)+"[ \\t\\n]*,")))
        {
            widgetFile->window()->open(filename);
            WidgetFile * bibtexFileWidget = FileManager::Instance.widgetFile(filename);
            QTextCursor findResult = bibtexFileWidget->widgetTextEdit()->document()->find(QRegExp("\\{[ \\t]*"+escapeStringForRegex(key)+"[ \\t\\n]*,"));
            if(!findResult.isNull())
            {
                bibtexFileWidget->widgetTextEdit()->setTextCursor(findResult);
                bibtexFileWidget->widgetTextEdit()->ensureCursorVisible();
            }
            return true;

        }
    }
    return false;
}

QTextCursor CiteLinkTextAction::match(QTextCursor clickCursor, WidgetFile *widgetFile)
{
     int cursorPosition = clickCursor.position();
     QTextBlock block = clickCursor.block();
     BlockData *data = static_cast<BlockData *>( block.userData() );
     if(!data && data->characterData.size() <= clickCursor.positionInBlock())
     {
         return QTextCursor();
     }

     CharacterData charData = data->characterData.at(clickCursor.positionInBlock());
     if(charData.state != SyntaxHighlighter::Other && charData.state != SyntaxHighlighter::Command)
     {
         return QTextCursor();
     }
    int left, right;
    left = clickCursor.positionInBlock();

    while(left < data->characterData.size() && data->characterData.at(left).state == SyntaxHighlighter::Command)
    {
        ++left;
    }
    clickCursor.setPosition(left+block.position());
    while(left < data->characterData.size() && (widgetFile->widgetTextEdit()->nextChar(clickCursor) == ' ' || widgetFile->widgetTextEdit()->nextChar(clickCursor) == '\t'))
    {
        ++left;
        clickCursor.setPosition(left+block.position());
    }
    right = left;

    if(data->characterData.at(left).state != SyntaxHighlighter::Other)
    {
        return QTextCursor();
    }


    while(left >= 0 && data->characterData.at(left).state == SyntaxHighlighter::Other)
    {
        --left;
    }
    if(data->characterData.at(left).state != SyntaxHighlighter::Other)
    {
        ++left;
    }
    while(right < data->characterData.size() && data->characterData.at(right).state == SyntaxHighlighter::Other)
    {
        ++right;
    }
    if(data->characterData.at(right).state != SyntaxHighlighter::Other)
    {
        --right;
    }


    int commandLeft = left - 1;
    clickCursor.setPosition(commandLeft+block.position());
    while(commandLeft >= 0 && (widgetFile->widgetTextEdit()->nextChar(clickCursor) == ' ' || widgetFile->widgetTextEdit()->nextChar(clickCursor) == '\t'))
    {
        --commandLeft;
          clickCursor.setPosition(commandLeft+block.position());
    }
    while(commandLeft >= 0 && data->characterData.at(commandLeft).state == SyntaxHighlighter::Command)
    {
        --commandLeft;
    }
    if(data->characterData.at(left).state != SyntaxHighlighter::Command)
    {
        ++commandLeft;
    }
    clickCursor.setPosition(commandLeft+block.position(), QTextCursor::MoveAnchor);
    clickCursor.setPosition(left+block.position(), QTextCursor::KeepAnchor);

    QString command = clickCursor.selectedText();
    if(command != "\\cite")
    {
        return QTextCursor();
    }

    clickCursor.setPosition(left + 1 + block.position(), QTextCursor::MoveAnchor);
    clickCursor.setPosition(right + 1 + block.position(), QTextCursor::KeepAnchor);

    if(clickCursor.selectedText().contains(","))
    {
        cursorPosition -= block.position();
        if(cursorPosition < left)
            cursorPosition = left;

        clickCursor.setPosition(cursorPosition + 1 + block.position(), QTextCursor::MoveAnchor);
        while(QString(widgetFile->widgetTextEdit()->nextChar(clickCursor)).contains(QRegExp("[^a-zA-Z0-9]")))
        {
            --cursorPosition;
            clickCursor.setPosition(cursorPosition + 1 + block.position(), QTextCursor::MoveAnchor);
        }

        left = right = cursorPosition;
        clickCursor.setPosition(left + 1 + block.position(), QTextCursor::MoveAnchor);
        while(QString(widgetFile->widgetTextEdit()->nextChar(clickCursor)).contains(QRegExp("[a-zA-Z0-9]")))
        {
            --left;
            clickCursor.setPosition(left + 1 + block.position(), QTextCursor::MoveAnchor);
        }
        ++left;
        clickCursor.setPosition(right + 1 + block.position(), QTextCursor::MoveAnchor);
        while(QString(widgetFile->widgetTextEdit()->nextChar(clickCursor)).contains(QRegExp("[a-zA-Z0-9]")))
        {
            ++right;
            clickCursor.setPosition(right + 1 + block.position(), QTextCursor::MoveAnchor);
        }
        clickCursor.setPosition(left + 1 + block.position(), QTextCursor::MoveAnchor);
        clickCursor.setPosition(right + 1 + block.position(), QTextCursor::KeepAnchor);
        return clickCursor;
    }
    return clickCursor;
}