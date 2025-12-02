/**
 * Note: operation of dictionary
 * Date: 2025/12/2
 * Author: frank
 */

#include "NextDictionary.h"

#include <string>

NextDictionary::~NextDictionary() { Clear(); }

void NextDictionary::Clear() {
    for (auto item: m_items) {
        if (item) {
            if (item->item_name) {
                delete[] item->item_name;
                item->item_name = nullptr;
            }
            FreeItemValue(item);
            delete item;
        }
    }
    m_items.clear();
}

void NextDictionary::SetInt64(const char *name, int64_t value) {
    Item *item = AllocateItem(name);
    if (item) {
        item->type = VALUE_TYPE_INT64;
        item->val_int64 = value;
    }
}

bool NextDictionary::FindInt64(const char *name, int64_t *value) const {
    const Item *item = FindItem(name, VALUE_TYPE_INT64);
        if (item) {
            *value = item->val_int64;
            return true;
        }
    return false;
}

int64_t NextDictionary::GetInt64(const char *name, int64_t defaultValue) const {
    const Item *item = FindItem(name, VALUE_TYPE_INT64);
    if (item) {
        return item->val_int64;
    }
    return defaultValue;
}


void NextDictionary::Item::SetName(const char *name, size_t len) {
    name_len = len;
    item_name = new char[len + 1];
    memcpy((void *) item_name, name, len + 1);
}

NextDictionary::Item *NextDictionary::AllocateItem(const char *name) {
    size_t len = strlen(name);
    size_t i = FindItemIndex(name, len);
    Item *item;

    if (i < m_items.size()) {
        item = m_items[i];
        FreeItemValue(item);
    } else {
        item = new Item();
        if (!item) {
            return item;
        }
        item->SetName(name, len);
        m_items.emplace_back(item);
    }

    return item;
}

void NextDictionary::FreeItemValue(Item *item) {
    switch (item->type) {
        case VALUE_TYPE_STRING:
            delete item->val_string;
            break;
        default:
            break;
    }
}

const NextDictionary::Item *NextDictionary::FindItem(const char *name, ValueType type) const {
    size_t i = FindItemIndex(name, strlen(name));

    if (i < m_items.size()) {
        const Item *item = m_items[i];
        return item->type == type ? item : nullptr;
    }

    return nullptr;
}

size_t NextDictionary::FindItemIndex(const char *name, size_t len) const {
    size_t i = 0;
    for (; i < m_items.size(); i++) {
        if (len != m_items[i]->name_len) {
            continue;
        }
        if (!memcmp(m_items[i]->item_name, name, len)) {
            break;
        }
    }
    return i;
}

void NextDictionary::SetString(const char *name, const std::string &s) {
    Item *item = AllocateItem(name);
    if (item) {
        item->type = VALUE_TYPE_STRING;
        item->val_string = new std::string(s);
    }
}

std::string NextDictionary::GetString(const char *name, std::string *defaultValue) const {
    const Item *item = FindItem(name, VALUE_TYPE_STRING);
    if (item) {
        return *(item->val_string);
    }
    return defaultValue ? (*defaultValue) : "";
}

size_t NextDictionary::GetSize() const {
    return m_items.size();
}

const char *NextDictionary::GetEntryNameAt(size_t index, ValueType *type) const {
    if (index >= m_items.size()) {
        *type = VALUE_TYPE_INT32;

        return nullptr;
    }
    *type = m_items[index]->type;
    return m_items[index]->item_name;
}
