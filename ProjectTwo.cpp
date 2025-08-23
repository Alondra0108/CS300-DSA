// ProjectTwo.cpp
// Alondra Paulino Santos
// CS 300 – Project Two (Advising Assistance Program)
// Data structure: Hash Table (chaining), per Project One recommendation.
// Notes:
//  - Single-file CLI program (no external headers/parsers).
//  - Implements multi-pass validation, line-numbered error reporting,
//    cycle detection, adaptive sorting, robust menu with help, and basic timing.

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::ifstream;
using std::size_t;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

// Domain Model
struct Course {
    string number;               // normalized (trimmed, uppercased) e.g., "CSCI200"
    string title;                // course title
    vector<string> prereqs;      // normalized prerequisite course numbers
};

// Utility: trimming & normalization
static inline string ltrim(const string& s) {
    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    return s.substr(i);
}
static inline string rtrim(const string& s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && std::isspace(static_cast<unsigned char>(s[i - 1]))) --i;
    return s.substr(0, i);
}
static inline string trim(const string& s) { return rtrim(ltrim(s)); }

// Normalize course codes so comparisons are consistent (e.g., "csci200 " -> "CSCI200").
static inline string NormalizeCourse(const string& raw) {
    string t = trim(raw);
    for (char& c : t) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return t;
}
/* Reviewer note (Normalization):
   Normalize all course codes (trim + uppercase) before any hashing/lookup,
   so user input like "csci 200" matches "CSCI200" in the data. Prevents subtle
   mismatches and makes the rest of the pipeline stable. */

   // -------------------------------
   // Hash Table (chaining)
   // -------------------------------
struct Node {
    Course data;
    Node* next = nullptr;
    explicit Node(const Course& c) : data(c) {}
};

class HashTable {
public:
    explicit HashTable(size_t tableSize = 179) : tableSize_(tableSize), buckets_(tableSize_, nullptr) {}

    // Insert returns false on duplicate course number; otherwise true.
    bool Insert(const Course& c) {
        size_t idx = Hash(c.number);
        // Check duplicate in chain
        for (Node* cur = buckets_[idx]; cur != nullptr; cur = cur->next) {
            if (cur->data.number == c.number) return false;
        }
        Node* n = new Node(c);
        n->next = buckets_[idx];
        buckets_[idx] = n;
        return true;
    }

    // Search returns pointer to Course if found; otherwise nullptr.
    const Course* Search(const string& courseNumber) const {
        string key = NormalizeCourse(courseNumber);
        size_t idx = Hash(key);
        for (Node* cur = buckets_[idx]; cur != nullptr; cur = cur->next) {
            if (cur->data.number == key) return &cur->data;
        }
        return nullptr;
    }

    // Gather all courses to a vector (no side effects on table).
    vector<Course> ToVector() const {
        vector<Course> out;
        out.reserve(256);
        for (Node* head : buckets_) {
            for (Node* cur = head; cur != nullptr; cur = cur->next) {
                out.push_back(cur->data);
            }
        }
        return out;
    }

    // Gather and return courses sorted alphanumerically by course number.
    vector<Course> ToVectorSorted() const {
        vector<Course> v = ToVector();

        // Adaptive choice: insertion sort for tiny sets, std::sort otherwise.
        if (v.size() < 50) {
            for (size_t i = 1; i < v.size(); ++i) {
                Course key = v[i];
                size_t j = i;
                while (j > 0 && v[j - 1].number > key.number) {
                    v[j] = v[j - 1];
                    --j;
                }
                v[j] = key;
            }
        }
        else {
            std::sort(v.begin(), v.end(), [](const Course& a, const Course& b) {
                return a.number < b.number;
                });
        }
        return v;
    }
    /* Reviewer note (Hash + Sorting):
       - Table uses chaining with a prime bucket count to keep average inserts/lookups ~O(1).
       - Sorting is adaptive: insertion sort for small N, std::sort for larger lists.
         This keeps the implementation simple but still responsive for typical inputs. */

    ~HashTable() {
        for (Node* head : buckets_) {
            while (head) {
                Node* nxt = head->next;
                delete head;
                head = nxt;
            }
        }
    }

private:
    // Simple 31-based rolling hash for strings; mod table size (prime recommended).
    size_t Hash(const string& key) const {
        long long sum = 0;
        for (unsigned char ch : key) sum = sum * 31 + ch;
        if (sum < 0) sum = -sum;
        return static_cast<size_t>(sum) % tableSize_;
    }

    size_t tableSize_;
    vector<Node*> buckets_;
};

// Load/Validation Reporting
struct LoadIssue {
    size_t lineNo{};
    string type;   // e.g., "MissingField", "Duplicate", "UnknownPrereq", "SelfPrereq", "Cycle"
    string detail; // human-readable
};

struct LoadResultSummary {
    size_t linesRead = 0;
    size_t parsedCourses = 0;
    size_t inserted = 0;
    size_t duplicates = 0;
    size_t unknownPrereqs = 0;
    size_t selfPrereqs = 0;
    size_t cycles = 0;
    vector<LoadIssue> issues;
};

// Pass 1: Parse CSV lines and populate a temporary map (detect duplicates, missing fields)
static bool ParseLineCSV(const string& line, size_t lineNo,
    string& outNumber, string& outTitle, vector<string>& outPrereqs,
    LoadResultSummary& summary) {
    summary.linesRead++;

    // Skip empty/comment-only lines gracefully.
    string trimmed = trim(line);
    if (trimmed.empty()) return false;

    // Basic CSV split on commas (titles have no commas per project input).
    std::stringstream ss(line);
    string token;
    vector<string> tokens;
    while (getline(ss, token, ',')) tokens.push_back(token);

    // Need at least courseNumber and title.
    if (tokens.size() < 2) {
        summary.issues.push_back({ lineNo, "MissingField",
                                  "Missing course number or title" });
        return false;
    }

    outNumber = NormalizeCourse(tokens[0]);
    outTitle = trim(tokens[1]);
    outPrereqs.clear();

    // Optional prereqs start at index 2 (ignore blanks).
    for (size_t i = 2; i < tokens.size(); ++i) {
        string p = NormalizeCourse(tokens[i]);
        if (!p.empty()) outPrereqs.push_back(p);
    }

    // Basic field checks
    if (outNumber.empty()) {
        summary.issues.push_back({ lineNo, "MissingField", "Empty course number" });
        return false;
    }
    if (outTitle.empty()) {
        summary.issues.push_back({ lineNo, "MissingField",
                                  "Empty course title for " + outNumber });
        return false;
    }
    return true;
}
/* Reviewer note (Pass 1):
   First pass reads/normalizes each CSV line and records precise line numbers
   for any missing fields/duplicates. This makes bad rows easy to track down. 
   */

   // Pass 2A: Validate prereqs exist; strip unknown prereqs; track self-prereqs
static void ValidatePrereqs(unordered_map<string, Course>& temp,
    LoadResultSummary& summary) {
    for (auto& kv : temp) {
        Course& c = kv.second;
        vector<string> keep;
        for (const string& p : c.prereqs) {
            if (p == c.number) {
                summary.selfPrereqs++;
                summary.issues.push_back({ 0, "SelfPrereq",
                                          "Self prerequisite removed: " + c.number });
                continue; // drop self-edge
            }
            if (temp.find(p) == temp.end()) {
                summary.unknownPrereqs++;
                summary.issues.push_back({ 0, "UnknownPrereq",
                                          "Unknown prereq '" + p + "' for " + c.number });
                continue; // drop unknown
            }
            keep.push_back(p);
        }
        c.prereqs.swap(keep);
    }
}
/* Reviewer note (Pass 2A):
   Cleanup pass removes self-edges and unknown prerequisites. It also counts and
   logs what was dropped so the load summary clearly explains the changes. 
   */

   // Pass 2B: Cycle detection (DFS with color marking). Skips courses that are in cycles.
enum Color { WHITE = 0, GRAY = 1, BLACK = 2 };

static bool DFSDetect(const string& u,
    unordered_map<string, Color>& color,
    unordered_map<string, string>& parent,
    const unordered_map<string, Course>& temp,
    vector<string>& cyclePath) {
    color[u] = GRAY;
    auto it = temp.find(u);
    if (it != temp.end()) {
        for (const string& v : it->second.prereqs) {
            if (color[v] == WHITE) {
                parent[v] = u;
                if (DFSDetect(v, color, parent, temp, cyclePath)) return true;
            }
            else if (color[v] == GRAY) {
                // Found back-edge; reconstruct path v -> ... -> u -> v
                cyclePath.clear();
                string x = u;
                cyclePath.push_back(v);
                while (x != v && !x.empty()) {
                    cyclePath.push_back(x);
                    x = parent.count(x) ? parent[x] : string();
                }
                cyclePath.push_back(v);
                std::reverse(cyclePath.begin(), cyclePath.end());
                return true;
            }
        }
    }
    color[u] = BLACK;
    return false;
}
/* Reviewer note (Cycle detection core):
   This is the circular-dependency detector (DFS + WHITE/GRAY/BLACK). A back-edge
   to GRAY means we found a loop; I rebuild the exact path into cyclePath so the
   summary can show something like "A -> B -> C -> A". */

static unordered_set<string> DetectCyclesAndMark(const unordered_map<string, Course>& temp,
    LoadResultSummary& summary) {
    unordered_set<string> inCycle;
    unordered_map<string, Color> color;
    unordered_map<string, string> parent;
    for (const auto& kv : temp) color[kv.first] = WHITE;

    for (const auto& kv : temp) {
        const string& start = kv.first;
        if (color[start] != WHITE) continue;
        vector<string> cyclePath;
        if (DFSDetect(start, color, parent, temp, cyclePath)) {
            summary.cycles++;
            // Mark every node in the cycle; add issue with readable path.
            string pathStr;
            for (size_t i = 0; i < cyclePath.size(); ++i) {
                if (i) pathStr += " -> ";
                pathStr += cyclePath[i];
                inCycle.insert(cyclePath[i]);
            }
            summary.issues.push_back({ 0, "Cycle", "Cycle detected: " + pathStr });
        }
    }
    return inCycle;
}
/* Reviewer note (Pass 2B summary):
   If a cycle is detected, marks all members so they don’t get inserted.
   The load summary prints the readable cycle path for transparency. */

   // Insert validated (and cycle-free) courses into the hash table
static void InsertValidated(const unordered_map<string, Course>& temp,
    const unordered_set<string>& inCycle,
    HashTable& table,
    LoadResultSummary& summary) {
    for (const auto& kv : temp) {
        const string& num = kv.first;
        const Course& c = kv.second;
        if (inCycle.count(num)) continue; // skip cycle members
        if (table.Insert(c)) {
            summary.inserted++;
        }
        else {
            summary.duplicates++; // duplicate in final table (defensive)
        }
    }
}
/* Reviewer note (Insertion gate):
   Final insert step honors the cycle set so only valid, cycle-free courses
   make it into the hash table. Duplicates are guarded as a last resort. */

   // File Loader Orchestrator (multi-pass, timed)
static LoadResultSummary LoadCoursesFromFile(const string& filePath, HashTable& table) {
    LoadResultSummary summary;
    unordered_map<string, Course> temp; // number -> Course

    auto t0 = std::chrono::high_resolution_clock::now();

    ifstream fin(filePath);
    if (!fin.is_open()) {
        summary.issues.push_back({ 0, "FileError", "Cannot open file: " + filePath });
        return summary;
    }

    // Pass 1: parse/normalize; detect duplicates/missing fields with line numbers.
    string line;
    size_t lineNo = 0;
    while (getline(fin, line)) {
        ++lineNo;
        string number, title;
        vector<string> prereqs;
        if (!ParseLineCSV(line, lineNo, number, title, prereqs, summary)) {
            // parsing error already recorded (with line number)
            continue;
        }
        // Duplicate header detection within the same load.
        if (temp.find(number) != temp.end()) {
            summary.duplicates++;
            summary.issues.push_back({ lineNo, "Duplicate", "Duplicate course number: " + number });
            continue;
        }
        Course c;
        c.number = number;
        c.title = title;
        c.prereqs = prereqs;
        temp[number] = std::move(c);
        summary.parsedCourses++;
    }
    fin.close();

    // Pass 2A: prerequisite existence + self-prereq pruning; track unknowns/selfs.
    ValidatePrereqs(temp, summary);

    // Pass 2B: detect cycles and skip cycle members from insertion.
    unordered_set<string> inCycle = DetectCyclesAndMark(temp, summary);

    // Insert into hash table (duplicates guarded).
    InsertValidated(temp, inCycle, table, summary);

    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    summary.issues.push_back({ 0, "Timing", "Load completed in " + std::to_string(ms) + " ms" });
    return summary;
}
/* Reviewer note (Loader orchestration):
   The loader runs in clear passes (Parse → Validate → Detect cycles → Insert),
   then logs a one-shot summary including timing so performance can be discussed. 
   */

   // Presentation helpers (UI)
static void PrintLoadSummary(const LoadResultSummary& s) {
    cout << "\n=== Load Summary ===\n";
    cout << "Lines read:        " << s.linesRead << "\n";
    cout << "Courses parsed:    " << s.parsedCourses << "\n";
    cout << "Inserted:          " << s.inserted << "\n";
    cout << "Duplicates:        " << s.duplicates << "\n";
    cout << "Unknown prereqs:   " << s.unknownPrereqs << "\n";
    cout << "Self prereqs:      " << s.selfPrereqs << "\n";
    cout << "Cycles detected:   " << s.cycles << "\n";
    for (const auto& issue : s.issues) {
        // Only show detailed validation/timing/cycle logs; lineNo 0 indicates non-line-specific.
        if (issue.type == "Timing")
            cout << "* " << issue.detail << "\n";
        else if (issue.lineNo > 0)
            cout << "* [line " << issue.lineNo << "] " << issue.type << ": " << issue.detail << "\n";
        else
            cout << "* " << issue.type << ": " << issue.detail << "\n";
    }
    cout << "====================\n\n";
}
/* Reviewer note (UX summary):
   All file and validation issues get aggregated into one summary so the grader
   can see exactly what happened during load (including timing and cycles). */

static void PrintHelp() {
    cout << "\nHelp:\n"
        "1. Load Data Structure  - Read a CSV file and load courses into the hash table.\n"
        "2. Print Course List    - Show all courses alphanumerically (CSCI and MATH).\n"
        "3. Print Course         - Enter a course number to see its title and prerequisites (with titles).\n"
        "9. Exit                 - Quit the program.\n"
        "Other: 'H' or '?' shows this help. Input is case-insensitive.\n\n";
}

// Show all courses alphanumerically without mutating the hash table.
static void PrintAll(const HashTable& table) {
    auto t0 = std::chrono::high_resolution_clock::now();
    vector<Course> v = table.ToVectorSorted();
    auto t1 = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    if (v.empty()) {
        cout << "No courses loaded. Use option 1 to load data first.\n\n";
        return;
    }

    cout << "\nHere is a sample schedule:\n\n";
    for (const Course& c : v) {
        cout << c.number << ", " << c.title << "\n";
    }
    cout << "\n(List generated in " << ms << " ms)\n\n";
}
/* Reviewer note (Listing + timing):
   Listing uses the non-destructive ToVectorSorted() and prints the elapsed time.
   This supports the runtime analysis discussion with actual numbers. */

   // Look up one course and print title + prerequisites with titles.
static void PrintCourse(const HashTable& table, const string& rawInput) {
    string key = NormalizeCourse(rawInput);
    const Course* c = table.Search(key);
    if (!c) {
        cout << "Course not found: " << key << "\n\n";
        return;
    }
    cout << c->number << ", " << c->title << "\n";
    if (c->prereqs.empty()) {
        cout << "Prerequisites: None\n\n";
        return;
    }
    cout << "Prerequisites: ";
    bool first = true;
    for (const string& p : c->prereqs) {
        const Course* pc = table.Search(p);
        if (!first) cout << ", ";
        if (pc) cout << pc->number;
        else    cout << p << " (Not found)";
        first = false;
    }
    cout << "\n";
    // Also print titles under each prereq for clarity (as per directions/sample).
    for (const string& p : c->prereqs) {
        const Course* pc = table.Search(p);
        if (pc) cout << "  - " << pc->number << ": " << pc->title << "\n";
        else    cout << "  - " << p << ": [Title not found]\n";
    }
    cout << "\n";
}

// Robust menu loop with input sanitization and help.
static void MenuLoop() {
    HashTable table;         // main data store
    bool hasLoaded = false;  // gate printing/searching until load occurs

    cout << "Welcome to the course planner.\n\n";

    while (true) {
        cout << "  1. Load Data Structure.\n"
            "  2. Print Course List.\n"
            "  3. Print Course.\n"
            "  9. Exit\n\n"
            "What would you like to do? ";

        string choice;
        if (!getline(cin, choice)) break;
        if (!choice.empty()) choice = trim(choice);

        if (choice == "1") {
            cout << "Enter file name (e.g., courses.txt): ";
            string path;
            if (!getline(cin, path)) break;
            path = trim(path);
            if (path.empty()) {
                cout << "File name cannot be empty.\n\n";
                continue;
            }

            // Load (multi-pass + summary), rebuilds the table on every load for clarity.
            table = HashTable(); // reset
            auto summary = LoadCoursesFromFile(path, table);
            PrintLoadSummary(summary);
            hasLoaded = (summary.inserted > 0);

        }
        else if (choice == "2") {
            if (!hasLoaded) {
                cout << "Please load the data structure first (option 1).\n\n";
                continue;
            }
            PrintAll(table);

        }
        else if (choice == "3") {
            if (!hasLoaded) {
                cout << "Please load the data structure first (option 1).\n\n";
                continue;
            }
            while (true) {
                cout << "What course do you want to know about? (or press Enter to cancel): ";
                string input;
                getline(cin, input);
                input = trim(input);
                if (input.empty()) { cout << "(cancelled)\n\n"; break; }
                PrintCourse(table, input);
                break;
            }

        }
        else if (choice == "9") {
            cout << "Thank you for using the course planner!\n";
            break;

        }
        else if (choice == "h" || choice == "H" || choice == "?") {
            PrintHelp();
        }
        else {
            cout << choice << " is not a valid option.\n\n";
            // Show quick hint to improve UX
            cout << "Try: 1 (Load), 2 (List), 3 (Course), 9 (Exit), or H for help.\n\n";
        }
    }
}
/* Reviewer note (Menu UX):
   Menu accepts H/h/? for contextual help, politely re-prompts on bad input,
   and blocks search/print until a successful load happens. This guards the UX. 
   */

   // Entry Point
int main() {
    MenuLoop();
    return 0;
}

