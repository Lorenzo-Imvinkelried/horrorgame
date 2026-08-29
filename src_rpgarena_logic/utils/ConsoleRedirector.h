#pragma once
#include <iostream>
#include <streambuf>
#include <string>
#include "../core/ui/ChatBox.h"

class ConsoleRedirector : public std::streambuf {
public:
    ConsoleRedirector(std::ostream& stream, ChatBox* chatBox)
        : mStream(stream), mChatBox(chatBox)
    {
        mOldBuf = stream.rdbuf(); // Save old buffer
        stream.rdbuf(this);       // Redirect to this
    }

    ~ConsoleRedirector() {
        mStream.rdbuf(mOldBuf);   // Restore
    }

protected:
    // Called for each character
    virtual int_type overflow(int_type c) override {
        if (c == EOF) return c;

        // 1. Forward to original stdout (Console)
        mOldBuf->sputc(c);

        // 2. Capture for ChatBox
        char ch = static_cast<char>(c);
        if (ch == '\n') {
            if (mChatBox) {
                mChatBox->addLine(mCurrentLine);
            }
            mCurrentLine.clear();
        } else {
            mCurrentLine += ch;
        }

        return c;
    }

private:
    std::ostream& mStream;
    std::streambuf* mOldBuf;
    ChatBox* mChatBox;
    std::string mCurrentLine;
};
