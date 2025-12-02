
#ifndef NEXT_DICTIONARY_H
#define NEXT_DICTIONARY_H

#include <vector>

enum ValueType {
    VALUE_TYPE_UNKNOWN = 0,
    VALUE_TYPE_INT32   = 1,
    VALUE_TYPE_INT64   = 2,
    VALUE_TYPE_STRING  = 3
};

class NextDictionary {
public:
    NextDictionary() = default;

    virtual ~NextDictionary();

    void Clear();

    void SetInt64(const char *name, int64_t value);

    bool FindInt64(const char *name, int64_t *value) const;

    int64_t GetInt64(const char *name, int64_t defaultValue) const;

    void SetString(const char *name, const std::string &s);

    std::string GetString(const char *name, std::string *defaultValue) const;

    size_t GetSize() const;

    const char *GetEntryNameAt(size_t index, ValueType *type) const;

protected:
    struct Item {
        // TODO: memory align
//        union {
            int64_t val_int64;
            std::string *val_string;
//        } u;
        const char *item_name;
        size_t name_len;
        ValueType type;

        void SetName(const char *name, size_t len);
    };

    std::vector<Item *> m_items;

    Item *AllocateItem(const char *name);

    static void FreeItemValue(Item *item);

    const Item *FindItem(const char *name, ValueType type) const;

    size_t FindItemIndex(const char *name, size_t len) const;
};

#endif
