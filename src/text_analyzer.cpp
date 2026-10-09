#include "text_analyzer.h"

TextAnalyzer::TextAnalyzer(const std::string& text, int rowLengthLimit)
	: mText(text),
	mRowLengthLimit(rowLengthLimit),
	mMaxRowLength(0),
	mWordStartIdx(0),
	mCurrentRow("")
{
	int len = mText.length();
	for (int i = 0; i < len; ++i) {
		processChar(i);
	}
	if (mWordStartIdx < len - 1) {
		processWord(mWordStartIdx, len - mWordStartIdx);
	}
	if (mCurrentRow.length() > 0) {
		processEndOfLine();
	}
}

void TextAnalyzer::processChar(int idx)
{
	char ch =  mText[idx];
	if (ch == ' ' || ch == '\n') {
		int worldLength = idx - mWordStartIdx;
		if (worldLength > 0) {
			processWord(mWordStartIdx, worldLength);
		}
		if (ch == '\n') {
			processEndOfLine();
		}
		mWordStartIdx = idx + 1;
	}
}

void TextAnalyzer::processWord(int wordStartIdx, int wordLength)
{
	int currentRowLength = mCurrentRow.length();
	bool wordOverflow = mRowLengthLimit > 0 && 
		currentRowLength + (currentRowLength == 0 ? wordLength : wordLength + 1) > mRowLengthLimit;
	if (wordOverflow) {
		processEndOfLine();
	}
	if (mCurrentRow.length() > 0) {
		mCurrentRow += " ";
	}
	mCurrentRow += mText.substr(wordStartIdx, wordLength);
}

void TextAnalyzer::processEndOfLine()
{
	mRows.push_back(mCurrentRow);
	if (mCurrentRow.length() > mMaxRowLength) {
		mMaxRowLength = mCurrentRow.length();
	}
	mCurrentRow = "";
}
