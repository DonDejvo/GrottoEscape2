#pragma once

#include <string>
#include <vector>

class TextAnalyzer {
public:
	TextAnalyzer(const std::string& text, int rowLengthLimit = 0);
	inline int GetRowCount() const { return mRows.size(); }
	inline const std::vector<std::string> GetRows() const { return mRows; }
	inline int GetMaxRowLength() const { return mMaxRowLength; }
	inline const std::string& GetText() { return mText; }

private:
	std::string mText;
	int mRowLengthLimit;
	std::vector<std::string> mRows;
	int mMaxRowLength;
	int mWordStartIdx;
	std::string mCurrentRow;

	void processChar(int idx);
	void processWord(int wordStartIdx, int wordLength);
	void processEndOfLine();
};