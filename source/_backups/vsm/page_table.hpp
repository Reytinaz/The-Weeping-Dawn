#pragma once
#include "vsm_constants.hpp"
#include <vector>
#include <cstdint>

namespace vsm {
    class PageTable {
    public:
        PageTable() : entries(VIRTUAL_PAGES* VIRTUAL_PAGES) {
            reset();
        }

        // Сброс всех записей
        void reset() {
            for (auto& e : entries) {
                e.flags = 0;
                e.physicalIndex = 0xFFFFFFFF; // невалидный индекс
            }
        }

        // Доступ к записи по виртуальным координатам страницы
        PageTableEntry& at(uint32_t pageX, uint32_t pageY) {
            return entries[pageY * VIRTUAL_PAGES + pageX];
        }

        const PageTableEntry& at(uint32_t pageX, uint32_t pageY) const {
            return entries[pageY * VIRTUAL_PAGES + pageX];
        }

    private:
        std::vector<PageTableEntry> entries;
    };
}