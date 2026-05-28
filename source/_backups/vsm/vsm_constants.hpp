#pragma once
#include <cstdint>

/*
namespace vsm{
    // Размеры страниц и атласа (можно будет менять)
    constexpr uint32_t PAGE_SIZE = 128;            // пикселей
    constexpr uint32_t VIRTUAL_SIZE = 16384;       // виртуальное разрешение 16K
    constexpr uint32_t VIRTUAL_PAGES = VIRTUAL_SIZE / PAGE_SIZE; // 128x128 страниц

    constexpr uint32_t PHYSICAL_SIZE = 4096;        // физический атлас 4K
    constexpr uint32_t PHYSICAL_PAGES_PER_ROW = PHYSICAL_SIZE / PAGE_SIZE; // 32
    constexpr uint32_t PHYSICAL_PAGES = PHYSICAL_PAGES_PER_ROW * PHYSICAL_PAGES_PER_ROW; // 1024

    // Максимальное количество страниц, которые можно пометить за один кадр
    // Это ограничивает размер буфера запросов
    constexpr uint32_t MAX_MARKED_PAGES = 1024;     // можно увеличить, например, до 4096

    // Битовая маска для состояния страницы (хранится в таблице)
    enum PageFlags : uint32_t {
        PAGE_VISIBLE = 1 << 0,
        PAGE_ALLOCATED = 1 << 1,
        PAGE_DIRTY = 1 << 2,
        PAGE_REQUESTED = 1 << 3
    };

    // Структура для элемента таблицы страниц (в памяти CPU)
    struct PageTableEntry {
        uint32_t flags;          // состояние
        uint32_t physicalIndex;  // индекс в физическом атласе (0..PHYSICAL_PAGES-1)
        // можно добавить lastUsedFrame для кэширования
    };
} // namespace vsm
*/
namespace vsm {
    constexpr uint32_t PAGE_SIZE = 128;
    constexpr uint32_t VIRTUAL_SIZE = 16384;
    constexpr uint32_t VIRTUAL_PAGES = 128;
    constexpr uint32_t PHYSICAL_SIZE = 4096;
    constexpr uint32_t PHYSICAL_PAGES_PER_ROW = 32;
    constexpr uint32_t PHYSICAL_PAGES = 1024;
    constexpr uint32_t MAX_MARKED_PAGES = 2048;
}