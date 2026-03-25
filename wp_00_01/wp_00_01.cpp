#include <iostream>
using namespace std;

const int MAX_LEN = 40;
const int MAX_WORDS = 20;

int strLength(const char s[])
{
    int i = 0;
    while (s[i] != '\0')
        i++;
    return i;
}

bool isLower(char c)
{
    return c >= 'a' && c <= 'z';
}

bool isUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool isAlpha(char c)
{
    return isLower(c) || isUpper(c);
}

char toLowerChar(char c)
{
    if (isUpper(c))
        return c + ('a' - 'A');
    return c;
}

char toUpperChar(char c)
{
    if (isLower(c))
        return c - ('a' - 'A');
    return c;
}

bool isValidSentence(const char s[])
{
    int len = strLength(s);

    if (len < 2 || len > MAX_LEN)
        return false;

    if (s[len - 1] != '.')
        return false;

    if (s[0] == ' ' || s[len - 2] == ' ')
        return false;

    for (int i = 0; i < len - 1; i++)
    {
        if (!(isAlpha(s[i]) || s[i] == ' '))
            return false;
    }

    return true;
}

bool isValidCommandChar(char c)
{
    if (isLower(c))
        return true;

    if (c == '0' || c == '1' || c == '2' || c == '3' || c == '4')
        return true;

    return false;
}

bool readCommand(char& cmd)
{
    char line[100];
    cin.getline(line, 100);

    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        return false;
    }

    if (strLength(line) != 1)
        return false;

    if (!isValidCommandChar(line[0]))
        return false;

    cmd = line[0];
    return true;
}

bool parseSentence(
    const char input[],
    char letters[],
    int wordStart[],
    int wordLen[],
    int gap[],
    int& wordCount,
    bool& hasPeriod)
{
    int i = 0;
    int letterCount = 0;
    wordCount = 0;
    hasPeriod = true;

    while (input[i] != '\0' && input[i] != '.')
    {
        while (input[i] == ' ')
            i++;

        if (input[i] == '.' || input[i] == '\0')
            break;

        if (wordCount >= MAX_WORDS)
            return false;

        wordStart[wordCount] = letterCount;
        wordLen[wordCount] = 0;

        while (input[i] != '\0' && input[i] != ' ' && input[i] != '.')
        {
            if (letterCount >= MAX_LEN)
                return false;

            letters[letterCount] = input[i];
            letterCount++;
            wordLen[wordCount]++;
            i++;
        }

        gap[wordCount] = 0;

        while (input[i] == ' ')
        {
            gap[wordCount]++;
            i++;
        }

        if (input[i] != '.' && input[i] != '\0')
            gap[wordCount]++;

        wordCount++;
    }

    letters[letterCount] = '\0';

    if (wordCount > 0)
        gap[wordCount - 1] = 0;

    return true;
}

int totalModelLength(const int wordLen[], const int gap[], int wordCount, bool hasPeriod)
{
    int total = 0;

    for (int i = 0; i < wordCount; i++)
        total += wordLen[i];

    for (int i = 0; i < wordCount - 1; i++)
        total += gap[i];

    if (hasPeriod)
        total += 1;

    return total;
}

void trimToMaxLength(int wordLen[], int gap[], int& wordCount, bool& hasPeriod)
{
    int extra = totalModelLength(wordLen, gap, wordCount, hasPeriod) - MAX_LEN;

    if (extra <= 0)
        return;

    while (extra > 0 && wordCount > 0)
    {
        int last = wordCount - 1;

        if (wordLen[last] > 0)
        {
            int cut = (wordLen[last] < extra) ? wordLen[last] : extra;
            wordLen[last] -= cut;
            extra -= cut;
        }

        if (wordLen[last] == 0)
        {
            if (wordCount == 1)
            {
                wordCount = 0;
                hasPeriod = false;
                break;
            }

            int beforeGapIndex = wordCount - 2;

            if (extra > 0)
            {
                int cutGap = (gap[beforeGapIndex] < extra) ? gap[beforeGapIndex] : extra;
                gap[beforeGapIndex] -= cutGap;
                extra -= cutGap;
            }

            wordCount--;
        }
    }
}

void buildSentence(
    char sentence[],
    const char letters[],
    const int wordStart[],
    const int wordLen[],
    const int gap[],
    int wordCount,
    bool hasPeriod)
{
    int idx = 0;

    for (int w = 0; w < wordCount; w++)
    {
        for (int j = 0; j < wordLen[w]; j++)
        {
            sentence[idx] = letters[wordStart[w] + j];
            idx++;
        }

        if (w != wordCount - 1)
        {
            for (int s = 0; s < gap[w]; s++)
            {
                sentence[idx] = ' ';
                idx++;
            }
        }
    }

    if (hasPeriod)
    {
        sentence[idx] = '.';
        idx++;
    }

    sentence[idx] = '\0';
}

void printNormalSentence(const char sentence[])
{
    cout << sentence << "\n";
}

void toggleLetterCase(
    char letters[],
    const int wordStart[],
    const int wordLen[],
    int wordCount,
    char cmd)
{
    char target = toLowerChar(cmd);

    for (int w = 0; w < wordCount; w++)
    {
        for (int j = 0; j < wordLen[w]; j++)
        {
            int idx = wordStart[w] + j;

            if (toLowerChar(letters[idx]) == target)
            {
                if (isLower(letters[idx]))
                    letters[idx] = toUpperChar(letters[idx]);
                else
                    letters[idx] = toLowerChar(letters[idx]);
            }
        }
    }
}

void decreaseSpaces(int gap[], int wordCount)
{
    for (int i = 0; i < wordCount - 1; i++)
    {
        if (gap[i] > 0)
            gap[i]--;
    }
}

void increaseSpaces(int gap[], int wordCount)
{
    for (int i = 0; i < wordCount - 1; i++)
    {
        if (gap[i] < 5)
            gap[i]++;
    }
}

void printAlphabetCount(
    const char letters[],
    const int wordStart[],
    const int wordLen[],
    int wordCount,
    bool hasPeriod)
{
    int lowerCount[26] = { 0 };
    int upperCount[26] = { 0 };

    for (int w = 0; w < wordCount; w++)
    {
        for (int j = 0; j < wordLen[w]; j++)
        {
            char c = letters[wordStart[w] + j];

            if (isLower(c))
                lowerCount[c - 'a']++;
            else if (isUpper(c))
                upperCount[c - 'A']++;
        }
    }

    for (int i = 0; i < 26; i++)
    {
        if (lowerCount[i] > 0)
            cout << (char)('a' + i) << lowerCount[i];
    }

    for (int i = 0; i < 26; i++)
    {
        if (upperCount[i] > 0)
            cout << (char)('A' + i) << upperCount[i];
    }

    if (hasPeriod)
        cout << '.';

    cout << "\n";
}

void printOneWord(const char letters[], int start, int len)
{
    for (int i = 0; i < len; i++)
        cout << letters[start + i];
}

void printWordsSortedByLength(
    const char letters[],
    const int wordStart[],
    const int wordLen[],
    int wordCount,
    bool hasPeriod)
{
    int order[MAX_WORDS];

    for (int i = 0; i < wordCount; i++)
        order[i] = i;

    for (int i = 0; i < wordCount - 1; i++)
    {
        for (int j = 0; j < wordCount - 1 - i; j++)
        {
            if (wordLen[order[j]] > wordLen[order[j + 1]])
            {
                int temp = order[j];
                order[j] = order[j + 1];
                order[j + 1] = temp;
            }
        }
    }

    for (int i = 0; i < wordCount; i++)
    {
        printOneWord(letters, wordStart[order[i]], wordLen[order[i]]);

        if (i != wordCount - 1)
            cout << ' ';
    }

    if (hasPeriod)
        cout << '.';

    cout << "\n";
}

int main()
{
    char input[200];
    char sentence[MAX_LEN + 1];

    char letters[MAX_LEN + 1];
    int wordStart[MAX_WORDS];
    int wordLen[MAX_WORDS];
    int gap[MAX_WORDS];

    int wordCount = 0;
    bool hasPeriod = true;

    int mode = 0; // 0: 일반 출력, 3: 알파벳 개수 출력 중, 4: 단어 길이 정렬 출력 중

    while (true)
    {
        cout << "문장을 입력해 주세요: ";
        cin.getline(input, 200);

        if (!isValidSentence(input))
        {
            cout << "잘못된 입력입니다.\n";
            cout << "영문자와 공백만 사용하고, 마지막은 마침표이며, 전체 길이는 40자 이하여야 합니다.\n\n";
            continue;
        }

        if (!parseSentence(input, letters, wordStart, wordLen, gap, wordCount, hasPeriod))
        {
            cout << "단어 수가 너무 많거나 배열 범위를 벗어났습니다.\n";
            cout << "다시 입력해 주세요.\n\n";
            continue;
        }

        break;
    }

    buildSentence(sentence, letters, wordStart, wordLen, gap, wordCount, hasPeriod);

    cout << "현재 문장: ";
    printNormalSentence(sentence);

    while (true)
    {
        char cmd;
        cout << "명령어를 입력해 주세요: ";

        if (!readCommand(cmd))
        {
            cout << "잘못된 명령어입니다.\n";
            cout << "소문자 1개 또는 0, 1, 2, 3, 4 중 하나만 입력해 주세요.\n";
            continue;
        }

        if (cmd == '0')
        {
            cout << "프로그램을 종료합니다.\n";
            break;
        }

        if (cmd == '3')
        {
            if (mode == 3)
            {
                mode = 0;
                printNormalSentence(sentence);
            }
            else
            {
                mode = 3;
                printAlphabetCount(letters, wordStart, wordLen, wordCount, hasPeriod);
            }
            continue;
        }

        if (cmd == '4')
        {
            if (mode == 4)
            {
                mode = 0;
                printNormalSentence(sentence);
            }
            else
            {
                mode = 4;
                printWordsSortedByLength(letters, wordStart, wordLen, wordCount, hasPeriod);
            }
            continue;
        }

        mode = 0;

        if (cmd == '1')
        {
            decreaseSpaces(gap, wordCount);
            buildSentence(sentence, letters, wordStart, wordLen, gap, wordCount, hasPeriod);
            printNormalSentence(sentence);
        }
        else if (cmd == '2')
        {
            increaseSpaces(gap, wordCount);
            trimToMaxLength(wordLen, gap, wordCount, hasPeriod);
            buildSentence(sentence, letters, wordStart, wordLen, gap, wordCount, hasPeriod);
            printNormalSentence(sentence);
        }
        else if (isLower(cmd))
        {
            toggleLetterCase(letters, wordStart, wordLen, wordCount, cmd);
            buildSentence(sentence, letters, wordStart, wordLen, gap, wordCount, hasPeriod);
            printNormalSentence(sentence);
        }
    }

    return 0;
}