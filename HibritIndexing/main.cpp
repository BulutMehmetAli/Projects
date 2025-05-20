#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
using namespace std;

// Trim function to remove leading and trailing whitespaces
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}
// Lowercase conversion function
string toLowerCase(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}
// BookRecord structure to store book details
struct BookRecord {
    int id;
    string title;
    string author;
    string category;
    int year;

    BookRecord() : id(0), title(""), author(""), category(""), year(0) {}

    BookRecord(int id, string title, string author, string category, int year)
        : id(id), title(trim(title)), author(trim(author)), category(trim(category)), year(year) {}
};

// B+ Tree Node structure
struct BPlusTreeNode {
    vector<string> keys;
    vector<BPlusTreeNode*> children;
    vector<BookRecord> records;
    BPlusTreeNode* next;
    bool isLeaf;

    BPlusTreeNode(bool leaf = false) : isLeaf(leaf), next(nullptr) {}
    ~BPlusTreeNode() {
        for (auto child : children) {
            delete child;
        }
    }
};

// B+ Tree structure
class BPlusTree {
public:
    BPlusTreeNode* root;
    int order;

    BPlusTree(int order = 4) : order(order) {
        root = new BPlusTreeNode(true);
    }

    void insert(const string& key, const BookRecord& record) {
        BPlusTreeNode* current = root;

        // If the root is full, divide the root
        if (current->keys.size() == order - 1) {
            BPlusTreeNode* newRoot = new BPlusTreeNode(false);
            newRoot->children.push_back(root);
            splitChild(newRoot, 0, root);
            root = newRoot;
        }

        // Don't land on a leaf
        while (!current->isLeaf) {
            int i = 0;
            while (i < current->keys.size() && key > current->keys[i]) i++;
            if (current->children[i]->keys.size() == order - 1) {
                splitChild(current, i, current->children[i]);
                if (key > current->keys[i]) i++;
            }
            current = current->children[i];
        }

        // Anahtarın doğru pozisyonunu bul
        auto it = lower_bound(current->keys.begin(), current->keys.end(), key);
        int index = distance(current->keys.begin(), it);

        // Anahtarı ve kaydı doğru pozisyona ekle
        current->keys.insert(it, key);
        current->records.insert(current->records.begin() + index, record);
    }

    void splitChild(BPlusTreeNode* parent, int index, BPlusTreeNode* child) {
        int mid = order / 2;
        BPlusTreeNode* newChild = new BPlusTreeNode(child->isLeaf);

        // Taşınacak anahtarları ve kayıtları belirle
        newChild->keys.assign(child->keys.begin() + mid, child->keys.end());
        child->keys.resize(mid);

        if (child->isLeaf) {
            newChild->records.assign(child->records.begin() + mid, child->records.end());
            child->records.resize(mid);
            newChild->next = child->next;
            child->next = newChild;
        } else {
            newChild->children.assign(child->children.begin() + mid, child->children.end());
            child->children.resize(mid);
        }

        // Orta anahtarın doğru yerleştirilmesi
        if (!child->isLeaf) {
            parent->keys.insert(parent->keys.begin() + index, child->keys[mid]);
            child->keys.erase(child->keys.begin() + mid);
        } else {
            parent->keys.insert(parent->keys.begin() + index, newChild->keys[0]);
        }
        parent->children.insert(parent->children.begin() + index + 1, newChild);
    }

    vector<BookRecord> search(const string& key) {
        vector<BookRecord> result;
        BPlusTreeNode* current = root;

        // Yaprağa inme
        while (!current->isLeaf) {
            int i = 0;
            while (i < current->keys.size() && key > current->keys[i]) i++;
            current = current->children[i];
        }

        // Yaprak düğümde arama (küçük harfe dönüştürülmüş tam eşleşme)
        string searchKey = toLowerCase(trim(key));
        while (current != nullptr) {
            for (size_t i = 0; i < current->keys.size(); i++) {
                // Anahtar ve arama terimini küçük harfe dönüştürerek karşılaştır
                if (toLowerCase(trim(current->keys[i])) == searchKey) {
                    // Tek tek kayıtları ekle
                    result.push_back(current->records[i]);
                }
            }
            current = current->next;
        }
        return result;
    }
    ~BPlusTree() {
        delete root;
    }
};

// Hybrid Index using B+ Trees for all fields
class Hibrit {
public:
    BPlusTree titleIndex;
    BPlusTree authorIndex;
    BPlusTree categoryIndex;

    void insert(const BookRecord& record) {
        titleIndex.insert(record.title, record);
        authorIndex.insert(record.author, record);
        categoryIndex.insert(record.category, record);
    }

    vector<BookRecord> searchByTitle(const string& title) {
        return titleIndex.search(trim(title));
    }

    vector<BookRecord> searchByAuthor(const string& author) {
        return authorIndex.search(trim(author));
    }

    vector<BookRecord> searchByCategory(const string& category) {
        return categoryIndex.search(trim(category));
    }
    void loadFromFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Error: Unable to open file: " << filename << endl;
            return;
        }
        cout << "File opened successfully: " << filename << endl;

        string line;
        int lineCount = 0;

        while (getline(file, line)) {
            line = trim(line);
            lineCount++;

            try {
                if (line.empty()) {
                    cout << "Warning: Empty line at line " << lineCount << endl;
                    continue;
                }

                // Veriyi virgüllere göre bölme
                size_t firstComma = line.find(',');
                if (firstComma == string::npos) throw runtime_error("Invalid line format");

                int id = stoi(trim(line.substr(0, firstComma)));

                // Başlığı bulma
                size_t secondComma = line.find(',', firstComma + 1);
                if (secondComma == string::npos) throw runtime_error("Invalid line format");
                string title = trim(line.substr(firstComma + 1, secondComma - firstComma - 1));

                // Yazarı bulma
                size_t thirdComma = line.find(',', secondComma + 1);
                if (thirdComma == string::npos) throw runtime_error("Invalid line format");
                string author = trim(line.substr(secondComma + 1, thirdComma - secondComma - 1));

                // Kategoriyi bulma
                size_t fourthComma = line.find(',', thirdComma + 1);
                if (fourthComma == string::npos) throw runtime_error("Invalid line format");
                string category = trim(line.substr(thirdComma + 1, fourthComma - thirdComma - 1));

                // Yılı bulma
                int year = stoi(trim(line.substr(fourthComma + 1)));

                // Kayıt oluştur ve indekse ekle
                BookRecord record(id, title, author, category, year);
                insert(record);

            } catch (const exception& e) {
                //cout << "Error: Parsing failed at line " << lineCount << " (" << e.what() << "): " << line << endl;
            }
        }
        file.close();
    }


};
int main() {
    Hibrit index;

    string filename = "C:\\Users\\bulut\\Desktop\\Projeler\\FileOrganization\\HibritIndexing\\books_dataset.txt";

    cout << "Loading data from: " << filename << endl;
    index.loadFromFile(filename);
    cout << "Data successfully loaded!" << endl;

    int choice;
    string query;
    while (true) {
        cout << "\nSelect the field to search:" << endl;
        cout << "1. Search by Title" << endl;
        cout << "2. Search by Author" << endl;
        cout << "3. Search by Category" << endl;
        cout << "4. Exit" << endl;
        cout << "Your choice: ";
        cin >> choice;

        if (choice == 4) break;

        cout << "Enter search term: ";
        cin.ignore();
        getline(cin, query);

        vector<BookRecord> result;
        switch (choice) {
            case 1:
                result = index.searchByTitle(query);
                break;
            case 2:
                result = index.searchByAuthor(query);
                break;
            case 3:
                result = index.searchByCategory(query);
                break;
            default:
                cout << "Invalid choice!" << endl;
                continue;
        }

        if (result.empty()) {
            cout << "No records found." << endl;
        } else {
            reverse(result.begin(), result.end());

            for (const auto& book : result) {
                cout << "ID: " << book.id << " | Title: " << book.title
                     << " | Author: " << book.author << " | Category: " << book.category
                     << " | Year: " << book.year << endl;
            }
        }
    }
};