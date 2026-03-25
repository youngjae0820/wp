#include <iostream>
#include <cstring>
#include <cctype>
#include <iomanip>

using namespace std;

const int THEATER_COUNT = 3;
const int TIME_COUNT = 3;
const int ROWS = 10;
const int COLS = 10;
const int TITLE_LEN = 20;
const int INPUT_LEN = 100;

struct cinema
{
    int number;
    char title[TITLE_LEN];
    int time[TIME_COUNT];
    int seat[TIME_COUNT][ROWS][COLS];
};

void copyText(char dest[], const char src[])
{
    int i = 0;

    while (src[i] != '\0')
    {
        dest[i] = src[i];
        i++;
    }



    dest[i] = '\0';
}

void initializeSeats(cinema theaters[])
{
    for (int i = 0; i < THEATER_COUNT; i++)
    {
        for (int t = 0; t < TIME_COUNT; t++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                for (int c = 0; c < COLS; c++)
                {
                    theaters[i].seat[t][r][c] = 0;
                }
            }
        }
    }
}

void setupCinemas(cinema theaters[])
{
    theaters[0].number = 1;
    copyText(theaters[0].title, "Avatar");
    theaters[0].time[0] = 1000;
    theaters[0].time[1] = 1400;
    theaters[0].time[2] = 1700;

    theaters[1].number = 2;
    copyText(theaters[1].title, "Jaws");
    theaters[1].time[0] = 1100;
    theaters[1].time[1] = 1500;
    theaters[1].time[2] = 2100;

    theaters[2].number = 3;
    copyText(theaters[2].title, "Love");
    theaters[2].time[0] = 900;
    theaters[2].time[1] = 1300;
    theaters[2].time[2] = 1700;

    initializeSeats(theaters);
}

bool readLine(char line[], int size)
{
    cin.getline(line, size);

    if (cin.fail())
    {
        cin.clear();
        cin.ignore(10000, '\n');
        return false;
    }

    return true;
}

void skipSpaces(const char line[], int& idx)
{
    while (line[idx] == ' ' || line[idx] == '\t')
    {
        idx++;
    }
}

bool isOnlyDigits(const char s[])
{
    if (s[0] == '\0')
        return false;

    for (int i = 0; s[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)s[i]))
            return false;
    }

    return true;
}

int toInt(const char s[])
{
    int value = 0;

    for (int i = 0; s[i] != '\0'; i++)
    {
        value = value * 10 + (s[i] - '0');
    }

    return value;
}

int findTheaterByNumber(cinema theaters[], int number)
{
    for (int i = 0; i < THEATER_COUNT; i++)
    {
        if (theaters[i].number == number)
            return i;
    }

    return -1;
}

int findTheaterByTitle(cinema theaters[], const char title[])
{
    for (int i = 0; i < THEATER_COUNT; i++)
    {
        if (strcmp(theaters[i].title, title) == 0)
            return i;
    }

    return -1;
}

int findTheaterByNumberOrTitle(cinema theaters[], const char input[])
{
    if (isOnlyDigits(input))
    {
        int number = toInt(input);
        return findTheaterByNumber(theaters, number);
    }

    return findTheaterByTitle(theaters, input);
}

int findTimeIndex(cinema theaters[], int theaterIndex, int targetTime)
{
    for (int i = 0; i < TIME_COUNT; i++)
    {
        if (theaters[theaterIndex].time[i] == targetTime)
            return i;
    }

    return -1;
}

void printTimeWithColon(int timeValue)
{
    int hour = timeValue / 100;
    int minute = timeValue % 100;

    cout << setw(2) << setfill('0') << hour << ":";
    cout << setw(2) << setfill('0') << minute;
    cout << setfill(' ');
}

void showCinemaInfo(cinema theaters[])
{
    cout << "\n===== 영화관 정보 =====\n";

    for (int i = 0; i < THEATER_COUNT; i++)
    {
        cout << theaters[i].number << " " << theaters[i].title << " ";
        for (int j = 0; j < TIME_COUNT; j++)
        {
            cout << theaters[i].time[j] << " ";
        }
        cout << "\n";
    }
}

void showSeatsOfOneTheater(cinema theaters[], int theaterIndex)
{
    for (int t = 0; t < TIME_COUNT; t++)
    {
        cout << "\n영화 제목: " << theaters[theaterIndex].title << " / 상영 시간: ";
        cout << theaters[theaterIndex].time[t] << "\n";

        cout << "    ";
        for (int c = 1; c <= COLS; c++)
        {
            cout << setw(3) << c;
        }
        cout << "\n";

        for (int r = 0; r < ROWS; r++)
        {
            cout << setw(3) << r + 1;
            for (int c = 0; c < COLS; c++)
            {
                if (theaters[theaterIndex].seat[t][r][c] == 0)
                    cout << setw(3) << "-";
                else
                    cout << setw(3) << theaters[theaterIndex].seat[t][r][c];
            }
            cout << "\n";
        }
    }
}

bool getSingleTokenAfterP(const char line[], char token[])
{
    int idx = 0;
    int k = 0;

    skipSpaces(line, idx);

    if (line[idx] != 'p')
        return false;

    idx++;
    skipSpaces(line, idx);

    if (line[idx] == '\0')
        return false;

    while (line[idx] != '\0' && line[idx] != ' ' && line[idx] != '\t')
    {
        token[k] = line[idx];
        k++;
        idx++;
    }
    token[k] = '\0';

    skipSpaces(line, idx);

    if (line[idx] != '\0')
        return false;

    return true;
}

bool reserveSeat(cinema theaters[], int theaterIndex, int timeIndex, int row, int col, int reservationNumber)
{
    if (row < 1 || row > 10 || col < 1 || col > 10)
    {
        cout << "잘못된 좌석 번호입니다.\n";
        return false;
    }

    int r = row - 1;
    int c = col - 1;

    if (theaters[theaterIndex].seat[timeIndex][r][c] != 0)
    {
        cout << "이미 예약된 좌석입니다.\n";
        return false;
    }

    theaters[theaterIndex].seat[timeIndex][r][c] = reservationNumber;

    cout << "\n===== 예약 완료 =====\n";
    cout << "영화 제목: " << theaters[theaterIndex].title << "\n";
    cout << "상영 시간: ";
    printTimeWithColon(theaters[theaterIndex].time[timeIndex]);
    cout << "\n";
    cout << "좌석 번호: (" << row << ", " << col << ")\n";
    cout << "예약 번호: " << reservationNumber << "\n";

    return true;
}

bool cancelReservation(cinema theaters[], int reservationNumber)
{
    for (int i = 0; i < THEATER_COUNT; i++)
    {
        for (int t = 0; t < TIME_COUNT; t++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                for (int c = 0; c < COLS; c++)
                {
                    if (theaters[i].seat[t][r][c] == reservationNumber)
                    {
                        theaters[i].seat[t][r][c] = 0;

                        cout << "\n===== 예약 취소 완료 =====\n";
                        cout << "영화 제목: " << theaters[i].title << "\n";
                        cout << "상영 시간: ";
                        printTimeWithColon(theaters[i].time[t]);
                        cout << "\n";
                        cout << "좌석 번호: (" << r + 1 << ", " << c + 1 << ")\n";
                        cout << "취소된 예약 번호: " << reservationNumber << "\n";

                        return true;
                    }
                }
            }
        }
    }

    cout << "해당 예약 번호를 찾을 수 없습니다.\n";
    return false;
}

void showReservationRate(cinema theaters[])
{
    cout << "\n===== 예약률 =====\n";

    for (int i = 0; i < THEATER_COUNT; i++)
    {
        cout << theaters[i].number << " " << theaters[i].title << " ";

        for (int t = 0; t < TIME_COUNT; t++)
        {
            int count = 0;

            for (int r = 0; r < ROWS; r++)
            {
                for (int c = 0; c < COLS; c++)
                {
                    if (theaters[i].seat[t][r][c] != 0)
                        count++;
                }
            }

            double rate = count;
            cout << theaters[i].time[t] << ": ";
            cout << fixed << setprecision(2) << rate << "% ";
        }

        cout << "\n";
    }
}

bool readNumberLine(const char prompt[], int& result)
{
    char line[INPUT_LEN];

    cout << prompt;

    if (!readLine(line, INPUT_LEN))
    {
        cout << "입력이 너무 깁니다.\n";
        return false;
    }

    if (!isOnlyDigits(line))
    {
        cout << "숫자로 입력해야 합니다.\n";
        return false;
    }

    result = toInt(line);
    return true;
}

bool readSeatPosition(int& row, int& col)
{
    char line[INPUT_LEN];
    char rowText[20];
    char colText[20];
    int idx = 0;
    int k = 0;

    if (!readLine(line, INPUT_LEN))
    {
        cout << "입력이 너무 깁니다.\n";
        return false;
    }

    rowText[0] = '\0';
    colText[0] = '\0';

    skipSpaces(line, idx);

    while (line[idx] != '\0' && line[idx] != ' ' && line[idx] != '\t')
    {
        rowText[k] = line[idx];
        k++;
        idx++;
    }
    rowText[k] = '\0';

    skipSpaces(line, idx);

    k = 0;
    while (line[idx] != '\0' && line[idx] != ' ' && line[idx] != '\t')
    {
        colText[k] = line[idx];
        k++;
        idx++;
    }
    colText[k] = '\0';

    skipSpaces(line, idx);

    if (rowText[0] == '\0' || colText[0] == '\0' || line[idx] != '\0')
    {
        cout << "좌석은 행 열 형식으로 입력해야 합니다. 예: 1 3\n";
        return false;
    }

    if (!isOnlyDigits(rowText) || !isOnlyDigits(colText))
    {
        cout << "좌석 번호는 숫자로 입력해야 합니다.\n";
        return false;
    }

    row = toInt(rowText);
    col = toInt(colText);

    return true;
}

void commandReserve(cinema theaters[], int& nextReservationNumber)
{
    char movieInput[INPUT_LEN];
    int theaterIndex;
    int targetTime;
    int timeIndex;
    int row, col;

    if (nextReservationNumber > 99)
    {
        cout << "예약 번호가 모두 사용되었습니다.\n";
        return;
    }

    cout << "영화관 번호 또는 영화 제목을 입력하세요: ";
    if (!readLine(movieInput, INPUT_LEN))
    {
        cout << "입력이 너무 깁니다.\n";
        return;
    }

    theaterIndex = findTheaterByNumberOrTitle(theaters, movieInput);
    if (theaterIndex == -1)
    {
        cout << "해당 영화관 번호 또는 영화 제목을 찾을 수 없습니다.\n";
        return;
    }

    if (!readNumberLine("상영 시간을 입력하세요: ", targetTime))
        return;

    timeIndex = findTimeIndex(theaters, theaterIndex, targetTime);
    if (timeIndex == -1)
    {
        cout << "해당 상영 시간을 찾을 수 없습니다.\n";
        return;
    }

    showSeatsOfOneTheater(theaters, theaterIndex);

    cout << "\n예약할 좌석의 행과 열을 입력하세요. 예: 1 3\n";
    cout << "입력: ";

    if (!readSeatPosition(row, col))
        return;

    if (reserveSeat(theaters, theaterIndex, timeIndex, row, col, nextReservationNumber))
    {
        nextReservationNumber++;
    }
}

void commandCancel(cinema theaters[])
{
    int reservationNumber;

    if (!readNumberLine("취소할 예약 번호를 입력하세요: ", reservationNumber))
        return;

    cancelReservation(theaters, reservationNumber);
}

void showCommandGuide()
{
    cout << "\n===== 명령어 안내 =====\n";
    cout << "d : 영화관 정보 출력\n";
    cout << "p x : 좌석 상태 출력 (x는 영화관 번호 또는 영화 제목)\n";
    cout << "r : 예약하기\n";
    cout << "c : 예약 취소하기\n";
    cout << "h : 예약률 출력\n";
    cout << "q : 종료\n";
}

int main()
{
    cinema theaters[THEATER_COUNT];
    int nextReservationNumber = 10;
    char commandLine[INPUT_LEN];
    char token[INPUT_LEN];

    setupCinemas(theaters);
    showCommandGuide();

    while (true)
    {
        cout << "\n명령어를 입력하세요: ";

        if (!readLine(commandLine, INPUT_LEN))
        {
            cout << "입력이 너무 깁니다.\n";
            continue;
        }

        if (strcmp(commandLine, "d") == 0)
        {
            showCinemaInfo(theaters);
        }
        else if (strcmp(commandLine, "r") == 0)
        {
            commandReserve(theaters, nextReservationNumber);
        }
        else if (strcmp(commandLine, "c") == 0)
        {
            commandCancel(theaters);
        }
        else if (strcmp(commandLine, "h") == 0)
        {
            showReservationRate(theaters);
        }
        else if (strcmp(commandLine, "q") == 0 || strcmp(commandLine, "Q") == 0)
        {
            cout << "프로그램을 종료합니다.\n";
            break;
        }
        else if (getSingleTokenAfterP(commandLine, token))
        {
            int theaterIndex = findTheaterByNumberOrTitle(theaters, token);

            if (theaterIndex == -1)
            {
                cout << "해당 영화관 번호 또는 영화 제목을 찾을 수 없습니다.\n";
            }
            else
            {
                showSeatsOfOneTheater(theaters, theaterIndex);
            }
        }
        else
        {
            cout << "잘못된 명령어입니다.\n";
        }
    }

    return 0;
}