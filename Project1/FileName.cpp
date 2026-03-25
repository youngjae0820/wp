#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using namespace std;

// 문제 조건 핵심
static const int MAX_TOTAL = 40; // 전체 길이(마침표 포함)
static const int MAX_BODY = 39; // 본문 길이(마침표 제외)
static const int GAP_MIN = 0;  // 공백 최소
static const int GAP_MAX = 5;  // 공백 최대

// --------------------------------------------
// 0) 문자열 다듬기(앞/뒤 공백 제거)
// --------------------------------------------
static string trim(const string& s) {
    size_t l = 0, r = s.size();
    while (l < r && s[l] == ' ') l++;
    while (r > l && s[r - 1] == ' ') r--;
    return s.substr(l, r - l);
}

// --------------------------------------------
// 1) 입력 정규화
// - 첫 '.'까지만 사용(뒤는 버림), '.'이 없으면 그냥 전체를 본문으로 취급
// - 여기서는 "본문(body)"만 만들고, 출력할 때 항상 '.'을 붙인다
// --------------------------------------------
static string normalizeToBody(const string& line) {
    string s = line;

    // 첫 '.' 위치
    size_t dot = s.find('.');
    if (dot != string::npos) {
        s = s.substr(0, dot); // '.' 앞까지만 본문
    }
    // 앞/뒤 공백 제거(예외 입력 방어)
    s = trim(s);

    return s;
}

// --------------------------------------------
// 2) 본문(body)을 words + gaps로 분해
// - words: 단어들
// - gaps : 단어 사이 공백 수(구간별). size = words.size()-1
//   (공백이 0이 돼도 "경계"를 잃지 않게 이 구조로 저장하는 게 포인트)
// --------------------------------------------
static void parseBody(const string& body, vector<string>& words, vector<int>& gaps) {
    words.clear();
    gaps.clear();

    int n = (int)body.size();
    int i = 0;

    while (i < n) {
        // 단어 앞 공백 스킵
        while (i < n && body[i] == ' ') i++;
        if (i >= n) break;

        // 단어 읽기
        int start = i;
        while (i < n && body[i] != ' ') i++;
        words.push_back(body.substr(start, i - start));

        // 다음 단어가 있으면 공백 개수 세기
        int cnt = 0;
        while (i < n && body[i] == ' ') {
            cnt++;
            i++;
        }
        if (i < n) {
            // 문제 공백 범위(0~5)로 클램프(예외 입력 방어)
            cnt = max(GAP_MIN, min(GAP_MAX, cnt));
            gaps.push_back(cnt);
        }
    }

    // gaps 길이 보정
    if (words.empty()) gaps.clear();
    else if (gaps.size() > words.size() - 1) gaps.resize(words.size() - 1);
    else if (gaps.size() < words.size() - 1) gaps.resize(words.size() - 1, 1);
}

// --------------------------------------------
// 3) words + gaps로 본문(body) 만들기
// (마침표는 밖에서 붙인다)
// --------------------------------------------
static string buildBody(const vector<string>& words, const vector<int>& gaps) {
    if (words.empty()) return "";

    string out;
    for (size_t i = 0; i < words.size(); i++) {
        out += words[i];
        if (i < gaps.size()) out.append(gaps[i], ' ');
    }

    // 끝이 공백이면 제거(마침표 앞 공백 방지)
    while (!out.empty() && out.back() == ' ') out.pop_back();

    return out;
}

// --------------------------------------------
// 4) 본문 길이(MAX_BODY=39) 강제
// - "뒤 문자부터 잘림"을 words/gaps 구조를 유지한 채로 반영
// - 단어 중간이 잘릴 수도 있다(문제는 문자 단위로 잘린다고 했으므로)
// --------------------------------------------
static void enforceBodyLimit(vector<string>& words, vector<int>& gaps) {
    int remaining = MAX_BODY;

    vector<string> nw;
    vector<int> ng;

    for (size_t i = 0; i < words.size(); i++) {
        if (remaining <= 0) break;

        // 단어 넣기(필요하면 단어 중간에서 잘림)
        int takeWord = min((int)words[i].size(), remaining);
        nw.push_back(words[i].substr(0, takeWord));
        remaining -= takeWord;

        // 단어가 잘렸으면 끝(뒤 문자 삭제)
        if (takeWord < (int)words[i].size()) break;
        if (remaining <= 0) break;

        // 다음 단어가 있고, gap이 있으면 gap 넣기
        if (i < gaps.size()) {
            int takeGap = min(gaps[i], remaining);
            ng.push_back(takeGap);
            remaining -= takeGap;

            // gap도 중간에서 잘렸으면 끝
            if (takeGap < gaps[i]) break;
        }
    }

    // 마지막이 공백으로 끝나면 제거(ng 조정)
    // buildBody에서 공백 제거도 하지만, 구조도 깔끔히 유지해두기
    // -> ng의 마지막 값이 0이어도 OK(공백 0구간 허용)
    if (nw.empty()) {
        words.clear();
        gaps.clear();
        return;
    }

    // gaps 길이 = words-1로 정리
    if (ng.size() > nw.size() - 1) ng.resize(nw.size() - 1);
    else if (ng.size() < nw.size() - 1) ng.resize(nw.size() - 1, 0);

    words.swap(nw);
    gaps.swap(ng);
}

// --------------------------------------------
// 5) 최종 문장 출력(본문 + '.')
// - '.'은 항상 마지막
// --------------------------------------------
static string buildSentence(const vector<string>& words, const vector<int>& gaps) {
    string body = buildBody(words, gaps);
    if ((int)body.size() > MAX_BODY) body = body.substr(0, MAX_BODY);
    return body + ".";
}

// --------------------------------------------
// 명령 a~z: 해당 글자만 대↔소문자 토글
// --------------------------------------------
static void cmdToggleLetter(vector<string>& words, char cmd) {
    cmd = (char)tolower((unsigned char)cmd);

    for (string& w : words) {
        for (char& c : w) {
            unsigned char uc = (unsigned char)c;
            if (!isalpha(uc)) continue;

            if (tolower(uc) == cmd) {
                if (islower(uc)) c = (char)toupper(uc);
                else             c = (char)tolower(uc);
            }
        }
    }
}

// --------------------------------------------
// 명령 1: 각 구간 공백 -1 (최소 0)
// --------------------------------------------
static void cmdDecreaseGaps(vector<int>& gaps) {
    for (int& g : gaps) g = max(GAP_MIN, g - 1);
}

// --------------------------------------------
// 명령 2: 각 구간 공백 +1 (최대 5) 후, 40자 넘으면 뒤 문자 잘림
// --------------------------------------------
static void cmdIncreaseGaps(vector<string>& words, vector<int>& gaps) {
    for (int& g : gaps) g = min(GAP_MAX, g + 1);
    enforceBodyLimit(words, gaps);
}

// --------------------------------------------
// 명령 3: 알파벳 오름차순으로 "중복 없이" 출력 + 개수(대소문자 구분)
// 예시 형태에 맞게 소문자 a~z 먼저, 대문자 A~Z 다음
// (이건 보기 모드 출력이라 원문(words/gaps)은 건드리지 않음)
// --------------------------------------------
static string makeLetterStats(const vector<string>& words) {
    int lower[26] = { 0 };
    int upper[26] = { 0 };

    for (const string& w : words) {
        for (char c : w) {
            if (c >= 'a' && c <= 'z') lower[c - 'a']++;
            else if (c >= 'A' && c <= 'Z') upper[c - 'A']++;
        }
    }

    string out;
    for (int i = 0; i < 26; i++) {
        if (lower[i] > 0) {
            out.push_back((char)('a' + i));
            out += to_string(lower[i]);
        }
    }
    for (int i = 0; i < 26; i++) {
        if (upper[i] > 0) {
            out.push_back((char)('A' + i));
            out += to_string(upper[i]);
        }
    }
    out.push_back('.');
    return out;
}

// --------------------------------------------
// 명령 4: 단어 길이(알파벳 개수=여기선 단어 길이) 오름차순 정렬 출력
// 공백은 1개 고정, 보기 모드(원문 상태는 유지)
// --------------------------------------------
static string makeWordLengthSorted(const vector<string>& words) {
    vector<int> idx(words.size());
    for (int i = 0; i < (int)words.size(); i++) idx[i] = i;

    // 같은 길이면 기존 순서 유지(초보에겐 안정적)
    stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
        return words[a].size() < words[b].size();
        });

    string out;
    for (int i = 0; i < (int)idx.size(); i++) {
        if (i > 0) out.push_back(' ');
        out += words[idx[i]];
    }
    out.push_back('.');
    return out;
}

// --------------------------------------------
// main: 입력 -> 상태(words/gaps) 구성 -> 명령 루프
// --------------------------------------------
int main() {
    cout << "Enter the sentence: ";
    string line;
    getline(cin, line);

    // (1) 입력 정규화 + 파싱
    string body = normalizeToBody(line);

    vector<string> words;
    vector<int> gaps;
    parseBody(body, words, gaps);

    // (2) 길이 제한(본문 39자) 적용
    enforceBodyLimit(words, gaps);

    // (3) 보기 모드 토글 상태
    bool view3 = false;
    bool view4 = false;

    // (4) 명령 루프
    while (true) {
        cout << "Command: ";
        string cmdLine;
        if (!getline(cin, cmdLine)) break;

        // 공백/빈 입력 방어: 첫 non-space 문자만 명령으로 취급
        size_t p = cmdLine.find_first_not_of(' ');
        if (p == string::npos) continue;
        char cmd = cmdLine[p];

        if (cmd == '0') { // 종료
            break;
        }

        // a~z (대문자로 입력해도 처리)
        if (isalpha((unsigned char)cmd)) {
            char lower = (char)tolower((unsigned char)cmd);
            if (lower >= 'a' && lower <= 'z') {
                view3 = view4 = false;          // 보기 모드 해제
                cmdToggleLetter(words, lower);  // 실제 문장 변경
                cout << buildSentence(words, gaps) << "\n";
                continue;
            }
        }

        if (cmd == '1') { // 공백 감소
            view3 = view4 = false;
            cmdDecreaseGaps(gaps);
            cout << buildSentence(words, gaps) << "\n";
            continue;
        }

        if (cmd == '2') { // 공백 증가 + 길이 제한
            view3 = view4 = false;
            cmdIncreaseGaps(words, gaps);
            cout << buildSentence(words, gaps) << "\n";
            continue;
        }

        if (cmd == '3') { // 알파벳 통계(토글)
            if (!view3) {
                view3 = true; view4 = false;
                cout << makeLetterStats(words) << "\n";
            }
            else {
                view3 = false;
                cout << buildSentence(words, gaps) << "\n";
            }
            continue;
        }

        if (cmd == '4') { // 단어 길이 정렬(토글)
            if (!view4) {
                view4 = true; view3 = false;
                cout << makeWordLengthSorted(words) << "\n";
            }
            else {
                view4 = false;
                cout << buildSentence(words, gaps) << "\n";
            }
            continue;
        }

        // 예외처리: 이상한 명령은 무시/안내
        cout << "(Invalid command)\n";
    }

    return 0;
}