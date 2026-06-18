#ifndef APP_RECORDMANAGER_H
#define APP_RECORDMANAGER_H

#include <string>
#include <vector>

struct RunRecord {
    std::string result;
    int reachedLayer = 1;
    std::vector<std::string> characterNames;
    int elapsedSeconds = 0;
    std::string finishedAt;
};

class RecordManager {
public:
    bool appendRecord(const RunRecord& record, std::string* error = nullptr) const;
    std::vector<RunRecord> loadRecords(std::string* error = nullptr) const;

private:
    std::string recordPath() const;
};

#endif // APP_RECORDMANAGER_H
